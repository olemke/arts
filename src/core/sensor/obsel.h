#pragma once

#include <matpack.h>
#include <rtepack.h>

#include <array>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include "debug.h"
#include "format_tags.h"
#include "matpack_constexpr.h"
#include "sorted_grid.h"

namespace sensor {
struct PosLos {
  Vector3 pos;
  Vector2 los;

  constexpr bool operator==(const PosLos& other) const = default;
  constexpr bool operator!=(const PosLos& other) const = default;

  constexpr bool operator<=(const PosLos& other) const {
    return std::tie(pos, los) <= std::tie(other.pos, other.los);
  };

  constexpr bool operator>=(const PosLos& other) const {
    return std::tie(pos, los) >= std::tie(other.pos, other.los);
  };

  constexpr bool operator<(const PosLos& other) const {
    return std::tie(pos, los) < std::tie(other.pos, other.los);
  };

  constexpr bool operator>(const PosLos& other) const {
    return std::tie(pos, los) > std::tie(other.pos, other.los);
  };

  friend std::ostream& operator<<(std::ostream& os, const PosLos& poslos);
};

using PosLosVector = matpack::matpack_data<PosLos, 1>;

class Obsel {
  //! Frequency grid, must be ascending
  std::shared_ptr<const AscendingGrid> f{
      std::make_shared<const AscendingGrid>()};

  //! Position and line of sight grid of the sensor
  std::shared_ptr<const PosLosVector> poslos{
      std::make_shared<const PosLosVector>()};

  // A matrix size of poslos_grid.size() x f_grid.size() with the polarized weight of the sensor
  StokvecMatrix w{};

 public:
  Obsel() = default;
  Obsel(std::shared_ptr<const AscendingGrid> fs,
        std::shared_ptr<const PosLosVector> pl,
        StokvecMatrix ws)
      : f{std::move(fs)}, poslos{std::move(pl)}, w{std::move(ws)} {
    check();
  }
  Obsel(const AscendingGrid& fs, const PosLosVector& pl, StokvecMatrix ws)
      : f{std::make_shared<const AscendingGrid>(fs)},
        poslos{std::make_shared<const PosLosVector>(pl)},
        w{std::move(ws)} {
    check();
  }

  Obsel(const Obsel&)            = default;
  Obsel(Obsel&&)                 = default;
  Obsel& operator=(const Obsel&) = default;
  Obsel& operator=(Obsel&&)      = default;

  void check() const;

  [[nodiscard]] bool same_freqs(const Obsel& other) const {
    return f == other.f;
  }

  [[nodiscard]] bool same_freqs(
      const std::shared_ptr<const AscendingGrid>& other) const {
    return f == other;
  }

  [[nodiscard]] bool same_poslos(const Obsel& other) const {
    return poslos == other.poslos;
  }

  [[nodiscard]] bool same_poslos(
      const std::shared_ptr<const PosLosVector>& other) const {
    return poslos == other;
  }

  [[nodiscard]] const auto& f_grid_ptr() const { return f; }
  [[nodiscard]] const auto& poslos_grid_ptr() const { return poslos; }

  [[nodiscard]] const AscendingGrid& f_grid() const { return *f; }
  [[nodiscard]] const PosLosVector& poslos_grid() const { return *poslos; }
  [[nodiscard]] const StokvecMatrix& weight_matrix() const { return w; }

  //! Constant indicating that the frequency or poslos is not found in the grid
  constexpr static Index dont_have = -1;

  friend std::ostream& operator<<(std::ostream& os, const Obsel& obsel);

  /** The weights are renormalized to the new value.
   * 
   * After calling this method, the sum of all weights dot-producted with the
   * polarization will be equal to the new value.
   *
   * @param pol The polarization that is sampled for the sum. The default is [1, 0, 0, 0].
   * @param new_value The new value for the normalization. The default is 1.0.
   */
  void normalize(Stokvec pol = {1., 0., 0., 0.}, Numeric new_value = 1.0);

  [[nodiscard]] Numeric sumup(const StokvecVectorView& i, Index ip) const;
  void sumup(VectorView out, const StokvecMatrixView& j, Index ip) const;
};

std::ostream& operator<<(std::ostream& os, const Array<Obsel>& obsel);
}  // namespace sensor

using SensorPosLos       = sensor::PosLos;
using SensorPosLosVector = sensor::PosLosVector;
using SensorObsel        = sensor::Obsel;
using ArrayOfSensorObsel = Array<SensorObsel>;
using SensorSimulations  = std::unordered_map<
     std::shared_ptr<const AscendingGrid>,
     std::unordered_set<std::shared_ptr<const SensorPosLosVector>>>;

void make_exhaustive(ArrayOfSensorObsel& obsels);

SensorSimulations collect_simulations(const ArrayOfSensorObsel& obsels);

template <>
struct std::formatter<SensorPosLos> {
  format_tags tags{};

  [[nodiscard]] constexpr auto& inner_fmt() { return *this; }
  [[nodiscard]] constexpr auto& inner_fmt() const { return *this; }

  constexpr std::format_parse_context::iterator parse(
      std::format_parse_context& ctx) {
    return parse_format_tags(tags, ctx);
  }

  template <class FmtContext>
  FmtContext::iterator format(const SensorPosLos& v, FmtContext& ctx) const {
    return tags.format(ctx, v.pos, tags.sep(), v.los);
  }
};

template <>
struct std::formatter<SensorObsel> {
  format_tags tags{};

  [[nodiscard]] constexpr auto& inner_fmt() { return *this; }
  [[nodiscard]] constexpr auto& inner_fmt() const { return *this; }

  constexpr std::format_parse_context::iterator parse(
      std::format_parse_context& ctx) {
    return parse_format_tags(tags, ctx);
  }

  template <class FmtContext>
  FmtContext::iterator format(const SensorObsel& v, FmtContext& ctx) const {
    return tags.format(ctx,
                       "Obsel:\n  frequency grid:           "sv,
                       v.f_grid(),
                       "\n  pos-los grid:                   "sv,
                       v.poslos_grid(),
                       "\n  weights:                        "sv,
                       v.weight_matrix());
  }
};
