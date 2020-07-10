//
// Created by cx on 2020-06-20.
//
#pragma once

#include <variant>
#include <datastructure/geometry.hpp>

namespace cx::events
{

    using Dir = geom::ScreenSpaceDirection;
    using Pos = geom::Position;
    using Geo = geom::Geometry;
    enum class ResizeType {
        Increase,
        Decrease
    };

    struct ResizeArgument {
        using Type = ResizeType;
        Dir dir;
        cx::uint step{10};
        ResizeType type;
        /// Translates values into screen space, where down is -y
        [[nodiscard]] int get_value() const;
    };

    struct EventArg {
        using ArgsTypes = std::variant<Dir, Pos, Geo, int, ResizeArgument, std::nullopt_t>;
        EventArg() noexcept = default;
        EventArg(EventArg&&) noexcept = default;
        EventArg(const EventArg&) noexcept = default;
        explicit EventArg(ArgsTypes arg) noexcept : arg(arg) {}

        ArgsTypes arg;
    };
} // namespace cx::events