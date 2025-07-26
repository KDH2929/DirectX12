#pragma once

constexpr int MAX_LIGHTS = 3;
constexpr int NUM_DIR_LIGHTS = 1;
constexpr int NUM_POINT_LIGHTS = 1;
constexpr int NUM_SPOT_LIGHTS = 1;


enum class LightType : int {
    Directional = 0,
    Point = 1,
    Spot = 2
};