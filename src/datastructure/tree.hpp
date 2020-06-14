#pragma once
#include <memory>
#include <optional>
#include <variant>

namespace cx::container
{

    struct Window {

    };
    struct SplitContainer {

    };

    struct TreeNode {
        std::variant<Window> window;
        std::unique_ptr<TreeNode> left, right;
        TreeNode* parent;
    }; 
} // namespace cx::xcontainers
