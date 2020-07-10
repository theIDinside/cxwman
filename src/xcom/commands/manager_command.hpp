//
// Created by cx on 2020-07-08.
//

#pragma once
#include <algorithm>
#include <coreutils/core.hpp>
#include <stack>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include <xcb/xcb.h>
#include <xcom/events.hpp>
#include <xcom/utility/config.hpp>
#include <xcom/window.hpp>
/// Forward declarations... oh how absolutely bat shit horrendous C++ is in this regard

namespace cx::workspace
{
    class ContainerTree;
    class Workspace;
}

namespace cx::commands
{
    namespace ws = cx::workspace;

    class ManagerCommand
    {
      public:
        ManagerCommand(std::string_view command_name) : cmd_name(command_name) {}
        virtual ~ManagerCommand() = default;
        [[nodiscard]] std::string_view command_name() const { return cmd_name; }
        virtual void perform(xcb_connection_t* c) const = 0;

      protected:
        std::string_view cmd_name;
    };

    class WindowCommand : public ManagerCommand
    {
      protected:
        /*
         * TODO: Maybe move this into a static global somewhere that gets initialized at start up? When I did that, I just seemed to encounter more
         *  problems with the pointer not living across T.Us. properly and the application would segfault. It would have to be "extern", but once
         *  again, introduces things I don't see a point in having for the convenience of not having to pass around this pointer to the connection
         */
        ws::Window window;

      public:
        explicit WindowCommand(ws::Window win, std::string_view cmd_name) noexcept : ManagerCommand(cmd_name), window(std::move(win)) {}
        virtual ~WindowCommand() noexcept = default;
    };

    class FocusWindow : public WindowCommand
    {
      public:
        FocusWindow(ws::Window activated_window, ws::Window deactivated_window, int active_color, int inactive_color) noexcept
            : WindowCommand(std::move(activated_window), "Focus Window"), defocused_window{std::move(deactivated_window)}, acol(active_color),
              icol(inactive_color)
        {
        }
        ~FocusWindow() noexcept override = default;
        /// x_windows is populated by whatever can accept a command, so the command is defined, but what it should operate on is passed in as
        /// parameter
        void perform(xcb_connection_t* c) const override;

      private:
        /// Activated/focused color and inactivated color
        ws::Window defocused_window;
        int acol, icol;
    };

    class ChangeWorkspace : public ManagerCommand
    {
      public:
        ~ChangeWorkspace() override = default;
        void perform(xcb_connection_t* c) const override;

      private:
        std::size_t from_workspace, to_workspace;
    };

    class ConfigureWindows : public WindowCommand
    {
      public:
        explicit ConfigureWindows(ws::Window window) noexcept : WindowCommand(std::move(window), "Configure Window (1)"), existing_window{} {}
        ConfigureWindows(ws::Window existing, ws::Window new_window) noexcept
            : WindowCommand(std::move(new_window), "Configure Windows (2)"), existing_window(existing)
        {
        }
        ~ConfigureWindows() override = default;
        void perform(xcb_connection_t* c) const override;
        void set_some_value(int j); /// This way we can set arbitrary values when we are not accessing this behind a WindowCommand*
      private:
        std::optional<ws::Window> existing_window;
    };

    class KillClient : public WindowCommand
    {
      public:
        explicit KillClient(ws::Window w) noexcept : WindowCommand{std::move(w), "Kill client"} {}
        ~KillClient() override = default;
        void perform(xcb_connection_t* c) const override;

      private:
    };

    class MoveWindow : public WindowCommand {
      public:
        MoveWindow(ws::Window focused_window, geom::ScreenSpaceDirection dir, ws::Workspace* ws) : WindowCommand(std::move(focused_window), "Move Window"), direction(dir), workspace{ws} {}
        ~MoveWindow() override = default;
        void perform(xcb_connection_t* c) const override;

      private:
        geom::ScreenSpaceDirection direction;
        ws::Workspace* workspace; /// Workspace where focused_window & window_swap_with exists
    };

    class UpdateWindows : public ManagerCommand
    {
      public:
        explicit UpdateWindows(std::vector<ws::Window> windows_update) noexcept
            : ManagerCommand("Display update windows"), windows{std::move(windows_update)}
        {
        }
        explicit UpdateWindows(const std::vector<ws::ContainerTree*>& nodes) noexcept;
        ~UpdateWindows() override = default;
        void perform(xcb_connection_t* c) const override;

      private:
        std::vector<ws::Window> windows;
    };

    struct CommandBuilder {
        CommandBuilder() = default;
        std::stack<config::KeyConfiguration> key_presses;
    };

} // namespace cx::commands
