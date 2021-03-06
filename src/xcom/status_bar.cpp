//
// Created by cx on 2020-06-26.
//

#include "status_bar.hpp"
#include "configuration.hpp"
#include <xcom/utility/drawing/util.h>
#include <xcom/utility/raii.hpp>
namespace cx::workspace
{
    StatusBar::StatusBar(xcb_connection_t* c, xcb_window_t assigned_id, geom::Geometry assigned_geometry,
                         std::vector<std::unique_ptr<WorkspaceBox>> boxes, xcb_gcontext_t active, xcb_gcontext_t inactive)
        : geometry(assigned_geometry), drawable(assigned_id), items{}, active_workspace{0}, gc_active{active}, gc_inactive{inactive}, c(c)
    {
        for(auto&& item : boxes) {
            items.emplace(item->button_id, std::move(item));
        }
    }
    void StatusBar::draw(std::size_t active_workspace)
    {
        for(const auto& [k, v] : items)
            v->draw(c);
    }
    void StatusBar::set_active(std::size_t item_index)
    {
        for(auto& [k, v] : items) {
            if(v->workspace_id == item_index)
                v->draw_props = gc_active;
            else
                v->draw_props = gc_inactive;
        }
    }
    void StatusBar::update() {}
    bool StatusBar::has_child(xcb_window_t window) { return items.count(window) > 0; }

    void WorkspaceBox::draw(xcb_connection_t* c)
    {
        local_persist auto box_width = 25;
        local_persist auto box_height = 25;
        // get graphics context
        // set graphics context according to values of this workspace box
        // draw graphics
        // close graphics context
        auto label = std::to_string(workspace_id);
        auto extents_cookie = xcb_query_text_extents(c, draw_props, label.length(), reinterpret_cast<const xcb_char2b_t*>(label.c_str()));
        cx::x11::X11Resource txt_extents = xcb_query_text_extents_reply(c, extents_cookie, nullptr);
        if(txt_extents) {
            auto [x_pos, y_pos] = cx::draw::utils::align_text_center_of(txt_extents, box_width, box_height);
            auto cookie_text = xcb_image_text_8_checked(c, label.length(), button_id, draw_props, x_pos, y_pos, label.c_str());
            if(auto error = xcb_request_check(c, cookie_text); error) {
                cx::println("Could not draw text '{}' in window {}", label, this->button_id);
            }
        } else {
            auto cookie_text = xcb_image_text_8_checked(c, label.length(), this->button_id, this->draw_props, 0, 10, label.c_str());
            if(auto error = xcb_request_check(c, cookie_text); error) {
                cx::println("Could not draw text '{}' in window {}", label, this->button_id);
            }
        }
    }

    WorkspaceBox::WorkspaceBox(cx::uint ws_id, xcb_drawable_t xid, geom::Geometry geometry)
        : workspace_id{ws_id}, button_id{xid}, dimension{geometry}, state{ItemState::INACTIVE}, draw_props{}
    {
    }

    SysBar make_system_bar(xcb_connection_t* c, xcb_screen_t* screen, std::size_t workspace_count, geom::Geometry sys_bar_geometry,
                           const cx::cfg::Configuration& wmcfg)
    {
        auto sys_bar_id = xcb_generate_id(c);
        const auto& [x, y, width, height] = sys_bar_geometry.xcb_value_list();
        uint32_t values[3];


        uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL;
        values[0] = wmcfg.status_bar_background_color;
        values[1] = 0xff12ab;
        mask |= XCB_CW_EVENT_MASK;
        // We want to set OVERRIDE_REDIRECT, as we don't actually want to handle framing of the Status bar or it's subwindows (which in this case are
        // buttons)
        values[2] = (cx::u32)XCB_EVENT_MASK_KEY_PRESS | XCB_CW_OVERRIDE_REDIRECT | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_ENTER_WINDOW |
                    XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_EXPOSURE;
        auto ck_sysbar_create = xcb_create_window_checked(c, XCB_COPY_FROM_PARENT, sys_bar_id, screen->root, x, y, width, height, 1,
                                                          XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
        if(auto err = xcb_request_check(c, ck_sysbar_create); err) {
            cx::println("Failed to create StatusBar window");
        }
        auto ck_sysbar_map = xcb_map_window_checked(c, sys_bar_id);
        if(auto err = xcb_request_check(c, ck_sysbar_map); err) {
            cx::println("Failed to map StatusBar window");
        }

        std::vector<WBox> workspace_boxes{};
        auto green = 0x00ff00;
        auto blue = 0x0000ff;

        auto active_draw_prop = x11::get_font_gc(c, sys_bar_id, 0x000000, (u32)green, "7x13");
        auto inactive_drawprop = x11::get_font_gc(c, sys_bar_id, 0x000000, (u32)blue, "7x13");
        auto x_anchor = 0;

        auto box_masks = XCB_CW_BACK_PIXEL | mask;
        u32 inactive_box_values[]{(u32)blue, values[1], values[2]};
        u32 active_box_values[]{(u32)green, values[1], values[2]};
        xcb_window_t awin;
        for(auto i = 0; i < workspace_count; i++) {
            auto id = xcb_generate_id(c);
            if(i == 0)
                awin = id;
            auto border_width = 1;
            x_anchor = (i * 25) + border_width;
            geom::Geometry wsb_geom{x_anchor, 0, 25, 25};
            const auto& [x, y, w, h] = wsb_geom.xcb_value_list();
            auto create_cookie =
                xcb_create_window_checked(c, XCB_COPY_FROM_PARENT, id, sys_bar_id, x, y, w, h, border_width, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                          screen->root_visual, box_masks, (i == 0) ? active_box_values : inactive_box_values);
            auto map_cookie = xcb_map_window_checked(c, id);
            auto no_error_found = true;
            if(auto err = xcb_request_check(c, create_cookie); err) {
                cx::println("Failed to create workspace box item {}", i);
                no_error_found = false;
            }
            if(auto err = xcb_request_check(c, map_cookie); err) {
                cx::println("Failed to map workspace box item {}", i);
                no_error_found = false;
            }
            if(no_error_found) {
                auto wsb = std::make_unique<WorkspaceBox>(i, id, wsb_geom);
                if(i == 0)
                    wsb->draw_props = active_draw_prop.value();
                else
                    wsb->draw_props = inactive_drawprop.value();
                workspace_boxes.push_back(std::move(wsb));
            } else {
                cx::println("Due to X error, workspace box item was not created");
            }
        }
        auto sbar = std::make_unique<StatusBar>(c, sys_bar_id, sys_bar_geometry, std::move(workspace_boxes), active_draw_prop.value(),
                                                inactive_drawprop.value());
        sbar->active_workspace_button = awin;
        return sbar;
    }

} // namespace cx::workspace
