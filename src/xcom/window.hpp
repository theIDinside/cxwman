#include <datastructure/geometry.hpp>
#include <string>
#include <xcb/xcb.h>

namespace cx::workspace
{

    struct Tag {
        explicit Tag(std::string tag);
        ~Tag() = default;
        std::string m_tag;
    };

    struct Window {

        Window(geom::Geometry g, xcb_window_t client, xcb_window_t frame, Tag tag);
        // The size of the window when not maximized
        cx::geom::Geometry original_size;
        // application window id
        xcb_window_t client_id;
        // the frame id, of which we reparent app window id to, thus controlling the app window
        xcb_window_t frame_id;
        Tag m_tag;
    };
}; // namespace cx::workspace
