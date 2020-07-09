#pragma once
#include <memory>
#include <optional>
#include <string>

#include <xcom/window.hpp>

namespace cx::workspace
{
    /// This is a utility wrapper for iterating in a straight line upwards a tree
    template<typename TreeNode>
    struct BubbleIterator {
        BubbleIterator(TreeNode* current, TreeNode* parent) : current(current), parent(parent) {}
        TreeNode* current;
        TreeNode* parent;
    };

    template<typename TreeNode>
    void next_up(TreeNode& current, TreeNode& next)
    {
        current = next;
        next = current->parent;
    }

    enum class Layout { Vertical, Horizontal, Floating };

    constexpr auto layout_string(Layout layout)
    {
        if(layout == Layout::Vertical) {
            return "vs";
        } else if(layout == Layout::Horizontal) {
            return "hs";
        } else {
            return "floating";
        }
    }
    auto split_at(const geom::Geometry& geometry, Layout layout, geom::Position p);

    /**
     * ContainerTree emulates a tree type which is a discriminated union. To fully make it as such, one should probably have it's "data"
     * as of type std::variant<Window. pair<unique_ptr<C...>, unique_ptr<C...>>.
     */
    struct ContainerTree {
        using TreeRef = std::unique_ptr<ContainerTree>&;
        using TreeOwned = std::unique_ptr<ContainerTree>;
        using TreePtr = ContainerTree*;

        explicit ContainerTree(std::string container_tag, geom::Geometry geometry, ContainerTree* parent, Layout layout, std::size_t height) noexcept;
        ~ContainerTree();

        std::string tag;
        std::optional<Window> client; // we either map a client, or we are "empty", in the sense that we hold two clients in left and right
        TreeOwned left, right;
        ContainerTree* parent;
        Layout policy;
        float split_ratio;
        geom::Geometry geometry;
        std::size_t m_tree_height;
        geom::Position split_position;
        [[nodiscard]] bool is_root() const;
        [[nodiscard]] bool is_split_container() const; // basically "is_branch?"
        [[nodiscard]] bool is_window() const;          // basically "is_leaf?"
        /// This is a aesthetic utility function. Returns a pair of pointers, one to the current object and one to it's parent, so that
        /// code blocks do not have to be cluttered up with setting pointers to the current and parent.
        [[nodiscard]] auto begin_bubble() -> BubbleIterator<ContainerTree>;
        [[nodiscard]] auto center_of_top() const -> geom::Position;
        [[nodiscard]] auto get_center() const -> geom::Position;

        void push_client(Window new_client);
        void update_subtree_geometry();
        void switch_layout_policy();

        void rotate_container_layout();
        void rotate_children();

        template<typename Predicate>
        friend auto tree_in_order_find(std::unique_ptr<ContainerTree>& tree, Predicate p) -> std::optional<ContainerTree*>;
        // run MapFn on each item in the tree, that is of window "type"
        template<typename MapFn>
        friend auto in_order_window_map(std::unique_ptr<ContainerTree>& tree_node, MapFn fn) -> void;
        friend void move_client(ContainerTree* from, ContainerTree* to);
        // Promote's child to parent. This function also calls update_subtree_geometry on promoted child so all it's children get proper
        // geometries
        friend auto promote_child(std::unique_ptr<ContainerTree> child) -> ContainerTree*;
        static auto make_root(const std::string& container_tag, geom::Geometry geometry, Layout layout) -> TreeOwned;

      protected:
        // Root constructor accessed from make_root
        ContainerTree(std::string container_tag, geom::Geometry geometry, Layout layout) noexcept;
    };
    using TreeRef = std::unique_ptr<ContainerTree>&;
    using TreeOwned = std::unique_ptr<ContainerTree>;
    void move_client(ContainerTree* from, ContainerTree* to);
    [[maybe_unused]] auto promote_child(std::unique_ptr<ContainerTree> child) -> ContainerTree*;
    std::unique_ptr<ContainerTree> make_tree(std::string ws_tag);

    template<typename MapFn>
    auto in_order_window_map(std::unique_ptr<ContainerTree>& tree_node, MapFn fn) -> void
    {
        if(!tree_node)
            return;
        in_order_window_map(tree_node->left, fn);
        if(tree_node->is_window())
            fn(*(tree_node->client));
        in_order_window_map(tree_node->right, fn);
    }

    template<typename Predicate>
    auto tree_in_order_find(std::unique_ptr<ContainerTree>& tree, Predicate p) -> std::optional<ContainerTree*>
    {
        if(!tree)
            return {};
        if(p(tree)) {
            cx::println("Found window!");
            return tree.get();
        } else {
            if(auto res = tree_in_order_find(tree->left, p); res)
                return res;
            if(auto res = tree_in_order_find(tree->right, p); res)
                return res;
            return {};
        }
    }

    template<typename Predicate>
    auto window_in_order_find(std::unique_ptr<ContainerTree>& tree, Predicate p) -> std::optional<workspace::Window>
    {
        if(!tree)
            return {};
        if(tree->is_window() && p(tree)) {
            cx::println("Found window!");
            return tree->client;
        } else {
            if(auto res = tree_in_order_find(tree->left, p); res)
                return res;
            if(auto res = tree_in_order_find(tree->right, p); res)
                return res;
            return {};
        }
    }

    template<typename Predicate>
    auto collect_treenodes_by(ContainerTree* tree, Predicate p)
    {
        std::vector<ContainerTree*> res{};
        auto factory = [p, &res](ContainerTree* tree) -> void {
            auto inner = [p, &res](ContainerTree* t, auto& self) mutable {
                if(!t) return;
                self(t->left.get(), self);
                if(p(t)) res.push_back(t);
                self(t->right.get(), self);
            };
            return inner(tree, inner);
        };
        factory(tree);
/*
        if(!tree)
            return;
        collect_treenodes_by(tree->left, result, p);
        if(p(tree)) {
            result.push_back(tree.get());
        }
        collect_treenodes_by(tree->right, result, p);
*/
        return res;
    }

    /// Collects all windows below root node tree, according to predicate p. If no lambda is passed in, the default predicate returns all windows
    template<typename Predicate>
    auto collect_windows_by(std::unique_ptr<ContainerTree>& tree, std::vector<Window>& result, Predicate p = [](auto& tree) { return true; })
    {
        if(!tree)
            return;
        collect_windows_by(tree->left, result, p);
        if(tree->is_window() && p(tree)) {
            result.push_back(tree->client.value());
        }
        collect_windows_by(tree->right, result, p);
    }

} // namespace cx::workspace
