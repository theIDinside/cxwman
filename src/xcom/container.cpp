#include <xcom/container.hpp>
#ifdef DEBUGGING
#include <cassert>
#endif

namespace cx::workspace
{
    ContainerTree::ContainerTree(std::string container_tag, geom::Geometry geometry) :   
        tag(std::move(container_tag)), client{}, 
        left(nullptr), right(nullptr), parent(nullptr), 
        policy(Layout::Vertical), split_ratio(0.5),
        geometry(geometry)
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
            auto existing_client = client.value();
            this->tag = "split_container";
            if(policy == Layout::Horizontal) {
                auto container_geometry = geom::vsplit(this->geometry);
                auto& [left_geo, right_geo] = container_geometry;
                left = std::make_unique<ContainerTree>(existing_client.m_tag.m_tag, left_geo);
                right = std::make_unique<ContainerTree>(new_client.m_tag.m_tag, right_geo);
                left->push_client(existing_client);
                right->push_client(new_client);
                left->parent = this;
                right->parent = this;
                client.reset();
            } else if(policy == Layout::Vertical) {
                auto container_geometry = geom::hsplit(this->geometry);
                auto& [top, bottom] = container_geometry;
                left = std::make_unique<ContainerTree>(existing_client.m_tag.m_tag, top);
                right = std::make_unique<ContainerTree>(new_client.m_tag.m_tag, bottom);
                left->push_client(existing_client);
                right->push_client(new_client);
                left->parent = this;
                right->parent = this;
                client.reset();
            }
        } else {
            DBGLOG("Container {} is not window", tag);
            new_client.set_geometry(this->geometry);
            this->tag = new_client.m_tag.m_tag;
            this->client = new_client;
            this->client.value().set_geometry(this->geometry);
        }
    }


    void ContainerTree::update_geometry_from_parent() {
        if(!is_root()) {
            if(parent->left.get() == this) {
                cx::println("We are left child");
                auto g = parent->child_requesting_geometry(BranchDir::Left);
                this->geometry = g;
            } else if(parent->right.get() == this) {
                cx::println("We are right child");
                auto g = parent->child_requesting_geometry(BranchDir::Right);
                this->geometry = g;
            }
        }
    } 

    geom::Geometry ContainerTree::child_requesting_geometry(BranchDir dir) {
        if(policy == Layout::Horizontal) {
            auto split_geometries = geom::vsplit(this->geometry);
            auto& [left_dir, right_dir] = split_geometries;
            if(dir == BranchDir::Left) return left_dir;
            else return right_dir;
        } else {
            auto split_geometries = geom::hsplit(this->geometry);
            auto& [left_dir, right_dir] = split_geometries;
            if(dir == BranchDir::Left) return left_dir;
            else return right_dir;
        }
    }


    void ContainerTree::update_subtree_geometry() {
        if(is_split_container()) {
            if(policy == Layout::Horizontal) {
                auto split_geometries = geom::vsplit(this->geometry);
                auto& [g_left, g_right] = split_geometries;
                if(left) {
                    left->geometry = g_left;
                    left->update_subtree_geometry();
                }
                if(right) {
                    right->geometry = g_right;
                    right->update_subtree_geometry();
                }
            } else if(policy == Layout::Vertical) {
                auto split_geometries = geom::hsplit(this->geometry);
                auto& [top, bottom] = split_geometries;
                if(left) {
                    left->geometry = top;
                    left->update_subtree_geometry();
                }
                if(right) {
                    right->geometry = bottom;
                    right->update_subtree_geometry();
                }
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

    void ContainerTree::rotate_pair_layout() {
        cx::println("Trying to rotate ContainerTree layout");
        if(is_window()) {
            cx::println("We are window. Attempting to rotate layouts");
            parent->switch_layout_policy();
            parent->update_subtree_geometry();
        }
    }

    void ContainerTree::rotate_pair_position() {
        if(!is_root()) {
            cx::println("We are not root. Rotating place with sibling. \nCurrent position geometry: {},{} - {}x{}", this->geometry.pos.x, this->geometry.pos.y, this->geometry.width, this->geometry.height);
            parent->left.swap(parent->right);
            parent->update_subtree_geometry();
            cx::println("New position geometry(?): {},{} - {}x{}", this->geometry.pos.x, this->geometry.pos.y, this->geometry.width, this->geometry.height);
        }
    }

    // Windows can freely be swapped with any other window container. 
    bool are_swappable(TreeRef from, TreeRef to) {
        bool has_left = from->has_left();
        bool has_right = from->has_right();


        return  (from->is_window() && to->is_window()) || 
                ((from->is_split_container() && from->left->is_window() && from->right->is_window()) && to->is_window());
    }

    void move_client(TreeRef from, TreeRef to) {
        auto from_parent    = from->parent;
        auto to_parent      = to->parent;
        if(are_swappable(from, to)) {
            // We are two children of the same parent.
            if(from_parent == to_parent) {
                from.swap(to);
                return;
            }

            if(from_parent->right == from) {
                if(to_parent->left == to) {
                    to_parent->left.swap(from_parent->right);
                } else if(to_parent->right == to) {
                    to_parent->right.swap(from_parent->right);
                } else {
                    DBGLOG("FAILURE: Tree 'to' did not find itself as a child of to's parent: {}", to_parent->tag);
                }
            } else if(from_parent->left == from) {
                if(to_parent->left == to) {
                    to_parent->left.swap(from_parent->left);
                } else if(to_parent->right == to) {
                    to_parent->right.swap(from_parent->left);
                } else {
                    DBGLOG("FAILURE: Tree 'to' did not find itself as a child of to's parent: {}", to_parent->tag);
                }
            } else {
                DBGLOG("FAILURE: Tree did not find itself as a child of from's parent: {}", from_parent->tag);
            }
        }
        from_parent->update_subtree_geometry();
        to_parent->update_subtree_geometry();
    }

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
