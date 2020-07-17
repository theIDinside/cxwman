#include <xcom/events.hpp>
#include <datastructure/geometry.hpp>
#include <stack>
#include <utility>
#include <xcom/commands/manager_command.hpp>
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
    auto Workspace::register_window(Window window, bool tiled) -> std::optional<commands::ConfigureWindows>
    {
        if(tiled) {
            if(!foc_con->is_window()) {
                foc_con->push_client(window);
                return commands::ConfigureWindows{foc_con->client.value()};
            } else {
                foc_con->push_client(window);
                auto existing_win = foc_con->left->client.value();
                auto new_win = foc_con->right->client.value();
                foc_con = foc_con->right.get();
                return commands::ConfigureWindows{existing_win, new_win};
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
                        auto ptr_to_tree_pos = promote_child(std::move(t->parent->right));
                        if(set_new_focus)
                            foc_con = ptr_to_tree_pos;
                    } else if(right_sibling.get() == t) {
                        auto ptr_to_tree_pos = promote_child(std::move(t->parent->left));
                        if(set_new_focus)
                            foc_con = ptr_to_tree_pos;
                    }
                } else {
                    auto root_tag = m_root->tag;
                    if(left_sibling.get() == t) {
                        if(set_new_focus)
                            foc_con = t->parent->right.get();
                        anchor_new_root(std::move(t->parent->right), root_tag);
                        left_sibling.reset();
                    } else if(right_sibling.get() == t) {
                        if(set_new_focus)
                            foc_con = t->parent->left.get();
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
        return tree_in_order_find(m_root, [xwin](auto& c_tree) {
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

    auto Workspace::move_focused(geom::ScreenSpaceDirection dir) -> commands::MoveWindow
    {
        return commands::MoveWindow{foc_con->client.value(), dir, this};
    }
    auto Workspace::increase_size_focused(cx::events::ResizeArgument arg) -> commands::UpdateWindows
    {
        using Dir = geom::ScreenSpaceDirection;
        using Vector = Pos; // To just illustrate further what Pos actually represents in this function
        switch(arg.dir) {
        case Dir::UP: {
            auto items = increase_height(
                arg.get_value(), [](auto& child, auto& parent) { return parent->policy == Layout::Vertical && parent->right.get() == child; });
            return commands::UpdateWindows{items};
        }
        case Dir::DOWN: {
            auto items = increase_height(arg.get_value(),
                                         [](auto& child, auto& parent) { return parent->policy == Layout::Vertical && parent->left.get() == child; });
            return commands::UpdateWindows{items};
        }
        case Dir::LEFT: {
            auto items = increase_width(
                arg.get_value(), [](auto& child, auto& parent) { return parent->policy == Layout::Horizontal && parent->right.get() == child; });
            return commands::UpdateWindows{items};
        }
        case Dir::RIGHT: {
            auto items = increase_width(
                arg.get_value(), [](auto& child, auto& parent) { return parent->policy == Layout::Horizontal && parent->left.get() == child; });
            return commands::UpdateWindows{items};
        }
        }
    }

    auto Workspace::decrease_size_focused(cx::events::ResizeArgument arg) -> commands::UpdateWindows
    {
        using Dir = geom::ScreenSpaceDirection;
        using Vector = Pos; // To just illustrate further what Pos actually represents in this function
        switch(arg.dir) {
        case Dir::UP: {
            auto items = decrease_height(
                arg.get_value(), [](auto& child, auto& parent) { return parent->policy == Layout::Vertical && parent->right.get() == child; });
            return commands::UpdateWindows{items};
        }
        case Dir::DOWN: {
            auto items = decrease_height(arg.get_value(),
                                         [](auto& child, auto& parent) { return parent->policy == Layout::Vertical && parent->left.get() == child; });
            return commands::UpdateWindows{items};
        }
        case Dir::LEFT: {
            auto items = decrease_width(
                arg.get_value(), [](auto& child, auto& parent) { return parent->policy == Layout::Horizontal && parent->right.get() == child; });
            return commands::UpdateWindows{items};
        }
        case Dir::RIGHT: {
            auto items = decrease_width(
                arg.get_value(), [](auto& child, auto& parent) { return parent->policy == Layout::Horizontal && parent->left.get() == child; });
            return commands::UpdateWindows{items};
        }
        }
    }

    template<typename Predicate>
    auto Workspace::increase_width(int steps, Predicate child_of) -> std::vector<ContainerTree*>
    {
        auto [child, parent] = focused().begin_bubble();
        for(; !child->is_root(); next_up(child, parent)) {
            if(child_of(child, parent)) { // Means it is this "parent" that needs a _decrease_ in size from it's left
                parent->split_position.x += steps;
                parent->update_subtree_geometry();
                return collect_treenodes_by(parent, [](auto& t) { return t->is_window(); });
            }
        }
        return {};
    }
    template<typename Predicate>
    auto Workspace::increase_height(int steps, Predicate child_of) -> std::vector<ContainerTree*>
    {
        /// this would otherwise become:
        /// auto child = foc_con; auto parent = foc_con->parent;
        for(auto [child, parent] = focused().begin_bubble(); !child->is_root(); next_up(child, parent)) {
            if(child_of(child, parent)) { // Means it is this "parent" that needs a _decrease_ in size from it's left
                parent->split_position.y += steps;
                parent->update_subtree_geometry();
                return collect_treenodes_by(parent, [](auto& t) { return t->is_window(); });
            }
        }
        return std::vector<ContainerTree*>{};
    }
    template<typename Predicate>
    auto Workspace::decrease_width(int steps, Predicate child_of) -> std::vector<ContainerTree*>
    {
        auto [child, parent] = focused().begin_bubble();
        for(; !child->is_root(); next_up(child, parent)) {
            if(child_of(child, parent)) { // Means it is this "parent" that needs a _decrease_ in size from it's left
                parent->split_position.x -= steps;
                parent->update_subtree_geometry();
                return collect_treenodes_by(parent, [](auto& t) { return t->is_window(); });
            }
        }
        return std::vector<ContainerTree*>{};
    }
    template<typename Predicate>
    auto Workspace::decrease_height(int steps, Predicate child_of) -> std::vector<ContainerTree*>
    {
        /// this would otherwise become:
        /// auto child = foc_con; auto parent = foc_con->parent;
        for(auto [child, parent] = focused().begin_bubble(); !child->is_root(); next_up(child, parent)) {
            if(child_of(child, parent)) { // Means it is this "parent" that needs a _decrease_ in size from it's left
                parent->split_position.y -= steps;
                parent->update_subtree_geometry();
                return collect_treenodes_by(parent, [](auto& t) { return t->is_window(); });
            }
        }
        return std::vector<ContainerTree*>{};
    }

    std::optional<commands::FocusWindow> Workspace::focus_client_with_xid(const xcb_window_t xwin)
    {
        auto c = tree_in_order_find(m_root, [xwin](auto& tree) {
            if(tree->is_window()) {
                return tree->client->client_id == xwin || tree->client->frame_id == xwin;
            }
            return false;
        });
        if(c) {
            auto client = c.value()->client.value();
            DBGLOG("Focused client: [Frame: {}, Client: {}] @ (x:{},y:{}) (w:{} x h:{})", client.frame_id, client.client_id, client.geometry.x(),
                   client.geometry.y(), client.geometry.width, client.geometry.height);
            auto de_focused_window = foc_con->client.value();
            foc_con = *c;
            auto cmd = commands::FocusWindow{client};
            cmd.set_defocused(de_focused_window);
            return cmd;
        } else {
            return {};
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
}; // namespace cx::workspace
