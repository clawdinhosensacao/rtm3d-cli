#pragma once

#include <cstddef>
#include <vector>

#include "rtm3d/core/Volume3D.hpp"
#include "rtm3d/model/GridModel2D.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

namespace rtm3d::rtm_internal {

Volume3D make_velocity_volume(const GridModel2D& model, const RtmConfig& cfg);
std::vector<std::size_t> make_receiver_positions(const Volume3D& vel, std::size_t receiver_stride);

void record_receivers(const Volume3D& vel, std::size_t sy, std::size_t sz,
                      const std::vector<std::size_t>& rx, const std::vector<float>& src_field,
                      std::vector<float>& rec_data, std::size_t it);

void inject_receivers(const Volume3D& vel, std::size_t sy, std::size_t sz,
                      const std::vector<std::size_t>& rx, const std::vector<float>& rec_data,
                      std::size_t it, std::vector<float>& rec_field);

std::vector<float> extract_inline_xz(const Volume3D& vel, const std::vector<float>& image);

}  // namespace rtm3d::rtm_internal
