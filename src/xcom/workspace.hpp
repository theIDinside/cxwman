#pragma once
// System headers
#include <string>
#include <vector>
// 3rd party headers

// Library/Application headers
#include <coreutils/core.hpp>
#include <datastructure/container.hpp>
#include <datastructure/geometry.hpp>

#include <xcom/constants.hpp>
#include <xcom/window.hpp>

namespace cx::workspace
{

    constexpr auto is_window_predicate = [](ContainerTree* t) { return t->is_window(); };

    struct SplitConfigurations {
        SplitConfigurations() : existing_window{}, new_window{}
        {
            existing_window.reset();
            new_window.reset();
        }
        SplitConfigurations(Window existing, Window new_window) : existing_window(existing), new_window(new_window) {}
        SplitConfigurations(Window new_window) : existing_window{}, new_window{new_window} { existing_window.reset(); }
        std::optional<Window> existing_window;
        std::optional<Window> new_window;
    };
    using Pos = geom::Position;
    struct Workspace {
        using TreeOwned = ContainerTree::TreeOwned;
        // Constructors & initializers
        Workspace(cx::uint ws_id, std::string ws_name, cx::geom::Geometry space);
        // This destructor has to be handled... very well defined. When we throw away a workspace, where will the windows end up?
        Workspace(Workspace&&) = default;
        ~Workspace() = default;

        // Members private & public
        cx::uint m_id;
        std::string m_name;
        cx::geom::Geometry m_space;
        bool is_pristine;
        std::unique_ptr<ContainerTree> m_containers;
        std::vector<Window> m_floating_containers;
        std::unique_ptr<ContainerTree> m_root;
        ContainerTree* foc_con;
        constexpr inline auto& focused() { return *foc_con; }

        /**
         * Returns geometry to the manager where we have stored this client, and where it should be mapped to. Mapping is still handled by
         * the manager not the individual workspace
         */
        auto register_window(Window w, bool tiled = true) -> std::optional<SplitConfigurations>;
        auto unregister_window(ContainerTree* t) -> void;

        /// Searches the ContainerTree in order, for a window with the id of xwin
        auto find_window(xcb_window_t xwin) -> std::optional<ContainerTree*>;
        /// Traverses the ContainerTree for this workspace in order, and calls xcb_configure for each window with
        /// the properties stored in each ws::Window.
        auto display_update(xcb_connection_t* c) -> void;

        /// rotates the focused client tile-pair layouts
        void rotate_focus_layout();
        /// rotates the focused client tile-pair positions
        void rotate_focus_pair();

        // This moves this window from it's anchor, in vector's dir.
        void move_focused(cx::events::ScreenSpaceDirection dir);

        void increase_size_focused(cx::events::ResizeArgument arg);
        void decrease_size_focused(cx::events::ScreenSpaceDirection dir, int steps);


        // Depending if sp_dir is negative or positive, determines what direction (left/right) the width will be increased to
        template <typename Predicate>
        void increase_width(int sp_dir, Predicate child_of);




        void focus_client(xcb_window_t xwin);

        // This gets all clients as a vector of references (not the v/h split containers that is)

        template<typename P>
        std::vector<ContainerTree*> get_clients(P p = is_window_predicate);

        void anchor_new_root(TreeOwned new_root, const std::string& tag);
    };
} // namespace cx::workspace
