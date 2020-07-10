//
// Created by cx on 2020-06-20.
//

#include "events.hpp"
namespace cx::events
{
    int ResizeArgument::get_value() const
    {
        switch(dir) {
        case Dir::LEFT:
            return -1 * static_cast<int>(step);
        case Dir::RIGHT:
            return static_cast<int>(step);
        case Dir::UP:
            return -1 * static_cast<int>(step);
        case Dir::DOWN:
            return static_cast<int>(step);
        }
    }
} // namespace cx::events