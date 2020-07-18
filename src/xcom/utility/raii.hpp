#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-explicit-constructor"
//
// Created by cx on 2020-07-17.
//

#pragma once
#include <coreutils/core.hpp>

namespace cx::x11
{

    /// Super simple and easy to use (and abuse) X11 resource holder.
    /// Using this with everything that requires a reply from xcb, will take care of the freeing up of the resources (as per xcb requirements)
    template<typename XCBType>
    struct X11Resource {
        X11Resource(XCBType* type) : pointer_to_data(type) {}
        ~X11Resource()
        {
            if(pointer_to_data)
                delete pointer_to_data;
        }

        XCBType* operator->() { return pointer_to_data; }
        constexpr operator XCBType*() { return pointer_to_data; }
        constexpr operator bool() { return pointer_to_data != nullptr; }
        XCBType* pointer_to_data;
    };

} // namespace cx::x11
#pragma clang diagnostic pop