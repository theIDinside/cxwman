#pragma once

#include <xcb/xproto.h>
namespace cx
{
    namespace xcb_config_masks
    {
        constexpr auto CXWIN_X = XCB_CONFIG_WINDOW_X;
        constexpr auto CXWIN_Y = XCB_CONFIG_WINDOW_Y;
        constexpr auto CXWIN_WIDTH = XCB_CONFIG_WINDOW_WIDTH;
        constexpr auto CXWIN_HEIGHT = XCB_CONFIG_WINDOW_HEIGHT;
        constexpr auto CXWIN_STACK = XCB_CONFIG_WINDOW_STACK_MODE;

        constexpr auto TELEPORT = CXWIN_X | CXWIN_Y | CXWIN_WIDTH | CXWIN_HEIGHT;
        constexpr auto MOVE = CXWIN_X | CXWIN_Y;
        constexpr auto RESIZE = CXWIN_WIDTH | CXWIN_HEIGHT;
    }; // namespace xcb_config_masks

    namespace xcb_key_masks
    {
        constexpr auto SUPER = XCB_MOD_MASK_4;
        constexpr auto SHIFT = XCB_MOD_MASK_SHIFT;
        constexpr auto SUPER_SHIFT = SUPER | SHIFT;
    }; // namespace xcb_key_masks
};     // namespace cx