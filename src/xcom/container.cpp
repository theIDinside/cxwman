#include <xcom/container.hpp>
#ifdef DEBUGGING
#include <cassert>
#endif

namespace cx::workspace
{

    // If layout policy is FLOATING, passing that in here will return Vertical.
    // But as a matter of fact, splitting a floating window makes no sense. So it means you are dumb.
    auto split(const geom::Geometry geometry, Layout policy, float split_ratio) {
        if(policy == Layout::Horizontal) {
            return geom::vsplit(geometry, split_ratio);
        } else {
            return geom::hsplit(geometry, split_ratio);
        }
    }

    ContainerTree::ContainerTree(std::string container_tag, geom::Geometry geometry) :   
        tag(std::move(container_tag)), client{}, 
        left(nullptr), right(nullptr), parent(nullptr), 
        policy(Layout::Vertical), split_ratio(0.5),
        geometry(geometry), height(0)
    {
        assert(!client.has_value() && "Client should not be set using this constructor");
    }

    ContainerTree::~ContainerTree() { }
    
    bool ContainerTree::is_root() const {
        return this->parent == this;
    }

    bool ContainerTree::is_split_container() const {
        return !client.has_value();
    }

    bool ContainerTree::is_window() const {
        return client.has_value();
    }

    bool ContainerTree::has_left() const {
        return left != nullptr;
    }

    bool ContainerTree::has_right() const {
        return right != nullptr;
    }
    // TODO(fix): This assumes we are always focusing a "bottom-most" window. So this will
    // cause errors in display, if we focus on a window that isn't newly created. This will of course be fixed.
    // I just want to see that mapping 4 windows of equal size to each corner works initially & automatically
    void ContainerTree::push_client(Window new_client) {
        if(is_window()) {
            DBGLOG("Container {} is window", tag);
            auto existing_client = *client;
            auto con_prefix = layout_string(policy);

            tag.clear(); tag.reserve(16); tag = con_prefix; tag.append("_container");

            auto [lsubtree_geometry, rsubtree_geometry] = split(geometry, policy);;
            left = std::make_unique<ContainerTree>(existing_client.m_tag.m_tag, lsubtree_geometry);
            right = std::make_unique<ContainerTree>(new_client.m_tag.m_tag, rsubtree_geometry);
            left->height = height + 1;
            right->height = height + 1;
            left->push_client(existing_client);
            right->push_client(new_client);
            left->parent = this;
            right->parent = this;
            client.reset(); // effectively making 'this' of branch type
        } else {
            DBGLOG("Container {} is not window", tag);
            this->tag = new_client.m_tag.m_tag;
            this->client = new_client;  // making 'this' of leaf type
            this->client->set_geometry(this->geometry);
        }
    }


    void ContainerTree::update_geometry_from_parent() {
        if(!is_root()) {
            if(parent->left.get() == this) {
                cx::println("We are left child");
                this->geometry = parent->child_requesting_geometry(BranchDir::Left);
            } else if(parent->right.get() == this) {
                cx::println("We are right child");
                this->geometry = parent->child_requesting_geometry(BranchDir::Right);
            }
        }
        if(client) client->set_geometry(geometry);
    } 

    geom::Geometry ContainerTree::child_requesting_geometry(BranchDir dir) {
        auto [ltree_geometry, rtree_geometry] = split(geometry, policy);
        return (dir == BranchDir::Left) ? ltree_geometry : rtree_geometry;
    }


    void ContainerTree::update_subtree_geometry() {
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

    void ContainerTree::switch_layout_policy() {
        fmt::print("Switching layout policy to: ");
        if(policy == Layout::Horizontal) {
            cx::println(" vertical");
            policy = Layout::Vertical;
        }
        else if(policy == Layout::Vertical) {
            cx::println(" horizontal");
            policy = Layout::Horizontal;
        }
        // else means it's floating, which we don't switch to or from
    }

    void ContainerTree::swap_clients(TreeRef other) {
        if(this->is_window() && other->is_window()) {
            other->client.swap(client);
        }
    }
    /// Changes the layout of a client tile-pair on the screen between horizontal/vertical
    void ContainerTree::rotate_pair_layout() {
        if(is_window()) {
            parent->switch_layout_policy();
            parent->update_subtree_geometry();
        }
    }
    /// Rotates position of a client-tile-pair 
    void ContainerTree::rotate_pair_position() {
        if(!is_root()) {
            parent->left.swap(parent->right);
            parent->update_subtree_geometry();
        }
    }

    /// Windows can freely be swapped with any other window container. 
    bool are_swappable(TreeRef from, TreeRef to) {
        bool has_left = from->has_left();
        bool has_right = from->has_right();


        return  (from->is_window() && to->is_window()) || 
                ((from->is_split_container() && from->left->is_window() && from->right->is_window()) && to->is_window());
    }

    // This looks rugged...
    void move_client(ContainerTree* from, ContainerTree* to) {
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
        }
    }

} // namespace cx::workspace
