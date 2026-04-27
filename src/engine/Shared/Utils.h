#pragma once

// TODO: Move to Shared/Utils

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

template<typename T>
inline T Util_RangeOverflow(T value, T min, T max);

template<typename T>
inline T Util_NextIntegerInRange(T value, T min, T max);

template<typename T>
inline T Util_ClampVelocity(T velocity, float maxLen);

constexpr uint32_t Util_ToFlagBit(uint32_t i);

inline double Util_GetSecondsSinceEpoch();

#include "Utils.ipp"
