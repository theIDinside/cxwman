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
#include <xcom/utility/key_config.hpp>
#include <xcom/window.hpp>
/// Forward declarations... oh how absolutely bat shit horrendous C++ is in this regard

namespace cx::workspace
{
    class ContainerTree;
    class Workspace;
} // namespace cx::workspace

namespace cx {
    class Manager;
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
        virtual void request_state(Manager* m) = 0;
      protected:
        std::string_view cmd_name;
    };

    class WindowCommand : public ManagerCommand
    {
      protected:
        ws::Window window; /// The window which the command is to be acted on (i.e. the focused window)

      public:
        explicit WindowCommand(ws::Window win, std::string_view cmd_name) noexcept : ManagerCommand(cmd_name), window(std::move(win)) {}
        virtual ~WindowCommand() noexcept = default;
    };

    class FocusWindow : public WindowCommand
    {
      public:
        FocusWindow(ws::Window activated_window) noexcept
            : WindowCommand(std::move(activated_window), "Focus Window"), defocused_window{}, acol(0),
              icol(0)
        {
        }
        ~FocusWindow() noexcept override = default;
        /// x_windows is populated by whatever can accept a command, so the command is defined, but what it should operate on is passed in as
        /// parameter
        void perform(xcb_connection_t* c) const override;
        void request_state(Manager* m) override;
        void set_defocused(ws::Window w);
      private:
        /// Activated/focused color and inactivated color
        std::optional<ws::Window> defocused_window;
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
        void request_state(Manager* m) override;
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

    class KillClientsByTag : public ManagerCommand
    {
      public:
        explicit KillClientsByTag(std::string tag) noexcept : ManagerCommand{"Kill clients by tag"} {}
        ~KillClientsByTag() override = default;
        void perform(xcb_connection_t* c) const override;
      private:
    };

    class MoveWindow : public WindowCommand
    {
      public:
        MoveWindow(ws::Window focused_window, geom::ScreenSpaceDirection dir, ws::Workspace* ws)
            : WindowCommand(std::move(focused_window), "Move Window"), direction(dir), workspace{ws}
        {
        }
        ~MoveWindow() override = default;
        void perform(xcb_connection_t* c) const override;
        void request_state(Manager* m) override;

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
        void request_state(Manager* m) override;

      private:
        std::vector<ws::Window> windows;
    };
} // namespace cx::commands
