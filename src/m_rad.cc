#include <atm.h>
#include <jacobian.h>
#include <path_point.h>
#include <physics_funcs.h>
#include <rtepack.h>
#include <surf.h>
#include <workspace.h>

#include <algorithm>
#include <exception>
#include <stdexcept>

#include "arts_omp.h"
#include "debug.h"
#include "enumsSensorJacobianModelType.h"
#include "fwd.h"
#include "rtepack_stokes_vector.h"
#include "sorted_grid.h"
#include "workspace_agenda_class.h"

void spectral_radiance_jacobianEmpty(
    StokvecMatrix &spectral_radiance_jacobian,
    const AscendingGrid &frequency_grid,
    const JacobianTargets &jacobian_targets) try {
  spectral_radiance_jacobian.resize(jacobian_targets.x_size(),
                                    frequency_grid.size());
  spectral_radiance_jacobian = Stokvec{0.0, 0.0, 0.0, 0.0};
}
ARTS_METHOD_ERROR_CATCH

void spectral_radiance_jacobianFromBackground(
    StokvecMatrix &spectral_radiance_jacobian,
    const StokvecMatrix &spectral_radiance_background_jacobian,
    const MuelmatVector &background_transmittance) try {
  ARTS_USER_ERROR_IF(
      spectral_radiance_background_jacobian.ncols() !=
          background_transmittance.nelem(),
      "spectral_radiance_background_jacobian must have same number of rows as the "
      "size of jacobian_targets")

  //! The radiance derivative shape is the background shape
  spectral_radiance_jacobian.resize(
      spectral_radiance_background_jacobian.shape());

  //! Set the background radiance derivative as that which is seen after "this" swath
  for (Index i = 0; i < spectral_radiance_jacobian.nrows(); i++) {
    std::transform(background_transmittance.begin(),
                   background_transmittance.end(),
                   spectral_radiance_background_jacobian[i].begin(),
                   spectral_radiance_jacobian[i].begin(),
                   std::multiplies<>());
  }
}
ARTS_METHOD_ERROR_CATCH

void spectral_radiance_jacobianAddPathPropagation(
    StokvecMatrix &spectral_radiance_jacobian,
    const ArrayOfStokvecMatrix &ray_path_spectral_radiance_jacobian,
    const JacobianTargets &jacobian_targets,
    const AtmField &atmospheric_field,
    const ArrayOfPropagationPathPoint &ray_path) try {
  const auto np = ray_path_spectral_radiance_jacobian.size();
  const auto nj = spectral_radiance_jacobian.nrows();
  const auto nf = spectral_radiance_jacobian.ncols();
  const auto nt = jacobian_targets.target_count();

  if (nt == 0) return;

  ARTS_USER_ERROR_IF(
      static_cast<Size>(spectral_radiance_jacobian.nrows()) !=
          jacobian_targets.x_size(),
      "Bad size of spectral_radiance_jacobian, it's inner dimension should match the size of jacobian_targets. Sizes: "
      "{} != {}",
      spectral_radiance_jacobian.nrows(),
      jacobian_targets.x_size())

  ARTS_USER_ERROR_IF(
      ray_path.size() != np,
      "ray_path must have same size as the size of ray_path_spectral_radiance_jacobian.  Sizes: ",
      "{} != {}",
      ray_path.size(),
      np)

  for (auto &dr : ray_path_spectral_radiance_jacobian) {
    ARTS_USER_ERROR_IF(
        dr.ncols() != nf or dr.nrows() != static_cast<Index>(nt),
        "ray_path_spectral_radiance_jacobian elements must have same number of rows as the size of "
        "jacobian_targets.  Sizes: "
        "{:B,} != [{}, {}]",
        dr.shape(),
        nt,
        nf)
  }

  //! Checks that the jacobian_targets can be used and throws if not
  jacobian_targets.throwing_check(nj);

  //! The derivative part from the atmosphere
  for (auto &atm_block : jacobian_targets.atm()) {
    ARTS_USER_ERROR_IF(not atmospheric_field.contains(atm_block.type),
                       "No {}"
                       " in atmospheric_field but in jacobian_targets",
                       atm_block.type)
    const auto &data = atmospheric_field[atm_block.type];
    for (Size ip = 0; ip < np; ip++) {
      const auto weights = data.flat_weight(ray_path[ip].pos);
      for (auto &w : weights) {
        if (w.second != 0.0) {
          const auto i = w.first + atm_block.x_start;
          ARTS_ASSERT(i < static_cast<Size>(nj))
          std::transform(
              ray_path_spectral_radiance_jacobian[ip][atm_block.target_pos]
                  .begin(),
              ray_path_spectral_radiance_jacobian[ip][atm_block.target_pos]
                  .end(),
              spectral_radiance_jacobian[i].begin(),
              spectral_radiance_jacobian[i].begin(),
              [x = w.second](auto &a, auto &b) { return x * a + b; });
        }
      }
    }
  }
}
ARTS_METHOD_ERROR_CATCH

void spectral_radianceFromPathPropagation(
    StokvecVector &spectral_radiance,
    const ArrayOfStokvecVector &ray_path_spectral_radiance) try {
  ARTS_USER_ERROR_IF(ray_path_spectral_radiance.empty(),
                     "Empty ray_path_spectral_radiance")
  spectral_radiance = ray_path_spectral_radiance.front();
}
ARTS_METHOD_ERROR_CATCH

void spectral_radiance_jacobianApplyUnit(
    StokvecMatrix &spectral_radiance_jacobian,
    const StokvecVector &spectral_radiance,
    const AscendingGrid &frequency_grid,
    const PropagationPathPoint &ray_path_point,
    const SpectralRadianceUnitType &spectral_radiance_unit) try {
  ARTS_USER_ERROR_IF(spectral_radiance.size() != frequency_grid.size(),
                     "spectral_radiance must have same size as frequency_grid")
  ARTS_USER_ERROR_IF(
      spectral_radiance_jacobian.size() != 0 and
          spectral_radiance_jacobian.ncols() != frequency_grid.size(),
      "spectral_radiance must have same size as frequency_grid")

  const auto dF =
      rtepack::dunit_converter(spectral_radiance_unit, ray_path_point.nreal);

  //! Must apply the unit to the spectral radiance jacobian first
  for (Index i = 0; i < spectral_radiance_jacobian.nrows(); i++) {
    for (Index j = 0; j < spectral_radiance_jacobian.ncols(); j++) {
      spectral_radiance_jacobian(i, j) = dF(spectral_radiance_jacobian(i, j),
                                            spectral_radiance[j],
                                            frequency_grid[j]);
    }
  }
}
ARTS_METHOD_ERROR_CATCH

void spectral_radianceApplyUnit(
    StokvecVector &spectral_radiance,
    const AscendingGrid &frequency_grid,
    const PropagationPathPoint &ray_path_point,
    const SpectralRadianceUnitType &spectral_radiance_unit) try {
  ARTS_USER_ERROR_IF(spectral_radiance.size() != frequency_grid.size(),
                     "spectral_radiance must have same size as frequency_grid")
  const auto F =
      rtepack::unit_converter(spectral_radiance_unit, ray_path_point.nreal);

  std::transform(spectral_radiance.begin(),
                 spectral_radiance.end(),
                 frequency_grid.begin(),
                 spectral_radiance.begin(),
                 [&F](const Stokvec &v, const Numeric f) { return F(v, f); });
}
ARTS_METHOD_ERROR_CATCH

void spectral_radiance_jacobianAddSensorJacobianPerturbations(
    const Workspace &ws,
    StokvecMatrix &spectral_radiance_jacobian,
    const StokvecVector &spectral_radiance,
    const ArrayOfSensorObsel &measurement_sensor,
    const AscendingGrid &frequency_grid,
    const JacobianTargets &jacobian_targets,
    const Vector3 &pos,
    const Vector2 &los,
    const AtmField &atmospheric_field,
    const SurfaceField &surface_field,
    const Agenda &spectral_radiance_observer_agenda) try {
  /*
  
  This method likely calls itself "recursively" for sensor parameters

  However, the flag for bailing and stopping this recursion
  is that there are no more jacobian targets.  So it calls
  itself always with an empty jacobian_targets.  This is how
  it bails out of the recursion.
  
  */
  if (jacobian_targets.sensor().empty()) return;

  ARTS_USER_ERROR_IF(
      spectral_radiance.size() != frequency_grid.size(),
      R"(spectral_radiance must have same size as element frequency grid

spectral_radiance.size() = {},
frequency_grid.size()    = {}
)",
      spectral_radiance.size(),
      frequency_grid.size())

  ARTS_USER_ERROR_IF(
      (spectral_radiance_jacobian.shape() !=
       std::array{static_cast<Index>(jacobian_targets.x_size()),
                  frequency_grid.size()}),
      R"(spectral_radiance_jacobian must be x-grid times frequency grid

spectral_radiance_jacobian.shape() = {:B,},
jacobian_targets.x_size()          = {},
frequency_grid.size()              = {}
)",
      spectral_radiance_jacobian.shape(),
      jacobian_targets.x_size(),
      frequency_grid.size())

  const JacobianTargets jacobian_targets_empty{};
  StokvecMatrix spectral_radiance_jacobian_empty{};
  ArrayOfPropagationPathPoint ray_path{};

  StokvecVector dsrad;
  auto call = [&](const AscendingGrid &frequency_grid_2,
                  const Vector3 &pos2,
                  const Vector2 &los2,
                  const Numeric d) {
    spectral_radiance_observer_agendaExecute(ws,
                                             dsrad,
                                             spectral_radiance_jacobian_empty,
                                             ray_path,
                                             frequency_grid_2,
                                             jacobian_targets_empty,
                                             pos2,
                                             los2,
                                             atmospheric_field,
                                             surface_field,
                                             spectral_radiance_observer_agenda);

    ARTS_USER_ERROR_IF(dsrad.size() != spectral_radiance.size(),
                       R"(Wrong size of perturbed spectral radiance:

    dsrad.size()             = {},
    spectral_radiance.size() = {}
)",
                       dsrad.size(),
                       spectral_radiance.size())

    // Convert to perturbed Jacobian
    dsrad -= spectral_radiance;
    for (auto &v : dsrad) v /= d;
    return dsrad;
  };

  const auto &x = frequency_grid;
  const auto b  = x.begin();
  const auto e  = x.end();

  for (auto &target : jacobian_targets.sensor()) {
    ARTS_USER_ERROR_IF(
        measurement_sensor.size() <= static_cast<Size>(target.type.elem),
        "Sensor element out of bounds");

    auto m = spectral_radiance_jacobian.slice(target.x_start, target.x_size);
    const Numeric d = target.d;

    // Check that the Jacobian targets are represented by this frequency grid and this pos-los pair
    const Index iposlos = measurement_sensor[target.type.elem].find(pos, los);
    if (iposlos == SensorObsel::dont_have) continue;
    if (measurement_sensor[target.type.elem].find(frequency_grid) ==
        SensorObsel::dont_have)
      continue;

    using enum SensorKeyType;
    switch (target.type.type) {
      case f:   call({b, e, [d](auto x) { return x + d; }}, pos, los, d);
      case za:  call(x, pos, {los[0] + d, los[1]}, d); break;
      case aa:  call(x, pos, {los[0], los[1] + d}, d); break;
      case alt: call(x, {pos[0] + d, pos[1], pos[2]}, los, d); break;
      case lat: call(x, {pos[0], pos[1] + d, pos[2]}, los, d); break;
      case lon: call(x, {pos[0], pos[1], pos[2] + d}, los, d); break;
    }

    switch (target.type.model) {
      using enum SensorJacobianModelType;
      case PolynomialOffset: {
        const auto &o = target.type.original_grid;

        if (target.type.type == f) {
          ARTS_USER_ERROR_IF(
              x.size() != o.size(),
              "Expects the frequency grid to be the same size as the original grid")

          for (Size i = 0; i < target.x_size; i++) {
            for (Index iv = 0; iv < x.size(); iv++) {
              m(i, iv) += dsrad[iv] * std::pow(o[iv], i);
            }
          }
        } else {
          ARTS_USER_ERROR_IF(
              static_cast<Index>(target.x_size) != o.size(),
              "Expects original grid to be the same as the required x-parameters in the jacobian target")

          for (Size i = 0; i < target.x_size; i++) {
            const Numeric g = std::pow(o[i], i);
            for (Index iv = 0; iv < x.size(); iv++) {
              m(i, iv) += dsrad[iv] * g;
            }
          }
        }
      } break;
      case None: {
        if (target.type.type == f) {
          ARTS_USER_ERROR_IF(m.ncols() != m.nrows(),
                             "Expects square matrix for frequency derivative")
          m.diagonal() += dsrad;
        } else {
          ARTS_USER_ERROR_IF(static_cast<Size>(iposlos) >= target.x_size,
                             "Bad pos-los index");
          m[iposlos] += dsrad;
        }
      } break;
    }
  }
}
ARTS_METHOD_ERROR_CATCH

void measurement_vectorFromSensor(
    const Workspace &ws,
    Vector &measurement_vector,
    Matrix &measurement_jacobian,
    const ArrayOfSensorObsel &measurement_sensor,
    const JacobianTargets &jacobian_targets,
    const AtmField &atmospheric_field,
    const SurfaceField &surface_field,
    const SpectralRadianceUnitType &spectral_radiance_unit,
    const Agenda &spectral_radiance_observer_agenda) try {
  measurement_vector.resize(measurement_sensor.size());
  measurement_vector = 0.0;

  measurement_jacobian.resize(measurement_sensor.size(),
                              jacobian_targets.x_size());
  measurement_jacobian = 0.0;

  if (measurement_sensor.empty()) return;

  //! Check the observational elements that their dimensions are correct
  for (auto &obsel : measurement_sensor) obsel.check();

  const SensorSimulations simulations = collect_simulations(measurement_sensor);

  for (auto &[f_grid_ptr, poslos_set] : simulations) {
    for (auto &poslos_gs : poslos_set) {
      for (Index ip = 0; ip < poslos_gs->size(); ++ip) {
        StokvecVector spectral_radiance;
        StokvecMatrix spectral_radiance_jacobian;
        ArrayOfPropagationPathPoint ray_path;

        const SensorPosLos &poslos = (*poslos_gs)[ip];

        spectral_radiance_observer_agendaExecute(
            ws,
            spectral_radiance,
            spectral_radiance_jacobian,
            ray_path,
            *f_grid_ptr,
            jacobian_targets,
            poslos.pos,
            poslos.los,
            atmospheric_field,
            surface_field,
            spectral_radiance_observer_agenda);

        ARTS_USER_ERROR_IF(
            spectral_radiance.size() != f_grid_ptr->size(),
            R"(spectral_radiance must have same size as element frequency grid

spectral_radiance.size() = {},
f_grid_ptr->size()       = {}
)",
            spectral_radiance.size(),
            f_grid_ptr->size())
        ARTS_USER_ERROR_IF(
            (spectral_radiance_jacobian.shape() !=
             std::array{measurement_jacobian.ncols(), f_grid_ptr->size()}),
            R"(spectral_radiance_jacobian must be targets x frequency grid size

spectral_radiance_jacobian.shape()  = {:B,},
f_grid_ptr->size()                  = {},
measurement_jacobian.ncols() = {}
)",
            spectral_radiance_jacobian.shape(),
            f_grid_ptr->size(),
            measurement_jacobian.ncols())

        spectral_radianceApplyUnitFromSpectralRadiance(
            spectral_radiance,
            spectral_radiance_jacobian,
            *f_grid_ptr,
            ray_path,
            spectral_radiance_unit);

        for (Size iv = 0; iv < measurement_sensor.size(); ++iv) {
          const SensorObsel &obsel = measurement_sensor[iv];
          if (obsel.same_freqs(f_grid_ptr)) {
            measurement_vector[iv] += obsel.sumup(spectral_radiance, ip);

            obsel.sumup(
                measurement_jacobian[iv], spectral_radiance_jacobian, ip);
          }
        }
      }
    }
  }
}
ARTS_METHOD_ERROR_CATCH
