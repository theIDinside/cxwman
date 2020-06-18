#pragma once
#include <optional>
#include <memory>
#include <string>

#include <xcom/window.hpp>


namespace cx::workspace
{
    enum BranchDir {
        Left,
        Right
    };

    enum class Layout {
        Vertical,
        Horizontal,
        Floating
    };

    constexpr auto layout_string(Layout layout) {
        if (layout == Layout::Vertical) {
            return "vs";
        } else if (layout == Layout::Horizontal) {
            return "hs";
        } else {
            return "floating";
        }
    }

    auto split(const geom::Geometry& geometry, Layout layout, float split_ratio = 0.5f);
    
    /**
     * ContainerTree emulates a tree type which is a discriminated union. To fully make it as such, one should probably have it's "data"
     * as of type std::variant<Window. pair<unique_ptr<C...>, unique_ptr<C...>>.
    */
    struct ContainerTree {
        using TreeRef   = std::unique_ptr<ContainerTree>&;
        using TreeOwned = std::unique_ptr<ContainerTree>;
        using TreePtr   = ContainerTree*;

        explicit ContainerTree(std::string container_tag, geom::Geometry geometry) noexcept;
        explicit ContainerTree(std::string container_tag, geom::Geometry geometry, ContainerTree* parent, Layout layout, std::size_t height) noexcept;
        ~ContainerTree();

        std::string tag;
        std::optional<Window> client; // we either map a client, or we are "empty", in the sense that we hold two clients in left and right
        TreeOwned left, right;
        ContainerTree* parent;
        Layout policy;
        float split_ratio;
        geom::Geometry geometry;
        std::size_t height;


        [[nodiscard]] bool is_root() const;
        [[nodiscard]] bool is_split_container() const;        // basically "is_branch?"
        [[nodiscard]] bool is_window() const;                 // basically "is_leaf?"
        [[nodiscard]] bool has_left() const;                  // TODO: remove these?
        [[nodiscard]] bool has_right() const;                 // TODO: remove these?

        void push_client(Window new_client);
        void update_geometry_from_parent();     // TODO: cleanup?
        geom::Geometry child_requesting_geometry(BranchDir dir);
        void update_subtree_geometry();
        void switch_layout_policy();
        void swap_clients(TreeRef other);       // TODO: cleanup?

        void rotate_container_layout();
        void rotate_children();

        template <typename Predicate>
        friend auto in_order_traverse_find(std::unique_ptr<ContainerTree>& tree, Predicate p) -> std::optional<ContainerTree*>;

        // run MapFn on each item in the tree, that is of window "type"
        template <typename MapFn>
        friend auto in_order_window_map(std::unique_ptr<ContainerTree>& tree, MapFn fn) -> void;
        friend bool are_swappable(TreeRef from, TreeRef to);
        friend void move_client(ContainerTree* from, ContainerTree* to);
        
        
    };
    using TreeRef   = std::unique_ptr<ContainerTree>&;
    using TreeOwned = std::unique_ptr<ContainerTree>;
    bool are_swappable(TreeRef from, TreeRef to);
    void move_client(ContainerTree* from, ContainerTree* to);

    std::unique_ptr<ContainerTree> make_tree(std::string ws_tag);


    template <typename MapFn>
    auto in_order_window_map(std::unique_ptr<ContainerTree>& tree, MapFn fn) -> void {
        if(!tree) 
            return;
        in_order_window_map(tree->left, fn);
        if(tree->is_window())
            fn(tree->client.value());
        in_order_window_map(tree->right, fn);
    }

    
    template <typename Predicate>
    auto in_order_traverse_find(std::unique_ptr<ContainerTree>& tree, Predicate p) -> std::optional<ContainerTree*> {
        if(!tree) return {};
        if(p(tree))  {
            return tree.get();
        } else {
            if(auto res = in_order_traverse_find(tree->left, p); res) 
                return res;
            if(auto res = in_order_traverse_find(tree->right, p); res) 
                return res;
            return {};
        }
    }

} // namespace cx::workspace
