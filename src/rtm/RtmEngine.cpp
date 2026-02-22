#include "rtm3d/rtm/RtmEngine.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "Boundary.hpp"
#include "Geometry.hpp"
#include "rtm3d/core/Volume3D.hpp"

namespace rtm3d {
namespace {

void validate_cfg(const GridModel2D& model, const RtmConfig& cfg) {
  if (model.nx < 8 || model.nz < 8) throw std::runtime_error("model too small");
  if (model.dx <= 0.0f || model.dz <= 0.0f) throw std::runtime_error("invalid model spacing");
  if (cfg.ny < 4 || cfg.nt < 2) throw std::runtime_error("ny/nt too small");
  if (cfg.dy <= 0.0f || cfg.dt <= 0.0f || cfg.f0 <= 0.0f) throw std::runtime_error("invalid RTM scalar parameter");
  if (cfg.receiver_stride == 0) throw std::runtime_error("receiver_stride must be > 0");
  if (cfg.pml == 0) throw std::runtime_error("pml must be > 0");
}

void step_fd3d(const Volume3D& vel, const std::vector<float>& damp, float dt, float dx, float dy,
               float dz, const std::vector<float>& prev, const std::vector<float>& cur,
               std::vector<float>& nxt) {
  std::fill(nxt.begin(), nxt.end(), 0.0f);

  for (std::size_t iz = 1; iz + 1 < vel.nz(); ++iz) {
    for (std::size_t iy = 1; iy + 1 < vel.ny(); ++iy) {
      for (std::size_t ix = 1; ix + 1 < vel.nx(); ++ix) {
        const auto i = vel.index(ix, iy, iz);
        const float d2x = (cur[vel.index(ix + 1, iy, iz)] - 2.0f * cur[i] + cur[vel.index(ix - 1, iy, iz)]) / (dx * dx);
        const float d2y = (cur[vel.index(ix, iy + 1, iz)] - 2.0f * cur[i] + cur[vel.index(ix, iy - 1, iz)]) / (dy * dy);
        const float d2z = (cur[vel.index(ix, iy, iz + 1)] - 2.0f * cur[i] + cur[vel.index(ix, iy, iz - 1)]) / (dz * dz);
        const float lap = d2x + d2y + d2z;
        const float v = vel.raw()[i];
        nxt[i] = (2.0f * cur[i] - prev[i] + (v * v) * (dt * dt) * lap) * damp[i];
      }
    }
  }
}

void forward_source_propagation(const GridModel2D& model, const RtmConfig& cfg, const Volume3D& vel,
                                const std::vector<float>& damp, const std::vector<float>& wavelet,
                                std::size_t sx, std::size_t sy, std::size_t sz,
                                const std::vector<std::size_t>& rx, std::vector<float>& src_snaps,
                                std::vector<float>& rec_data) {
  const auto n = vel.size();
  std::vector<float> src_prev(n, 0.0f), src_cur(n, 0.0f), src_nxt(n, 0.0f);

  for (std::size_t it = 0; it < cfg.nt; ++it) {
    step_fd3d(vel, damp, cfg.dt, model.dx, cfg.dy, model.dz, src_prev, src_cur, src_nxt);
    src_nxt[vel.index(sx, sy, sz)] += wavelet[it];

    rtm_internal::record_receivers(vel, sy, sz, rx, src_nxt, rec_data, it);
    std::copy(src_nxt.begin(), src_nxt.end(), src_snaps.begin() + it * n);

    src_prev.swap(src_cur);
    src_cur.swap(src_nxt);
  }
}

void receiver_backpropagation_and_imaging(const GridModel2D& model, const RtmConfig& cfg,
                                          const Volume3D& vel, const std::vector<float>& damp,
                                          std::size_t sy, std::size_t sz,
                                          const std::vector<std::size_t>& rx,
                                          const std::vector<float>& src_snaps,
                                          const std::vector<float>& rec_data,
                                          std::vector<float>& image) {
  const auto n = vel.size();
  std::vector<float> rec_prev(n, 0.0f), rec_cur(n, 0.0f), rec_nxt(n, 0.0f);

  for (std::size_t rit = 0; rit < cfg.nt; ++rit) {
    const std::size_t it = cfg.nt - 1 - rit;
    step_fd3d(vel, damp, cfg.dt, model.dx, cfg.dy, model.dz, rec_prev, rec_cur, rec_nxt);

    rtm_internal::inject_receivers(vel, sy, sz, rx, rec_data, it, rec_nxt);

    const auto* src = src_snaps.data() + it * n;
    for (std::size_t i = 0; i < n; ++i) image[i] += src[i] * rec_nxt[i];

    rec_prev.swap(rec_cur);
    rec_cur.swap(rec_nxt);
  }
}

}  // namespace

std::vector<float> ricker_wavelet(std::size_t nt, float dt, float f0) {
  if (nt < 2 || dt <= 0.0f || f0 <= 0.0f) throw std::runtime_error("invalid wavelet arguments");

  std::vector<float> w(nt, 0.0f);
  const float t0 = 1.0f / f0;
  constexpr float kPi = 3.14159265358979323846f;
  for (std::size_t it = 0; it < nt; ++it) {
    const float t = static_cast<float>(it) * dt - t0;
    const float a = kPi * f0 * t;
    const float a2 = a * a;
    w[it] = (1.0f - 2.0f * a2) * std::exp(-a2);
  }
  return w;
}

MigrationResult run_single_shot_rtm(const GridModel2D& model, const RtmConfig& cfg) {
  validate_cfg(model, cfg);

  const Volume3D vel = rtm_internal::make_velocity_volume(model, cfg);
  const auto n = vel.size();

  const auto damp = rtm_internal::make_damp(vel.nx(), vel.ny(), vel.nz(), cfg.pml);
  const auto wavelet = ricker_wavelet(cfg.nt, cfg.dt, cfg.f0);

  const std::size_t sx = vel.nx() / 2;
  const std::size_t sy = vel.ny() / 2;
  const std::size_t sz = 2;

  const auto rx = rtm_internal::make_receiver_positions(vel, cfg.receiver_stride);
  std::vector<float> src_snaps(cfg.nt * n, 0.0f);
  std::vector<float> rec_data(cfg.nt * rx.size(), 0.0f);

  forward_source_propagation(model, cfg, vel, damp, wavelet, sx, sy, sz, rx, src_snaps, rec_data);

  std::vector<float> image(n, 0.0f);
  receiver_backpropagation_and_imaging(model, cfg, vel, damp, sy, sz, rx, src_snaps, rec_data, image);

  MigrationResult out;
  out.nx = vel.nx();
  out.nz = vel.nz();
  out.inline_xz = rtm_internal::extract_inline_xz(vel, image);
  return out;
}

}  // namespace rtm3d
