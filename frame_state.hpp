#pragma once

#include <map>
#include <string>

struct FrameState {
    double time = 0.0;
    std::map<std::string, double> quantities;
};
