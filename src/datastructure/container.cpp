#include <cassert>
#include <datastructure/container.hpp>

namespace cx::workspace
{

    // If layout policy is FLOATING, passing that in here will return Vertical.
    // But as a matter of fact, splitting a floating window makes no sense. So it means you are dumb.
    auto split(const geom::Geometry& geometry, Layout policy, float split_ratio)
    {
        if(policy == Layout::Horizontal) {
            return geom::v_split(geometry, split_ratio);
        } else {
            return geom::h_split(geometry, split_ratio);
        }
    }

    ContainerTree::ContainerTree(std::string container_tag, geom::Geometry geometry) noexcept
        : tag(std::move(container_tag)), client{}, left(nullptr), right(nullptr), parent(nullptr), policy(Layout::Vertical), split_ratio(0.5),
          geometry(geometry), m_tree_height(0)
    {
        assert(!client.has_value() && "Client should not be set using this constructor");
    }

    ContainerTree::ContainerTree(std::string container_tag, geom::Geometry geometry, ContainerTree* parent, Layout layout,
                                 std::size_t height) noexcept
        : tag(std::move(container_tag)), client{}, left(nullptr), right(nullptr), parent(parent), policy(layout), split_ratio(0.5),
          geometry(geometry), m_tree_height(height)
    {
    }
    // Private constructor accessed from make_root
    ContainerTree::ContainerTree(std::string container_tag, geom::Geometry geometry, Layout layout) noexcept
        : tag(std::move(container_tag)), client{}, left(nullptr), right(nullptr), parent(this), policy(layout), split_ratio(0.5f), geometry(geometry),
          m_tree_height(0)
    {
        DBGLOG("Constructing root container: {}", tag);
    }

    ContainerTree::~ContainerTree()
    {
        if(is_window()) {
            DBGLOG("Destroying Window Container. Client id: {} - Frame id: {}. Window tag: {} on workspace {}", client->client_id, client->frame_id,
                   client->m_tag.m_tag, client->m_tag.m_ws_id);
        }
    }

    bool ContainerTree::is_root() const { return this->parent == this; }

    bool ContainerTree::is_split_container() const { return !client.has_value(); }

    bool ContainerTree::is_window() const { return client.has_value(); }

    void ContainerTree::push_client(Window new_client)
    {
        if(is_window()) {
            DBGLOG("ContainerTree node {} is window. Mutating to split container", tag);
            auto existing_client = *client;
            auto con_prefix = layout_string(policy);
            tag.clear();
            tag.reserve(16);
            tag = con_prefix;
            tag.append("_container");
            auto [lsubtree_geometry, rsubtree_geometry] = split(geometry, policy);
            ;
            left = std::make_unique<ContainerTree>(existing_client.m_tag.m_tag, lsubtree_geometry, this, policy, m_tree_height + 1);
            right = std::make_unique<ContainerTree>(new_client.m_tag.m_tag, rsubtree_geometry, this, policy, m_tree_height + 1);
            left->push_client(existing_client);
            right->push_client(new_client);
            client.reset(); // effectively making 'this' of branch type
        } else {
            // default behavior is that each sub-division moves between horizontal / vertical layouts. if it's set to FLOATING we let it be
            if(this->policy == Layout::Vertical)
                policy = Layout::Horizontal;
            else if(this->policy == Layout::Horizontal)
                policy = Layout::Vertical;
            DBGLOG("Window container. Client tag name {}", tag);
            this->tag = new_client.m_tag.m_tag;
            this->client = new_client; // making 'this' of leaf type
            this->client->set_geometry(this->geometry);
        }
    }

    void ContainerTree::update_subtree_geometry()
    {
        if(is_split_container()) {
            auto [ltree_geo, rtree_geo] = split(geometry, policy);
            if(left) {
                left->geometry = ltree_geo;
                left->update_subtree_geometry();
            }
            if(right) {
                right->geometry = rtree_geo;
                right->update_subtree_geometry();
            }
        }
        if(is_window()) {
            this->client->set_geometry(this->geometry);
        }
    }

    void ContainerTree::switch_layout_policy()
    {
        fmt::print("Switching layout policy to: ");
        if(policy == Layout::Horizontal) {
            cx::println(" vertical");
            policy = Layout::Vertical;
        } else if(policy == Layout::Vertical) {
            cx::println(" horizontal");
            policy = Layout::Horizontal;
        }
        // else means it's floating, which we don't switch to or from
    }

    /// Changes the layout of a client tile-pair on the screen between horizontal/vertical
    void ContainerTree::rotate_container_layout()
    {
        if(is_window()) {
            parent->switch_layout_policy();
            parent->update_subtree_geometry();
        }
    }
    /// Rotates position of a client-tile-pair
    void ContainerTree::rotate_children()
    {
        if(!is_root()) {
            parent->left.swap(parent->right);
            parent->update_subtree_geometry();
        }
    }

    // This looks rugged...
    void move_client(ContainerTree* from, ContainerTree* to)
    {
        // this should work...
        auto parent_from = from->parent;
        auto parent_to = to->parent;
        if(!from->is_root() && !to->is_root()) {
            if(parent_from == parent_to) {
                cx::println("Client from and Client to has the same parent. Swapping sibling positions");
                parent_from->left.swap(parent_from->right);
            } else {
                if(parent_from->right.get() == from) {
                    if(parent_to->left.get() == to) {
                        parent_to->left.swap(parent_from->right);
                    } else if(parent_to->right.get() == to) {
                        parent_to->right.swap(parent_from->right);
                    } else {
                        DBGLOG("FAILURE: Tree 'to' did not find itself as a child of to's parent: {}", parent_to->tag);
                    }
                } else if(parent_from->left.get() == from) {
                    if(parent_to->left.get() == to) {
                        parent_to->left.swap(parent_from->left);
                    } else if(parent_to->right.get() == to) {
                        parent_to->right.swap(parent_from->left);
                    } else {
                        DBGLOG("FAILURE: Tree 'to' did not find itself as a child of to's parent: {}", parent_to->tag);
                    }
                }
                from->parent = parent_to;
                to->parent = parent_from;
            }
            parent_from->update_subtree_geometry();
            parent_to->update_subtree_geometry();
        } else {
            DBGLOG("Root windows can not be moved! {}", "");
        }
    }
    void promote_child(std::unique_ptr<ContainerTree> child, ContainerTree* parent)
    {
        cx::println("Promotion of child");
        auto grand_parent_geometry = parent->parent->geometry;
        if(parent->is_root()) {
            cx::println("Promoting child to ROOT parent not yet implemented");
        } else {
            auto grand_parent = parent->parent;
            if(grand_parent->left.get() == parent) {
                child->parent = grand_parent;
                grand_parent->left.swap(child);
            } else {
                child->parent = grand_parent;
                grand_parent->right.swap(child);
            }
            grand_parent->update_subtree_geometry();
        }
    }
    auto ContainerTree::make_root(const std::string& container_tag, geom::Geometry geometry, Layout layout) -> TreeOwned
    {
        return std::unique_ptr<ContainerTree>{new ContainerTree(container_tag, geometry, layout)};
    }
    geom::Position ContainerTree::center_of_top() {
        auto& client_window = client.value();
        auto pos = client_window.geometry.pos;
        pos.x += client_window.geometry.width / 2;
        return pos;
    }

} // namespace cx::workspace
