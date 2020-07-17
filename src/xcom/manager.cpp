// Library / Application headers
#include <coreutils/core.hpp>
#include <xcom/manager.hpp>

// System headers xcb
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_keysyms.h>
// STD System headers
#include <algorithm>
#include <cassert>
#include <fcntl.h>
#include <memory>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <xcom/utility/drawing/util.h>

using namespace std::literals;

namespace cx
{
    template<typename StrT>
    constexpr auto name_len(StrT str)
    {
        return std::strlen(str);
    }

#define x_replace_str_prop(c, window, atom, string)                                                                                                  \
    xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window, atom, XCB_ATOM_STRING, 8, name_len(string), string)

    auto Manager::setup() -> void
    {
        // TODO(implement): grab pre-existing windows, reparent them properly and manage them
        // xcb_grab_server(get_conn());
        setup_root_workspace_container();

        // xcb_ungrab_server(get_conn());
    }

    auto Manager::setup_root_workspace_container() -> void
    {
        // auto win_geom = xcb_get_geometry_reply(get_conn(), xcb_get_geometry(get_conn(), get_root()), nullptr);
        add_workspace("Workspace 1", 0);
        add_workspace("Workspace 2", 0);
        add_workspace("Workspace 3", 0);
        add_workspace("Workspace 4", 0);
        add_workspace("Workspace 5", 0);
        add_workspace("Workspace 6", 0);
        add_workspace("Workspace 7", 0);
        add_workspace("Workspace 8", 0);
        add_workspace("Workspace 9", 0);
        add_workspace("Workspace 10", 0);
        add_workspace("Workspace 11", 0);
        this->status_bar = ws::make_system_bar(get_conn(), get_screen(), m_workspaces.size(), geom::Geometry{0, 0, 800, 25}, configuration);
        this->focused_ws = m_workspaces[0].get();
    }

    auto Manager::initialize() -> std::unique_ptr<Manager>
    {
        int screen_number;
        x11::XCBScreen* screen = nullptr;
        x11::XCBDrawable root_drawable;
        auto c = xcb_connect(nullptr, &screen_number);
        if(xcb_connection_has_error(c)) {
            throw std::runtime_error{"XCB Error: failed trying to connect to X-server"};
        }

        auto screen_iter = xcb_setup_roots_iterator(xcb_get_setup(c));

        // Find our screen
        for(auto i = 0; i < screen_number; ++i)
            xcb_screen_next(&screen_iter);

        screen = screen_iter.data;

        if(!screen) {
            xcb_disconnect(c);
            throw std::runtime_error{"XCB Error: Failed to find screen. Disconnecting from X-server."};
        }

        root_drawable = screen->root;
        x11::XCBWindow window = screen->root;

        // Set this in pre-processor variable in CMake, to run this code
        DBGLOG("Screen size {} x {} pixels. Root window: {}", screen->width_in_pixels, screen->height_in_pixels, root_drawable);
        // TODO: remove this call to setup_mouse... completely for root?
        // x11::setup_mouse_button_request_handling(c, window);
        x11::setup_redirection_of_map_requests(c, window);
        x11::setup_key_press_listening(c, window);
        // TODO: grab keys & set up keysymbols and configs
        auto ewmh_window = xcb_generate_id(c);
        xcb_create_window(c, XCB_COPY_FROM_PARENT, ewmh_window, window, -1, -1, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT,
                          XCB_CW_OVERRIDE_REDIRECT, (uint32_t[]){1});
        auto atoms = cx::x11::get_supported_atoms(c, x11::atom_names);

        auto support_check_cookie = xcb_intern_atom(c, 0, name_len("_NET_SUPPORTING_WM_CHECK"), "_NET_SUPPORTING_WM_CHECK");
        auto wm_name_cookie = xcb_intern_atom(c, 0, name_len("_NET_WM_NAME"), "_NET_WM_NAME");

        cx::x11::X11Resource support_r = xcb_intern_atom_reply(c, support_check_cookie, nullptr);
        cx::x11::X11Resource wm_name_r = xcb_intern_atom_reply(c, wm_name_cookie, nullptr);

        auto get_atom = [c](auto atom_name) -> xcb_atom_t {
            auto cookie = xcb_intern_atom(c, 0, strlen(atom_name), atom_name);
            /* ... do other work here if possible ... */
            if(cx::x11::X11Resource reply = xcb_intern_atom_reply(c, cookie, nullptr); reply) {
                DBGLOG("The {} atom has ID {}", atom_name, reply->atom);
                auto atom = reply->atom;
                return atom;
            } else {
                DBGLOG("Failed to get atom for: {}. In debug mode we crash here.", atom_name);
                exit(-1);
            }
        };
        auto a_wm = get_atom("_NET_WM_NAME");
        auto a_supp = get_atom("_NET_SUPPORTING_WM_CHECK");

        std::vector<xcb_void_cookie_t> cookies{};

        cookies.push_back(xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, ewmh_window, a_supp, XCB_ATOM_WINDOW, 32, 1, &ewmh_window));
        cookies.push_back(x_replace_str_prop(c, ewmh_window, a_wm, "CXWMAN"));
        cookies.push_back(xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window, a_supp, XCB_ATOM_WINDOW, 32, 1, &ewmh_window));
        cookies.push_back(x_replace_str_prop(c, window, a_wm, "CXWMAN"));

        // TODO(implement): Set the supported atoms by calling change prop with _NET_SUPPORTED as the... property, XCB_ATOM_ATOM as the
        // type, and then the atoms as the data
        for(const auto& cookie : cookies) {
            if(auto err = xcb_request_check(c, cookie); err) {
                cx::println("Failed to change property of EWMH Window or Root window");
            }
        }
        cookies.clear();
        cookies.push_back(xcb_map_window_checked(c, ewmh_window));
        cookies.push_back(xcb_configure_window_checked(c, ewmh_window, XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t[]){XCB_STACK_MODE_BELOW}));
        for(const auto& cookie : cookies) {
            if(auto err = xcb_request_check(c, cookie); err) {
                cx::println("Failed to map/configure ewmh window");
            }
        }
        auto symbols = xcb_key_symbols_alloc(c);
        auto xcb_fd = xcb_get_file_descriptor(c);

        epoll_event event{};
        event.events = EPOLLET | EPOLLIN | EPOLLOUT;

        constexpr auto make_socket_non_blocking = [](auto fileDescriptor) {
            auto sock_flags = fcntl(fileDescriptor, F_GETFL);
            if(sock_flags == -1) {
                cx::println("Failed to get flags for socket {}", fileDescriptor);
                return false;
            }
            if(fcntl(fileDescriptor, F_SETFL, sock_flags | O_NONBLOCK) == -1) {
                cx::println("Failed to set socket to non-blocking");
                return false;
            }
            return true;
        };
        make_socket_non_blocking(xcb_fd);

        event.data.fd = xcb_fd;
        auto xcb_epfd = epoll_create1(0);
        epoll_ctl(xcb_epfd, EPOLL_CTL_ADD, xcb_fd, &event);

        return std::make_unique<Manager>(c, screen, root_drawable, window, ewmh_window, symbols, xcb_fd,
                                         ipc::factory::ipc_setup_unix_socket("cxwman_ipc", xcb_epfd), xcb_epfd);
    }

    // Private constructor called via public interface function Manager::initialize()
    Manager::Manager(x11::XCBConn* connection, x11::XCBScreen* screen, x11::XCBDrawable root_drawable, x11::XCBWindow root_window,
                     x11::XCBWindow ewmh_window, xcb_key_symbols_t* symbols, int xcb_fd, std::unique_ptr<ipc::IPCInterface> messenger,
                     int epoll_fd) noexcept
        : x_detail{connection, screen, root_drawable, root_window, ewmh_window, symbols, xcb_fd},
          m_running(false), client_to_frame_mapping{}, frame_to_client_mapping{}, focused_ws(nullptr), m_workspaces{}, event_dispatcher{this},
          status_bar{nullptr}, inactive_windows{1, 0xff0000}, active_windows{1, 0x00ff00}, ipc_interface{std::move(messenger)}, epoll_fd(epoll_fd),
          configuration()
    {
    }

    [[nodiscard]] inline constexpr auto Manager::get_conn() const -> x11::XCBConn* { return x_detail.c; }

    [[nodiscard]] inline constexpr auto Manager::get_root() const -> x11::XCBWindow { return x_detail.screen->root; }

    [[nodiscard]] inline constexpr auto Manager::get_screen() const -> x11::XCBScreen* { return x_detail.screen; }

    // TODO(EWMHints): Grab EWM hints & set up supported hints
    auto Manager::handle_map_request(xcb_map_request_event_t* evt) -> void
    {
        frame_window(evt->window, false);
        xcb_map_window(get_conn(), evt->window);
    }

    // FIXME: When killing clients down to only 1 client, mapping new clients fails.
    auto Manager::handle_unmap_request(xcb_unmap_window_request_t* event) -> void
    {
        DBGLOG("Handle unmap request for {}", event->window);
        auto window_container = focused_ws->find_window(event->window);
        if(window_container) {
            auto window = *window_container.value()->client;
            unframe_window(window, false);
            focused_ws->unregister_window(*window_container);
            focused_ws->display_update(get_conn());
        }
    }

    auto Manager::handle_config_request(xcb_configure_request_event_t* e) -> void
    {
        xcb_generic_error_t* err;
        uint32_t values[7], mask = 0, i = 0;
        if(client_to_frame_mapping.count(e->parent) == 1) {
            xcb_window_t frame = client_to_frame_mapping[e->window];
            DBGLOG("Handle cfg for frame {} of client {}", frame, e->window);
            if(e->value_mask & XCB_CONFIG_WINDOW_X) {
                mask |= XCB_CONFIG_WINDOW_X;
                values[i++] = e->x;
            }
            if(e->value_mask & XCB_CONFIG_WINDOW_Y) {
                mask |= XCB_CONFIG_WINDOW_Y;
                // Modify y to be within limits
                values[i++] = e->y;
            }
            if(e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
                mask |= XCB_CONFIG_WINDOW_WIDTH;
                values[i++] = e->width;
            }
            if(e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
                mask |= XCB_CONFIG_WINDOW_HEIGHT;
                values[i++] = e->height;
            }
            if(e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
                mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
                values[i++] = e->border_width;
            }
            err = xcb_request_check(get_conn(), xcb_configure_window_checked(get_conn(), frame, mask, values));
            if(err) {
                cx::println("xcb_configure_window_checked(): Error code: {}", err->error_code);
            }
        }
        mask = 0;
        i = 0;
        if(e->value_mask & XCB_CONFIG_WINDOW_X) {
            mask |= XCB_CONFIG_WINDOW_X;
            values[i++] = e->x;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_Y) {
            mask |= XCB_CONFIG_WINDOW_Y;
            values[i++] = e->y;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
            mask |= XCB_CONFIG_WINDOW_WIDTH;
            values[i++] = e->width;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
            mask |= XCB_CONFIG_WINDOW_HEIGHT;
            values[i++] = e->height;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
            mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
            values[i++] = e->border_width;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
            mask |= XCB_CONFIG_WINDOW_SIBLING;
            values[i++] = e->sibling;
        }
        if(e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
            mask |= XCB_CONFIG_WINDOW_STACK_MODE;
            values[i++] = e->stack_mode;
        }
        err = xcb_request_check(get_conn(), xcb_configure_window_checked(get_conn(), e->window, mask, values));
        if(err) {
            cx::println("xcb_configure_window_checked(): Error code: {}", err->error_code);
        }
    }

    auto Manager::handle_key_press(xcb_key_press_event_t* event) -> void
    {
        namespace xkm = xcb_key_masks;
        auto ksym = xcb_key_press_lookup_keysym(x_detail.keysyms, event, 0);
        // debug::print_modifiers(event->state);
        DBGLOG("Key code {} - KeySym: {}", event->detail, ksym);
        auto cfg = config::KeyConfiguration{ksym, event->state};
        // TODO: Is this the right way to deal with something like this? Too complex? I have no idea.
        // "Safe" event handler. Calls Manager::noop() if key-combo is not registered with a specific member function of Manager
        event_dispatcher.handle(cfg);
    }

    auto Manager::frame_window(x11::XCBWindow window, bool create_before_wm) -> void
    {
        namespace xkm = xcb_key_masks;
        std::array<xcb_void_cookie_t, 6> cookies{};
        const auto& c = get_conn();
        if(client_to_frame_mapping.count(window)) {
            DBGLOG("Framing an already framed window (id: {}) is unhandled behavior. Returning early from framing function.", window);
            return;
        }

        cx::x11::X11Resource win_attr = xcb_get_window_attributes_reply(c, xcb_get_window_attributes(c, window), nullptr);
        auto client_geometry = process_x_geometry(c, window);
        DBGLOG("Client geometry: {},{} -- {}x{}", client_geometry->x(), client_geometry->y(), client_geometry->width, client_geometry->height);
        if(create_before_wm) {
            cx::println("Window was created before WM.");
            if(win_attr->override_redirect || win_attr->map_state != XCB_MAP_STATE_VIEWABLE) {
                delete win_attr;
                return;
            }
        }

        // construct frame
        auto frame_id = xcb_generate_id(c);
        uint32_t values[3];
        /* see include/xcb.h for the FRAME_EVENT_MASK */

        uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL;
        values[0] = 0x4d4d33;

        values[1] = inactive_windows.border_color;
        mask |= XCB_CW_EVENT_MASK;
        values[2] = (cx::u32)XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_BUTTON_PRESS |
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_EXPOSURE |
                    XCB_EVENT_MASK_PROPERTY_CHANGE;
        cookies[0] = xcb_create_window_checked(c, 0, frame_id, get_root(), 0, 0, client_geometry->width, client_geometry->height,
                                               inactive_windows.border_width, XCB_WINDOW_CLASS_INPUT_OUTPUT, get_screen()->root_visual, mask, values);

        cookies[1] = xcb_reparent_window_checked(c, window, frame_id, 0, configuration.frame_title_height);
        auto tag = x11::get_client_wm_name(c, window);

        ws::Window win{client_geometry.value_or(geom::Geometry::window_default()), window, frame_id,
                       ws::Tag{tag.value_or("cxw_" + std::to_string(window)), focused_ws->m_id}, configuration};
        if(!focused_ws) {
            DBGLOG("No workspace container was created. {}!", "Error");
            std::abort();
        }
        if(auto configure_command = focused_ws->register_window(win); configure_command) {
            execute(&configure_command.value());
            cookies[2] = xcb_map_window_checked(c, frame_id);
            cookies[3] = xcb_map_subwindows_checked(c, frame_id);
            cookies[5] = xcb_grab_button_checked(c, 1, frame_id, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, get_root(),
                                                 XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_ANY);
            process_request(cookies[0], win, [](auto w) { cx::println("Failed to create X window/frame"); });
            process_request(cookies[1], win, [](auto w) { cx::println("Re-parenting window {} to frame {} failed", w.client_id, w.frame_id); });
            process_request(cookies[2], win, [](auto w) { cx::println("Failed to map frame {}", w.frame_id); });
            process_request(cookies[3], win, [](auto w) { cx::println("Failed to map sub-windows of frame {} -> {}", w.frame_id, w.client_id); });
            process_request(cookies[4], win, [](auto w) { cx::println("Failed button grab on window {}", w.client_id); });
            process_request(cookies[5], win, [](auto w) { cx::println("Failed button grab on frame {}", w.frame_id); });
        } else {
            cx::println("FOUND NO LAYOUT ATTRIBUTES!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        }

        auto font_gc = x11::get_font_gc(c, frame_id, 0x000000, (u32)configuration.frame_background_color, "7x13");
        auto text_extents_cookie = xcb_query_text_extents(c, font_gc.value(), tag->length(), reinterpret_cast<const xcb_char2b_t*>(tag->c_str()));
        cx::x11::X11Resource text_extents = xcb_query_text_extents_reply(c, text_extents_cookie, nullptr);
        auto [x_pos, y_pos] = cx::draw::utils::align_vertical_middle_left_of(text_extents, client_geometry->width, configuration.frame_title_height);
        xcb_image_text_8(c, tag->length(), frame_id, font_gc.value(), x_pos, y_pos, tag->c_str());
        int v[1]{XCB_EVENT_MASK_PROPERTY_CHANGE};
        xcb_change_window_attributes(c, window, XCB_CW_EVENT_MASK, v);

        client_to_frame_mapping[window] = frame_id;
        frame_to_client_mapping[frame_id] = window;
        // x11::setup_mouse_button_request_handling(c, window);
    }

    auto Manager::unframe_window(const ws::Window& w, bool destroy_client) -> void
    {
        xcb_unmap_window(get_conn(), w.frame_id);
        xcb_reparent_window(get_conn(), w.client_id, get_root(), 0, 0);
        xcb_destroy_window(get_conn(), w.frame_id);
        if(destroy_client)
            xcb_destroy_window(get_conn(), w.client_id);
        xcb_flush(get_conn());
        client_to_frame_mapping.erase(w.client_id);
        frame_to_client_mapping.erase(w.frame_id);
    }

    // The event loop
    auto Manager::event_loop() -> void
    {
        setup();
        setup_input_functions();
        this->m_running = true;
        const auto& c = get_conn();
        auto xfd = xcb_get_file_descriptor(c);
        while(m_running) {
            xcb_allow_events(c, XCB_ALLOW_REPLAY_POINTER, XCB_CURRENT_TIME);
            auto ev = xcb_poll_for_event(c);
            if(ev == nullptr) {
                epoll_event event_list[10];
                auto event_count = epoll_wait(this->epoll_fd, event_list, 10, -1);
                if(event_count == -1) {
                    cx::println("Epoll error. Abort. Abort. Abort");
                    m_running = false;
                } else {
                    for(auto index = 0; index < event_count; index++) {
                        if(event_list[index].data.fd != xfd) { // we let our poll in the top of the while loop handle it in next iteration
                            if(event_list[index].events & EPOLLRDHUP) {
                                ipc_interface->drop_client(event_list[index].data.fd);
                            } else {
                                handle_file_descriptor_event(event_list[index].data.fd);
                            }
                        }
                    }
                }
            } else {
                handle_generic_event(ev);
                delete ev;
            }
        }
    }

    auto Manager::handle_file_descriptor_event(int fd) -> void
    {
        if(ipc_interface->is_connection_request(fd)) {
            ipc_interface->handle_incoming_connection();
        } else {
            ipc_interface->read_from_input(fd);
        }
    }

    auto Manager::handle_generic_event(xcb_generic_event_t* evt) -> void
    {
        switch(evt->response_type /*& ~0x80 = 127 = 0b01111111*/) {
        case XCB_MAP_REQUEST: {
            handle_map_request((xcb_map_request_event_t*)(evt));
            break;
        }
        case XCB_MAP_NOTIFY: {
            break;
        }
        case XCB_UNMAP_NOTIFY:
            handle_unmap_request((xcb_unmap_window_request_t*)evt);
            break;
        case XCB_CONFIGURE_REQUEST: {
            handle_config_request((xcb_configure_request_event_t*)evt);
            break;
        }
        case XCB_BUTTON_PRESS: {

            auto e = (xcb_button_press_event_t*)evt;
            auto id = (e->event == x_detail.root_window) ? e->child : e->event;
            if(auto cmd = focused_ws->focus_client_with_xid(id); cmd) {
                // if we didn't click any client handled by focused_ws, check if we clicked the sys bar
                execute(&cmd.value());
            } else {
                status_bar->clicked_workspace(id, [this](auto workspace_id) { this->change_workspace(workspace_id); });
            }
            break;
        }
        case XCB_BUTTON_RELEASE:
            break;
        case XCB_KEY_PRESS:
            handle_key_press((xcb_key_press_event_t*)evt);
            break;
        case XCB_MAPPING_NOTIFY: // alerts us if a *key mapping* has been done, NOT a window one
            break;
        case XCB_MOTION_NOTIFY: // We just fall through all these for now, since we don't do anything right now anyway
            break;
        case XCB_CLIENT_MESSAGE: // TODO(implement) XCB_CLIENT_MESSAGE:
            break;
        case XCB_CONFIGURE_NOTIFY: // TODO(implement) XCB_CONFIGURE_NOTIFY
            break;
        case XCB_KEY_RELEASE: // TODO(implement)? XCB_KEY_RELEASE
            break;
        case XCB_EXPOSE: {
            auto e = (xcb_expose_event_t*)evt;
            if(status_bar->has_child(e->window)) {
                status_bar->draw();
            } else {
                handle_expose_event(e);
            }
            break; // TODO(implement) XCB_EXPOSE
        }
        case XCB_PROPERTY_NOTIFY: {
            auto e = (xcb_property_notify_event_t*)evt;
            if(e->atom == XCB_ATOM_WM_NAME) {
                auto c = get_conn();
                focused_ws->find_window_then(e->window, [c](auto& window) { window.draw_title(c, x11::get_client_wm_name(c, window.client_id)); });
            }
            break;
        }
        }
    }

    auto Manager::add_workspace(const std::string& workspace_tag, std::size_t screen_number) -> void
    {
        auto status_bar_height = 25;
        // FIXME: This has hardcoded screen width by height size, as during testing we know. This OBVIOUSLY has to be fixed so that correct size
        //  settings get passsed
        m_workspaces.emplace_back(
            std::make_unique<ws::Workspace>(m_workspaces.size(), workspace_tag, geom::Geometry{0, status_bar_height, 800, 600 - status_bar_height}));
    }
    auto Manager::setup_input_functions() -> void
    {
        namespace xkm = xcb_key_masks;
        using KC = cx::config::KeyConfiguration;
        using Arg = cx::events::EventArg;
        using cx::events::Dir;
        using ResizeArg = cx::events::ResizeArgument;

        event_dispatcher.register_action(KC{XK_F4, xkm::SUPER_SHIFT}, &Manager::rotate_focused_layout);
        event_dispatcher.register_action(KC{XK_r, xkm::SUPER_SHIFT}, &Manager::rotate_focused_pair);

        event_dispatcher.register_action(KC{XK_Left, xkm::SUPER}, &Manager::move_focused, Arg{Dir::LEFT});
        event_dispatcher.register_action(KC{XK_Right, xkm::SUPER}, &Manager::move_focused, Arg{Dir::RIGHT});
        event_dispatcher.register_action(KC{XK_Up, xkm::SUPER}, &Manager::move_focused, Arg{Dir::UP});
        event_dispatcher.register_action(KC{XK_Down, xkm::SUPER}, &Manager::move_focused, Arg{Dir::DOWN});

        event_dispatcher.register_action(KC{XK_Left, xkm::SUPER_SHIFT}, &Manager::increase_size_focused, Arg{ResizeArg{Dir::LEFT, 10}});
        event_dispatcher.register_action(KC{XK_Right, xkm::SUPER_SHIFT}, &Manager::increase_size_focused, Arg{ResizeArg{Dir::RIGHT, 10}});
        event_dispatcher.register_action(KC{XK_Up, xkm::SUPER_SHIFT}, &Manager::increase_size_focused, Arg{ResizeArg{Dir::UP, 10}});
        event_dispatcher.register_action(KC{XK_Down, xkm::SUPER_SHIFT}, &Manager::increase_size_focused, Arg{ResizeArg{Dir::DOWN, 10}});

        event_dispatcher.register_action(KC{XK_Left, xkm::SUPER_CTRL}, &Manager::decrease_size_focused, Arg{ResizeArg{Dir::RIGHT, 10}});
        event_dispatcher.register_action(KC{XK_Right, xkm::SUPER_CTRL}, &Manager::decrease_size_focused, Arg{ResizeArg{Dir::LEFT, 10}});
        event_dispatcher.register_action(KC{XK_Up, xkm::SUPER_CTRL}, &Manager::decrease_size_focused, Arg{ResizeArg{Dir::DOWN, 10}});
        event_dispatcher.register_action(KC{XK_Down, xkm::SUPER_CTRL}, &Manager::decrease_size_focused, Arg{ResizeArg{Dir::UP, 10}});
        event_dispatcher.register_action(KC{XK_q, xkm::SUPER_SHIFT}, &Manager::kill_client, Arg{std::nullopt});
    }

    // Manager window/client actions
    auto Manager::rotate_focused_layout() -> void
    {
        focused_ws->rotate_focus_layout();
        focused_ws->display_update(get_conn());
    }

    auto Manager::rotate_focused_pair() -> void
    {
        focused_ws->rotate_focus_pair();
        focused_ws->display_update(get_conn());
    }
    auto Manager::noop() -> void { cx::println("Key combination not yet handled"); }

    auto Manager::move_focused(cx::events::EventArg arg) -> void
    {
        auto cmd_arg = std::get<geom::ScreenSpaceDirection>(arg.arg);
        auto move_command = focused_ws->move_focused(cmd_arg);
        execute(&move_command);
    }
    auto Manager::increase_size_focused(cx::events::EventArg arg) -> void
    {
        auto resize_arg = std::get<cx::events::ResizeArgument>(arg.arg);
        auto cmd = focused_ws->increase_size_focused(resize_arg);
        execute(&cmd);
    }
    auto Manager::decrease_size_focused(cx::events::EventArg arg) -> void
    {
        auto size_arg = std::get<cx::events::ResizeArgument>(arg.arg);
        auto cmd = focused_ws->decrease_size_focused(size_arg);
        execute(&cmd);
    }

    auto Manager::change_workspace(std::size_t ws_id) -> void
    {
        if(ws_id < m_workspaces.size()) {
            auto c = get_conn();
            focused_ws->unmap_workspace([&c](xcb_window_t window) { xcb_unmap_window(c, window); });
            focused_ws = m_workspaces[ws_id].get();
            focused_ws->map_workspace([&c](xcb_window_t window) { xcb_map_window(c, window); });
            xcb_flush(get_conn());
        } else {
            cx::println("There is no workspace with id {}", ws_id);
        }
    }
    auto Manager::kill_client(cx::events::EventArg arg) -> void
    {
        auto focused_client = focused_ws->focused().client->client_id;
        auto cookie = xcb_kill_client_checked(get_conn(), focused_client);
        if(auto err = xcb_request_check(get_conn(), cookie); err) {
            cx::println("Failed to kill client");
        }
    }
    void Manager::execute(commands::ManagerCommand* cmd)
    {
        cx::println("Executing command {}", cmd->command_name());
        cmd->request_state(this);
        cmd->perform(get_conn());
    }
    ws::Window Manager::focused_window() const { return focused_ws->focused().client.value(); }
    const cfg::Configuration& Manager::get_config() const { return configuration; }
    void Manager::handle_expose_event(xcb_expose_event_t* pEvent)
    {
        const auto& c = get_conn();
        if(auto con = focused_ws->find_window(pEvent->window); con) {
            auto tag = x11::get_client_wm_name(c, con.value()->client.value().client_id);
            con.value()->client.value().m_tag.m_tag = tag.value();
            auto window = con.value()->client.value();
            auto font_gc = x11::get_font_gc(c, pEvent->window, 0x000000, (u32)configuration.frame_background_color, "7x13");
            auto text_extents_cookie = xcb_query_text_extents(c, font_gc.value(), window.m_tag.m_tag.length(),
                                                              reinterpret_cast<const xcb_char2b_t*>(window.m_tag.m_tag.c_str()));
            cx::x11::X11Resource text_extents = xcb_query_text_extents_reply(c, text_extents_cookie, nullptr);
            auto [x_pos, y_pos] =
                cx::draw::utils::align_vertical_middle_left_of(text_extents, window.geometry.width, configuration.frame_title_height);
            auto cookie_text =
                xcb_image_text_8_checked(c, window.m_tag.m_tag.length(), window.frame_id, font_gc.value(), x_pos, y_pos, window.m_tag.m_tag.c_str());

            if(auto error = xcb_request_check(c, cookie_text); error) {
                cx::println("Could not draw text '{}' in window {}", window.m_tag.m_tag, window.frame_id);
            }
        }
    }

    auto process_x_geometry(xcb_connection_t* c, xcb_window_t window) -> std::optional<geom::Geometry>
    {
        if(cx::x11::X11Resource reply = xcb_get_geometry_reply(c, xcb_get_geometry(c, window), nullptr); reply) {
            auto res = geom::Geometry{static_cast<geom::GU>(reply->x), static_cast<geom::GU>(reply->y), static_cast<geom::GU>(reply->width),
                                      static_cast<geom::GU>(reply->height)};
            return res;
        } else {
            return {};
        }
    }
} // namespace cx
