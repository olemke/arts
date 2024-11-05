/* Copyright (C) 2023
   Simon Pfreundschuh <simon.pfreundschuh@chalmers.se>

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA. */

/*****************************************************************************
 ***  File description
 *****************************************************************************/

/** \file scattering/integration.h
 *
 * This file defines a grid classes representing specific quadratures. The
 * primary purpose is the integration of scattering properties over certain
 * zenith angle grid quadratures.
 *
 * @author Simon Pfreundschuh, 2020 - 2023
 */
#ifndef ARTS_CORE_SCATTERING_INTEGRATION_H_
#define ARTS_CORE_SCATTERING_INTEGRATION_H_

#include <map>
#include <memory>
#include <numbers>

#include "arts_conversions.h"
#include "debug.h"
#include "matpack/matpack_data.h"
#include "matpack/matpack_math.h"
#include "matpack/matpack_sparse.h"

namespace scattering {

using std::numbers::pi_v;

namespace detail {
template <std::floating_point Scalar>
bool small(Scalar a, Scalar epsilon = 1e-6) {
  return std::abs(a) < epsilon;
}
}  // namespace detail

/// Enum class to represent different quadrature classes.
enum class QuadratureType {
  Trapezoidal,
  GaussLegendre,
  DoubleGauss,
  Lobatto,
  ClenshawCurtis,
  Fejer
};

////////////////////////////////////////////////////////////////////////////////
// Gauss-Legendre Quadrature
////////////////////////////////////////////////////////////////////////////////

/** Gauss-Legendre Quadarature
 *
 * This class implements a Gauss-Legendre quadrature for the integration
 * of functions over the interval [-1, 1].
 */
class GaussLegendreQuadrature {
 private:
  /** Find Gauss-Legendre nodes and weights.
   *
   * Uses the Newton root finding algorithm to find the roots of the
   * Legendre polynomial of degree n. Legendre functions are evaluated
   * using a recursion relation.
   */
  void calculate_nodes_and_weights() {
    const Index n = degree_;
    const Index n_half_nodes = (n + 1) / 2;
    const Index n_max_iter = 10;
    Numeric x, x_old, p_l, p_l_1, p_l_2, dp_dx, n_f;
    n_f = static_cast<Numeric>(n);

    for (int i = 1; i <= n_half_nodes; ++i) {
      p_l = pi_v<Numeric>;
      p_l_1 = 2.0 * n_f;
      //
      // Initial guess.
      //
      x = -(1.0 - (n_f - 1.0) / (p_l_1 * p_l_1 * p_l_1)) *
          cos((p_l * (4.0 * i - 1.0)) / (4.0 * n_f + 2.0));

      //
      // Evaluate Legendre Polynomial and its derivative at node.
      //
      for (Index j = 0; j < n_max_iter; ++j) {
        p_l = x;
        p_l_1 = 1.0;
        for (int l = 2; l <= n; ++l) {
          // Legendre recurrence relation
          p_l_2 = p_l_1;
          p_l_1 = p_l;
          p_l = ((2.0 * l - 1.0) * x * p_l_1 - (l - 1.0) * p_l_2) / l;
        }
        dp_dx = ((1.0 - x) * (1.0 + x)) / (n_f * (p_l_1 - x * p_l));
        x_old = x;

        //
        // Perform Newton step.
        //
        x -= p_l * dp_dx;
        auto dx = x - x_old;
        if (detail::small(std::abs(dx * (x + x_old)), 1e-10)) {
          break;
        }
      }
      nodes_[i - 1] = x;
      weights_[i - 1] = 2.0 * dp_dx * dp_dx / ((1.0 - x) * (1.0 + x));
      nodes_[n - i] = -x;
      weights_[n - i] = weights_[i - 1];
    }
  }

 public:
  GaussLegendreQuadrature() {}
  GaussLegendreQuadrature(Index degree)
      : degree_(degree), nodes_(degree), weights_(degree) {
    calculate_nodes_and_weights();
  }

  static constexpr QuadratureType type = QuadratureType::GaussLegendre;

  Index get_degree() const { return degree_; }
  const Vector& get_nodes() const { return nodes_; }
  const Vector& get_weights() const { return weights_; }

  Index degree_;
  Vector nodes_;
  Vector weights_;
};

////////////////////////////////////////////////////////////////////////////////
// Double Gauss Quadrature
////////////////////////////////////////////////////////////////////////////////

/** Double Gauss Quadrature
 *
 * This class implements a double Gauss-Legendre for the integration of
 * functions of the interval [-1, 1]. The double Gauss-Legendre quadrature
 * of degree N consists of two mirrored Gauss-Legendre quadratures of
 * degree N / 2 covering * the sub-intervals [-1, 0] and [0, -1], respectively.
 */
class DoubleGaussQuadrature {
 private:
 public:
  DoubleGaussQuadrature() {}
  /** Create double Gauss-Legendre quadrature.
   * @param degree: The degree of the quadrature. Must be even.
   */
  DoubleGaussQuadrature(Index degree)
      : degree_(degree), nodes_(degree), weights_(degree) {
    ARTS_ASSERT(degree % 2 == 0);
    auto gq = GaussLegendreQuadrature(degree / 2);
    auto nodes = gq.get_nodes();
    auto weights = gq.get_weights();

    for (Index i = 0; i < degree / 2; ++i) {
      nodes_[i] = -0.5 + nodes[i] / 2.0;
      nodes_[degree / 2 + i] = 0.5 + nodes[i] / 2.0;
      weights_[i] = 0.5 * weights[i];
      weights_[degree / 2 + i] = 0.5 * weights[i];
    }
  }

  static constexpr QuadratureType type = QuadratureType::DoubleGauss;

  Index get_degree() const { return degree_; }
  const Vector& get_nodes() const { return nodes_; }
  const Vector& get_weights() const { return weights_; }

  Index degree_;
  Vector nodes_;
  Vector weights_;
};

/** Nodes and weights of the Lobatto quadrature.
 *
 * This class calculates the weights and nodes of a Lobatto quadrature. The
 * Lobatto quadrature includes the endpoints [-1, 1] of the integration interval
 * and the internal nodes are derived from Legendre polynomials.
 *
 * More information can be found here: https://mathworld.wolfram.com/LobattoQuadrature.html
 *
 */
class LobattoQuadrature {
 private:
  /// Calculate nodes and weights of the Lobatto quadrature.
  void calculate_nodes_and_weights() {
    const Index n = degree_;
    const Index n_half_nodes = (n + 1) / 2;
    const Index n_max_iter = 10;
    Numeric x, x_old, p_l, p_l_1, p_l_2, dp_dx, d2p_dx;

    const Index left = (n + 1) / 2 - 1;
    const Index right = n / 2;
    Numeric n_f = static_cast<Numeric>(n);

    for (int i = 0; i < n_half_nodes - 1; ++i) {
      //
      // Initial guess.
      //
      Numeric d_i = ((n % 2) == 0) ? 0.5 : 0.0;
      x = sin(pi_v<Numeric> * (i + d_i) / (n_f - 0.5));

      //
      // Evaluate Legendre Polynomial and its derivative at node.
      //
      for (Index j = 0; j < n_max_iter; ++j) {
        p_l = x;
        p_l_1 = 1.0;
        for (int l = 2; l < n; ++l) {
          // Legendre recurrence relation
          p_l_2 = p_l_1;
          p_l_1 = p_l;
          p_l = ((2.0 * l - 1.0) * x * p_l_1 - (l - 1.0) * p_l_2) / l;
        }
        dp_dx = (n_f - 1.0) * (x * p_l - p_l_1) / (x * x - 1.0);
        d2p_dx = (2.0 * x * dp_dx - (n_f - 1.0) * n_f * p_l) / (1.0 - x * x);

        x_old = x;

        //
        // Perform Newton step.
        //
        x -= dp_dx / d2p_dx;
        auto dx = x - x_old;
        if (detail::small(std::abs(dx * (x + x_old)), 1e-10)) {
          break;
        }
      }
      nodes_[right + i] = x;
      weights_[right + i] = 2.0 / (n_f * (n_f - 1.0) * p_l * p_l);
      nodes_[left - i] = -x;
      weights_[left - i] = weights_[right + i];
    }
    nodes_[0] = -1.0;
    weights_[0] = 2.0 / (n_f * (n_f - 1.0));
    nodes_[n - 1] = -nodes_[0];
    weights_[n - 1] = weights_[0];
  }

 public:
  LobattoQuadrature() {}
  LobattoQuadrature(Index degree)
      : degree_(degree), nodes_(degree), weights_(degree) {
    calculate_nodes_and_weights();
  }

  static constexpr QuadratureType type = QuadratureType::Lobatto;

  Index get_degree() const { return degree_; }
  const Vector& get_nodes() const { return nodes_; }
  const Vector& get_weights() const { return weights_; }

  Index degree_;
  Vector nodes_;
  Vector weights_;
};

////////////////////////////////////////////////////////////////////////////////
// ClenshawCurtis
////////////////////////////////////////////////////////////////////////////////

/** Clenshaw-Curtis quadrature
*
* This class implements the Clenshaw-Curtis quadrature rule, which uses
* the points
*  $x_i = cos(\pi * i / N)$ for $i = 0,\dots, N$.
*  as integration nodes.
*/
class ClenshawCurtisQuadrature {
 private:
  void calculate_nodes_and_weights();

 public:
  ClenshawCurtisQuadrature() {}
  ClenshawCurtisQuadrature(Index degree)
      : degree_(degree), nodes_(degree), weights_(degree) {
    calculate_nodes_and_weights();
  }

  static constexpr QuadratureType type = QuadratureType::ClenshawCurtis;

  Index get_degree() const { return degree_; }
  const Vector& get_nodes() const { return nodes_; }
  const Vector& get_weights() const { return weights_; }

  Index degree_;
  Vector nodes_;
  Vector weights_;
};

////////////////////////////////////////////////////////////////////////////////
// Fejer quadrature
////////////////////////////////////////////////////////////////////////////////

/** Fejer quadrature.
*
* This class implements the Fejer quadrature, which is similar to the
* Clenshaw-Curtis quadrature but uses the points
*  $x_i = cos(\pi * (i + 0.5) / N)$ for $i = 0,\dots, N-1$.
* as integration nodes.
*/
class FejerQuadrature {
 private:
  void calculate_nodes_and_weights();

 public:
  static constexpr QuadratureType type = QuadratureType::Fejer;

  FejerQuadrature() {}
  FejerQuadrature(Index degree)
      : degree_(degree), nodes_(degree), weights_(degree) {
    calculate_nodes_and_weights();
  }

  Index get_degree() const { return degree_; }
  const Vector& get_nodes() const { return nodes_; }
  const Vector& get_weights() const { return weights_; }

  Index degree_;
  Vector nodes_;
  Vector weights_;
};

template <typename Quadrature>
class QuadratureProvider {
 public:
  QuadratureProvider() {}

  Quadrature get_quadrature(Index degree) {
    auto found = quadratures_.find(degree);
    if (found != quadratures_.end()) {
      return found->second;
    } else {
      quadratures_.insert({degree, Quadrature(degree)});
      return quadratures_[degree];
    }
  }

 private:
  std::map<Index, Quadrature> quadratures_;
};

namespace detail {
/** Base class for zenith-angle grids.
*
* This class defines the basic interface for zenith-angle grids.
* It is used to store the cosine values of the grid points and,
* in addition to that, the integration weights of the corresponding
* quadrature.
*/
class ZenithAngleGrid : public Vector {
 public:
  ZenithAngleGrid() : Vector() {}
  ZenithAngleGrid(const Vector& zenith_angles) : Vector(zenith_angles) {}

  virtual ~ZenithAngleGrid(){};

  ZenithAngleGrid(const ZenithAngleGrid&) = default;
  ZenithAngleGrid& operator=(const ZenithAngleGrid&) = default;
  ZenithAngleGrid(ZenithAngleGrid&&) = default;
  ZenithAngleGrid& operator=(ZenithAngleGrid&&) = default;

  /// The cosine of the grid points.
  virtual const Vector& get_angle_cosines() const = 0;
  /// The grid points in radians.
  virtual const Vector& get_angles() const { return *this; }
  /// The integration weights.
  virtual const Vector& get_weights() const = 0;

  /// The type of quadrature.
  virtual QuadratureType get_type() = 0;
};
}


using ZenithAngleGridPtr = std::shared_ptr<detail::ZenithAngleGrid>;
using ConstZenithAngleGridPtr = std::shared_ptr<const detail::ZenithAngleGrid>;

class IrregularZenithAngleGrid : public detail::ZenithAngleGrid {
 public:
  IrregularZenithAngleGrid() {}
  /** Create new zenith-angle grid.
  * @param zenith_angles Vector containing the zenith-angle grid points in radians.
  * @param weights The integration weight corresponding to each grid point.
  */
  IrregularZenithAngleGrid(const Vector& zenith_angles)
      : ZenithAngleGrid(zenith_angles),
        weights_(zenith_angles.size()),
        cos_theta_(zenith_angles),
        type_(QuadratureType::Trapezoidal) {
    std::transform(
        cos_theta_.begin(),
        cos_theta_.end(),
        cos_theta_.begin(),
        [](Numeric lat) { return -1.0 * cos(Conversion::deg2rad(lat)); });
    weights_ = 0.0;
    Index n = static_cast<Index>(Vector::size());
    for (Index i = 0; i < n - 1; ++i) {
      auto dx = 0.5 * (cos_theta_[i + 1] - cos_theta_[i]);
      weights_[i] += dx;
      weights_[i + 1] += dx;
    }
    weights_[0] += cos_theta_[0] + 1.0;
    weights_[n - 1] += 1.0 - cos_theta_[n - 1];
  }

  /// The cosines of the zenith-angle grid points in radians.
  const Vector& get_angle_cosines() const { return cos_theta_; }

  /// The zenith-angle grid points in radians.
  const Vector& get_weights() const { return weights_; }

  /// The type of quadrature.
  QuadratureType get_type() { return QuadratureType::Trapezoidal; }

 protected:
  Vector weights_;
  Vector cos_theta_;
  QuadratureType type_;
};

template <typename Quadrature>
class QuadratureZenithAngleGrid : public detail::ZenithAngleGrid {
 public:
  /** Create new quadrature zenith-angle grid with given number of points.
  *
  * Creates a zenith-angle grid using the nodes and weights of the given quadrature
  * class as grid points.
  *
  * @tparam Quadrature The quadrature class to use to calculate the nodes and
  * weights of the quadrature.
  * @param degree The number of points of the quadrature.
  */
  QuadratureZenithAngleGrid() : detail::ZenithAngleGrid() {}
  QuadratureZenithAngleGrid(const QuadratureZenithAngleGrid&) = default;
  QuadratureZenithAngleGrid(Index n_points)
      : detail::ZenithAngleGrid(n_points), quadrature_(n_points) {
    auto nodes = quadrature_.get_nodes();
    std::transform(nodes.begin(), nodes.end(), begin(), [](Numeric x) {
      return Conversion::rad2deg(acos(-1.0 * x));
    });
  }

  QuadratureZenithAngleGrid(Index n_points, Index /*unused*/)
      : QuadratureZenithAngleGrid(n_points) {}

  Index get_degree() const { return quadrature_.get_degree(); }

  /// The cosines of the grid points in radians.
  const Vector& get_angle_cosines() const { return quadrature_.get_nodes(); }

  /// The integration weights.
  const Vector& get_weights() const { return quadrature_.get_weights(); }

  /// The type of quadrature.
  QuadratureType get_type() { return Quadrature::type; }

 protected:
  Quadrature quadrature_;
};

using GaussLegendreGrid = QuadratureZenithAngleGrid<GaussLegendreQuadrature>;
using DoubleGaussGrid = QuadratureZenithAngleGrid<DoubleGaussQuadrature>;
using LobattoGrid = QuadratureZenithAngleGrid<LobattoQuadrature>;
using FejerGrid = QuadratureZenithAngleGrid<FejerQuadrature>;

////////////////////////////////////////////////////////////////////////////////
// Integration functions
////////////////////////////////////////////////////////////////////////////////

static QuadratureProvider<FejerQuadrature> quadratures =
    QuadratureProvider<FejerQuadrature>();

using ZenithAngleGrid = std::variant<
  IrregularZenithAngleGrid,
  GaussLegendreGrid,
  LobattoGrid,
  FejerGrid
  >;

  inline Index grid_size(const ZenithAngleGrid &grid) {
    return std::visit([](const auto &grd) { return grd.size(); }, grid);
  }

  inline VectorView grid_vector(ZenithAngleGrid &grid) {
    return std::visit([](auto &grd) { return static_cast<VectorView>(grd); }, grid);
  }

   inline ConstVectorView grid_vector(const ZenithAngleGrid &grid) {
    return std::visit([](const auto &grd) { return static_cast<ConstVectorView>(grd); }, grid);
  }


template <typename VectorType>
auto integrate_zenith_angle(VectorType&& vec, const ZenithAngleGrid& grid) {
  typename std::remove_reference<decltype(vec[0])>::type result = vec[0];
  result *= 0.0;
  auto vec_it = vec.elem_begin();
  auto vec_end = vec.elem_end();
  auto weights_it = std::visit([](const auto &grd) {return grd.get_weights().begin();}, grid );
  for (; vec_it != vec_end; ++vec_it, ++weights_it) {
    result += *weights_it * *vec_it;
  }
  return result;
}

/** Integrate the given data over a spherical surface.
 *
 * @param data: Matrix containing the field evaluated at the given azimuth
 * and zenight-angle coordinates.
 * @param azimuth_angle_grid: The azimuth-angle grid.
 * @param zenith_angle_grid: The zenith-angle grid.
 *
 * @return The integral value.
 */
template <typename MatrixType>
Numeric integrate_sphere(MatrixType&& data,
                         const Vector& azimuth_angle_grid,
                         const ZenithAngleGrid& zenith_angle_grid) {
  Numeric result = 0.0;
  Index n = azimuth_angle_grid.size();

  Numeric zenith_angle_integral_first =
      integrate_zenith_angles(data.row(0), zenith_angle_grid);
  Numeric zenith_angle_integral_left = zenith_angle_integral_first;
  Numeric zenith_angle_integral_right = zenith_angle_integral_first;

  for (Index i = 0; i < n - 1; ++i) {
    zenith_angle_integral_right =
        integrate_zenith_angles<Numeric>(data.row(i + 1), zenith_angle_grid);
    Numeric dl = azimuth_angle_grid[i + 1] - azimuth_angle_grid[i];
    result += 0.5 * (zenith_angle_integral_left + zenith_angle_integral_right) * dl;
    zenith_angle_integral_left = zenith_angle_integral_right;
  }

  Numeric dl = 2.0 * pi_v<Numeric> + azimuth_angle_grid[0] - azimuth_angle_grid[n - 1];
  result += 0.5 * (zenith_angle_integral_first + zenith_angle_integral_right) * dl;

  return result;
}

/** Calculate integral preserving smoothing matrix.
 *
 * @param old_grid: The old, high-resolution grid.
 * @param new_grid: The new, low-resolution grid.
 */
template <std::floating_point Scalar>
Sparse calculate_downsampling_matrix(const VectorView& old_grid,
                                     const VectorView& new_grid) {
  using WeightMatrix = matpack::matpack_data<Scalar, 2>;
  Index n_old = old_grid.size();
  Index n_new = new_grid.size();
  matpack::matpack_data<Scalar, 1> limits(n_new + 1);
  for (Index i = 1; i < n_new; ++i) {
    limits[i] = 0.5 * (new_grid[i - 1] + new_grid[i]);
  }
  limits[n_new] = std::max(new_grid[n_new - 1], old_grid[n_old - 1]);
  limits[0] = std::min(new_grid[0], old_grid[0]);

  WeightMatrix weights(n_new, n_old);

  if (n_old == 1) {
    ArrayOfIndex row_inds(n_new);
    ArrayOfIndex col_inds(n_new);
    Vector comps(n_new);
    for (Index i_new = 0; i_new < n_new; ++i_new) {
      row_inds[i_new] = i_new;
      col_inds[i_new] = 0;
      comps[i_new] = 1.0;
    }
    Sparse result(n_new, n_old);
    result.insert_elements(n_new, row_inds, col_inds, comps);
    return result;
  }

  for (Index i_new = 0; i_new < n_new; ++i_new) {
    Scalar left_new = limits[i_new];
    Scalar right_new = limits[i_new + 1];
    for (Index i_old = 0; i_old < n_old - 1; ++i_old) {
      Scalar left_old = old_grid[i_old];
      Scalar right_old = old_grid[i_old < n_old - 1 ? i_old + 1 : n_old - 1];

      Scalar overlap =
          std::min(right_new, right_old) - std::max(left_new, left_old);
      if (overlap > 0.0) {
        weights(i_new, i_old) += 0.5 * overlap;
        weights(i_new, i_old + 1) += 0.5 * overlap;
        ;
      }
    }
    weights(i_new, joker) /= (right_new - left_new);
  }

  // Convert matrix to sparse.
  Index nnz = 0;
  std::for_each(weights.elem_begin(), weights.elem_end(), [&nnz](Numeric x) {
    if (x > 0.0) ++nnz;
  });
  ArrayOfIndex row_inds(nnz), col_inds(nnz);
  Vector comps(nnz);
  Index comp_index = 0;
  for (Index i_r = 0; i_r < weights.nrows(); ++i_r) {
    for (Index i_c = 0; i_c < weights.ncols(); ++i_c) {
      Numeric elem = weights(i_r, i_c);
      if (elem > 0.0) {
        row_inds[comp_index] = i_r;
        col_inds[comp_index] = i_c;
        comps[comp_index] = elem;
        ++comp_index;
      }
    }
  }
  Sparse result(n_new, n_old);
  result.insert_elements(nnz, row_inds, col_inds, comps);
  return result;
}


}  // namespace scattering

#endif
