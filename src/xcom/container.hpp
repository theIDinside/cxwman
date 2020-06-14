#pragma once
#include <optional>
#include <memory>
#include <string>

#include <xcom/window.hpp>


namespace cx::workspace
{

    enum class Layout {
        Vertical,
        Horizontal,
        Floating
    };

    struct ContainerTree {

        explicit ContainerTree(std::string container_tag, geom::Geometry geometry);
        ~ContainerTree();

        std::string tag;
        std::optional<Window> client; // we either map a client, or we are "empty", in the sense that we hold two clients in left and right
        std::unique_ptr<ContainerTree> left, right;
        ContainerTree* parent;
        Layout policy;
        float split_ratio;
        geom::Geometry geometry;

        void register_window(Window w);

        bool is_split_container() const;        // basically "is_branch?"
        bool is_window() const;                 // basically "is_leaf?"

        void push_client(Window new_client);
    };

    std::unique_ptr<ContainerTree> make_tree(std::string ws_tag);
} // namespace cx::workspace
