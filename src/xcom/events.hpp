//
// Created by cx on 2020-06-20.
//
#pragma once
#include <datastructure/geometry.hpp>
#include <variant>

namespace cx::events
{
    enum ScreenSpaceDirection { LEFT, RIGHT, UP, DOWN };

    struct ScreenSpaceDir {
        cx::uint value{};
        ScreenSpaceDirection dir;
        explicit ScreenSpaceDir(cx::uint k, ScreenSpaceDirection dir) : value{k}, dir(dir) {}
        virtual ~ScreenSpaceDir() = default;
        [[nodiscard]] int get_value() {
            switch(dir) {
            case LEFT:  return -1 * static_cast<int>(value);
            case RIGHT: return static_cast<int>(value);
            case UP:    return -1 * static_cast<int>(value);
            case DOWN:  return static_cast<int>(value);
            }
        }
    };
    using Dir = ScreenSpaceDirection;
    using Pos = geom::Position;
    using Geo = geom::Geometry;

    struct ResizeArgument {
        ScreenSpaceDirection dir;
        cx::uint step{10};

        [[nodiscard]] int get_value();

    };

    struct EventArg {
        using ArgsList = std::variant<Dir, Pos, Geo, int, ResizeArgument>;
        EventArg() = default;
        EventArg(EventArg&&) = default;
        EventArg(const EventArg&) = default;
        EventArg(ArgsList arg) : arg(arg) {}

        ArgsList arg;
    };
} // namespace cx::events

class Events
{
};