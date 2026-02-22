#pragma once

#include <vector>

#include "rtm3d/core/Volume3D.hpp"

namespace rtm3d::rtm_internal {

void step_fd3d(const Volume3D& vel, const std::vector<float>& damp, float dt, float dx, float dy,
               float dz, const std::vector<float>& prev, const std::vector<float>& cur,
               std::vector<float>& nxt);

}  // namespace rtm3d::rtm_internal
