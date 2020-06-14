#include <X11/Xlib.h>
#include <coreutils/core.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>
#include <xcb/xcb.h>
#include <xcom/connect.hpp>

// TODO(simon): set up a configuration file in cmake, so this can be added by cmake automatically, instead
// manually keeping track of these variables
constexpr auto VERSION = "0.0.1";

int main(int argc, const char **argv)
{
    cx::println("CX Window Manager. Version {}", VERSION);
    std::unique_ptr<cx::Manager> wm_handle = nullptr;
    try {
        cx::println("Initializing wm...");
        wm_handle = cx::Manager::initialize();
        cx::println("Succesfull!");
    } catch (std::exception &e) {
        cx::println("Fatal error caught while initializing window manager. Exiting. Message:\n{}", e.what());
        exit(EXIT_FAILURE);
    }
    cx::println("Running event loop");
    wm_handle->event_loop();
}
