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
        SplitConfigurations() noexcept : existing_window{}, new_window{}
        {
            existing_window.reset();
            new_window.reset();
        }
        SplitConfigurations(Window existing, Window new_window) noexcept : existing_window(existing), new_window(new_window) {}
        explicit SplitConfigurations(Window new_window) noexcept : existing_window{}, new_window{new_window} { existing_window.reset(); }
        std::optional<Window> existing_window;
        std::optional<Window> new_window;
    };
    using Pos = geom::Position;
    struct Workspace {
        using TreeOwned = ContainerTree::TreeOwned;
        // Constructors & initializers
        Workspace(cx::uint ws_id, std::string ws_name, cx::geom::Geometry space) noexcept;
        // This destructor has to be handled... very well defined. When we throw away a workspace, where will the windows end up?
        Workspace(Workspace&&) noexcept = default;
        ~Workspace() noexcept = default;

        // Members private & public
        cx::uint m_id;
        std::string m_name;
        cx::geom::Geometry m_space;
        bool is_pristine;
        std::unique_ptr<ContainerTree> m_containers;
        std::vector<Window> m_floating_containers;
        std::unique_ptr<ContainerTree> m_root;
        ContainerTree* foc_con;
        [[nodiscard]] constexpr inline auto& focused() const { return *foc_con; }

        /**
         * Returns geometry to the manager where we have stored this client, and where it should be mapped to. Mapping is still handled by
         * the manager not the individual workspace
         */
        auto register_window(Window w, bool tiled = true) -> std::optional<SplitConfigurations>;
        auto unregister_window(ContainerTree* t) -> void;

        /// Searches the ContainerTree in order, for a window with the id of xwin
        auto find_window(xcb_window_t xwin) -> std::optional<ContainerTree*>;
        /// Traverses the ContainerTree for this workspace in order, and calls xcb_configure for each window with
        /// the properties stored in each ws::Window, updating the display so that any and all changes made, will show up on screen
        auto display_update(xcb_connection_t* c) -> void;
        /// rotates the focused client tile-pair layouts
        void rotate_focus_layout() const;
        /// rotates the focused client tile-pair positions
        void rotate_focus_pair() const;
        // This moves this window from it's anchor, in vector's dir.
        void move_focused(cx::events::ScreenSpaceDirection dir);
        /// Increases width or height of window, in all four directions, depending on the parameter arg
        void increase_size_focused(cx::events::ResizeArgument arg);
        /// Decreases width or height of window, in all four directions, depending on the parameter arg
        [[maybe_unused]] void decrease_size_focused(cx::events::ResizeArgument arg);
        // Depending if sp_dir is negative or positive, determines what direction (left/right) the width will be increased to
        template<typename Predicate>
        void increase_width(int sp_dir, Predicate child_of);

        template<typename Predicate>
        void increase_height(int sp_dir, Predicate child_of);

        template<typename Predicate>
        void decrease_height(int sp_dir, Predicate child_of);

        template<typename Predicate>
        void decrease_width(int sp_dir, Predicate child_of);

        bool focus_client_with_xid(const xcb_window_t xwin);

        // This gets all clients as a vector of references (not the v/h split containers that is)
        template<typename P>
        [[maybe_unused]] std::vector<ContainerTree*> get_clients(P p = is_window_predicate);

        void anchor_new_root(TreeOwned new_root, const std::string& tag);

        template <typename XCBUnMapFn>
        void unmap_workspace(XCBUnMapFn fn)  {
            in_order_window_map(m_root, [fn](auto window) {
              DBGLOG("Unmapping window {}", "");
                fn(window.frame_id);
            });
        }

        template <typename XCBMapFn>
        void map_workspace(XCBMapFn fn)  {
            in_order_window_map(m_root, [fn](auto window) {
              DBGLOG("Mapping window {}", "");
              fn(window.frame_id);
            });
        }
    };
} // namespace cx::workspace
