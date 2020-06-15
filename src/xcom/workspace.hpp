#pragma once
// System headers
#include <string>
#include <vector>
// 3rd party headers

// Library/Application headers
#include <coreutils/core.hpp>
#include <datastructure/geometry.hpp>

#include <xcom/window.hpp>
#include <xcom/container.hpp>
#include <xcom/constants.hpp>

namespace cx::workspace
{

    struct SplitConfigurations {
        SplitConfigurations() : existing_window{}, new_window{} {
            existing_window.reset();
            new_window.reset();
        }
        SplitConfigurations(Window existing, Window new_window) : existing_window(existing), new_window(new_window) {}
        SplitConfigurations(Window new_window) : existing_window{}, new_window{new_window} {
            existing_window.reset();
        }
        std::optional<Window> existing_window;
        std::optional<Window> new_window;
    };

    struct Workspace {
        // Constructors & initializers
        Workspace(cx::uint ws_id, std::string ws_name, cx::geom::Geometry space);
        // This destructor has to be handled... very well defined. When we throw away a workspace, where will the windows end up?
        ~Workspace();

        // Members private & public
        cx::uint m_id;
        std::string m_name;
        cx::geom::Geometry m_space;
        bool is_pristine;
        std::unique_ptr<ContainerTree> m_containers;
        std::vector<Window> m_floating_containers;
        std::unique_ptr<ContainerTree> m_root;
        ContainerTree* focused_container;


        /*  Public & Private Interface API */

        /** 
         * Returns geometry to the manager where we have stored this client, and where it should be mapped to. Mapping is still handled by the manager
         * not the individual workspace
         */
        auto register_window(Window w, bool tiled=true) -> std::optional<SplitConfigurations>;
        auto unregister_window(Window* w) -> std::unique_ptr<Window>;
        auto find_window(xcb_window_t xwin) -> std::optional<Window>;
        auto display_update(xcb_connection_t* c) -> void;

        void rotate_focus_layout(); // rotates the focused pair's layout
        void rotate_focus_pair();   // rotates the focused pair's positions
    };
} // namespace cx::workspace
