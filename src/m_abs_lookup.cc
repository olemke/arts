/*!
  \file   m_absorption_lookup_table_data.cc
  \author Stefan Buehler <sbuehler@ltu.se>
  \date   Wed Nov 20 18:04:20 2002
  
  \brief  Methods related to absorption, lookup table, etc.
*/

#include <rtepack.h>
#include <workspace.h>

#include <algorithm>
#include <cmath>

#include "atm.h"
#include "debug.h"
#include "enums.h"
#include "gas_abs_lookup.h"
#include "interp.h"
#include "jacobian.h"
#include "m_basic_types.h"
#include "matpack_data.h"
#include "matpack_math.h"
#include "matpack_view.h"
#include "sorted_grid.h"
#include "species_tags.h"

/* Workspace method: Doxygen documentation will be auto-generated */
void absorption_lookup_table_dataInit(GasAbsLookup& x) {
  x = GasAbsLookup();
}

//! Find continuum species in absorption_species.
/*! 
  Returns an index array with indexes of those species in absorption_species
  that have continuum tags that require h2o_abs, and hence require
  nonlinear treatment in the absorption lookup table.

  H2O itself is ignored here since that is treated separately.

  We are a bit conservative here, it is possible that some of the
  continua do not really require H2O. Check yourself, if you want, and
  improve the guessing here.

  \retval   cont         indices of those species with continua.
  \param    absorption_species  list of absorption species.
  
  \author Stefan Buehler
  \date   2007-11-16
*/
void find_nonlinear_continua(
    ArrayOfIndex& cont, const ArrayOfArrayOfSpeciesTag& absorption_species) {
  cont.resize(0);

  // This is quite complicated, unfortunately. The approach here
  // is copied from abs_xsec_per_speciesAddConts. For explanation,
  // see there.

  // Loop tag groups:
  for (Size i = 0; i < absorption_species.size(); ++i) {
    // Loop tags in tag group
    for (Size s = 0; s < absorption_species[i].size(); ++s) {
      // Check for continuum tags
      if (absorption_species[i][s].type == SpeciesTagType::Predefined ||
          absorption_species[i][s].type == SpeciesTagType::Cia) {
        const String thisname = absorption_species[i][s].Name();
        // Ok, now we know this is a continuum tag.

        // Check whether we want nonlinear treatment for
        // this or not. We have three classes of continua:
        // 1. Those that we know do not require it
        // 2. Those that we know require h2o_abs
        // 3. Those for which we do not know

        // The list here is not at all perfect, just a first
        // guess. If you need particular models, you better
        // check that the right thing is done with your model.

        // 1. Continua known to not use h2o_abs
        // We take also H2O itself here, since this is
        // handled separately
        if (to<SpeciesEnum>("H2O") == absorption_species[i][s].Spec() ||
            "N2-" == thisname.substr(0, 3) || "CO2-" == thisname.substr(0, 4) ||
            "O2-CIA" == thisname.substr(0, 6) ||
            "O2-v0v" == thisname.substr(0, 6) ||
            "O2-v1v" == thisname.substr(0, 6) ||
            "H2-CIA" == thisname.substr(0, 6) ||
            "He-CIA" == thisname.substr(0, 6) ||
            "CH4-CIA" == thisname.substr(0, 7) ||
            "liquidcloud-MPM93" == thisname.substr(0, 17) ||
            "liquidcloud-ELL07" == thisname.substr(0, 17)) {
          break;
        }

        // 2. Continua known to use h2o_abs
        if ("O2-" == thisname.substr(0, 3)) {
          cont.push_back(i);
          break;
        }

        // 3. absorption_species tags that are NOT allowed in LUT
        // calculations
        if ("icecloud-" == thisname.substr(0, 9) ||
            "rain-" == thisname.substr(0, 5)) {
          std::ostringstream os;
          os << "Tag " << thisname << " not allowed in absorption "
             << "lookup tables.";
          throw std::runtime_error(os.str());
        }

        // If we get here, then the tag was neither in the
        // posivitive nor in the negative list. We through a
        // runtime error.
        std::ostringstream os;
        os << "Unknown whether tag " << thisname
           << " is a nonlinear species (i.e. uses h2o_abs) or not.\n"
           << "Cannot set abs_nls automatically.";
        throw std::runtime_error(os.str());
      }
    }
  }
}

//! Choose species for abs_nls
/*!
  Make an intelligent choice for abs_nls, based on absorption_species.

  \author Stefan Buehler

  \param[out] abs_nls     The list of nonlinear species.
  \param[in]  absorption_species Absorption species.
*/
void choose_abs_nls(ArrayOfArrayOfSpeciesTag& abs_nls,
                    const ArrayOfArrayOfSpeciesTag& absorption_species) {
  abs_nls.resize(0);

  // Add all H2O species as non-linear:
  Index next_h2o = 0;
  while (-1 !=
         (next_h2o = find_next_species(
              absorption_species, to<SpeciesEnum>("H2O"), next_h2o))) {
    abs_nls.push_back(absorption_species[next_h2o]);
    ++next_h2o;
  }

  // Certain continuum models also depend on abs_h2o. There is a
  // helper function that contains a list of these.
  ArrayOfIndex cont;
  find_nonlinear_continua(cont, absorption_species);

  // Add these to abs_nls:
  for (Size i = 0; i < cont.size(); ++i) {
    abs_nls.push_back(absorption_species[cont[i]]);
  }
}

//! Chose the temperature perturbations abs_t_pert
/*!  
  This simple function creates a vector of temperature
  perturbations, relative to the reference temperature profile, that
  covers the minimum and maximum temperature profile. 
  
  \author Stefan Buehler

  \param[out] abs_t_pert Temperature perturbations
  \param[in] abs_t       Reference temperature profile
  \param[in] tmin        Minimum temperature profile
  \param[in] tmax        Maximum temperature profile
  \param[in] t_step      Temperature perturbation step
*/
void choose_abs_t_pert(Vector& abs_t_pert,
                       ConstVectorView abs_t,
                       ConstVectorView tmin,
                       ConstVectorView tmax,
                       const Numeric& step,
                       const Index& p_interp_order,
                       const Index& t_interp_order) {
  // The code to find out the range for perturbation is a bit
  // complicated. The problem is that, since we use higher order
  // interpolation for p, we may require temperatures well outside the
  // local min/max range at the reference level. We solve this by
  // really looking at the min/max range for those levels required by
  // the p_interp_order.

  Numeric mindev = 1e9;
  Numeric maxdev = -1e9;

  Vector the_grid = uniform_grid(0, abs_t.nelem(), 1);
  for (Index i = 0; i < the_grid.nelem(); ++i) {
    const Index idx0 =
        my_interp::pos_finder<true>(i, Numeric(i), the_grid, p_interp_order);

    for (Index j = 0; j < p_interp_order + 1; ++j) {
      // Our pressure grid for the lookup table may be coarser than
      // the original one for the batch cases. This may lead to max/min
      // values in the original data that exceed those we assumed
      // above. We add some extra margin here to account for
      // that. (The margin is +-10K)

      Numeric delta_min = tmin[i] - abs_t[idx0 + j] - 10;
      Numeric delta_max = tmax[i] - abs_t[idx0 + j] + 10;

      if (delta_min < mindev) mindev = delta_min;
      if (delta_max > maxdev) maxdev = delta_max;
    }
  }

  // We divide the interval between mindev and maxdev, so that the
  // steps are of size *step* or smaller. But we also need at least
  // *t_interp_order*+1 points.
  Index div = t_interp_order;
  Numeric effective_step;
  do {
    effective_step = (maxdev - mindev) / (Numeric)div;
    ++div;
  } while (effective_step > step);

  abs_t_pert = uniform_grid(mindev, div, effective_step);
}

//! Chose the H2O perturbations abs_nls_pert
/*!  
  This simple function creates a vector of fractional H2O VMR
  perturbations, relative to the reference H2O profile, that
  covers the minimum and maximum profile. 
  
  \author Stefan Buehler

  \param[out] abs_nls_pert H2O VMR perturbations
  \param[in] refprof       Reference profile
  \param[in] minprof       Minimum profile
  \param[in] maxprof       Maximum profile
  \param[in] step          Fractional perturbation step
*/
void choose_abs_nls_pert(Vector& abs_nls_pert,
                         ConstVectorView refprof,
                         ConstVectorView minprof,
                         ConstVectorView maxprof,
                         const Numeric& step,
                         const Index& p_interp_order,
                         const Index& nls_interp_order) {
  // The code to find out the range for perturbation is a bit
  // complicated. The problem is that, since we use higher order
  // interpolation for p, we may require humidities well outside the
  // local min/max range at the reference level. We solve this by
  // really looking at the min/max range for those levels required by
  // the p_interp_order.

  Numeric mindev = 0;
  Numeric maxdev = -1e9;

  // mindev is set to zero from the start, since we always want to
  // include 0.

  Vector the_grid = uniform_grid(0, refprof.nelem(), 1);
  for (Index i = 0; i < the_grid.nelem(); ++i) {
    //       cout << "!!!!!! i = " << i << "\n";
    //       cout << " min/ref/max = " << minprof[i] << " / "
    //            << refprof[i] << " / "
    //            << maxprof[i] << "\n";

    const Index idx0 =
        my_interp::pos_finder<true>(i, Numeric(i), the_grid, p_interp_order);

    for (Index j = 0; j < p_interp_order + 1; ++j) {
      //           cout << "!!!!!! j = " << j << "\n";
      //           cout << "  ref[j] = " << refprof[gp.idx[j]] << "   ";
      //           cout << "  minfrac[j] = " << minprof[i] / refprof[gp.idx[j]] << "   ";
      //           cout << "  maxfrac[j] = " << maxprof[i] / refprof[gp.idx[j]] << "  \n";

      // Our pressure grid for the lookup table may be coarser than
      // the original one for the batch cases. This may lead to max/min
      // values in the original data that exceed those we assumed
      // above. We add some extra margin to the max value here to account for
      // that. (The margin is a factor of 2.)

      Numeric delta_min = minprof[i] / refprof[idx0 + j];
      Numeric delta_max = 2 * maxprof[i] / refprof[idx0 + j];

      if (delta_min < mindev) mindev = delta_min;
      // do not update maxdev, when delta_max is infinity (this results from
      // refprof being 0)
      if (!std::isinf(delta_max) && (delta_max > maxdev)) maxdev = delta_max;
    }
  }

  bool allownegative = false;
  if (mindev < 0) {
    allownegative = true;
  }

  if (!allownegative) {
    mindev = 0;
  }

  if (std::isinf(maxdev)) {
    std::ostringstream os;
    os << "Perturbation upper limit is infinity (likely due to the reference\n"
       << "profile being 0 at at least one pressure level). Can not work\n"
       << "with that.";
    throw std::runtime_error(os.str());
  }

  // We divide the interval between mindev and maxdev, so that the
  // steps are of size *step* or smaller. But we also need at least
  // *nls_interp_order*+1 points.
  Index div = nls_interp_order;
  Numeric effective_step;
  do {
    effective_step = (maxdev - mindev) / (Numeric)div;
    ++div;
  } while (effective_step > step);

  abs_nls_pert = uniform_grid(mindev, div, effective_step);

  // If there are negative values, we also add 0. The reason for this
  // is that 0 is a turning point.
  if (allownegative) {
    VectorInsertGridPoints(abs_nls_pert, abs_nls_pert, {0});
  }
}

/* Workspace method: Doxygen documentation will be auto-generated */
void absorption_speciesAdd(  // WS Output:
    ArrayOfArrayOfSpeciesTag& absorption_species,
    // Control Parameters:
    const ArrayOfString& names) {
  // Each element of the array of Strings names defines one tag
  // group. Let's work through them one by one.
  for (Size i = 0; i < names.size(); ++i) {
    absorption_species.emplace_back(names[i]);
  }
}

/* Workspace method: Doxygen documentation will be auto-generated */
void absorption_speciesInit(ArrayOfArrayOfSpeciesTag& absorption_species) {
  absorption_species.resize(0);
}

/* Workspace method: Doxygen documentation will be auto-generated */
void absorption_speciesSet(  // WS Output:
    ArrayOfArrayOfSpeciesTag& absorption_species,
    // Control Parameters:
    const ArrayOfString& names) try {
  absorption_species.resize(names.size());

  //cout << "Names: " << names << "\n";

  // Each element of the array of Strings names defines one tag
  // group. Let's work through them one by one.
  for (Size i = 0; i < names.size(); ++i) {
    // This part has now been moved to array_species_tag_from_string.
    // Call this function.
    absorption_species[i] = ArrayOfSpeciesTag(names[i]);
  }
} ARTS_METHOD_ERROR_CATCH

/* Workspace method: Doxygen documentation will be auto-generated */
void absorption_lookup_table_dataAdapt(
    GasAbsLookup& absorption_lookup_table_data,
    const ArrayOfArrayOfSpeciesTag& absorption_species,
    const AscendingGrid& frequency_grid) {
  absorption_lookup_table_data.Adapt(absorption_species,
                                             frequency_grid);
}

/* Workspace method: Doxygen documentation will be auto-generated */
void propagation_matrixAddFromLookup(
    PropmatVector& propagation_matrix,
    PropmatMatrix& propagation_matrix_jacobian,
    const GasAbsLookup& absorption_lookup_table_data,
    const AscendingGrid& frequency_grid,
    const AtmPoint& atm_point,
    const JacobianTargets& jacobian_targets,
    const ArrayOfArrayOfSpeciesTag& absorption_species,
    const SpeciesEnum& select_species,
    const Numeric& extpolfac,
    const Index& no_negatives,
    const Index& abs_p_interp_order,
    const Index& abs_t_interp_order,
    const Index& abs_nls_interp_order,
    const Index& abs_f_interp_order) {
  // Variables needed by absorption_lookup_table_data.Extract:
  Matrix abs_scalar_gas, dabs_scalar_gas_df, dabs_scalar_gas_dt;

  const auto do_freq_jac = jacobian_targets.find_all<Jacobian::AtmTarget>(
      AtmKey::wind_u, AtmKey::wind_v, AtmKey::wind_w);
  const auto do_temp_jac =
      jacobian_targets.find<Jacobian::AtmTarget>(AtmKey::t);
  const Numeric df = field_perturbation(do_freq_jac);
  const Numeric dt = field_perturbation(std::span{&do_temp_jac, 1});

  const Vector a_vmr_list = [&]() {
    Vector vmr(absorption_species.size());
    std::transform(absorption_species.begin(),
                   absorption_species.end(),
                   vmr.begin(),
                   [&](const ArrayOfSpeciesTag& spec) -> Numeric {
                     return atm_point[spec.Species()];
                   });
    return vmr;
  }();

  // The combination of doing frequency jacobian together with an
  // absorption lookup table is quite dangerous. If the frequency
  // interpolation order for the table is zero, the Jacobian will be
  // zero, and the cause for this may be difficult for a user to
  // find. So we do not allow this combination.
  if (df != 0.0 and (1 > abs_f_interp_order))
    throw std::runtime_error(
        "Wind/frequency Jacobian is not possible without at least first\n"
        "order frequency interpolation in the lookup table.  Please use\n"
        "abs_f_interp_order>0 or remove wind/frequency Jacobian.");

  // The function we are going to call here is one of the few helper
  // functions that adjust the size of their output argument
  // automatically.
  absorption_lookup_table_data.Extract(abs_scalar_gas,
                                               select_species,
                                               abs_p_interp_order,
                                               abs_t_interp_order,
                                               abs_nls_interp_order,
                                               abs_f_interp_order,
                                               atm_point.pressure,
                                               atm_point.temperature,
                                               a_vmr_list,
                                               frequency_grid,
                                               extpolfac);
  if (df != 0.0) {
    Vector dfreq = frequency_grid;
    dfreq += df;
    absorption_lookup_table_data.Extract(dabs_scalar_gas_df,
                                                 select_species,
                                                 abs_p_interp_order,
                                                 abs_t_interp_order,
                                                 abs_nls_interp_order,
                                                 abs_f_interp_order,
                                                 atm_point.pressure,
                                                 atm_point.temperature,
                                                 a_vmr_list,
                                                 dfreq,
                                                 extpolfac);
  }
  if (do_temp_jac.first) {
    const Numeric dtemp = atm_point.temperature + dt;
    absorption_lookup_table_data.Extract(dabs_scalar_gas_dt,
                                                 select_species,
                                                 abs_p_interp_order,
                                                 abs_t_interp_order,
                                                 abs_nls_interp_order,
                                                 abs_f_interp_order,
                                                 atm_point.pressure,
                                                 dtemp,
                                                 a_vmr_list,
                                                 frequency_grid,
                                                 extpolfac);
  }

  if (no_negatives) {
    //Check for negative values due to interpolation and set them to zero
    for (Index ir = 0; ir < abs_scalar_gas.nrows(); ir++) {
      for (Index ic = 0; ic < abs_scalar_gas.ncols(); ic++) {
        if (abs_scalar_gas(ir, ic) < 0) abs_scalar_gas(ir, ic) = 0;
      }
    }
  }

  // Now add to the right place in the absorption matrix.
  for (Index isp = 0; isp < abs_scalar_gas.nrows(); isp++) {
    for (Index iv = 0; iv < abs_scalar_gas.ncols(); iv++) {
      propagation_matrix[iv].A() += abs_scalar_gas(isp, iv);

      if (do_temp_jac.first) {
        const auto iq = do_temp_jac.second->target_pos;
        propagation_matrix_jacobian(iq, iv).A() +=
            (dabs_scalar_gas_dt(isp, iv) - abs_scalar_gas(isp, iv)) / dt;
      }

      for (auto& j : do_freq_jac) {
        if (j.first) {
          const auto iq = j.second->target_pos;
          propagation_matrix_jacobian(iq, iv).A() +=
              (dabs_scalar_gas_df(isp, iv) - abs_scalar_gas(isp, iv)) / df;
        }
      }

      if (const auto j = jacobian_targets.find<Jacobian::AtmTarget>(
              absorption_species[isp].Species());
          j.first) {
        const auto iq = j.second->target_pos;
        propagation_matrix_jacobian(iq, iv).A() +=
            (std::isnormal(a_vmr_list[isp]))
                ? abs_scalar_gas(isp, iv) / a_vmr_list[isp]
                : 0;
      }
    }
  }
}

/* Workspace method: Doxygen documentation will be auto-generated */
void frequency_gridFromGasAbsLookup(
    AscendingGrid& frequency_grid,
    const GasAbsLookup& absorption_lookup_table_data) {
  const Vector& lookup_frequency_grid =
      absorption_lookup_table_data.GetFgrid();
  frequency_grid = lookup_frequency_grid;
}

/* Workspace method: Doxygen documentation will be auto-generated */
void p_gridFromGasAbsLookup(
    Vector& p_grid, const GasAbsLookup& absorption_lookup_table_data) {
  const Vector& lookup_p_grid = absorption_lookup_table_data.GetPgrid();
  p_grid.resize(lookup_p_grid.nelem());
  p_grid = lookup_p_grid;
}
