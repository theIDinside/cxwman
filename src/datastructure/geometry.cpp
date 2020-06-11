#include <datastructure/geometry.hpp>

namespace cx::geom
{
    std::pair<Geometry, Geometry> Geometry::vsplit(float split_ratio)
    {
        auto sp = std::clamp(split_ratio, 0.1f, 1.0f);
        auto lwidth = static_cast<GU>(static_cast<float>(this->width) * sp);
        auto rwidth = static_cast<GU>(this->width - lwidth);
        auto rx = static_cast<GU>(x() + lwidth);
        return {Geometry{x(), y(), lwidth, this->height}, Geometry{rx, y(), rwidth, this->height}};
    }
    std::pair<Geometry, Geometry> Geometry::hsplit(float split_ratio)
    {
        auto sp = std::clamp(split_ratio, 0.1f, 1.0f);
        auto theight = static_cast<GU>(static_cast<float>(this->height) * sp);
        auto bheight = static_cast<GU>(this->height - theight);
        auto by = static_cast<GU>(y() + theight);

        return {Geometry{x(), y(), this->width, theight}, Geometry{x(), by, this->width, bheight}};
    }

} // namespace cx::geom
