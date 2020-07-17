//
// Created by cx on 2020-07-16.
//

#pragma once

#include <map>

namespace cx::cfg {

    struct StateColor {
        int active, inactive;
    };

    struct Configuration
    {
      public:
        Configuration();
        StateColor borders;
        int background_color;
        int frame_title_height;
    };
}
