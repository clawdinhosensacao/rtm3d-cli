#include "Geometry.hpp"

#include <algorithm>

namespace rtm3d::rtm_internal {

Volume3D make_velocity_volume(const GridModel2D& model, const RtmConfig& cfg) {
  Volume3D vel(model.nx, cfg.ny, model.nz, 1500.0f);
  for (std::size_t iz = 0; iz < model.nz; ++iz) {
    for (std::size_t iy = 0; iy < cfg.ny; ++iy) {
      for (std::size_t ix = 0; ix < model.nx; ++ix) {
        vel(ix, iy, iz) = model.values[iz * model.nx + ix];
      }
    }
  }
  return vel;
}

std::vector<std::size_t> make_receiver_positions(const Volume3D& vel, std::size_t receiver_stride) {
  const std::size_t nrec = std::max<std::size_t>(2, vel.nx() / receiver_stride);
  std::vector<std::size_t> rx(nrec, 1);
  for (std::size_t ir = 0; ir < nrec; ++ir) {
    rx[ir] = std::min(1 + ir * receiver_stride, vel.nx() - 2);
  }
  return rx;
}

void record_receivers(const Volume3D& vel, std::size_t sy, std::size_t sz,
                      const std::vector<std::size_t>& rx, const std::vector<float>& src_field,
                      std::vector<float>& rec_data, std::size_t it) {
  for (std::size_t ir = 0; ir < rx.size(); ++ir) {
    rec_data[it * rx.size() + ir] = src_field[vel.index(rx[ir], sy, sz)];
  }
}

void inject_receivers(const Volume3D& vel, std::size_t sy, std::size_t sz,
                      const std::vector<std::size_t>& rx, const std::vector<float>& rec_data,
                      std::size_t it, std::vector<float>& rec_field) {
  for (std::size_t ir = 0; ir < rx.size(); ++ir) {
    rec_field[vel.index(rx[ir], sy, sz)] += rec_data[it * rx.size() + ir];
  }
}

std::vector<float> extract_inline_xz(const Volume3D& vel, const std::vector<float>& image) {
  std::vector<float> inline_xz(vel.nx() * vel.nz(), 0.0f);
  const std::size_t ymid = vel.ny() / 2;
  for (std::size_t iz = 0; iz < vel.nz(); ++iz) {
    for (std::size_t ix = 0; ix < vel.nx(); ++ix) {
      inline_xz[iz * vel.nx() + ix] = image[vel.index(ix, ymid, iz)];
    }
  }
  return inline_xz;
}

}  // namespace rtm3d::rtm_internal
