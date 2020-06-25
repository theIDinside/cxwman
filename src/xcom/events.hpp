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

    struct ResizeArgument {
        ScreenSpaceDirection dir;
        cx::uint step{10};
        /// Translates values into screen space, where down is -y
        [[nodiscard]] int get_value() const;
    };

    struct EventArg {
        using ArgsList = std::variant<Dir, Pos, Geo, int, ResizeArgument>;
        EventArg() noexcept = default;
        EventArg(EventArg&&) noexcept = default;
        EventArg(const EventArg&) noexcept = default;
        explicit EventArg(ArgsList arg) noexcept : arg(arg) {}

        ArgsList arg;
    };
} // namespace cx::events