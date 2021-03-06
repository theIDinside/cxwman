#include <cassert>
#include <datastructure/container.hpp>

namespace cx::workspace
{

    auto split_at(const geom::Geometry& geometry, Layout layout, geom::Position p)
    {
        if(layout == Layout::Horizontal) {
            return geom::v_split_at(geometry, p.x);
        } else {
            return geom::h_split_at(geometry, p.y);
        }
    }

    ContainerTree::ContainerTree(std::string container_tag, geom::Geometry geometry, ContainerTree* parent, Layout layout,
                                 std::size_t height) noexcept
        : tag(std::move(container_tag)), client{}, left(nullptr), right(nullptr), parent(parent), policy(layout), split_ratio(0.5),
          geometry(geometry), m_tree_height(height)
    {
    }

    ContainerTree::ContainerTree(std::string container_tag, geom::Geometry geometry, Layout layout) noexcept
        : tag(std::move(container_tag)), client{}, left(nullptr), right(nullptr), parent(this), policy(layout), split_ratio(0.5f), geometry(geometry),
          m_tree_height(0)
    {
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
            if(policy == Layout::Horizontal) {
                auto [lgeo, rgeo] = geom::v_split_at(geometry, geometry.width / 2);
                split_position.x = geometry.width / 2;
                split_position.y = 0;
                left = std::make_unique<ContainerTree>(existing_client.m_tag.m_tag, lgeo, this, policy, m_tree_height + 1);
                right = std::make_unique<ContainerTree>(new_client.m_tag.m_tag, rgeo, this, policy, m_tree_height + 1);
                left->push_client(existing_client);
                right->push_client(new_client);
                client.reset(); // effectively making 'this' of branch type
                assert(!client && "Client must be reset so that we are no longer considered a window / leaf type");
            } else if(policy == Layout::Vertical) {
                auto [lgeo, rgeo] = geom::h_split_at(geometry, geometry.height / 2);
                split_position.x = 0;
                split_position.y = geometry.height / 2;
                left = std::make_unique<ContainerTree>(existing_client.m_tag.m_tag, lgeo, this, policy, m_tree_height + 1);
                right = std::make_unique<ContainerTree>(new_client.m_tag.m_tag, rgeo, this, policy, m_tree_height + 1);
                left->push_client(existing_client);
                right->push_client(new_client);
                client.reset(); // effectively making 'this' of branch type
                assert(!client && "Client must be reset so that we are no longer considered a window / leaf type");
            }
        } else {
            // default behavior is that each sub-division moves between horizontal / vertical layouts. if it's set to FLOATING we let it be
            if(this->policy == Layout::Vertical) {
                policy = Layout::Horizontal;
            } else if(this->policy == Layout::Horizontal) {
                policy = Layout::Vertical;
            }
            this->tag = new_client.m_tag.m_tag;
            this->client = new_client; // making 'this' of leaf type
            this->client->set_geometry(this->geometry);
        }
    }

    void ContainerTree::update_subtree_geometry()
    {
        if(is_split_container()) {
            auto [ltree_geo, rtree_geo] = split_at(geometry, policy, this->split_position);
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
        if(policy == Layout::Horizontal) {
            policy = Layout::Vertical;
            split_position.x = 0;
            split_position.y = geometry.height / 2;
        } else if(policy == Layout::Vertical) {
            policy = Layout::Horizontal;
            split_position.x = geometry.width / 2;
            split_position.y = 0;
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
            if(parent_from == parent_to) { // Client from and Client to has the same parent. Swapping sibling positions
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
    auto promote_child(std::unique_ptr<ContainerTree> child) -> ContainerTree*
    {
        if(child->parent->is_root()) {
            cx::println("Promoting child to ROOT parent not yet implemented. PANICKING");
            std::abort();
        } else {
            auto grand_parent = child->parent->parent;
            if(grand_parent->left.get() == child->parent) {
                child->parent = grand_parent;
                grand_parent->left.swap(child);
                grand_parent->update_subtree_geometry();
                return grand_parent->left.get();
            } else {
                child->parent = grand_parent;
                grand_parent->right.swap(child);
                grand_parent->update_subtree_geometry();
                return grand_parent->right.get();
            }
        }
    }
    auto ContainerTree::make_root(const std::string& container_tag, geom::Geometry geometry, Layout layout) -> TreeOwned
    {
        return std::unique_ptr<ContainerTree>{new ContainerTree(container_tag, geometry, layout)};
    }
    geom::Position ContainerTree::center_of_top() const
    {
        auto& client_window = client.value();
        auto pos = client_window.geometry.pos;
        pos.x += client_window.geometry.width / 2;
        return pos;
    }
    geom::Position ContainerTree::get_center() const
    {
        auto& client_window = client.value();
        auto pos = client_window.geometry.pos + geom::Vector{client_window.geometry.width / 2, client_window.geometry.height / 2};
        return pos;
    }
    auto ContainerTree::begin_bubble() -> BubbleIterator<ContainerTree> { return BubbleIterator{this, this->parent}; }

} // namespace cx::workspace
