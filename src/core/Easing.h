#pragma once

#include <algorithm>
#include <cmath>

namespace Omareel::Easing {

inline double easeInOutCubic(double t)
{
    t = std::clamp(t, 0.0, 1.0);
    return t < 0.5 ? 4.0 * t * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 3.0) / 2.0;
}

} // namespace Omareel::Easing
