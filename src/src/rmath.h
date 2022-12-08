#pragma once

namespace rmath
{
    struct vec2
    {
        double x = 0;
        double y = 0;
    };

    struct rect
    {
        vec2 min;
        vec2 max;
    };
}