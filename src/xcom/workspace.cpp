#include "events.hpp"
#include <cassert>
#include <stack>
#include <utility>
#include <xcom/core.hpp>
#include <xcom/workspace.hpp>

namespace cx::workspace
{
    Workspace::Workspace(cx::uint ws_id, std::string ws_name, cx::geom::Geometry space) noexcept
        : m_id(ws_id), m_name(std::move(ws_name)), m_space(space),
          m_containers(nullptr), m_floating_containers{}, m_root{ContainerTree::make_root("root container", space, Layout::Horizontal)},
          foc_con(nullptr), is_pristine(true)
    {
        // std::make_unique<ContainerTree>("root container", geom::Geometry::default_new())
        foc_con = m_root.get();
        m_root->parent = m_root.get();
    }

    // FIXME: If all windows get unmapped, and a new client try to register, we crash, sometime inside or almost directly after this method
    auto Workspace::register_window(Window window, bool tiled) -> std::optional<SplitConfigurations>
    {
        if(tiled) {
            if(!foc_con->is_window()) {
                foc_con->push_client(window);
                return SplitConfigurations{foc_con->client.value()};
            } else {
                foc_con->push_client(window);
                auto existing_win = foc_con->left->client.value();
                auto new_win = foc_con->right->client.value();
                foc_con = foc_con->right.get();
                return SplitConfigurations{existing_win, new_win};
            }
        } else {
            m_floating_containers.push_back(std::move(window));
            return {};
        }
    }
    // TODO: Promote sibling in client-pair to parent and destroy child t
    auto Workspace::unregister_window(ContainerTree* t) -> void
    {
        auto& left_sibling = t->parent->left;
        auto& right_sibling = t->parent->right;
        bool set_new_focus = (foc_con == t);
        if(t->is_window()) {
            if(left_sibling && right_sibling) {
                if(!t->parent->is_root()) {
                    if(left_sibling.get() == t) {
                        // FIXME: Workspace's FOCUS POINTER must be set somewhere
                        auto ptr_to_tree_pos = promote_child(std::move(t->parent->right), t->parent);
                        if(set_new_focus)
                            foc_con = ptr_to_tree_pos;
                    } else if(right_sibling.get() == t) {
                        auto ptr_to_tree_pos = promote_child(std::move(t->parent->left), t->parent);
                        if(set_new_focus)
                            foc_con = ptr_to_tree_pos;
                    }
                } else {
                    auto root_tag = m_root->tag;
                    if(left_sibling.get() == t) {
                        if(set_new_focus) foc_con = t->parent->right.get();
                        anchor_new_root(std::move(t->parent->right), root_tag);
                        left_sibling.reset();
                    } else if(right_sibling.get() == t) {
                        if(set_new_focus) foc_con = t->parent->left.get();
                        anchor_new_root(std::move(t->parent->left), root_tag);
                        right_sibling.reset();
                    }
                    m_root->update_subtree_geometry();
                }
            } else {
                auto root_tag = m_root->tag;
                auto m_root_geometry = m_space;
                auto m_root_layout = m_root->policy;
                m_root.reset();
                m_root = ContainerTree::make_root(root_tag, m_root_geometry, m_root_layout);
                foc_con = m_root.get();
            }
        }
    }

    auto Workspace::find_window(xcb_window_t xwin) -> std::optional<ContainerTree*>
    {
        return in_order_traverse_find(m_root, [xwin](auto& c_tree) {
            if(c_tree->is_window())
                return c_tree->client->client_id == xwin || c_tree->client->frame_id == xwin;
            else
                return false;
        });
    }

    auto Workspace::display_update(xcb_connection_t* c) -> void
    {
        auto mapper = [c](auto& window) {
            // auto window = window_opt;
            namespace xcm = cx::xcb_config_masks;
            const auto& [x, y, width, height] = window.geometry.xcb_value_list();
            auto frame_properties = xcm::TELEPORT;
            auto child_properties = xcm::RESIZE;
            cx::uint frame_values[] = {(cx::uint)x, (cx::uint)y, (cx::uint)width, (cx::uint)height};
            cx::uint child_values[] = {(cx::uint)width, (cx::uint)height};
            auto cookies = std::array<xcb_void_cookie_t, 2>{xcb_configure_window_checked(c, window.frame_id, frame_properties, frame_values),
                                                            xcb_configure_window_checked(c, window.client_id, child_properties, child_values)};
            for(const auto& cookie : cookies) {
                if(auto err = xcb_request_check(c, cookie); err) {
                    DBGLOG("Failed to configure item {}. Error code: {}", err->resource_id, err->error_code);
                }
            }
        };
        in_order_window_map(m_root, mapper);
        std::for_each(m_floating_containers.begin(), m_floating_containers.end(), mapper);
    }

    void Workspace::rotate_focus_layout() const { foc_con->rotate_container_layout(); }

    void Workspace::rotate_focus_pair() const { foc_con->rotate_children(); }

    void Workspace::move_focused(cx::events::ScreenSpaceDirection dir)
    {
        using Dir = cx::events::ScreenSpaceDirection;
        using Vec = cx::geom::Vector;
        Pos target_space{0, 0};
        const auto& bounds = m_root->geometry;
        switch(dir) {
        case Dir::UP:
            target_space = geom::wrapping_add(foc_con->center_of_top(), Vec{0, -10}, bounds, 10);
            break;
        case Dir::DOWN:
            target_space = geom::wrapping_add(foc_con->center_of_top(), Vec{0, foc_con->geometry.height + 10}, bounds, 10);
            break;
        case Dir::LEFT:
            target_space = geom::wrapping_add(foc_con->get_center() + Vec{-1 * (foc_con->geometry.width / 2), 0}, Vec{-10, 0}, bounds, 10);
            break;
        case Dir::RIGHT:
            target_space = geom::wrapping_add(foc_con->get_center() + Vec{foc_con->geometry.width / 2, 0}, Vec{10, 0}, bounds, 10);
            break;
        }
        if(!geom::is_inside(target_space, foc_con->geometry)) {
            auto target_client =
                in_order_traverse_find(m_root, [&](auto& tree) { return geom::is_inside(target_space, tree->geometry) && tree->is_window(); });
            if(target_client) {
                move_client(foc_con, *target_client);
            } else {
                DBGLOG("Could not find a suitable window to swap with. Position: ({},{})", target_space.x, target_space.y);
            }
        } else {
            DBGLOG("Target position is inside source geometry. No move {}", "");
        }
    }
    void Workspace::increase_size_focused(cx::events::ResizeArgument arg)
    {
        using Dir = cx::events::ScreenSpaceDirection;
        using Vector = Pos; // To just illustrate further what Pos actually represents in this function
        switch(arg.dir) {
        case Dir::UP:
            increase_height(arg.get_value(),
                            [](auto& child, auto& parent) { return parent->policy == Layout::Vertical && parent->right.get() == child; });
            break;
        case Dir::DOWN:
            increase_height(arg.get_value(),
                            [](auto& child, auto& parent) { return parent->policy == Layout::Vertical && parent->left.get() == child; });
            break;
        case Dir::LEFT:
            increase_width(arg.get_value(),
                           [](auto& child, auto& parent) { return parent->policy == Layout::Horizontal && parent->right.get() == child; });
            break;
        case Dir::RIGHT:
            increase_width(arg.get_value(),
                           [](auto& child, auto& parent) { return parent->policy == Layout::Horizontal && parent->left.get() == child; });
            break;
        }
    }

    void Workspace::decrease_size_focused(cx::events::ResizeArgument arg)
    {
        using Dir = cx::events::ScreenSpaceDirection;
        using Vector = Pos; // To just illustrate further what Pos actually represents in this function
        switch(arg.dir) {
        case Dir::UP:
            decrease_height(arg.get_value(),
                            [](auto& child, auto& parent) { return parent->policy == Layout::Vertical && parent->right.get() == child; });
            break;
        case Dir::DOWN:
            decrease_height(arg.get_value(),
                            [](auto& child, auto& parent) { return parent->policy == Layout::Vertical && parent->left.get() == child; });
            break;
        case Dir::LEFT:
            decrease_width(arg.get_value(),
                           [](auto& child, auto& parent) { return parent->policy == Layout::Horizontal && parent->right.get() == child; });
            break;
        case Dir::RIGHT:
            decrease_width(arg.get_value(),
                           [](auto& child, auto& parent) { return parent->policy == Layout::Horizontal && parent->left.get() == child; });
            break;
        }
    }

    template<typename Predicate>
    void Workspace::increase_width(int steps, Predicate child_of)
    {
        auto found = false;
        auto [child, parent] = focused().begin_bubble();
        for(; !child->is_root() && !found; next_up(child, parent)) {
            if(child_of(child, parent)) { // Means it is this "parent" that needs a _decrease_ in size from it's left
                found = true;
                parent->split_position.x += steps;
                parent->update_subtree_geometry();
            }
        }
    }
    template<typename Predicate>
    void Workspace::increase_height(int steps, Predicate child_of)
    {
        auto found = false;
        /// this would otherwise become:
        /// auto child = foc_con; auto parent = foc_con->parent;
        for(auto [child, parent] = focused().begin_bubble(); !child->is_root() && !found; next_up(child, parent)) {
            if(child_of(child, parent)) { // Means it is this "parent" that needs a _decrease_ in size from it's left
                found = true;
                parent->split_position.y += steps;
                parent->update_subtree_geometry();
            }
        }
    }
    template<typename Predicate>
    void Workspace::decrease_width(int steps, Predicate child_of)
    {
        auto found = false;
        auto [child, parent] = focused().begin_bubble();
        for(; !child->is_root() && !found; next_up(child, parent)) {
            if(child_of(child, parent)) { // Means it is this "parent" that needs a _decrease_ in size from it's left
                found = true;
                parent->split_position.x -= steps;
                parent->update_subtree_geometry();
            }
        }
    }
    template<typename Predicate>
    void Workspace::decrease_height(int steps, Predicate child_of)
    {
        auto found = false;
        /// this would otherwise become:
        /// auto child = foc_con; auto parent = foc_con->parent;
        for(auto [child, parent] = focused().begin_bubble(); !child->is_root() && !found; next_up(child, parent)) {
            if(child_of(child, parent)) { // Means it is this "parent" that needs a _decrease_ in size from it's left
                found = true;
                parent->split_position.y -= steps;
                parent->update_subtree_geometry();
            }
        }
    }

    bool Workspace::focus_client_with_xid(const xcb_window_t xwin)
    {
        auto c = in_order_traverse_find(m_root, [xwin](auto& tree) {
            if(tree->is_window()) {
                return tree->client->client_id == xwin || tree->client->frame_id == xwin;
            }
            return false;
        });
        if(c) {
            auto client = c.value()->client.value();
            DBGLOG("Focused client: [Frame: {}, Client: {}] @ (x:{},y:{}) (w:{} x h:{})", client.frame_id, client.client_id, client.geometry.x(),
                   client.geometry.y(), client.geometry.width, client.geometry.height);
            foc_con = *c;
            return true;
        } else {
            return false;
        }
    }

    // This makes it, so we can "teleport" windows. We can an in-order list
    // so moving a window right, will move it along the bottom of the tree to the right, and vice versa
    template<typename P>
    [[maybe_unused]] std::vector<ContainerTree*> Workspace::get_clients(P p)
    {
        std::vector<ContainerTree*> clients{};
        std::stack<ContainerTree*> iterator_stack{};
        ContainerTree* iter = m_root.get();

        while(iter != nullptr || !iterator_stack.empty()) {
            while(iter != nullptr) {
                iterator_stack.push(iter);
                iter = iter->left.get();
            }

            iter = iterator_stack.top();
            if(p(iter))
                clients.push_back(iter);
            iterator_stack.pop();
            iter = iter->right.get();
        }
        DBGLOG("Found {} clients in workspace. {} left on stack", clients.size(), iterator_stack.size());
        return clients;
    }
    void Workspace::anchor_new_root(TreeOwned new_root, const std::string& tag)
    {
        // cx::println("ANCHORING NEW ROOT. UNTESTED FUNCTION. MIGHT BE BUGGY.");
        m_root = std::move(new_root);
        m_root->tag = tag;
        m_root->geometry = m_space;
        if(m_root->is_window()) { // means we have 1 window, full screened
            m_root->client->set_geometry(m_space);
            m_root->parent = m_root.get();
        } else {
            m_root->geometry = m_space;
            m_root->parent = m_root.get();
        }
    }
    void Workspace::focus_pointer_to_sibling()
    {
        if(foc_con->parent->left.get() == foc_con) {
            foc_con = foc_con->parent->right.get();
        } else {
            foc_con = foc_con->parent->left.get();
        }
    }
}; // namespace cx::workspace
