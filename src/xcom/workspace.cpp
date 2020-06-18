#include <xcom/workspace.hpp>
#include <xcom/core.hpp>
#include <stack>

namespace cx::workspace
{
    Workspace::Workspace(cx::uint ws_id, std::string ws_name, cx::geom::Geometry space) :
        m_id(ws_id), m_name(ws_name), 
        m_space(space), 
        m_containers(nullptr), 
        m_floating_containers{},
        m_root{ContainerTree::make_root("root container", space, Layout::Horizontal)},
        focused_container(nullptr), 
        is_pristine(true)
    {
        // std::make_unique<ContainerTree>("root container", geom::Geometry::default_new())
        focused_container = m_root.get();
        m_root->parent = m_root.get();
    }

    auto Workspace::register_window(Window window, bool tiled) -> std::optional<SplitConfigurations> {
        DBGLOG("Warning - function {} not implemented or implemented to test simple use cases", "Workspace::register_window");
        if(tiled) { 
            if(!focused_container->is_window()) {
                focused_container->push_client(window);
                return SplitConfigurations{focused_container->client.value()};
            } 
            else {
                focused_container->push_client(window);
                auto existing_win = focused_container->left->client.value();
                auto new_win = focused_container->right->client.value();
                focused_container = focused_container->right.get();
                return SplitConfigurations{existing_win, new_win};
            }
            // are we "dirty"? If so, re-map the workspace 
            is_pristine = false;
        } else {
            m_floating_containers.push_back(std::move(window));
            return {};
        }
    }
    // TODO: Promote sibling in client-pair to parent and destroy child t
    auto Workspace::unregister_window(ContainerTree* t) -> void {
        auto& left_sibling = t->parent->left;
        auto& right_sibling = t->parent->right;
        if(focused_container == t) {
            // cx::println("This window is about to be unregistered. Focus pointer must be re-focused on another client");
            auto xwin = t->client->client_id;
            auto predicate = [xwin](auto& c_tree) {
              if(c_tree->is_window()) {
                  if(c_tree->client->client_id == xwin || c_tree->client->frame_id == xwin) return true;
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

    auto Workspace::find_window(xcb_window_t xwin) -> std::optional<ContainerTree*> {
        return in_order_traverse_find(m_root, [xwin](auto& c_tree) {
            if(c_tree->is_window()) {
                if(c_tree->client->client_id == xwin || c_tree->client->frame_id == xwin) return true;
            }
            return false;
        });
    }

    auto Workspace::display_update(xcb_connection_t* c) -> void {

        auto mapper = [c](auto& window) {
            // auto window = window_opt;
            namespace xcm = cx::xcb_config_masks;
            const auto& [x, y, width, height] = window.geometry.xcb_value_list();
            auto frame_properties = xcm::TELEPORT;
            auto child_properties = xcm::RESIZE;
            cx::uint frame_values[] = { x, y, width, height };
            cx::uint child_values[] = { width, height };
            auto cookies = std::array<xcb_void_cookie_t, 2>{
                xcb_configure_window_checked(c, window.frame_id, frame_properties, frame_values),
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

    void Workspace::rotate_focus_layout() {
        focused_container->rotate_container_layout();
    }

    void Workspace::rotate_focus_pair() {
        focused_container->rotate_children();
    }

    void Workspace::move_focused_right() {
        auto cs = get_clients();
        auto index = 0;
        auto found = false;
        for(auto p : cs) {
            if(p == focused_container) {
                found = true;
                break;
            }
            index++;
        }
        if(found) {
            DBGLOG("Found client at {}", index);
            if(index == cs.size()-1) {
                move_client(focused_container, *std::begin(cs));
            } else {
                move_client(focused_container, cs[index+1]);
            }
        }
    }
    void Workspace::move_focused_left() {
        auto cs = get_clients();
        bool found = false;
        auto index = 0;
        for(auto p : cs) {
            if(p == focused_container) {
                found = true;
                break;
            } 
            index++;
        }
        if(found) {
            DBGLOG("Found client at {}", index);
            if(index == 0) {
                move_client(focused_container, *std::rbegin(cs));
            } else {
                move_client(focused_container, cs[index-1]);
            }
        }
    }
        

    void Workspace::focus_client(const xcb_window_t xwin) {
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
            focused_container = *c;
        } else {
            cx::println("Could not find managed window with id {}", xwin);
        }
    }

    // This makes it, so we can "teleport" windows. We can an in-order list
    // so moving a window right, will move it along the bottom of the tree to the right, and vice versa
    std::vector<ContainerTree*> Workspace::get_clients() {
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
            if(iter->is_window())
                clients.push_back(iter);
            iterator_stack.pop();
            iter = iter->right.get();
        }
        DBGLOG("Found {} clients in workspace. {} left on stack", clients.size(), iterator_stack.size());
        return clients;
    }
    void Workspace::anchor_new_root(TreeOwned new_root, const std::string &tag) {
        m_root = std::move(new_root);
        m_root->tag = tag;
        m_root->geometry = m_space;
        m_root->client->set_geometry(m_space);
        m_root->parent = m_root.get();
    }
};
