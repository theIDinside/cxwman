//
// Created by cx on 2020-06-20.
//
#pragma once
#include <datastructure/geometry.hpp>
#include <variant>

namespace cx::events
{
    enum ScreenSpaceDirection { LEFT, RIGHT, UP, DOWN };
    using Dir = ScreenSpaceDirection;
    using Pos = geom::Position;
    using Geo = geom::Geometry;

    struct EventArg {
        using ArgsList = std::variant<Dir, Pos, Geo>;
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