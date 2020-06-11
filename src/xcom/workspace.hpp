#pragma once
// System headers
#include <string>
#include <vector>
// 3rd party headers

// Library/Application headers
#include <coreutils/core.hpp>
#include <datastructure/geometry.hpp>
#include <xcom/window.hpp>

namespace cx::workspace
{
    struct Workspace {
        Workspace();

        // This destructor has to be handled... very well defined. When we throw away a workspace, where will the windows end up?
        ~Workspace();

        cx::geom::Geometry m_space;
        std::string m_name;
        cx::uint m_id;

        void register_window(Window *w);

        std::vector<Window> m_windows;
    };
} // namespace cx::workspace
