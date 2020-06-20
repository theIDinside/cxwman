#include "events.hpp"
#include <stack>
#include <xcom/core.hpp>
#include <xcom/workspace.hpp>

namespace cx::workspace
{
    Workspace::Workspace(cx::uint ws_id, std::string ws_name, cx::geom::Geometry space)
        : m_id(ws_id), m_name(ws_name), m_space(space),
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
        DBGLOG("Warning - function {} not implemented or implemented to test simple use cases", "Workspace::register_window");
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
        if(foc_con == t) {
            // cx::println("This window is about to be unregistered. Focus pointer must be re-focused on another client");
            auto xwin = t->client->client_id;
            auto predicate = [xwin](auto& c_tree) {
                if(c_tree->is_window()) {
                    if(c_tree->client->client_id == xwin || c_tree->client->frame_id == xwin)
                        return true;
                }
                return false;
            };
        }
        if(t->is_window()) {
            if(left_sibling && right_sibling) {
                if(!t->parent->is_root()) {
                    if(left_sibling.get() == t) {
                        // FIXME: Workspace's FOCUS POINTER must be set somewhere
                        promote_child(std::move(t->parent->right), t->parent);
                    } else if(right_sibling.get() == t) {
                        promote_child(std::move(t->parent->left), t->parent);
                    }
                } else {
                    auto root_tag = m_root->tag;
                    if(left_sibling.get() == t) {
                        anchor_new_root(std::move(t->parent->right), root_tag);
                    } else if(right_sibling.get() == t) {
                        anchor_new_root(std::move(t->parent->left), root_tag);
                    }
                    m_root->update_subtree_geometry();
                }
            }
        }
    }

    auto Workspace::find_window(xcb_window_t xwin) -> std::optional<ContainerTree*>
    {
        return in_order_traverse_find(m_root, [xwin](auto& c_tree) {
            if(c_tree->is_window()) {
                if(c_tree->client->client_id == xwin || c_tree->client->frame_id == xwin)
                    return true;
            }
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

    void Workspace::rotate_focus_layout() { foc_con->rotate_container_layout(); }

    void Workspace::rotate_focus_pair() { foc_con->rotate_children(); }
    // TODO: reimplement as up and down versions to get more accurate results. Right now it might or might not actually move window left/right
    void Workspace::move_focused_right()
    {
        auto target_space = geom::wrapping_add(foc_con->get_center() + Pos{foc_con->geometry.width / 2, 0}, Pos{10, 0}, m_root->geometry, 10);
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
    // TODO: reimplement as up and down versions to get more accurate results. Right now it might or might not actually move window left/right
    void Workspace::move_focused_left()
    {
        auto target_space = geom::wrapping_add(foc_con->get_center() + Pos{-(foc_con->geometry.width / 2), 0}, Pos{-10, 0}, m_root->geometry, 10);
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

    void Workspace::move_focused_up()
    {
        auto target_space = geom::wrapping_add(foc_con->center_of_top(), Pos{0, -10}, m_root->geometry, 10);
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
    void Workspace::move_focused_down()
    {
        auto source_position = foc_con->geometry.pos;
        auto target_space = geom::wrapping_add(foc_con->center_of_top(), Pos{0, foc_con->geometry.height + 10}, m_root->geometry, 10);
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

    void Workspace::move_focused(cx::events::ScreenSpaceDirection dir)
    {
        using Dir = cx::events::ScreenSpaceDirection;
        using Vector = Pos; // To just illustrate further what Pos actually represents in this function
        Pos target_space{0, 0};
        const auto& bounds = m_root->geometry;
        switch(dir) {
        case Dir::UP:
            target_space = geom::wrapping_add(foc_con->center_of_top(), Pos{0, -10}, bounds, 10);
            break;
        case Dir::DOWN:
            target_space = geom::wrapping_add(foc_con->center_of_top(), Pos{0, foc_con->geometry.height + 10}, bounds, 10);
            break;
        case Dir::LEFT:
            target_space = geom::wrapping_add(foc_con->get_center() + Pos{-1 * (foc_con->geometry.width / 2), 0}, Pos{-10, 0}, bounds, 10);
            break;
        case Dir::RIGHT:
            target_space = geom::wrapping_add(foc_con->get_center() + Pos{foc_con->geometry.width / 2, 0}, Pos{10, 0}, bounds, 10);
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

    void Workspace::focus_client(const xcb_window_t xwin)
    {
        auto c = in_order_traverse_find(m_root, [xwin](auto& tree) {
            if(tree->is_window()) {
                if(tree->client->client_id == xwin || tree->client->frame_id == xwin) {
                    return true;
                }
            }
            return false;
        });
        if(c) {
            DBGLOG("Focused client is: [Frame: {}, Client: {}]", c.value()->client->frame_id, c.value()->client->client_id);
            foc_con = *c;
        } else {
            cx::println("Could not find managed window with id {}", xwin);
        }
    }

    // This makes it, so we can "teleport" windows. We can an in-order list
    // so moving a window right, will move it along the bottom of the tree to the right, and vice versa
    template<typename P>
    std::vector<ContainerTree*> Workspace::get_clients(P p)
    {
        // TODO(implement): Add template to this, so we can pass in a predicate, so we can say "we want windows according to rule X"
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
        m_root = std::move(new_root);
        m_root->tag = tag;
        m_root->geometry = m_space;
        m_root->client->set_geometry(m_space);
        m_root->parent = m_root.get();
    }

}; // namespace cx::workspace
