#include "workspace_methods.h"

#include <mystring.h>

#include <exception>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "workspace_agendas.h"
#include "workspace_meta_methods.h"
#include "workspace_variables.h"

#if defined(__clang__)
#pragma clang optimize off
#endif

bool WorkspaceMethodInternalRecord::has_any() const {
  const auto cmp = Cmp::eq("Any");
  return std::ranges::any_of(gout_type, cmp) +
         std::ranges::any_of(gin_type, cmp);
}

bool WorkspaceMethodInternalRecord::has_overloads() const {
  for (auto& str : gout_type) {
    if (std::any_of(str.begin(), str.end(), Cmp::eq(','))) return true;
  }

  for (auto& str : gin_type) {
    if (std::any_of(str.begin(), str.end(), Cmp::eq(','))) return true;
  }

  return false;
}

std::string WorkspaceMethodInternalRecord::docstring() const try {
  std::stringstream os;

  os << "/** " << desc << "\n";
  if (pass_workspace) os << "  @param[in] ws Workspace reference\n";

  for (auto& str : out) {
    if (std::any_of(
            in.begin(), in.end(), [&str](auto& var) { return str == var; }))
      os << "  @param[inout] " << str << " As WSV" << '\n';
    else
      os << "  @param[out] " << str << " As WSV" << '\n';
  }

  for (std::size_t i = 0; i < gout.size(); i++) {
    os << "  @param[out] " << gout[i] << " " << gout_desc[i] << '\n';
  }

  for (auto& str : in) {
    if (std::any_of(
            out.begin(), out.end(), [&str](auto& var) { return str == var; }))
      continue;
    os << "  @param[in] " << str << " As WSV" << '\n';
  }

  for (std::size_t i = 0; i < gin.size(); i++) {
    os << "  @param[in] " << gin[i] << " " << gin_desc[i] << '\n';
  }

  os << " */";

  return os.str();
} catch (std::exception& e) {
  throw std::runtime_error("Error in meta-function docstring():\n\n" +
                           std::string(e.what()));
}

std::vector<std::vector<std::string>>
WorkspaceMethodInternalRecord::generic_overloads() const {
  const auto cmp = Cmp::eq(',');

  std::vector<std::vector<std::string>> genvar;

  for (auto& str : gout_type) {
    if (std::any_of(str.begin(), str.end(), cmp))
      genvar.push_back(split(str, ","));
    else
      genvar.push_back(std::vector<std::string>{str});
  }

  for (auto& str : gin_type) {
    if (std::any_of(str.begin(), str.end(), cmp))
      genvar.push_back(split(str, ","));
    else
      genvar.push_back(std::vector<std::string>{str});
  }

  for (auto& v : genvar) {
    for (auto& s : v) {
      trim(s);
    }
  }

  return genvar;
}

std::string WorkspaceMethodInternalRecord::header(const std::string& name,
                                                  int overload) const try {
  const static auto wsv = internal_workspace_variables();
  const static auto wsa = internal_workspace_agendas();

  const std::string spaces(name.size() + 6, ' ');

  const auto overloads = generic_overloads();
  int GVAR             = 0;

  std::stringstream os;

  if (has_any()) {
    os << "template <WorkspaceGroup T>\n";
  }

  os << "void " << name << '(';

  bool first = true;
  if (pass_workspace) {
    os << "const Workspace& ws";
    first = false;
  }

  for (auto& str : out) {
    auto wsv_ptr = wsv.find(str);
    if (wsv_ptr != wsv.end()) {
      os << comma(first, spaces) << wsv_ptr->second.type << "& " << str;
      continue;
    }

    auto wsa_ptr = wsa.find(str);
    if (wsa_ptr != wsa.end()) {
      if (wsa_ptr->second.array)
        os << comma(first, spaces) << "ArrayOfAgenda"
           << "& " << str;
      else
        os << comma(first, spaces) << "Agenda"
           << "& " << str;
      continue;
    }

    throw std::runtime_error("WorkspaceMethodInternalRecord::header " + name);
  }

  for (const auto& i : gout) {
    os << comma(first, spaces)
       << any(overloads[GVAR][std::min<int>(
              overload, static_cast<int>(overloads[GVAR].size() - 1))])
       << "& " << i;
    GVAR++;
  }

  for (auto& str : in) {
    if (std::any_of(
            out.begin(), out.end(), [&str](auto& var) { return str == var; }))
      continue;

    auto wsv_ptr = wsv.find(str);
    if (wsv_ptr != wsv.end()) {
      os << comma(first, spaces) << "const " << wsv_ptr->second.type << "& "
         << str;
      continue;
    }

    auto wsa_ptr = wsa.find(str);
    if (wsa_ptr != wsa.end()) {
      if (wsa_ptr->second.array)
        os << comma(first, spaces) << "const ArrayOfAgenda"
           << "& " << str;
      else
        os << comma(first, spaces) << "const Agenda"
           << "& " << str;
      continue;
    }

    throw std::runtime_error("WorkspaceMethodInternalRecord::header " + name);
  }

  for (const auto& i : gin) {
    os << comma(first, spaces) << "const "
       << any(overloads[GVAR][std::min<int>(
              overload, static_cast<int>(overloads[GVAR].size() - 1))])
       << "& " << i;
    GVAR++;
  }

  os << ")";

  return os.str();
} catch (std::exception& e) {
  throw std::runtime_error(var_string("Error in meta-function header(",
                                      '"',
                                      name,
                                      '"',
                                      ", ",
                                      overload,
                                      "):\n\n",
                                      e.what()));
}

int WorkspaceMethodInternalRecord::count_overloads() const try {
  const auto overloads = generic_overloads();
  int g                = 1;
  for (auto& x : overloads) {
    int ng = static_cast<int>(x.size());
    if (ng == 1) continue;
    if (g != ng and g != 1)
      throw std::runtime_error("Inconsistent number of overloads");
    g = ng;
  }
  return g;
} catch (std::exception& e) {
  throw std::runtime_error("Error in meta-function count_overloads():\n\n" +
                           std::string(e.what()));
}

std::string WorkspaceMethodInternalRecord::call(const std::string& name) const
    try {
  const std::string spaces(6, ' ');

  std::stringstream os;

  os << name << "(\n      ";

  bool first = true;
  if (pass_workspace) {
    os << "ws";
    first = false;
  }

  for (auto& str : out) {
    os << comma(first, spaces) << str;
  }

  for (const auto& i : gout) {
    os << comma(first, spaces) << i;
  }

  for (auto& str : in) {
    if (std::any_of(
            out.begin(), out.end(), [&str](auto& var) { return str == var; }))
      continue;

    os << comma(first, spaces) << str;
  }

  for (const auto& i : gin) {
    os << comma(first, spaces) << i;
  }

  os << ");";

  return os.str();
} catch (std::exception& e) {
  throw std::runtime_error(var_string(
      "Cannot create call of method ", '"', name, '"', ":\n\n", e.what()));
}

std::unordered_map<std::string, WorkspaceMethodInternalRecord>
internal_workspace_methods_create() try {
  std::unordered_map<std::string, WorkspaceMethodInternalRecord> wsm_data;

  for (auto& [agname, ag] : internal_workspace_agendas()) {
    std::vector<std::string> input = ag.input;
    input.push_back(agname);
    wsm_data[agname + "Execute"] = {
        .desc   = "Executes *" + agname + "*, see it for more details\n",
        .author = {"``Automatically Generated``"},
        .out    = ag.output,
        .in     = input,
        .pass_workspace = true,
    };
  }

  wsm_data["Ignore"] = {
      .desc      = R"--(Ignore a workspace variable.

This method is handy for use in agendas in order to suppress warnings
about unused input workspace variables. What it does is: Nothing!
In other words, it just ignores the variable it is called on.

This method can ignore any workspace variable you want.
)--",
      .author    = {"Stefan Buehler"},
      .gin       = {"input"},
      .gin_type  = {"Any"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Variable to be ignored.)--"},
  };

  wsm_data["surface_fieldSetPlanetEllipsoid"] = {
      .desc =
          R"--(Sets the planet base surface field

See *PlanetOrMoonType* for valid ``option``.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"surface_field"},
      .gin       = {"option"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Choice of planet or moon)--"},
  };

  wsm_data["ReadXML"] = {
      .desc      = R"--(Reads a workspace variable from an XML file.

This method can read variables of any group.

If the filename is omitted, the variable is read
from <basename>.<variable_name>.xml.
If the given filename does not exist, this method will
also look for files with an added .xml, .xml.gz and .gz extension
)--",
      .author    = {"Oliver Lemke"},
      .gout      = {"output"},
      .gout_type = {"Any"},
      .gout_desc = {R"--(Variable to be read.)--"},
      .gin       = {"filename"},
      .gin_type  = {"String"},
      .gin_value = {String("")},
      .gin_desc  = {R"--(Name of the XML file.)--"},
  };

  wsm_data["ReadXMLIndexed"] = {
      .desc      = R"--(As *ReadXML*, but reads indexed file names.

The variable is read from a file with name::

   <filename>.<file_index>.xml.

where <file_index> is the value of ``file_index``.

This means that ``filename`` shall here not include the .xml
extension. Omitting filename works as for *ReadXML*.
)--",
      .author    = {"Oliver Lemke"},
      .gout      = {"output"},
      .gout_type = {"Any"},
      .gout_desc = {R"--(Workspace variable to be read.)--"},
      .gin       = {"file_index", "filename", "digits"},
      .gin_type  = {"Index", "String", "Index"},
      .gin_value = {std::nullopt, String(""), Index{0}},
      .gin_desc =
          {R"--(Index of the file to read.)--",
           R"--(File name. See above.)--",
           R"--(Equalize the widths of all numbers by padding with zeros as necessary. 0 means no padding (default).)--"},
  };

  wsm_data["absorption_xsec_fit_dataReadSpeciesSplitCatalog"] = {
      .desc      = R"--(Reads HITRAN Crosssection coefficients

Reads coefficient files for HITRAN Xsec species defined
in *absorption_species*.
)--",
      .author    = {"Oliver Lemke"},
      .out       = {"absorption_xsec_fit_data"},
      .in        = {"absorption_species"},
      .gin       = {"basename"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Basepath to the files)--"},
  };

  wsm_data["Touch"] = {
      .desc      = R"--(As *Ignore* but for agenda output.

This method is handy for use in agendas in order to suppress
warnings about not-produced output workspace variables.

What it does, in case the variable is initialized already, is:
Nothing!
In case the variable is not yet initialized, it is set to NaN.
)--",
      .author    = {"Oliver Lemke"},
      .gout      = {"input"},
      .gout_type = {"Any"},
      .gout_desc = {R"--(Variable to do nothing with.)--"},
  };

  wsm_data["WignerInit"] = {
      .desc      = R"--(Initialize the Wigner tables

The default values take about 1 Gb memory.

The static data is kept in an external library and is therefore
only available inside ARTS.  Nevertheless, this must be set by
the application because any default value might be too small or
too large for the needs of any one application.
)--",
      .author    = {"Richard Larsson"},
      .gin       = {"fast_wigner_stored_symbols",
                    "largest_wigner_symbol_parameter",
                    "symbol_type"},
      .gin_type  = {"Index", "Index", "Index"},
      .gin_value = {Index{20000000}, Index{250}, Index{6}},
      .gin_desc =
          {R"--(Number of stored symbols possible before replacements)--",
           R"--(Largest symbol used for initializing factorials (e.g., largest J or L))--",
           "Type of symbol (3 or 6)"},
  };

  wsm_data["WignerUnload"] = {
      .desc =
          R"--(Unloads the Wigner tables from static data (see *WignerInit*)
)--",
      .author = {"Richard Larsson"},
  };

  wsm_data["WriteBuiltinPartitionFunctionsXML"] = {
      .desc      = R"--(Writes all the builtin partition functions to file.

All available partition functions are written to files in the select format
in the select directory

The temperature will be linearly spaced between [Tlow, Tupp] with N values

See *FileType* for valid ``output_file_format``.
)--",
      .author    = {"Richard Larsson"},
      .gin       = {"output_file_format", "dir", "Tlow", "Tupp", "N"},
      .gin_type  = {"String", "String", "Numeric", "Numeric", "Index"},
      .gin_value = {String{"ascii"},
                    std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    std::nullopt},
      .gin_desc  = {"The format of the output",
                    R"--(The directory to write the data towards)--",
                    R"--(The lowest temperature)--",
                    R"--(The highest temperature)--",
                    R"--(The number of temperature points)--"},
  };

  wsm_data["WriteXML"] = {
      .desc      = R"--(Writes a workspace variable to an XML file.

This method can write variables of any group.

If the filename is omitted, the variable is written
to <basename>.<variable_name>.xml.
If no_clobber is set to 1, an increasing number will be
appended to the filename if the file already exists.

See *FileType* for valid ``output_file_format``.
)--",
      .author    = {"Oliver Lemke"},
      .gin       = {"output_file_format", "input", "filename", "no_clobber"},
      .gin_type  = {"String", "Any", "String", "Index"},
      .gin_value = {String("ascii"), std::nullopt, String(""), Index{0}},
      .gin_desc =
          {"The format of the output",
           R"--(Variable to be saved.)--",
           R"--(Name of the XML file.)--",
           R"--(0: Overwrite existing files, 1: Use unique filenames)--"},
  };

  wsm_data["WriteXMLIndexed"] = {
      .desc   = R"--(As *WriteXML*, but creates indexed file names.

The variable is written to a file with name::

  <filename>.<file_index>.xml.

where <file_index> is the value of ``file_index``.

This means that ``filename`` shall here not include the .xml
extension. Omitting filename works as for *WriteXML*.

See *FileType* for valid ``output_file_format``.
)--",
      .author = {"Patrick Eriksson, Oliver Lemke"},
      .gin =
          {"output_file_format", "file_index", "input", "filename", "digits"},
      .gin_type = {"String", "Index", "Any", "String", "Index"},
      .gin_value =
          {String("ascii"), std::nullopt, std::nullopt, String(""), Index{0}},
      .gin_desc =
          {"The format of the output",
           R"--(Index number for files.)--",
           R"--(Workspace variable to be saved.)--",
           R"--(File name. See above.)--",
           R"--(Equalize the widths of all numbers by padding with zeros as necessary. 0 means no padding (default).)--"},
  };

  wsm_data["absorption_cia_dataAddCIARecord"] = {
      .desc =
          R"--(Takes CIARecord as input and appends the results in the appropriate place.

If CIARecord has same species as species in *absorption_cia_data*, then the array
position is used to append all of the CIARecord into the array.  If clobber
evaluates as true, cia_record overwrites the appropriate *absorption_cia_data*.  If
species in cia_record are not in *absorption_cia_data*, the CIARecord is pushed back.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_cia_data"},
      .in        = {"absorption_cia_data"},
      .gin       = {"cia_record", "clobber"},
      .gin_type  = {"CIARecord", "Index"},
      .gin_value = {std::nullopt, Index{0}},
      .gin_desc  = {R"--(CIA record to append to *absorption_cia_data*.)--",
                    R"--(If true, the new input clobbers the old cia data.)--"},
  };

  wsm_data["absorption_cia_dataReadFromCIA"] = {
      .desc =
          R"--(Read data from a CIA data file for all CIA molecules defined
in *absorption_species*.

The units in the HITRAN file are:
 - Frequency: cm^(-1)
 - Binary absorption cross-section: cm^5 molec^(-2)

Upon reading we convert this to the ARTS internal SI units 
of Hz and m^5 molec^(-2).
)--",
      .author    = {"Oliver Lemke"},
      .out       = {"absorption_cia_data"},
      .in        = {"absorption_species"},
      .gin       = {"catalogpath"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Path to the CIA catalog directory.)--"},
  };

  wsm_data["absorption_cia_dataReadFromXML"] = {
      .desc =
          R"--(Read data from a CIA XML file and check that all CIA tags defined
in *absorption_species* are present in the file.

The units of the data are described in *absorption_cia_dataReadFromCIA*.
)--",
      .author    = {"Oliver Lemke"},
      .out       = {"absorption_cia_data"},
      .in        = {"absorption_species"},
      .gin       = {"filename"},
      .gin_type  = {"String"},
      .gin_value = {String("")},
      .gin_desc  = {R"--(Name of the XML file.)--"},
  };

  wsm_data["absorption_cia_dataReadSpeciesSplitCatalog"] = {
      .desc      = R"--(Reads a species split CIA dataset.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_cia_data"},
      .in        = {"absorption_species"},
      .gin       = {"basename", "robust"},
      .gin_type  = {"String", "Index"},
      .gin_value = {std::nullopt, Index{0}},
      .gin_desc =
          {R"--(The path to the split catalog files)--",
           R"--(Flag to continue in case nothing is found [0 throws, 1 continues])--"},
  };

  wsm_data["abs_lines_per_speciesReadSpeciesSplitCatalog"] = {
      .desc =
          R"--(Reads old style catalog but only for *absorption_species*
)--",
      .author    = {"Richard Larsson"},
      .gout      = {"abs_lines_per_species"},
      .gout_type = {"ArrayOfArrayOfAbsorptionLines"},
      .gout_desc = {R"--(Absorption lines per species)--"},
      .in        = {"absorption_species"},
      .gin       = {"basename", "robust"},
      .gin_type  = {"String", "Index"},
      .gin_value = {std::nullopt, Index{0}},
      .gin_desc =
          {R"--(The path to the split catalog files)--",
           R"--(Flag to continue in case nothing is found [0 throws, 1 continues])--"},
  };

  wsm_data["absorption_predefined_model_dataReadSpeciesSplitCatalog"] = {
      .desc =
          R"--(Reads *absorption_predefined_model_data* catalog but only for *absorption_species*

If ``name_missing`` is true, missing models are set to named model, which is the most
common form of a predefined model.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_predefined_model_data"},
      .in        = {"absorption_species"},
      .gin       = {"basename", "name_missing"},
      .gin_type  = {"String", "Index"},
      .gin_value = {std::nullopt, Index{1}},
      .gin_desc  = {R"--(The path to the split catalog files)--",
                    R"--(Flag to name models that are missing)--"},
  };

  wsm_data["absorption_bandsFromAbsorbtionLines"] = {
      .desc      = R"--(Gets modern line catalog from old style
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_bands"},
      .in        = {"absorption_species"},
      .gin       = {"abs_lines_per_species"},
      .gin_type  = {"ArrayOfArrayOfAbsorptionLines"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Absorption lines per species)--"},
  };

  wsm_data["absorption_speciesDefineAll"] = {
      .desc   = R"--(Sets *absorption_species* [i][0] to all species in ARTS
)--",
      .author = {"Richard Larsson"},
      .out    = {"absorption_species"},
  };

  wsm_data["absorption_speciesSet"] = {
      .desc      = R"--(Set up a list of absorption species tag groups.

Workspace variables like *absorption_species* contain several tag
groups. Each tag group contains one or more tags. This method converts
descriptions of tag groups given in the keyword to the ARTS internal
representation (an *ArrayOfArrayOfSpeciesTag*). A tag group selects
spectral features which belong to the same species.

A tag is defined in terms of the name of the species, isotopologue, and a
range of frequencies. Species are named after the standard chemical
names, e.g., ``"O3"``. Isotopologues are given by the last digit of the atomic
weight, i.g., ``"O3-668"`` for the asymmetric ozone molecule including an
oxygen 18 atom. Groups of transitions are specified by giving a lower
and upper limit of a frequency range, e.g., ``"O3-666-500e9-501e9"``.

The symbol ``"*"`` acts as a wild card. Furthermore, frequency range or
frequency range and isotopologue may be omitted.

Example:

>>> species = [ "O3-666-500e9-501e9, O3-686", "O3", "H2O-PWR98" ]

   The first tag group selects all O3-666 lines between 500 and
   501 GHz plus all O3-686 lines. 

   The second tag group selects all remaining O3 transitions.

   The third tag group selects H2O, with one of the complete
   absorption models (Rosenkranz 98). No spectrocopic line catalogue
   data will be used for that third tag group.  For more available full
   absorption models see *propagation_matrixAddPredefined*

   Note that order of tag groups in the species list matters. In our
   example, changing the order of the first two tag group will give
   different results: as ``"O3"`` already selects all O3 transitions,
   no lines will remain to be selected by the
   ``"O3-666-500e9-501e9, O3-686"`` tag.

For CIA species the tag consists of the two involved species and
a dataset index. CIA species can be defined for multiple regions
The dataset index determines which region to use from the corresponding
CIARecord in *absorption_cia_data*.

Example

>>> species = [ "N2-CIA-N2-0, N2-CIA-N2-1" ]

For Hitran cross section species the tag consists of the species and
the tagtype XFIT, e.g. CFC11-XFIT. The data for the species must be
available in the *absorption_xsec_fit_data* variable.
)--",
      .author    = {"Stefan Buehler"},
      .out       = {"absorption_species"},
      .gin       = {"species"},
      .gin_type  = {"ArrayOfString"},
      .gin_value = {std::nullopt},
      .gin_desc =
          {R"--(Specify one String for each tag group that you want to create. Inside the String, separate the tags by commas (plus optional blanks).)--"},
  };

  wsm_data["atmospheric_fieldIGRF"] = {
      .desc      = R"--(Use IGRF to compute the magnetic field at each point.

The IGRF model is a model of the Earth's magnetic field. It is based on
spherical harmonics and is only valid for a limited time period.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field"},
      .gin       = {"time"},
      .gin_type  = {"Time"},
      .gin_value = {Time{}},
      .gin_desc  = {"Time of data to use"},
  };

  wsm_data["atmospheric_fieldInit"] = {
      .desc =
          R"--(Initialize the atmospheric field with some altitude and isotopologue ratios

See *IsoRatioOption* for valid ``default_isotopologue``.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .gin       = {"toa", "default_isotopologue"},
      .gin_type  = {"Numeric", "String"},
      .gin_value = {std::nullopt, String{"Builtin"}},
      .gin_desc  = {R"--(Top of atmosphere altitude [m].)--",
                    "Default option for the isotopologue ratios"},
  };

  wsm_data["atmospheric_pointInit"] = {
      .desc =
          R"--(Initialize an atmospheric point with some isotopologue ratios

See *IsoRatioOption* for valid ``default_isotopologue``.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_point"},
      .gin       = {"default_isotopologue"},
      .gin_type  = {"String"},
      .gin_value = {String{"Builtin"}},
      .gin_desc  = {"Default option for the isotopologue ratios"},
  };

  wsm_data["transmission_matrix_backgroundFromPathPropagationBack"] = {
      .desc =
          R"--(Sets *transmission_matrix_background* to back of *ray_path_transmission_matrix_cumulative*
)--",
      .author = {"Richard Larsson"},
      .out    = {"transmission_matrix_background"},
      .in     = {"ray_path_transmission_matrix_cumulative"},
  };

  wsm_data["transmission_matrix_backgroundFromPathPropagationFront"] = {
      .desc =
          R"--(Sets *transmission_matrix_background* to front of *ray_path_transmission_matrix_cumulative*
)--",
      .author = {"Richard Larsson"},
      .out    = {"transmission_matrix_background"},
      .in     = {"ray_path_transmission_matrix_cumulative"},
  };

  wsm_data["disort_settings_agendaSet"] = {
      .desc      = R"--(Sets *disort_settings_agenda*

See *disort_settings_agendaPredefined* for valid ``option``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"disort_settings_agenda"},
      .gin       = {"option"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Default agenda option (see description))--"},
  };

  wsm_data["spectral_radiance_observer_agendaSet"] = {
      .desc      = R"--(Sets *spectral_radiance_space_agenda*

See *spectral_radiance_space_agendaPredefined* for valid ``option``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"spectral_radiance_observer_agenda"},
      .gin       = {"option"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Default agenda option (see description))--"},
  };

  wsm_data["ray_path_zeeman_magnetic_fieldFromPath"] = {
      .desc      = R"--(Sets A path of Zeeman effec magnetic field properties.

This will return a list of magnetic field properties along the path.
The magnetic properties in Zeeman coordinates are the absolute strength [H],
the angle between the magnetic field and the line of sight [theta], and the
the rotation of the magnetic field in the plane perpendicular to the line of
sight [eta].
)--",
      .author    = {"Richard Larsson"},
      .gout      = {"ray_path_zeeman_magnetic_field"},
      .gout_type = {"ArrayOfVector3"},
      .gout_desc = {R"--(Along-the-path [H, theta, eta])--"},
      .in        = {"ray_path", "ray_path_atmospheric_point"},
  };

  wsm_data["ray_path_observer_agendaSet"] = {
      .desc      = R"--(Sets *ray_path_observer_agenda*

See *ray_path_observer_agendaPredefined* for valid ``option``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"ray_path_observer_agenda"},
      .gin       = {"option"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Default agenda option (see description))--"},
  };

  wsm_data["spectral_radiance_space_agendaSet"] = {
      .desc      = R"--(Sets *spectral_radiance_space_agenda*

See *spectral_radiance_space_agendaPredefined* for valid ``option``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"spectral_radiance_space_agenda"},
      .gin       = {"option"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Default agenda option (see description))--"},
  };

  wsm_data["spectral_radiance_surface_agendaSet"] = {
      .desc      = R"--(Sets *spectral_radiance_surface_agenda*

See *spectral_radiance_surface_agendaPredefined* for valid ``option``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"spectral_radiance_surface_agenda"},
      .gin       = {"option"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Default agenda option (see description))--"},
  };

  wsm_data["ecs_dataAddMakarov2020"] = {
      .desc   = R"--(Sets the O2-66 microwave band data for ECS.
)--",
      .author = {"Richard Larsson"},
      .out    = {"ecs_data"},
      .in     = {"ecs_data"},
  };

  wsm_data["ecs_dataAddTran2011"] = {
      .desc   = R"--(Sets the CO2-626, CO2-628, and CO2-636 band data for ECS.

Sets CO2 species
)--",
      .author = {"Richard Larsson"},
      .out    = {"ecs_data"},
      .in     = {"ecs_data"},
  };

  wsm_data["ecs_dataAddRodrigues1997"] = {
      .desc   = R"--(Sets the CO2-626, CO2-628, and CO2-636 band data for ECS.

Sets N2 and O2 speces
)--",
      .author = {"Richard Larsson"},
      .out    = {"ecs_data"},
      .in     = {"ecs_data"},
  };

  wsm_data["ecs_dataInit"] = {
      .desc   = R"--(Resets/initializes the ECS data.
)--",
      .author = {"Richard Larsson"},
      .out    = {"ecs_data"},
  };

  wsm_data["ecs_dataAddMeanAir"] = {
      .desc      = R"--(Sets ECS data for air from other data if available.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"ecs_data"},
      .in        = {"ecs_data"},
      .gin       = {"vmrs", "species"},
      .gin_type  = {"Vector", "ArrayOfSpeciesEnum"},
      .gin_value = {std::nullopt, std::nullopt},
      .gin_desc  = {R"--(VMRs of air species)--", R"--(Air species)--"},
  };

  wsm_data["ray_path_atmospheric_pointFromPath"] = {
      .desc   = R"--(Gets the atmospheric points along the path.
)--",
      .author = {"Richard Larsson"},
      .out    = {"ray_path_atmospheric_point"},
      .in     = {"ray_path", "atmospheric_field"},
  };

  wsm_data["ray_path_transmission_matrix_cumulativeFromPath"] = {
      .desc =
          R"--(Sets *ray_path_transmission_matrix_cumulative* by forward iteration of *ray_path_transmission_matrix*
)--",
      .author = {"Richard Larsson"},
      .out    = {"ray_path_transmission_matrix_cumulative"},
      .in     = {"ray_path_transmission_matrix"},
  };

  wsm_data["ray_path_frequency_gridFromPath"] = {
      .desc      = R"--(Gets the frequency grid along the path.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"ray_path_frequency_grid"},
      .in        = {"frequency_grid", "ray_path", "ray_path_atmospheric_point"},
      .gin       = {"rte_alonglos_v"},
      .gin_type  = {"Numeric"},
      .gin_value = {Numeric{0.0}},
      .gin_desc =
          {R"--(Velocity along the line-of-sight to consider for a RT calculation.)--"},
  };

  wsm_data["ray_path_propagation_matrixFromPath"] = {
      .desc =
          R"--(Gets the propagation matrix and non-LTE source term along the path.

The calculations are in parallel if the program is not in parallel already.
)--",
      .author         = {"Richard Larsson"},
      .out            = {"ray_path_propagation_matrix",
                         "ray_path_propagation_matrix_source_vector_nonlte",
                         "ray_path_propagation_matrix_jacobian",
                         "ray_path_propagation_matrix_source_vector_nonlte_jacobian"},
      .in             = {"propagation_matrix_agenda",
                         "jacobian_targets",
                         "ray_path_frequency_grid",
                         "ray_path",
                         "ray_path_atmospheric_point"},
      .pass_workspace = true,
  };

  wsm_data["ray_path_propagation_matrix_scatteringFromPath"] = {
      .desc =
          R"--(Gets the propagation matrix for scattering along the path.

The calculations are in parallel if the program is not in parallel already.
)--",
      .author         = {"Richard Larsson"},
      .out            = {"ray_path_propagation_matrix_scattering"},
      .in             = {"propagation_matrix_scattering_agenda",
                         "ray_path_frequency_grid",
                         "ray_path_atmospheric_point"},
      .pass_workspace = true,
  };

  wsm_data["ray_path_propagation_matrixAddScattering"] = {
      .desc =
          R"--(Adds the scattering part of the propagation matrix to the rest along the path.

The calculations are in parallel if the program is not in parallel already.
)--",
      .author = {"Richard Larsson"},
      .out    = {"ray_path_propagation_matrix"},
      .in     = {"ray_path_propagation_matrix",
                 "ray_path_propagation_matrix_scattering"},
  };

  wsm_data["ray_path_spectral_radiance_sourceAddScattering"] = {
      .desc =
          R"--(Adds the scattering part of the propagation matrix to the rest along the path.

The calculations are in parallel if the program is not in parallel already.
)--",
      .author = {"Richard Larsson"},
      .out    = {"ray_path_spectral_radiance_source"},
      .in     = {"ray_path_spectral_radiance_source",
                 "ray_path_spectral_radiance_scattering",
                 "ray_path_propagation_matrix"},
  };

  wsm_data["ray_path_spectral_radiance_sourceFromPropmat"] = {
      .desc   = R"--(Gets the source term along the path.
)--",
      .author = {"Richard Larsson"},
      .out    = {"ray_path_spectral_radiance_source",
                 "ray_path_spectral_radiance_source_jacobian"},
      .in     = {"ray_path_propagation_matrix",
                 "ray_path_propagation_matrix_source_vector_nonlte",
                 "ray_path_propagation_matrix_jacobian",
                 "ray_path_propagation_matrix_source_vector_nonlte_jacobian",
                 "ray_path_frequency_grid",
                 "ray_path_atmospheric_point",
                 "jacobian_targets"},
  };

  wsm_data["absorption_predefined_model_dataAddWaterMTCKD400"] = {
      .desc      = R"--(Sets the data for MT CKD 4.0 Water model

Note that the vectors must have the same length, and that wavenumbers must be growing
at a constant rate.  The minimum length is 4.

Note also that as this is predefined model data, the units of the values of the vectors
must be as described by each vector.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_predefined_model_data"},
      .in        = {"absorption_predefined_model_data"},
      .gin       = {"ref_temp",
                    "ref_press",
                    "ref_h2o_vmr",
                    "self_absco_ref",
                    "for_absco_ref",
                    "wavenumbers",
                    "self_texp"},
      .gin_type  = {"Numeric",
                    "Numeric",
                    "Numeric",
                    "Vector",
                    "Vector",
                    "Vector",
                    "Vector"},
      .gin_value = {std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    std::nullopt},
      .gin_desc  = {R"--(Reference temperature)--",
                    R"--(Reference pressure)--",
                    R"--(Reference volume mixing ratio of water)--",
                    R"--(Self absorption [1/(cm-1 molecules/cm^2])--",
                    R"--(Foreign absorption [1/(cm-1 molecules/cm^2)])--",
                    R"--(Wavenumbers [cm-1])--",
                    R"--(Self temperature exponent [-])--"},
  };

  wsm_data["absorption_predefined_model_dataInit"] = {
      .desc   = R"--(Initialize the predefined model data
)--",
      .author = {"Richard Larsson"},
      .out    = {"absorption_predefined_model_data"},
  };

  wsm_data["propagation_matrixAddCIA"] = {
      .desc =
          R"--(Calculate absorption coefficients per tag group for HITRAN CIA continua.

This interpolates the cross sections from *absorption_cia_data*.

The robust option is intended only for testing. Do not use for normal
runs, since subsequent functions will not be able to deal with NAN values.
)--",
      .author    = {"Stefan Buehler, Oliver Lemke"},
      .out       = {"propagation_matrix", "propagation_matrix_jacobian"},
      .in        = {"propagation_matrix",
                    "propagation_matrix_jacobian",
                    "propagation_matrix_select_species",
                    "jacobian_targets",
                    "frequency_grid",
                    "atmospheric_point",
                    "absorption_cia_data"},
      .gin       = {"T_extrapolfac", "ignore_errors"},
      .gin_type  = {"Numeric", "Index"},
      .gin_value = {Numeric{0.5}, Index{0}},
      .gin_desc =
          {R"--(Temperature extrapolation factor (relative to grid spacing).)--",
           R"--(Set to 1 to suppress runtime errors (and return NAN values instead).)--"},
  };

  wsm_data["propagation_matrixAddFaraday"] = {
      .desc   = R"--(Calculates absorption matrix describing Faraday rotation.

Faraday rotation is a change of polarization state of an
electromagnetic wave propagating through charged matter by
interaction with a magnetic field. Hence, this method requires
*absorption_species* to contain 'free_electrons' and electron content field
(as part of ``vmr_field``) as well as magnetic field (``mag_u_field``,
``mag_v_field``, ``mag_w_field``) to be specified.

Faraday rotation affects Stokes parameters 2 and 3 (but not
intensity!). Therefore, this method requires stokes_dim>2.

Like all 'propagation_matrixAdd*' methods, the method is additive,
i.e., does not overwrite the propagation matrix *propagation_matrix*,
but adds further contributions.
)--",
      .author = {"Patrick Eriksson"},
      .out    = {"propagation_matrix", "propagation_matrix_jacobian"},
      .in     = {"propagation_matrix",
                 "propagation_matrix_jacobian",
                 "frequency_grid",
                 "absorption_species",
                 "propagation_matrix_select_species",
                 "jacobian_targets",
                 "atmospheric_point",
                 "ray_path_point"},
  };

  wsm_data["propagation_matrixAddPredefined"] = {
      .desc =
          R"--(Adds all of the predefined models in *absorption_species* to the propagation_matrix

Only supports temperature and wind speed derivatives

Available models:

- O2-MPM2020:
  60 GHz and 118 GHz lines only (no continua, no higher Hz line centers) 

  Dmitriy S. Makarov, Mikhail Yu. Tretyakov, Philip W. Rosenkranz, JQSRT 243, 2020, Revision of the 
  60-GHz atmospheric oxygen absorption band models for practical use, 
  https://doi.org/10.1016/j.jqsrt.2019.106798

- H2O-ForeignContCKDMT350:
  Foreign continua.  Expects H2O line center cutoff at 25 cm-1

  CKD_MTv3.50 H2O foreign continuum from the FORTRAN77 code written by Atmospheric and Environmental Research Inc. (AER),
  Radiation and Climate Group 131 Hartwell Avenue Lexington, MA 02421, USA
  http://www.rtweb.aer.com/continuum_frame.html

- H2O-SelfContCKDMT350:
  Self continua.  Expects H2O line center cutoff at 25 cm-1

  CKD_MTv3.50 H2O self continuum from the FORTRAN77 code written by Atmospheric and Environmental Research Inc. (AER),
  Radiation and Climate Group 131 Hartwell Avenue Lexington, MA 02421, USA
  http://www.rtweb.aer.com/continuum_frame.html

- H2O-ForeignContCKDMT320:
  Foreign continua.  Expects H2O line center cutoff at 25 cm-1

  CKD_MTv3.20 H2O foreign continuum from the FORTRAN77 code written by Atmospheric and Environmental Research Inc. (AER),
  Radiation and Climate Group 131 Hartwell Avenue Lexington, MA 02421, USA
  http://www.rtweb.aer.com/continuum_frame.html

- H2O-SelfContCKDMT320:
  Self continua.  Expects H2O line center cutoff at 25 cm-1

  CKD_MTv3.20 H2O self continuum from the FORTRAN77 code written by Atmospheric and Environmental Research Inc. (AER),
  Radiation and Climate Group 131 Hartwell Avenue Lexington, MA 02421, USA
  http://www.rtweb.aer.com/continuum_frame.html

- H2O-SelfContCKDMT400:
  Self continuum for water.  General reference: Mlawer et al. (2012), doi:10.1098/rsta.2011.0295

  Our code is reimplemented based on original Fortran90 code that is/was/will-be-made available via hitran.org

  Note that this model comes with the copyright statement [1].

  Note also that this model requires *absorption_predefined_model_data* to contain relevant data set either using
  *absorption_predefined_model_dataAddWaterMTCKD400* or via some file reading routine.

- H2O-ForeignContCKDMT400:
  Foreign continuum for water.  General reference: Mlawer et al. (2012), doi:10.1098/rsta.2011.0295

  Our code is reimplemented based on original Fortran90 code that is/was/will-be-made available via hitran.org

  Note that this model comes with the copyright statement [1].

  Note also that this model requires *absorption_predefined_model_data* to contain relevant data set either using
  *absorption_predefined_model_dataAddWaterMTCKD400* or via some file reading routine.

- H2O-ForeignContStandardType:
  Water microwave continua

  P. W. Rosenkranz., Radio Science, 33(4), 919, 1998 and
  Radio Science, Vol. 34(4), 1025, 1999.

- H2O-SelfContStandardType:
  Water microwave continua

  P. W. Rosenkranz., Radio Science, 33(4), 919, 1998 and
  Radio Science, Vol. 34(4), 1025, 1999.

- H2O-MPM89:
  Microwave water absorption model

  H. J. Liebe, Int. J. Infrared and Millimeter Waves, 10(6), 1989, 631.

- H2O-PWR98:
  Microwave water absorption model

  P. W. Rosenkranz., Radio Science, 33(4), 919, 1998 and
  Radio Science, Vol. 34(4), 1025, 1999.

- H2O-PWR2021:
  Microwave water absorption model developed by P.W. Rosenkranz.

  Our code is reimplemented based on the Fortran code available at http://cetemps.aquila.infn.it/mwrnet/lblmrt_ns.html

- H2O-PWR2022:
  Microwave water absorption model developed by P.W. Rosenkranz.

  Our code is reimplemented based on the Fortran code available at http://cetemps.aquila.infn.it/mwrnet/lblmrt_ns.html

- CO2-CKDMT252:
  MT CKD absorption for CO2

  This absorption model is taken from the FORTRAN77 code of
  CKD_MT version 2.50 written by<br>
  Atmospheric and Environmental Research Inc. (AER),<br>
  Radiation and Climate Group<br>
  131 Hartwell Avenue<br>
  Lexington, MA 02421, USA<br>
  http://www.rtweb.aer.com/continuum_frame.html

- O2-CIAfunCKDMT100:
  CIA for oxygen from MT CKD

  F. Thibault, V. Menoux, R. Le Doucen, L. Rosenman,
  J.-M. Hartmann, Ch. Boulet,<br>
  Infrared collision-induced absorption by O2 near 6.4 microns for
  atmospheric applications: measurements and emprirical modeling,<br>
  Appl. Optics, 35, 5911-5917, (1996).

  This absorption model is taken from the FORTRAN77 code of
  CKD_MT version 1.00 written by<br>
  Atmospheric and Environmental Research Inc. (AER),<br>
  Radiation and Climate Group<br>
  131 Hartwell Avenue<br>
  Lexington, MA 02421, USA<br>
  http://www.rtweb.aer.com/continuum_frame.html

- O2-MPM89:
  Oxygen microwave absorption model

  Reference: H. J. Liebe and G. A. Hufford and M. G. Cotton,<br>
  <i>Propagation modeling of moist air and suspended water/ice
  particles at frequencies below 1000 GHz</i>,<br>
  AGARD 52nd Specialists Meeting of the Electromagnetic Wave
  Propagation Panel,<br> Palma de Mallorca, Spain, 1993, May 17-21

- O2-PWR98:
  Oxygen microwave absorption model

  P.W. Rosenkranz, CHAP. 2 and appendix, in ATMOSPHERIC REMOTE SENSING
  BY MICROWAVE RADIOMETRY (M.A. Janssen, ed., 1993).
  H.J. Liebe et al, JQSRT V.48, PP.629-643 (1992).
  M.J. Schwartz, Ph.D. thesis, M.I.T. (1997).
  SUBMILLIMETER LINE INTENSITIES FROM HITRAN96.

- O2-PWR2021:
  Oxygen microwave absorption model developed by P.W. Rosenkranz.

  Our code is reimplemented based on the Fortran code available at http://cetemps.aquila.infn.it/mwrnet/lblmrt_ns.html

- O2-PWR2022:
  Oxygen microwave absorption model developed by P.W. Rosenkranz.

  Our code is reimplemented based on the Fortran code available at http://cetemps.aquila.infn.it/mwrnet/lblmrt_ns.html

- O2-SelfContStandardType:
  Microwave continua term

  Reference: P. W. Rosenkranz, Chapter 2, in M. A. Janssen, <br>
  <I>Atmospheric Remote Sensing by Microwave Radiometry</i>,<br>
  John Wiley & Sons, Inc., 1993.<br>
  <br>
  Reference: H. J. Liebe and G. A. Hufford and M. G. Cotton,<br>
  <i>Propagation modeling of moist air and suspended water/ice
  particles at frequencies below 1000 GHz</i>,<br>
  AGARD 52nd Specialists Meeting of the Electromagnetic Wave
  Propagation Panel,<br> Palma de Mallorca, Spain, 1993, May 17-21

- O2-TRE05:
  Oxygen microwave absorption model

  References: H. J. Liebe and G. A. Hufford and M. G. Cotton,<br>
  <i>Propagation modeling of moist air and suspended water/ice
  particles at frequencies below 1000 GHz</i>,<br>
  AGARD 52nd Specialists Meeting of the Electromagnetic Wave
  Propagation Panel,<br> Palma de Mallorca, Spain, 1993, May 17-21

  M.Yu. Tretyakov, M.A. Koshelev, V.V. Dorovskikh,
  D.S. Makarov, P.W. Rosenkranz; 60-GHz oxygen band: precise broadening and central frequencies
  of fine-structure lines, absolute absorption profile
  at atmospheric pressure, and revision of mixing coefficients
  doi:10.1016/j.jms.2004.11.011

- O2-v0v0CKDMT100:
  MT CKD

  CKD_MT 1.00 implementation of oxygen collision induced fundamental model of
  O2 continuum formulated by
  Mate et al. over the spectral region 7550-8486 cm-1:
  B. Mate, C. Lugez, G.T. Fraser, W.J. Lafferty,
  "Absolute Intensities for the O2 1.27 micron
  continuum absorption",
  J. Geophys. Res., 104, 30,585-30,590, 1999.

  Also, refer to the paper "Observed  Atmospheric
  Collision Induced Absorption in Near Infrared Oxygen Bands",
  Mlawer, Clough, Brown, Stephen, Landry, Goldman, & Murcray,
  Journal of Geophysical Research (1997).

  This absorption model is taken from the FORTRAN77 code of
  CKD_MT version 1.00 written by<br>
  Atmospheric and Environmental Research Inc. (AER),<br>
  Radiation and Climate Group<br>
  131 Hartwell Avenue<br>
  Lexington, MA 02421, USA<br>
  http://www.rtweb.aer.com/continuum_frame.html<br>
  <br>

- O2-v1v0CKDMT100:
  MT CKD

  Mlawer, Clough, Brown, Stephen, Landry, Goldman, Murcray,<br>
  Observed  Atmospheric Collision Induced Absorption in Near Infrared Oxygen Bands,<br>
  J. Geophys. Res., 103, D4, 3859-3863, 1998.

  This absorption model is taken from the FORTRAN77 code of
  CKD_MT version 1.00 written by<br>
  Atmospheric and Environmental Research Inc. (AER),<br>
  Radiation and Climate Group<br>
  131 Hartwell Avenue<br>
  Lexington, MA 02421, USA<br>
  http://www.rtweb.aer.com/continuum_frame.html<br>

- O2-visCKDMT252:
  MT CKD

  O2 continuum formulated by Greenblatt et al. over the spectral region
  8797-29870 cm-1:  "Absorption Coefficients of Oxygen Between 
  330 and 1140 nm, G.D. Green blatt, J.J. Orlando, J.B. Burkholder,
  and A.R. Ravishabkara,  J. Geophys. Res., 95, 18577-18582, 1990.

  This absorption model is taken from the FORTRAN77 code of
  CKD_MT version 2.50 written by<br>
  Atmospheric and Environmental Research Inc. (AER),<br>
  Radiation and Climate Group<br>
  131 Hartwell Avenue<br>
  Lexington, MA 02421, USA<br>
  http://www.rtweb.aer.com/continuum_frame.html<br>

- N2-CIAfunCKDMT252:
  MT CKD

  Lafferty, W.J., A.M. Solodov,A. Weber, W.B. Olson and
  J._M. Hartmann,<br>
  Infrared collision-induced absorption by
  N2 near 4.3 microns for atmospheric applications:
  Measurements and emprirical modeling, <br>
  Appl. Optics, 35, 5911-5917, (1996)

  This absorption model is taken from the FORTRAN77 code of
  CKD_MT version 1.00 written by<br>
  Atmospheric and Environmental Research Inc. (AER),<br>
  Radiation and Climate Group<br>
  131 Hartwell Avenue<br>
  Lexington, MA 02421, USA<br>
  http://www.rtweb.aer.com/continuum_frame.html

- N2-CIArotCKDMT252:
  MT CKD

  Borysow, A, and L. Frommhold,<br>
  Collision-induced rototranslational absorption spectra of N2-N2
  pairs for temperatures from 50 to 300 K,<br>
  The Astrophysical Journal, 311, 1043-1057, 1986.

  This absorption model is taken from the FORTRAN77 code of
  CKD_MT version 1.00 written by<br>
  Atmospheric and Environmental Research Inc. (AER),<br>
  Radiation and Climate Group<br>
  131 Hartwell Avenue<br>
  Lexington, MA 02421, USA<br>
  http://www.rtweb.aer.com/continuum_frame.html

- N2-SelfContStandardType:
  Microwave nitrogen absorption continua

  Reference: P. W. Rosenkranz, Chapter 2, in M. A. Janssen, <br>
  <I>Atmospheric Remote Sensing by Microwave Radiometry</i>,<br>
  John Wiley & Sons, Inc., 1993.

- N2-SelfContMPM93:
  Microwave nitrogen absorption continua from MPM93 model

  Reference: H. J. Liebe and G. A. Hufford and M. G. Cotton,<br>
  <i>Propagation modeling of moist air and suspended water/ice
  particles at frequencies below 1000 GHz</i>,<br>
  AGARD 52nd Specialists Meeting of the Electromagnetic Wave
  Propagation Panel,<br> Palma de Mallorca, Spain, 1993, May 17-21

- N2-SelfContPWR2021:
  Microwave nitrogen absorption continua developed by P.W. Rosenkranz.

  Note that this also includes O2-N2 and O2-O2 collision-induced absorption and is
  only applicable to Earth

  Our code is reimplemented based on the Fortran code available at http://cetemps.aquila.infn.it/mwrnet/lblmrt_ns.html

- liquidcloud-ELL07:
  Water droplet absorption

  W. J. Ellison, <br>
  <i>Permittivity of Pure Water, at Standard Atmospheric Pressure, over the
  Frequency Range 0-25 THz and Temperature Range 0-100C</i>,<br>
  J. Phys. Chem. Ref. Data, Vol. 36, No. 1, 2007
)--",
      .author = {"Richard Larsson"},
      .out    = {"propagation_matrix", "propagation_matrix_jacobian"},
      .in     = {"propagation_matrix",
                 "propagation_matrix_jacobian",
                 "absorption_predefined_model_data",
                 "propagation_matrix_select_species",
                 "jacobian_targets",
                 "frequency_grid",
                 "atmospheric_point"},
  };

  wsm_data["propagation_matrixAddXsecFit"] = {
      .desc =
          R"--(Calculate absorption cross sections per tag group for HITRAN xsec species.

This broadens the cross section data from *absorption_xsec_fit_data* and
interpolates it onto the current frequency_grid.

Model data needs to be read in with *absorption_xsec_fit_dataReadSpeciesSplitCatalog* before calling
this method.
)--",
      .author    = {"Oliver Lemke"},
      .out       = {"propagation_matrix", "propagation_matrix_jacobian"},
      .in        = {"propagation_matrix",
                    "propagation_matrix_jacobian",
                    "propagation_matrix_select_species",
                    "jacobian_targets",
                    "frequency_grid",
                    "atmospheric_point",
                    "absorption_xsec_fit_data"},
      .gin       = {"force_p", "force_t"},
      .gin_type  = {"Numeric", "Numeric"},
      .gin_value = {Numeric{-1}, Numeric{-1}},
      .gin_desc  = {R"--(Positive value forces constant pressure [Pa].)--",
                    R"--(Positive value forces constant temperature [K].)--"},
  };

  wsm_data["propagation_matrix_scatteringInit"] = {
      .desc =
          R"--(Initialize *propagation_matrix_scattering* to zeroes.

This method must be used inside *propagation_matrix_scattering_agenda* and then be called first.
)--",
      .author = {"Richard Larsson"},
      .out    = {"propagation_matrix_scattering"},
      .in     = {"frequency_grid"},
  };

  wsm_data["propagation_matrix_scatteringAirSimple"] = {
      .desc =
          R"--(Add simple air to *propagation_matrix_scattering*.
)--",
      .author = {"Jon Petersen", "Richard Larsson"},
      .out    = {"propagation_matrix_scattering"},
      .in     = {"propagation_matrix_scattering",
                 "frequency_grid",
                 "atmospheric_point"},
  };

  wsm_data["propagation_matrixInit"] = {
      .desc =
          R"--(Initialize *propagation_matrix*, *propagation_matrix_source_vector_nonlte*, and their derivatives to zeroes.

This method must be used inside *propagation_matrix_agenda* and then be called first.
)--",
      .author = {"Oliver Lemke", "Richard Larsson"},
      .out    = {"propagation_matrix",
                 "propagation_matrix_source_vector_nonlte",
                 "propagation_matrix_jacobian",
                 "propagation_matrix_source_vector_nonlte_jacobian"},
      .in     = {"jacobian_targets", "frequency_grid"},
  };

  wsm_data["propagation_matrix_agendaAuto"] = {
      .desc      = R"--(Sets the *propagation_matrix_agenda* automatically

This method introspects the input and uses it for generating the
*propagation_matrix_agenda* automatically.  If ``use_absorption_lookup_table_data``, all
methods that can be used to generate the absorption lookup table
are ignored and instead the calculations from the absorption
lookup are used.

The following methods are considered for addition:

1) *propagation_matrixInit*
2) *propagation_matrixAddCIA*
3) *propagation_matrixAddLines*
4) *propagation_matrixAddFaraday*
5) *propagation_matrixAddXsecFit*
7) *propagation_matrixAddPredefined*

To perform absorption lookupo table calculation, call:

1) *propagation_matrix_agendaAuto*
2) ``absorption_lookup_table_dataCalc``  FIXME: HOW TO COMPUTE IT
3) *propagation_matrix_agendaAuto* (use_absorption_lookup_table_data=1)
4) Perform other calculations
)--",
      .author    = {"Richard Larsson"},
      .out       = {"propagation_matrix_agenda"},
      .in        = {"absorption_species", "absorption_bands"},
      .gin       = {"T_extrapolfac", "force_p", "force_t", "ignore_errors"},
      .gin_type  = {"Numeric", "Numeric", "Numeric", "Index"},
      .gin_value = {Numeric{0.5}, Numeric{-1}, Numeric{-1}, Index{0}},
      .gin_desc  = {R"--(See *propagation_matrixAddCIA*)--",
                    R"--(See *propagation_matrixAddXsecFit*)--",
                    R"--(See *propagation_matrixAddXsecFit*)--",
                    R"--(See *propagation_matrixAddCIA*)--"},
  };

  wsm_data["propagation_matrix_agendaSet"] = {
      .desc      = R"--(Sets *propagation_matrix_agenda* to a default value

Please consider using *propagation_matrix_agendaAuto* instead of one of these options
as it will ensure you have the best coverage of use cases.  The options below are
available for feature testing

See *propagation_matrix_agendaPredefined* for valid ``option``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"propagation_matrix_agenda"},
      .gin       = {"option"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Default agenda option (see description))--"},
  };

  wsm_data["propagation_matrix_scattering_agendaSet"] = {
      .desc =
          R"--(Sets *propagation_matrix_scattering_agenda* to a default value

See *propagation_matrix_scattering_agendaPredefined* for valid ``option``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"propagation_matrix_scattering_agenda"},
      .gin       = {"option"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {R"--(Default agenda option (see description))--"},
  };

  wsm_data["surface_fieldEarth"] = {
      .desc      = R"--(Earth reference ellipsoids.

The reference ellipsoid is set to model the Earth.

See *EarthEllipsoid* for valid ``model``
)--",
      .author    = {"Patrick Eriksson"},
      .out       = {"surface_field"},
      .gin       = {"model"},
      .gin_type  = {"String"},
      .gin_value = {String("Sphere")},
      .gin_desc  = {R"--(Model ellipsoid to use. Options listed above.)--"},
  };

  wsm_data["surface_fieldEuropa"] = {
      .desc      = R"--(Europa reference ellipsoids.

The reference ellipsoid is set to model the Europa.

See *EuropaEllipsoid* for valid ``model``.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"surface_field"},
      .gin       = {"model"},
      .gin_type  = {"String"},
      .gin_value = {String("Sphere")},
      .gin_desc  = {R"--(Model ellipsoid to use. Options listed above.)--"},
  };

  wsm_data["surface_fieldGanymede"] = {
      .desc      = R"--(Ganymede reference ellipsoids.

See *GanymedeEllipsoid* for valid ``model``.
)--",
      .author    = {"Takayoshi Yamada"},
      .out       = {"surface_field"},
      .gin       = {"model"},
      .gin_type  = {"String"},
      .gin_value = {String("Sphere")},
      .gin_desc  = {R"--(Model ellipsoid to use. Options listed above.)--"},
  };

  wsm_data["surface_fieldInit"] = {
      .desc      = R"--(Manual setting of the reference ellipsoid.

The two values of the reference ellipsoid are set manually. The two
arguments correspond directly to first and second element of
reference ellipsoid.
)--",
      .author    = {"Patrick Eriksson"},
      .out       = {"surface_field"},
      .gin       = {"a", "b"},
      .gin_type  = {"Numeric", "Numeric"},
      .gin_value = {std::nullopt, std::nullopt},
      .gin_desc  = {R"--(Average or equatorial radius.)--",
                    R"--(Average or polar radius.)--"},
  };

  wsm_data["surface_fieldIo"] = {
      .desc      = R"--(Io reference ellipsoids.

The reference ellipsoid is set to model the Io.

See *IoEllipsoid* for valid ``model``.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"surface_field"},
      .gin       = {"model"},
      .gin_type  = {"String"},
      .gin_value = {String("Sphere")},
      .gin_desc  = {R"--(Model ellipsoid to use. Options listed above.)--"},
  };

  wsm_data["surface_fieldJupiter"] = {
      .desc      = R"--(Jupiter reference ellipsoids.

The reference ellipsoid is set to model the Jupiter.

See *JupiterEllipsoid* for valid ``model``.
)--",
      .author    = {"Patrick Eriksson"},
      .out       = {"surface_field"},
      .gin       = {"model"},
      .gin_type  = {"String"},
      .gin_value = {String("Sphere")},
      .gin_desc  = {R"--(Model ellipsoid to use. Options listed above.)--"},
  };

  wsm_data["surface_fieldMars"] = {
      .desc      = R"--(Mars reference ellipsoids.

The reference ellipsoid is set to model the Mars.

See *MarsEllipsoid* for valid ``model``.
)--",
      .author    = {"Patrick Eriksson"},
      .out       = {"surface_field"},
      .gin       = {"model"},
      .gin_type  = {"String"},
      .gin_value = {String("Sphere")},
      .gin_desc  = {R"--(Model ellipsoid to use. Options listed above.)--"},
  };

  wsm_data["surface_fieldMoon"] = {
      .desc      = R"--(Moon reference ellipsoids.

The reference ellipsoid is set to model the Moon.

See *MoonEllipsoid* for valid ``model``.
)--",
      .author    = {"Patrick Eriksson"},
      .out       = {"surface_field"},
      .gin       = {"model"},
      .gin_type  = {"String"},
      .gin_value = {String("Sphere")},
      .gin_desc  = {R"--(Model ellipsoid to use. Options listed above.)--"},
  };

  wsm_data["surface_fieldVenus"] = {
      .desc      = R"--(Venus reference ellipsoids.

The reference ellipsoid is set to model the Venus.

See *VenusEllipsoid* for valid ``model``.
)--",
      .author    = {"Patrick Eriksson"},
      .out       = {"surface_field"},
      .gin       = {"model"},
      .gin_type  = {"String"},
      .gin_value = {String("Sphere")},
      .gin_desc  = {R"--(Model ellipsoid to use. Options listed above.)--"},
  };

  wsm_data["water_equivalent_pressure_operatorMK05"] = {
      .desc =
          R"--(Calculate equivalent water pressure according to Murphy and Koop, 2005

Default is setting the saturation pressure to the one with respect
to water at temperatures >= 0C, and to the one with respect to ice
for <0C. The GIN ``only_liquid`` allows you to apply the liquid value
at all temperatures.

The saturation pressure with respect to liquid and ice water is
calculated according to Eq. 10 and 7, respectively, of:
Murphy, D. M., & Koop, T. (2005). Review of the vapour pressures of
ice and supercooled water for atmospheric applications. Quarterly
Journal of the Royal Meteorological Society, 131(608), 1539-1565.
)--",
      .author    = {"Patrick Eriksson", "Richard Larsson"},
      .gout      = {"water_equivalent_pressure_operator"},
      .gout_type = {"NumericUnaryOperator"},
      .gout_desc = {"The water equivalent pressure operator."},
      .gin       = {"only_liquid"},
      .gin_type  = {"Index"},
      .gin_value = {Index{0}},
      .gin_desc =
          {"Set to 1 to use liquid saturation pressure at all temperatures"},
  };

  wsm_data["atmospheric_fieldHydrostaticPressure"] = {
      .desc      = R"-x-(Add the hydrostatic pressure to the atmospheric field
    
The field must already be able to compute temperature as a function of
altitude, latitude, and longitude.

If it also contains species data, the species are used to compute the
average mass of the atmospheric molecules to get the specific gas
constant.  Note that this can also be overwritte with a positive
value for the equivalent GIN.

The ``alts`` vector contains the altitude grid values that limits the
extrapolation distance in altitude.  The first altitude in this
list should corresond to the altitude of the ``p0`` grid.  The extrapolation
outside of this range simply uses the hydrostatic equation
$P_1 = P_0 - g * h * \rho$ by means of the specific gas constant omputed
as desribed above and the pressure of the lower or first altitude level.

See *HydrostaticPressureOption* for valid ``hydrostatic_option``.
)-x-",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field", "gravity_operator"},
      .gin       = {"p0",
                    "alts",
                    "fixed_specific_gas_constant",
                    "fixed_atmospheric_temperature",
                    "hydrostatic_option"},
      .gin_type  = {"GriddedField2", "Vector", "Numeric", "Numeric", "String"},
      .gin_value = {std::nullopt,
                    std::nullopt,
                    Numeric{-1},
                    Numeric{-1},
                    String{"HydrostaticEquation"}},
      .gin_desc =
          {"Lowest altitude pressure field",
           "Altitude vector",
           "Specific gas constant if larger than 0",
           "Constant atmospheric temprature if larger than 0",
           "Computational option for levels, [HydrostaticEquation, HypsometricEquation]"},
  };

  wsm_data["gravity_operatorCentralMass"] = {
      .desc =
          R"-x-(Sets a gravity operator from the gravitational constant and the mass of the planet

Gets the ellispoid from *surface_field*
)-x-",
      .author    = {"Richard Larsson"},
      .out       = {"gravity_operator"},
      .in        = {"surface_field"},
      .gin       = {"mass"},
      .gin_type  = {"Numeric"},
      .gin_value = {std::nullopt},
      .gin_desc =
          {"Gravitation constant so that the gravity at radius ``r`` is ``GM / r^2``"},
  };

  wsm_data["spectral_radiance_backgroundAgendasAtEndOfPath"] = {
      .desc           = R"--(Computes the background radiation.
)--",
      .author         = {"Richard Larsson"},
      .out            = {"spectral_radiance_background",
                         "spectral_radiance_background_jacobian"},
      .in             = {"frequency_grid",
                         "jacobian_targets",
                         "ray_path_point",
                         "surface_field",
                         "spectral_radiance_space_agenda",
                         "spectral_radiance_surface_agenda"},
      .pass_workspace = true,
  };

  wsm_data["spectral_radianceDefaultTransmission"] = {
      .desc =
          R"--(Sets default *spectral_radiance* and *spectral_radiance_jacobian* for transmission.

The Jacobian variable is all 0s, the background is [1 0 0 0] everywhere
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance", "spectral_radiance_jacobian"},
      .in     = {"frequency_grid", "jacobian_targets"},
  };

  wsm_data["spectral_radianceUniformCosmicBackground"] = {
      .desc =
          R"--(Background spectral radiance is from a uniform cosmic background temperature.
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance"},
      .in     = {"frequency_grid"},
  };

  wsm_data["spectral_radianceSurfaceBlackbody"] = {
      .desc =
          R"--(Set surface spectral radiance from Planck function of the surface temperature
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance", "spectral_radiance_jacobian"},
      .in     = {"frequency_grid",
                 "surface_field",
                 "jacobian_targets",
                 "ray_path_point"},
  };

  wsm_data["spectral_radiance_jacobianEmpty"] = {
      .desc   = R"--(Set the cosmic background radiation derivative to empty.

Size : (*jacobian_targets*, *frequency_grid*)
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance_jacobian"},
      .in     = {"frequency_grid", "jacobian_targets"},
  };

  wsm_data["spectral_radiance_jacobianEmpty"] = {
      .desc   = R"--(Set the cosmic background radiation derivative to empty.

Size : (*jacobian_targets*, *frequency_grid*)
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance_jacobian"},
      .in     = {"frequency_grid", "jacobian_targets"},
  };

  wsm_data["spectral_radiance_jacobianFromBackground"] = {
      .desc =
          R"--(Sets *spectral_radiance_jacobian* from the background values
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance_jacobian"},
      .in     = {"spectral_radiance_background_jacobian",
                 "transmission_matrix_background"},
  };

  wsm_data["spectral_radiance_jacobianAddPathPropagation"] = {
      .desc =
          R"--(Adds the propagation variables to *spectral_radiance_jacobian*
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance_jacobian"},
      .in     = {"spectral_radiance_jacobian",
                 "ray_path_spectral_radiance_jacobian",
                 "jacobian_targets",
                 "atmospheric_field",
                 "ray_path"},
  };

  wsm_data["spectral_radiance_jacobianApplyUnit"] = {
      .desc   = R"(Applies a unit to *spectral_radiance*, returning a new field

See *SpectralRadianceUnitType* and *spectral_radiance_unit* for valid use cases
and limitations.

Also be aware that *spectral_radiance_jacobianApplyUnit* must be called before *spectral_radianceApplyUnit*.
)",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance_jacobian"},
      .in     = {"spectral_radiance_jacobian",
                 "spectral_radiance",
                 "frequency_grid",
                 "ray_path_point",
                 "spectral_radiance_unit"},
  };

  wsm_data["spectral_radianceApplyUnit"] = {
      .desc   = R"(Applies a unit to *spectral_radiance*, returning a new field

See *SpectralRadianceUnitType* and *spectral_radiance_unit* for valid use cases
and limitations.

Also be aware that *spectral_radiance_jacobianApplyUnit* must be called before *spectral_radianceApplyUnit*.
)",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance"},
      .in     = {"spectral_radiance",
                 "frequency_grid",
                 "ray_path_point",
                 "spectral_radiance_unit"},
  };

  wsm_data["propagation_matrixAddLines"] = {
      .desc      = R"--(Modern line-by-line calculations
)--",
      .author    = {"Richard Larsson"},
      .out       = {"propagation_matrix",
                    "propagation_matrix_source_vector_nonlte",
                    "propagation_matrix_jacobian",
                    "propagation_matrix_source_vector_nonlte_jacobian"},
      .in        = {"propagation_matrix",
                    "propagation_matrix_source_vector_nonlte",
                    "propagation_matrix_jacobian",
                    "propagation_matrix_source_vector_nonlte_jacobian",
                    "frequency_grid",
                    "jacobian_targets",
                    "propagation_matrix_select_species",
                    "absorption_bands",
                    "ecs_data",
                    "atmospheric_point",
                    "ray_path_point"},
      .gin       = {"no_negative_absorption"},
      .gin_type  = {"Index"},
      .gin_value = {Index{1}},
      .gin_desc =
          {"Turn off to allow individual absorbers to have negative absorption"},
  };

  wsm_data["jacobian_targetsInit"] = {
      .desc   = R"--(Initialize or reset the *jacobian_targets*
)--",
      .author = {"Richard Larsson"},
      .out    = {"jacobian_targets"},
  };

  wsm_data["jacobian_targetsFinalize"] = {
      .desc   = R"--(Finalize *jacobian_targets* for use in RT methods
)--",
      .author = {"Richard Larsson"},
      .out    = {"jacobian_targets"},
      .in     = {"jacobian_targets",
                 "atmospheric_field",
                 "surface_field",
                 "absorption_bands"},
  };

  wsm_data["jacobian_targetsAddTemperature"] = {
      .desc      = R"--(Set temperature derivative
)--",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets"},
      .in        = {"jacobian_targets"},
      .gin       = {"d"},
      .gin_type  = {"Numeric"},
      .gin_value = {Numeric{0.1}},
      .gin_desc =
          {"The perturbation used in methods that cannot compute derivatives analytically"},
  };

  wsm_data["jacobian_targetsAddPressure"] = {
      .desc      = R"--(Set pressure derivative
)--",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets"},
      .in        = {"jacobian_targets"},
      .gin       = {"d"},
      .gin_type  = {"Numeric"},
      .gin_value = {Numeric{0.1}},
      .gin_desc =
          {"The perturbation used in methods that cannot compute derivatives analytically"},
  };

  wsm_data["jacobian_targetsAddMagneticField"] = {
      .desc      = R"--(Set magnetic field derivative

See *FieldComponent* for valid ``component``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets"},
      .in        = {"jacobian_targets"},
      .gin       = {"component", "d"},
      .gin_type  = {"String", "Numeric"},
      .gin_value = {std::nullopt, Numeric{0.1}},
      .gin_desc =
          {"The component to use [u, v, w]",
           "The perturbation used in methods that cannot compute derivatives analytically"},
  };

  wsm_data["jacobian_targetsAddWindField"] = {
      .desc      = R"--(Set wind field derivative

Note that the derivatives from methods that takes the freqeuncy will return
their derivatives as if these were frequency derivatives.

See *FieldComponent* for valid ``component``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets"},
      .in        = {"jacobian_targets"},
      .gin       = {"component", "d"},
      .gin_type  = {"String", "Numeric"},
      .gin_value = {std::nullopt, Numeric{0.1}},
      .gin_desc =
          {"The component to use [u, v, w]",
           "The perturbation used in methods that cannot compute derivatives analytically"},
  };

  wsm_data["jacobian_targetsAddSpeciesVMR"] = {
      .desc      = R"--(Set volume mixing ratio derivative

See *SpeciesEnum* for valid ``species``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets"},
      .in        = {"jacobian_targets"},
      .gin       = {"species", "d"},
      .gin_type  = {"SpeciesEnum", "Numeric"},
      .gin_value = {std::nullopt, Numeric{0.1}},
      .gin_desc =
          {"The species of interest",
           "The perturbation used in methods that cannot compute derivatives analytically"},
  };

  wsm_data["jacobian_targetsAddAtmosphere"] = {
      .desc   = R"--(Sets an atmospheric target
)--",
      .author = {"Richard Larsson"},
      .out    = {"jacobian_targets"},
      .in     = {"jacobian_targets"},
      .gin    = {"target", "d"},
      .gin_type =
          {"AtmKey,SpeciesEnum,SpeciesIsotope,QuantumIdentifier",
           "Numeric"},
      .gin_value = {std::nullopt, Numeric{0.1}},
      .gin_desc =
          {"The target of interest",
           "The perturbation used in methods that cannot compute derivatives analytically"},
  };

  wsm_data["jacobian_targetsAddSurface"] = {
      .desc      = R"--(Sets a surface target
)--",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets"},
      .in        = {"jacobian_targets"},
      .gin       = {"target", "d"},
      .gin_type  = {"SurfaceKey,SurfaceTypeTag,SurfacePropertyTag", "Numeric"},
      .gin_value = {std::nullopt, Numeric{0.1}},
      .gin_desc =
          {"The target of interest",
           "The perturbation used in methods that cannot compute derivatives analytically"},
  };

  wsm_data["jacobian_targetsAddSpeciesIsotopologueRatio"] = {
      .desc      = R"--(Set isotopologue ratio derivative

See *SpeciesIsotope* for valid ``species``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets"},
      .in        = {"jacobian_targets"},
      .gin       = {"species", "d"},
      .gin_type  = {"SpeciesIsotope", "Numeric"},
      .gin_value = {std::nullopt, Numeric{0.1}},
      .gin_desc =
          {"The species isotopologue of interest",
           "The perturbation used in methods that cannot compute derivatives analytically"},
  };

  wsm_data["absorption_bandsSelectFrequency"] = {
      .desc =
          R"--(Remove all lines/bands that strictly falls outside a frequency range

The line's of each band must be sorted by frequency
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_bands"},
      .in        = {"absorption_bands"},
      .gin       = {"fmin", "fmax", "by_line"},
      .gin_type  = {"Numeric", "Numeric", "Index"},
      .gin_value = {-std::numeric_limits<Numeric>::infinity(),
                    std::numeric_limits<Numeric>::infinity(),
                    Index{0}},
      .gin_desc =
          {"Minimum frequency to keep",
           "Maximum frequency to keep",
           "Selection is done line-by-line (if true) or band-by-band (if false)"},
  };

  wsm_data["absorption_bandsRemoveID"] = {
      .desc      = R"--(Remove first band of with a matching ID
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_bands"},
      .in        = {"absorption_bands"},
      .gin       = {"id"},
      .gin_type  = {"QuantumIdentifier"},
      .gin_value = {std::nullopt},
      .gin_desc  = {"Identifier to remove"},
  };

  wsm_data["absorption_bandsKeepID"] = {
      .desc      = R"--(Keeps first band of ID

If ``line`` is positive, also keep only the line of this index
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_bands"},
      .in        = {"absorption_bands"},
      .gin       = {"id", "line"},
      .gin_type  = {"QuantumIdentifier", "Index"},
      .gin_value = {std::nullopt, Index{-1}},
      .gin_desc  = {"Band to keep", "Line to keep (if positive)"},
  };

  wsm_data["sortedIndexOfBands"] = {
      .desc =
          R"--(Get the sorting of the bands by first quantum identifier then some ``criteria``

The reverse sorting can also be achieved by setting ``reverse``.

See *AbsorptionBandSortingOption* for valid ``criteria``.
)--",
      .author    = {"Richard Larsson"},
      .gout      = {"sorted"},
      .gout_type = {"ArrayOfIndex"},
      .gout_desc = {"Sorted band indices (of *absorption_bands*)"},
      .in        = {"absorption_bands"},
      .gin       = {"criteria", "reverse", "temperature"},
      .gin_type  = {"String", "Index", "Numeric"},
      .gin_value = {String{"None"}, Index{0}, Numeric{296.}},
      .gin_desc  = {"Internal sorting criteria",
                    "Sort in reverse order if true",
                    "Temperature to use for integrated intensity"},
  };

  wsm_data["absorption_bandsReadSpeciesSplitCatalog"] = {
      .desc      = R"--(Saves all bands fin *absorption_bands* to a directory

This will create the directory if it does not exist.  It will also create
subdirectories that are the short-form of the isotopologue names.  The bands
will be stored as 0.xml, 1.xml, 2.xml, and so on

The ``dir`` path has to be absolute or relative to the working path, the environment
variables are not considered
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_bands"},
      .in        = {"absorption_species"},
      .gin       = {"basename"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {"Absolute or relative path to the directory"},
  };

  wsm_data["absorption_bandsReadSplit"] = {
      .desc      = R"--(Saves all bands fin *absorption_bands* to a directory

This will create the directory if it does not exist.  It will also create
subdirectories that are the short-form of the isotopologue names.  The bands
will be stored as 0.xml, 1.xml, 2.xml, and so on

The ``dir`` path has to be absolute or relative to the working path, the environment
variables are not considered
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_bands"},
      .gin       = {"dir"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {"Absolute or relative path to the directory"},
  };

  wsm_data["absorption_bandsSaveSplit"] = {
      .desc      = R"--(Saves all bands fin *absorption_bands* to a directory

This will create the directory if it does not exist.  It will also create
subdirectories that are the short-form of the isotopologue names.  The bands
will be stored as 0.xml, 1.xml, 2.xml, and so on

The ``dir`` path has to be absolute or relative to the working path, the environment
variables are not considered
)--",
      .author    = {"Richard Larsson"},
      .in        = {"absorption_bands"},
      .gin       = {"dir"},
      .gin_type  = {"String"},
      .gin_value = {std::nullopt},
      .gin_desc  = {"Absolute or relative path to the directory"},
  };

  wsm_data["ray_pathGeometricUplooking"] = {
      .desc      = R"--(Wraps *ray_pathGeometric* for straight uplooking paths from the surface altitude at the position
)--",
      .author    = {"Richard Larsson"},
      .out       = {"ray_path"},
      .in        = {"atmospheric_field", "surface_field"},
      .gin       = {"latitude", "longitude", "max_step"},
      .gin_type  = {"Numeric", "Numeric", "Numeric"},
      .gin_value = {std::nullopt, std::nullopt, Numeric{1e3}},
      .gin_desc  = {"The Latitude", "The Longitude", "The maximum step length"},
  };

  wsm_data["ray_pathGeometricDownlooking"] = {
      .desc      = R"--(Wraps *ray_pathGeometric* for straight downlooking paths from the top-of-the-atmosphere altitude
)--",
      .author    = {"Richard Larsson"},
      .out       = {"ray_path"},
      .in        = {"atmospheric_field", "surface_field"},
      .gin       = {"latitude", "longitude", "max_step"},
      .gin_type  = {"Numeric", "Numeric", "Numeric"},
      .gin_value = {std::nullopt, std::nullopt, Numeric{1e3}},
      .gin_desc  = {"The Latitude", "The Longitude", "The maximum step length"},
  };

  wsm_data["ray_pathGeometric"] = {
      .desc      = R"--(Get a geometric radiation path

The path is defined by the origo and the line of sight.

The ``pos`` is either at the end or at the beginning of the path depending 
on the ``as_observer`` flag.  A value that evaluates to true means that it is
at the end of the path.  If ``as_observer`` is true, the ``los`` is therefore
looking backwards along the path.  Basically, ``as_observer`` true means that
``pos`` and ``los`` behaves as sensor pos and los.

The ``max_step`` is the maximum step length in meters.  The path is first
created between the two extremes of either space and/or surface.  Afterwards,
there are additional points added every ``max_step`` meters between these
points until no more fits (the last step is shorter or exactly ``max_step``).

Upon closing the method, the following options are available to modify
the output:

If ``add_limb`` is true, the limb point is added to the path at the end.  It
is computed using bisections to ensure that the zenith angle of the tangent
point is as close to 90 degrees as it can numerically be.

If ``remove_non_atm`` is true, all points that are not in the atmosphere are
removed.  It is recommended to remove these points as multiple methods will
either perform poorly or not at all with these points present.

If ``fix_updown_azimuth`` is true, the azimuthal angle of the path is
fixed to the initial azimuthal angle of the path.  Because calculations
of the azimuth angle makes use of IEEE atan2, some paths may produce
bad angles if this is turned off.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"ray_path"},
      .in        = {"atmospheric_field", "surface_field"},
      .gin       = {"pos",
                    "los",
                    "max_step",
                    "surface_search_accuracy",
                    "as_observer",
                    "add_limb",
                    "remove_non_atm",
                    "fix_updown_azimuth",
                    "surface_safe_search"},
      .gin_type  = {"Vector3",
                    "Vector2",
                    "Numeric",
                    "Numeric",
                    "Index",
                    "Index",
                    "Index",
                    "Index",
                    "Index"},
      .gin_value = {std::nullopt,
                    std::nullopt,
                    Numeric{1e3},
                    Numeric{0.1},
                    Index{1},
                    Index{0},
                    Index{1},
                    Index{1},
                    Index{1}},
      .gin_desc =
          {"The origo of the radiation path",
           "The line of sight of the radiation path",
           "The maximum step length",
           "The accuracy within which the surface intersection is counted as a hit",
           "Whether or not the path is as seen by the sensor or by the radiation (see text)",
           "Wheter or not to add the limb point",
           "Wheter or not to keep only atmospheric points",
           "Whether or not to attempt fix a potential issue with the path azimuthal angle",
           "Whether or not to search for the surface intersection in a safer but slower manner"},
  };

  wsm_data["ray_pathGeometricTangentAltitude"] = {
      .desc =
          R"--(Get a geometric radiation path that crosses the tangent altitude

The path is defined by an azimuth, a position, and a tangent altitude.
If the path ends up crossing the surface altitude, an error is thrown.

The ``pos`` is either at the end or at the beginning of the path depending 
on the ``as_observer`` flag.  A value that evaluates to true means that it
is at the end of the path.  If ``as_observer`` is true, the ``azimuth`` is
therefore looking backwards along the path.  Basically, ``as_observer`` true
means that ``pos`` and ``azimuth`` behaves as sensor pos and azimuth.

The ``max_step`` is the maximum step length in meters.  The path is first
created between the two extremes of space and space.  Afterwards,
there are additional points added every ``max_step`` meters between these
points until no more fits (the last step is shorter or exactly ``max_step``).

Upon closing the method, the following options are available to modify
the output:

If ``add_limb`` is true, the limb point is added to the path at the end.  It
is computed using bisections to ensure that the zenith angle of the tangent
point is as close to 90 degrees as it can numerically be.

If ``remove_non_atm`` is true, all points that are not in the atmosphere are
removed.  It is recommended to remove these points as multiple methods will
either perform poorly or not at all with these points present.

If ``fix_updown_azimuth`` is true, the azimuthal angle of the path is
fixed to the initial azimuthal angle of the path.  Because calculations
of the azimuth angle makes use of IEEE atan2, some paths may produce
bad angles if this is turned off.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"ray_path"},
      .in        = {"atmospheric_field", "surface_field"},
      .gin       = {"pos",
                    "tangent_altitude",
                    "azimuth",
                    "max_step",
                    "as_observer",
                    "add_limb",
                    "remove_non_atm",
                    "fix_updown_azimuth"},
      .gin_type  = {"Vector3",
                    "Numeric",
                    "Numeric",
                    "Numeric",
                    "Index",
                    "Index",
                    "Index",
                    "Index"},
      .gin_value = {std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    Numeric{1e3},
                    Index{1},
                    Index{1},
                    Index{1},
                    Index{1}},
      .gin_desc =
          {"The origo of the radiation path",
           "The tangent altitude of the radiation path",
           "The azimuth from the origo of the radiation path towards the tangent altitude",
           "The maximum step length",
           "Whether or not the path is as seen by the sensor or by the radiation (see text)",
           "Wheter or not to add the limb point",
           "Wheter or not to keep only atmospheric points",
           "Whether or not to attempt fix a potential issue with the path azimuthal angle"},
  };

  wsm_data["spectral_radiance_operatorClearsky1D"] = {
      .desc     = R"--(Set up a 1D spectral radiance operator

The operator is set up to compute the spectral radiance at any point as seen from
a 1D atmospheric profile.

This method will share line-by-line,cross-section, collision-induced absorption, and
predefined model data with the workspace (if they exist already when this method is
called).
)--",
      .author   = {"Richard Larsson"},
      .out      = {"spectral_radiance_operator"},
      .in       = {"atmospheric_field", "surface_field"},
      .gin      = {"altitude_grid",
                   "latitude",
                   "longitude",
                   "cia_extrapolation",
                   "cia_robust"},
      .gin_type = {"AscendingGrid", "Numeric", "Numeric", "Numeric", "Index"},
      .gin_value =
          {std::nullopt, Numeric{0.0}, Numeric{0.0}, Numeric{0.0}, Index{0}},
      .gin_desc       = {"The altitude grid",
                         "The latitude",
                         "The longitude",
                         "The extrapolation distance for cia",
                         "The robustness of the cia extrapolation"},
      .pass_workspace = true,
  };

  wsm_data["spectral_radiance_fieldFromOperatorPlanarGeometric"] = {
      .desc =
          R"--(Computes the spectral radiance field assuming planar geometric paths

A planar geometric path is just defined by a 1D atmospheric profile.  If the
*spectral_radiance_operator* contains more than one latitude and/or longitude
point, their altitude profiles are treated independently.

Limitations:

- The zenith grid is not allowed to contain the value 90 degrees.
)--",
      .author    = {"Richard Larsson"},
      .gout      = {"spectral_radiance_field"},
      .gout_type = {"StokvecGriddedField6"},
      .gout_desc = {"The spectral radiance field"},
      .in        = {"spectral_radiance_operator", "frequency_grid"},
      .gin       = {"zenith_grid", "azimuth_grid"},
      .gin_type  = {"AscendingGrid", "AscendingGrid"},
      .gin_value = {std::nullopt, std::nullopt},
      .gin_desc  = {"The zenith grid", "The azimuth grid"},
  };

  wsm_data["spectral_radiance_fieldFromOperatorPath"] = {
      .desc =
          R"--(Computes the spectral radiance field using *ray_path_observer_agenda*.

Each point is in computed individually, so there will be
zenith x azimuth x altitude x latitude x longitude x frequency number of calculations.
The positional arguments are taken from *spectral_radiance_operator*.

If the code is not already in parallel operation mode when this method is called,
the first 5 dimensions are computed in parallel.
)--",
      .author         = {"Richard Larsson"},
      .gout           = {"spectral_radiance_field"},
      .gout_type      = {"StokvecGriddedField6"},
      .gout_desc      = {"The spectral radiance field"},
      .in             = {"spectral_radiance_operator",
                         "ray_path_observer_agenda",
                         "frequency_grid"},
      .gin            = {"zenith_grid", "azimuth_grid"},
      .gin_type       = {"AscendingGrid", "AscendingGrid"},
      .gin_value      = {std::nullopt, std::nullopt},
      .gin_desc       = {"The zenith grid", "The azimuth grid"},
      .pass_workspace = true,
  };

  wsm_data["atmospheric_fieldAppendBaseData"] = {
      .desc      = R"--(Append base data to the atmospheric field

This will look at the valid ``basename`` for files matching base
data.  The base data file names are of the form

- "...t.xml"
- "...p.xml"
- "...wind_u.xml"
- "...wind_v.xml"
- "...wind_w.xml"
- "...mag_u.xml"
- "...mag_v.xml"
- "...mag_w.xml"

If any of these files are found, they are appended to the atmospheric field.

See *InterpolationExtrapolation* for valid ``extrapolation``.

See *MissingFieldComponentError* for valid ``deal_with_field_component``.

The ``replace_existing`` is used to determine if the data should be replaced if it already
exists in the atmospheric field.

The ``allow_missing_pressure`` and ``allow_missing_temperature`` are used to determine
if the method should throw if the pressure or temperature is missing.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field"},
      .gin       = {"basename",
                    "extrapolation",
                    "deal_with_field_component",
                    "replace_existing",
                    "allow_missing_pressure",
                    "allow_missing_temperature"},
      .gin_type  = {"String", "String", "String", "Index", "Index", "Index"},
      .gin_value = {std::nullopt,
                    String{"Linear"},
                    String{"Throw"},
                    Index{1},
                    Index{0},
                    Index{0}},
      .gin_desc  = {"The base name of the files",
                    "The extrapolation to use.",
                    "How to deal with the field component.",
                    "Whether or not to replace existing data",
                    "Whether or not to allow missing pressure data",
                    "Whether or not to allow missing temperature data"},
  };

  wsm_data["atmospheric_fieldAppendLineSpeciesData"] = {
      .desc =
          R"--(Append species data to the atmospheric field based on line data

This will look at the valid ``basename`` for files matching base
data.  The base data file names are of the short-name form: "species.xml" (e.g., "H2O.xml").
See :class:`~pyarts.arts.SpeciesEnum` for valid short names.

See *InterpolationExtrapolation* for valid ``extrapolation``.

The ``missing_is_zero`` sets missing data to zero.

The ``replace_existing`` is used to determine if the data should be replaced if it already
exists in the atmospheric field.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field", "absorption_bands"},
      .gin       = {"basename",
                    "extrapolation",
                    "missing_is_zero",
                    "replace_existing"},
      .gin_type  = {"String", "String", "Index", "Index"},
      .gin_value = {std::nullopt, String{"Linear"}, Index{0}, Index{0}},
      .gin_desc  = {"The base name of the files",
                    "The extrapolation to use.",
                    "Whether or not to zero-out missing data",
                    "Whether or not to replace existing data"},
  };

  wsm_data["atmospheric_fieldAppendLineIsotopologueData"] = {
      .desc =
          R"--(Append isotopologue data to the atmospheric field based on line data

This will look at the valid ``basename`` for files matching base
data.  The base data file names are of the form: "species-n.xml" (e.g., "H2O-161.xml").
See :class:`~pyarts.arts.SpeciesIsotopeRecord` for valid isotopologue names.

See *InterpolationExtrapolation* for valid ``extrapolation``.

The ``missing_is_zero`` sets missing data to zero.

The ``replace_existing`` is used to determine if the data should be replaced if it already
exists in the atmospheric field.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field", "absorption_bands"},
      .gin       = {"basename",
                    "extrapolation",
                    "missing_is_zero",
                    "replace_existing"},
      .gin_type  = {"String", "String", "Index", "Index"},
      .gin_value = {std::nullopt, String{"Linear"}, Index{0}, Index{0}},
      .gin_desc  = {"The base name of the files",
                    "The extrapolation to use.",
                    "Whether or not to zero-out missing data",
                    "Whether or not to replace existing data"},
  };

  wsm_data["atmospheric_fieldAppendLineLevelData"] = {
      .desc =
          R"--(Append NLTE data to the atmospheric field based on line data

This will look at the valid ``basename`` for files matching base
data.  The base data file names are of the form: "species-n QN1 N1 N1 QN2 N2 N2.xml" (e.g., "O2-66 J 1 1 N 0 0.xml").
See :class:`~pyarts.arts.SpeciesIsotopeRecord` for valid isotopologue names and
:class:`~pyarts.arts.QuantumNumberValue` for valid quantum numbers.

See *InterpolationExtrapolation* for valid ``extrapolation``.

The ``missing_is_zero`` sets missing data to zero.

The ``replace_existing`` is used to determine if the data should be replaced if it already
exists in the atmospheric field.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field", "absorption_bands"},
      .gin       = {"basename",
                    "extrapolation",
                    "missing_is_zero",
                    "replace_existing"},
      .gin_type  = {"String", "String", "Index", "Index"},
      .gin_value = {std::nullopt, String{"Linear"}, Index{0}, Index{0}},
      .gin_desc  = {"The base name of the files",
                    "The extrapolation to use.",
                    "Whether or not to zero-out missing data",
                    "Whether or not to replace existing data"},
  };

  wsm_data["atmospheric_fieldAppendTagsSpeciesData"] = {
      .desc =
          R"--(Append species data to the atmospheric field based on species data

This will look at the valid ``basename`` for files matching base
data.  The base data file names are of the short-name form: "species.xml" (e.g., "H2O.xml").
See :class:`~pyarts.arts.SpeciesEnum` for valid short names.

See *InterpolationExtrapolation* for valid ``extrapolation``.

The ``missing_is_zero`` sets missing data to zero.

The ``replace_existing`` is used to determine if the data should be replaced if it already
exists in the atmospheric field.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field", "absorption_species"},
      .gin       = {"basename",
                    "extrapolation",
                    "missing_is_zero",
                    "replace_existing"},
      .gin_type  = {"String", "String", "Index", "Index"},
      .gin_value = {std::nullopt, String{"Linear"}, Index{0}, Index{0}},
      .gin_desc  = {"The base name of the files",
                    "The extrapolation to use.",
                    "Whether or not to zero-out missing data",
                    "Whether or not to replace existing data"},
  };

  wsm_data["atmospheric_fieldAppendCIASpeciesData"] = {
      .desc =
          R"--(Append species data to the atmospheric field based on collision-induced data data

This will look at the valid ``basename`` for files matching base
data.  The base data file names are of the short-name form: "species.xml" (e.g., "H2O.xml").
See :class:`~pyarts.arts.SpeciesEnum` for valid short names.

See *InterpolationExtrapolation* for valid ``extrapolation``.

The ``missing_is_zero`` sets missing data to zero.

The ``replace_existing`` is used to determine if the data should be replaced if it already
exists in the atmospheric field.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field", "absorption_cia_data"},
      .gin       = {"basename",
                    "extrapolation",
                    "missing_is_zero",
                    "replace_existing"},
      .gin_type  = {"String", "String", "Index", "Index"},
      .gin_value = {std::nullopt, String{"Linear"}, Index{0}, Index{0}},
      .gin_desc  = {"The base name of the files",
                    "The extrapolation to use.",
                    "Whether or not to zero-out missing data",
                    "Whether or not to replace existing data"},
  };

  wsm_data["atmospheric_fieldAppendXsecSpeciesData"] = {
      .desc =
          R"--(Append species data to the atmospheric field based on cross-section data

This will look at the valid ``basename`` for files matching base
data.  The base data file names are of the short-name form: "species.xml" (e.g., "H2O.xml").
See :class:`~pyarts.arts.SpeciesEnum` for valid short names.

See *InterpolationExtrapolation* for valid ``extrapolation``.

The ``missing_is_zero`` sets missing data to zero.

The ``replace_existing`` is used to determine if the data should be replaced if it already
exists in the atmospheric field.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field", "absorption_xsec_fit_data"},
      .gin       = {"basename",
                    "extrapolation",
                    "missing_is_zero",
                    "replace_existing"},
      .gin_type  = {"String", "String", "Index", "Index"},
      .gin_value = {std::nullopt, String{"Linear"}, Index{0}, Index{0}},
      .gin_desc  = {"The base name of the files",
                    "The extrapolation to use.",
                    "Whether or not to zero-out missing data",
                    "Whether or not to replace existing data"},
  };

  wsm_data["atmospheric_fieldAppendPredefSpeciesData"] = {
      .desc =
          R"--(Append species data to the atmospheric field based on predefined model data

This will look at the valid ``basename`` for files matching base
data.  The base data file names are of the short-name form: "species.xml" (e.g., "H2O.xml").
See :class:`~pyarts.arts.SpeciesEnum` for valid short names.

See *InterpolationExtrapolation* for valid ``extrapolation``.

The ``missing_is_zero`` sets missing data to zero.

The ``replace_existing`` is used to determine if the data should be replaced if it already
exists in the atmospheric field.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field", "absorption_predefined_model_data"},
      .gin       = {"basename",
                    "extrapolation",
                    "missing_is_zero",
                    "replace_existing"},
      .gin_type  = {"String", "String", "Index", "Index"},
      .gin_value = {std::nullopt, String{"Linear"}, Index{0}, Index{0}},
      .gin_desc  = {"The base name of the files",
                    "The extrapolation to use.",
                    "Whether or not to zero-out missing data",
                    "Whether or not to replace existing data"},
  };

  wsm_data["atmospheric_fieldAppendAbsorptionData"] = {
      .desc =
          R"--(Append data to the atmospheric field based all absorption data

See *InterpolationExtrapolation* for valid ``extrapolation``.

Wraps:

- *atmospheric_fieldAppendLineSpeciesData* if the workspace contains *absorption_bands*
- *atmospheric_fieldAppendLineIsotopologueData* if ``load_isot`` is true and if the workspace contains *absorption_bands*
- *atmospheric_fieldAppendLineLevelData* if ``load_nlte`` is true and if the workspace contains *absorption_bands*
- *atmospheric_fieldAppendTagsSpeciesData* if the workspace contains *absorption_species*
- ``atmospheric_fieldAppendLookupTableSpeciesData`` if the workspace contains *absorption_species*
- *atmospheric_fieldAppendCIASpeciesData* if the workspace contains *absorption_cia_data*
- *atmospheric_fieldAppendXsecSpeciesData* if the workspace contains *absorption_xsec_fit_data*
- *atmospheric_fieldAppendPredefSpeciesData* if the workspace contains *absorption_predefined_model_data*
)--",
      .author    = {"Richard Larsson"},
      .out       = {"atmospheric_field"},
      .in        = {"atmospheric_field"},
      .gin       = {"basename",
                    "extrapolation",
                    "missing_is_zero",
                    "replace_existing",
                    "load_isot",
                    "load_nlte"},
      .gin_type  = {"String", "String", "Index", "Index", "Index", "Index"},
      .gin_value = {std::nullopt,
                    String{"Linear"},
                    Index{0},
                    Index{0},
                    Index{0},
                    Index{0}},
      .gin_desc  = {"The base name of the files",
                    "The extrapolation to use.",
                    "Whether or not to zero-out missing data",
                    "Whether or not to replace existing data",
                    "Whether or not to load isotopologue data",
                    "Whether or not to load NLTE data"},
      .pass_workspace = true,
  };

  wsm_data["ReadCatalogData"] = {
      .desc =
          R"--(Reads split catalog data from a folder structure similar to ``arts-cat-data``

Wraps:

- *absorption_bandsReadSpeciesSplitCatalog* with "lines/" added to ``basename``
- *absorption_cia_dataReadSpeciesSplitCatalog* with "cia/" added to ``basename`` and ``robust`` = 0
- *absorption_xsec_fit_dataReadSpeciesSplitCatalog* with "xsec/" added to ``basename``
- *absorption_predefined_model_dataReadSpeciesSplitCatalog* with "predef/" added to ``basename`` and ``name_missing`` = 1
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_predefined_model_data",
                    "absorption_xsec_fit_data",
                    "absorption_cia_data",
                    "absorption_bands"},
      .in        = {"absorption_species"},
      .gin       = {"basename"},
      .gin_type  = {"String"},
      .gin_value = {String{}},
      .gin_desc  = {"Absolute or relative path to the data"},
  };

  wsm_data["absorption_bandsSetZeeman"] = {
      .desc = R"--(Set the Zeeman splitting for lines within the frequency range

See *SpeciesIsotope* for valid ``species``
)--",
      .author    = {"Richard Larsson"},
      .out       = {"absorption_bands"},
      .in        = {"absorption_bands"},
      .gin       = {"species", "fmin", "fmax", "on"},
      .gin_type  = {"SpeciesIsotope", "Numeric", "Numeric", "Index"},
      .gin_value = {std::nullopt, std::nullopt, std::nullopt, Index{1}},
      .gin_desc  = {"Isotopologue of the species",
                    "Minimum line frequency to set Zeeman splitting for",
                    "Maximum line frequency to set Zeeman splitting for",
                    "On or off"},
  };

  wsm_data["ray_path_pointBackground"] = {
      .desc =
          R"--(Sets *ray_path_point* to the expected background point of *ray_path*
)--",
      .author = {"Richard Larsson"},
      .out    = {"ray_path_point"},
      .in     = {"ray_path"},
  };

  wsm_data["ray_path_pointForeground"] = {
      .desc =
          R"--(Sets *ray_path_point* to the expected foreground point of *ray_path*
)--",
      .author = {"Richard Larsson"},
      .out    = {"ray_path_point"},
      .in     = {"ray_path"},
  };

  wsm_data["ray_path_pointLowestFromPath"] = {
      .desc =
          R"(Sets *ray_path_point* to the lowest point of *ray_path*.
)",
      .author = {"Richard Larsson"},
      .out    = {"ray_path_point"},
      .in     = {"ray_path"},
  };

  wsm_data["measurement_vectorFromOperatorPath"] = {
      .desc =
          R"--(Sets measurement vector by looping over all sensor elements

The core calculations happens inside the *spectral_radiance_operator*.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"measurement_vector"},
      .in        = {"measurement_sensor",
                    "spectral_radiance_operator",
                    "ray_path_observer_agenda"},
      .pass_workspace = true,
  };

  wsm_data["measurement_vectorFromSensor"] = {
      .desc =
          R"--(Sets measurement vector by looping over all sensor elements

The core calculations happens inside the *spectral_radiance_observer_agenda*.

User choices of *spectral_radiance_unit* does not adversely affect this method.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"measurement_vector", "measurement_jacobian"},
      .in        = {"measurement_sensor",
                    "jacobian_targets",
                    "atmospheric_field",
                    "surface_field",
                    "spectral_radiance_unit",
                    "spectral_radiance_observer_agenda"},
      .pass_workspace = true,
  };

  wsm_data["measurement_sensorSimple"] = {
      .desc =
          R"--(Sets a sensor with a Gaussian channel opening around the frequency grid.

All elements share position, line-of-sight, and frequency grid.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"measurement_sensor"},
      .in        = {"frequency_grid"},
      .gin       = {"pos", "los", "pol"},
      .gin_type  = {"Vector3", "Vector2", "Stokvec"},
      .gin_value = {std::nullopt,
                    std::nullopt,
                    rtepack::to_stokvec(PolarizationChoice::I)},
      .gin_desc =
          {"A position [alt, lat, lon]",
           "A line of sight [zenith, azimuth]",
           "The polarization whos dot-product with the spectral radiance becomes the measurement"},
  };

  wsm_data["measurement_sensorSimpleGaussian"] = {
      .desc =
          R"--(Sets a sensor with a Gaussian channel opening around the frequency grid.

All elements share position, line-of-sight, and frequency grid.

Note that this means you only get "half" a Gaussian channel for the outermost channels.
)--",
      .author = {"Richard Larsson"},
      .out    = {"measurement_sensor"},
      .in     = {"frequency_grid"},
      .gin    = {"fwhm", "pos", "los", "pol"},
      .gin_type =
          {"Vector, Numeric", "Vector3", "Vector2", "Stokvec"},
      .gin_value = {std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    rtepack::to_stokvec(PolarizationChoice::I)},
      .gin_desc =
          {"The full-width half-maximum of the channel(s)",
           "A position [alt, lat, lon]",
           "A line of sight [zenith, azimuth]",
           "The polarization whos dot-product with the spectral radiance becomes the measurement"},
  };

  wsm_data["sun_pathFromObserverAgenda"] = {
      .desc =
          R"--(Find a path that hits the sun if possible

The algorithm finds the pair of angles with the least error in regards to angular zenith and 
azimuth offset from the sun.  It uses this pair of angles to compute said path.  The algorithm
is iterative.  It first finds the geometric pair of angles pointing at the sun.  It then
computes the path, using the space-facing path point's pointing offset relative to the sun
to change the angles in the four directions (up, left, right, down) until it finds a better
solution.  If no better solution is found, the algorithm it refines the angular search to half
for every level of refinement above 1, it then stops.

Note that special care is taken to eliminate surface intersections so that part of the sun may
still be hit if it is above the horizon.  If the sun is entirerly below the horizon, the path
will point close to the horizon.

The two control parameters are the ``angle_cut`` and ``just_hit``.  The ``angle_cut`` is the limit
in degrees to which the algorithm should search for a better solution.  The ``just_hit`` is a flag
that just returns the first time a path hits the sun.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"sun_path"},
      .in        = {"surface_field", "ray_path_observer_agenda", "sun"},
      .gin       = {"pos", "angle_cut", "refinement", "just_hit"},
      .gin_type  = {"Vector3", "Numeric", "Index", "Index"},
      .gin_value = {std::nullopt, Numeric{0.0}, Index{1}, Index{0}},
      .gin_desc =
          {"An observer position [alt, lat, lon]",
           "The angle delta-cutoff in the iterative solver [0.0, ...]",
           "The refinement of the search algorithm (twice the power of this is the resultion)",
           "Whether or not it is enough to just hit the sun or if better accuracy is needed"},
      .pass_workspace = true,
  };

  wsm_data["ray_path_suns_pathFromPathObserver"] = {
      .desc =
          R"--(Wraps *sun_pathFromObserverAgenda* for all paths to all suns.
)--",
      .author = {"Richard Larsson"},
      .out    = {"ray_path_suns_path"},
      .in  = {"surface_field", "ray_path_observer_agenda", "ray_path", "suns"},
      .gin = {"angle_cut", "refinement", "just_hit"},
      .gin_type  = {"Numeric", "Index", "Index"},
      .gin_value = {Numeric{0.0}, Index{1}, Index{0}},
      .gin_desc =
          {"The angle delta-cutoff in the iterative solver [0.0, ...]",
           "The refinement of the search algorithm (twice the power of this is the resultion)",
           "Whether or not it is enough to just hit the sun or if better accuracy is needed"},
      .pass_workspace = true,
  };

  wsm_data["spectral_radianceSunsOrCosmicBackground"] = {
      .desc =
          R"--(Get the spectral radiance of a sun or of the cosmic background if no sun is hit.

Note that only the first sun is used if multiple suns are defined, so it is advantageous to
have sorted *suns* by distance before running this code.
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance"},
      .in     = {"frequency_grid", "ray_path_point", "suns", "surface_field"},
  };

  wsm_data["spectral_radianceSunOrCosmicBackground"] = {
      .desc =
          R"--(Get the spectral radiance of a sun or of the cosmic background if the sun is not hit.
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance"},
      .in     = {"frequency_grid", "sun_path", "sun", "surface_field"},
  };

  wsm_data["sunBlackbody"] = {
      .desc =
          R"--(Set *sun* to blackbody.

.. note::
    For a Sol-like sun there are huge differences in the UV-range
    between the actual sun spectrum and the blackbody spectrum
    with the effective temperature of the sun. The blackbody sun
    strongly overestimates the UV radiation.
)--",
      .author = {"Jon Petersen", "Richard Larsson"},
      .out    = {"sun"},
      .in     = {"frequency_grid"},
      .gin    = {"radius", "distance", "temperature", "latitude", "longitude"},
      .gin_type  = {"Numeric", "Numeric", "Numeric", "Numeric", "Numeric"},
      .gin_value = {6.963242e8, 1.495978707e11, 5772.0, 0.0, 0.0},
      .gin_desc =
          {"The radius of the sun in meter. "
           "Default is the radius of our sun. ",
           "The average distance between the sun and the planet in meter. "
           "Default value is set to 1 a.u. ",
           "The effective temperature of the suns photosphere in Kelvin. "
           "Default is the temperature of our sun - 5772 Kelvin ",
           "The latitude or the zenith position of the sun in the sky. ",
           "The longitude or azimuthal position of the sun in the sky. "},
  };

  wsm_data["sunFromGrid"] = {
      .desc =
          R"(Extracts a sun spectrum from a field of such data.
          
The method allows to obtain the sun spectrum by
interpolation from a field of such data. 
The sun spectrum is expected to be stored as
the irradiance at the suns photosphere.

Unit:

- GriddedField2: [W m-2 Hz-1]

- Vector *frequency_grid* [Hz]
- Vector ``stokes_dim`` [1]

Dimensions: [*frequency_grid*, stokes_dim]

This method performs an interpolation onto the *frequency_grid*.
The point of *frequency_grid* that are outside the data frequency grid
are initialized according to planck's law of the temperature variable.
Hence, a temperature of 0 means 0s the edges of the *frequency_grid*.
)",
      .author    = {"Jon Petersen", "Richard Larsson"},
      .out       = {"sun"},
      .in        = {"frequency_grid"},
      .gin       = {"sun_spectrum_raw",
                    "radius",
                    "distance",
                    "temperature",
                    "latitude",
                    "longitude",
                    "description"},
      .gin_type  = {"GriddedField2",
                    "Numeric",
                    "Numeric",
                    "Numeric",
                    "Numeric",
                    "Numeric",
                    "String"},
      .gin_value = {std::nullopt,
                    6.963242e8,
                    1.495978707e11,
                    5772.0,
                    0.0,
                    0.0,
                    String{"A sun"}},
      .gin_desc =
          {"A raw spectrum",
           "The radius of the sun in meter. Default is the radius of our sun. ",
           "The average distance between the sun and the planet in meter. Default value is set to 1 a.u. ",
           "The effective temperature of the suns photosphere in Kelvin. Default is the temperature of our sun - 5772 Kelvin ",
           "The latitude or the zenith position of the sun in the sky. ",
           "The longitude or azimuthal position of the sun in the sky. ",
           "A description of the sun."},
  };

  wsm_data["sunsAddSun"] = {
      .desc   = "Add *sun* to *suns*, only exist for composability.\n",
      .author = {"Richard Larsson"},
      .out    = {"suns"},
      .in     = {"suns", "sun"},
  };

  wsm_data["ray_path_spectral_radiance_scatteringSunsFirstOrderRayleigh"] = {
      .desc     = R"--(Add *suns* to *ray_path_spectral_radiance_source*.
)--",
      .author   = {"Richard Larsson"},
      .out      = {"ray_path_spectral_radiance_scattering"},
      .in       = {"ray_path_propagation_matrix_scattering",
                   "ray_path",
                   "ray_path_suns_path",
                   "suns",
                   "jacobian_targets",
                   "frequency_grid",
                   "atmospheric_field",
                   "surface_field",
                   "propagation_matrix_agenda"},
      .gin      = {"rte_alonglos_v", "depolarization_factor", "hse_derivative"},
      .gin_type = {"Numeric", "Numeric", "Index"},
      .gin_value = {Numeric{0.0}, Numeric{0.0}, Index{0}},
      .gin_desc =
          {R"--(Velocity along the line-of-sight to consider for a RT calculation.)--",
           R"--(The depolarization factor to use.)--",
           "Flag to compute the hypsometric distance derivatives"},
      .pass_workspace = true,
  };

  wsm_data["atmospheric_fieldFromModelState"] = {
      .desc   = R"--(Sets *atmospheric_field* to the state of the model.
)--",
      .author = {"Richard Larsson"},
      .out    = {"atmospheric_field"},
      .in     = {"atmospheric_field", "model_state_vector", "jacobian_targets"},
  };

  wsm_data["surface_fieldFromModelState"] = {
      .desc   = R"--(Sets *surface_field* to the state of the model.
)--",
      .author = {"Richard Larsson"},
      .out    = {"surface_field"},
      .in     = {"surface_field", "model_state_vector", "jacobian_targets"},
  };

  wsm_data["absorption_bandsFromModelState"] = {
      .desc   = R"--(Sets *absorption_bands* to the state of the model.
)--",
      .author = {"Richard Larsson"},
      .out    = {"absorption_bands"},
      .in     = {"absorption_bands", "model_state_vector", "jacobian_targets"},
  };

  wsm_data["model_state_vectorSize"] = {
      .desc =
          R"--(Sets *model_state_vector* to the size *jacobian_targets* demand.

Warning:

    Does not zero out existing data. Use *model_state_vectorZero* if that is desired.
)--",
      .author = {"Richard Larsson"},
      .out    = {"model_state_vector"},
      .in     = {"jacobian_targets"},
  };

  wsm_data["model_state_vectorZero"] = {
      .desc   = R"--(Sets *model_state_vector* to 0.0
)--",
      .author = {"Richard Larsson"},
      .out    = {"model_state_vector"},
      .in     = {"model_state_vector"},
  };

  wsm_data["model_state_vectorFromAtmosphere"] = {
      .desc   = R"--(Sets *model_state_vector*'s atmospheric part.
)--",
      .author = {"Richard Larsson"},
      .out    = {"model_state_vector"},
      .in     = {"model_state_vector", "atmospheric_field", "jacobian_targets"},
  };

  wsm_data["model_state_vectorFromSurface"] = {
      .desc   = R"--(Sets *model_state_vector*'s surface part.
)--",
      .author = {"Richard Larsson"},
      .out    = {"model_state_vector"},
      .in     = {"model_state_vector", "surface_field", "jacobian_targets"},
  };

  wsm_data["model_state_vectorFromBands"] = {
      .desc   = R"--(Sets *model_state_vector*'s absorption line part.
)--",
      .author = {"Richard Larsson"},
      .out    = {"model_state_vector"},
      .in     = {"model_state_vector", "absorption_bands", "jacobian_targets"},
  };

  wsm_data["ray_path_transmission_matrixFromPath"] = {
      .desc      = R"--(Gets the transmission matrix in layers along the path.

The assumption is that each path variable forms a layer from the 
ray path.  So there is a reduction in size by one.  A demand therefore
is that there are at least 2 points in the path.

The derivatives first dimensions are also 2, the first for the derivative wrt
the level before and one for the level after.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"ray_path_transmission_matrix",
                    "ray_path_transmission_matrix_jacobian"},
      .in        = {"ray_path_propagation_matrix",
                    "ray_path_propagation_matrix_jacobian",
                    "ray_path",
                    "ray_path_atmospheric_point",
                    "surface_field",
                    "jacobian_targets"},
      .gin       = {"hse_derivative"},
      .gin_type  = {"Index"},
      .gin_value = {Index{0}},
      .gin_desc  = {"Flag to compute the hypsometric distance derivatives"},
  };

  wsm_data["spectral_radianceStepByStepEmission"] = {
      .desc   = R"--(Gets the spectral radiance from the path.

This uses a step-by-step solver to propagate background radiation along the path.
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance", "ray_path_spectral_radiance_jacobian"},
      .in     = {"ray_path_transmission_matrix",
                 "ray_path_transmission_matrix_cumulative",
                 "ray_path_transmission_matrix_jacobian",
                 "ray_path_spectral_radiance_source",
                 "ray_path_spectral_radiance_source_jacobian",
                 "spectral_radiance_background"},
  };

  wsm_data["spectral_radianceCumulativeEmission"] = {
      .desc   = R"--(Gets the spectral radiance from the path emission.

Also get the Jacobian of the spectral radiance with regards to the
path parameters.
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance", "ray_path_spectral_radiance_jacobian"},
      .in     = {"ray_path_transmission_matrix",
                 "ray_path_transmission_matrix_cumulative",
                 "ray_path_transmission_matrix_jacobian",
                 "ray_path_spectral_radiance_source",
                 "ray_path_spectral_radiance_source_jacobian",
                 "spectral_radiance_background"},
  };

  wsm_data["spectral_radianceCumulativeTransmission"] = {
      .desc   = R"--(Gets the spectral radiance from the path transmission.

Also get the Jacobian of the spectral radiance with regards to the
path parameters.
)--",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance", "ray_path_spectral_radiance_jacobian"},
      .in     = {"ray_path_transmission_matrix",
                 "ray_path_transmission_matrix_cumulative",
                 "ray_path_transmission_matrix_jacobian",
                 "spectral_radiance_background"},
  };

  wsm_data["OEM"] = {
      .desc   = R"(Inversion by the so called optimal estimation method (OEM).

Work in progress ...

The cost function to minimise, including a normalisation with length
of *measurement_vector*, is:

    cost = cost_y + cost_x

where:

    cost_y = 1/m * [y-yf]' * covmat_se_inv * [y-yf]
    cost_x = 1/m * [x-xa]' * covmat_sx_inv * [x-xa]

The current implementation provides 3 methods for the minimization of
the cost functional: Linear, Gauss-Newton and Levenberg-Marquardt.
The Gauss-Newton minimizer attempts to find a minimum solution by 
fitting a quadratic function to the cost functional. The linear minimizer
is a special case of the Gauss-Newton method, since for a linear forward
model the exact solution of the minimization problem is obtained after
the first step. The Levenberg-Marquardt method adaptively constrains the
search region for the next iteration step by means of the so-called gamma-factor.
This makes the method more suitable for strongly non-linear problems.
If the gamma-factor is 0, Levenberg-Marquardt and Gauss-Newton method
are identical. Each minimization method (li,gn,lm) has an indirect
variant (li_cg,gn_cg,lm_cg), which uses the conjugate gradient solver
for the linear system that has to be solved in each minimzation step.
This of advantage for very large problems, that would otherwise require
the computation of expensive matrix products.

Description of the special input arguments:

    - ``method``:

        - ``"li"``: A linear problem is assumed and a single iteration is performed.
        - ``"li_cg"``: A linear problem is assumed and solved using the CG solver.
        - ``"gn"``: Non-linear, with Gauss-Newton iteration scheme.
        - ``"gn_cg"``: Non-linear, with Gauss-Newton and conjugate gradient solver.
        - ``"lm"``: Non-linear, with Levenberg-Marquardt (LM) iteration scheme.
        - ``"lm_cg"``: Non-linear, with Levenberg-Marquardt (LM) iteration scheme and conjugate gradient solver.

    - ``max_start_cost``:

      No inversion is done if the cost matching the a priori state is above
      this value. If set to a negative value, all values are accepted.
      This argument also controls if the start cost is calculated. If
      set to <= 0, the start cost in ``oem_diagnostics`` is set to NaN
      when using "li" and "gn".
    
    - ``x_norm``:

      A normalisation vector for *model_state_vector*. A normalisation of *model_state_vector* can be needed
      due to limited numerical precision. If this vector is set to be empty
      no normalisation is done (defualt case). Otherwise, this must be a
      vector with same length as *model_state_vector*, just having values above zero.
      Elementwise division between *model_state_vector* and ``x_norm`` (x./x_norm) shall give
      a vector where all values are in the order of unity. Maybe the best
      way to set ``x_norm`` is x_norm = sqrt( diag( Sx ) ).

    - ``max_iter``:

      Maximum number of iterations to perform. No effect for "li".

    - ``stop_dx``:\n"

      Iteration stop criterion. The criterion used is the same as given in Rodgers\' "Inverse Methods for Atmospheric Sounding"

    - ``lm_ga_settings``:

      Settings controlling the gamma factor, part of the "LM" method.
      This is a vector of length 6, having the elements (0-based index):

            0. Start value.
            1. Fractional decrease after succesfull iteration.
            2. Fractional increase after unsuccessful iteration.
            3. Maximum allowed value. If the value is passed, the inversion is halted.
            4. Lower treshold. If the threshold is passed, gamma is set to zero. If gamma must be increased from zero, gamma is set to this value.
            5. Gamma limit. This is an additional stop criterion. Convergence is not considered until there has been one succesful iteration having a gamma <= this value.
      
      The default setting triggers an error if "lm" is selected.

    - ``clear matrices``:

      With this flag set to 1, *measurement_jacobian* and *measurement_gain_matrix* are returned as empty matrices.

    - ``display_progress``:

      Controls if there is any screen output. The overall report level is ignored by this WSM.
)",
      .author = {"Patrick Eriksson"},
      .out    = {"model_state_vector",
                 "measurement_vector_fitted",
                 "measurement_jacobian",
                 "measurement_gain_matrix"},
      .gout   = {"oem_diagnostics", "lm_ga_history", "errors"},
      .gout_type = {"Vector", "Vector", "ArrayOfString"},
      .gout_desc =
          {"Basic diagnostics of an OEM type inversion",
           "The series of gamma values for a Marquardt-levenberg inversion",
           "Errors encountered during OEM execution"},
      .in        = {"model_state_vector",
                    "measurement_vector_fitted",
                    "measurement_jacobian",
                    "model_state_vector_apriori",
                    "model_state_covariance_matrix",
                    "measurement_vector",
                    "measurement_vector_error_covariance_matrix",
                    "inversion_iterate_agenda"},
      .gin       = {"method",
                    "max_start_cost",
                    "model_state_covariance_matrix_normalization",
                    "max_iter",
                    "stop_dx",
                    "lm_ga_settings",
                    "clear_matrices",
                    "display_progress"},
      .gin_type  = {"String",
                    "Numeric",
                    "Vector",
                    "Index",
                    "Numeric",
                    "Vector",
                    "Index",
                    "Index"},
      .gin_value = {std::nullopt,
                    Numeric{std::numeric_limits<Numeric>::infinity()},
                    Vector{},
                    Index{10},
                    Numeric{0.01},
                    Vector{},
                    Index{0},
                    Index{0}},
      .gin_desc =
          {"Iteration method. For this and all options below, see further above",
           "Maximum allowed value of cost function at start",
           "Normalisation of Sx",
           "Maximum number of iterations",
           "Stop criterion for iterative inversions",
           "Settings associated with the ga factor of the LM method",
           "An option to save memory",
           "Flag to control if inversion diagnostics shall be printed on the screen"},
      .pass_workspace = true,
  };

  wsm_data["measurement_vector_error_covariance_matrix_observation_systemCalc"] = {
      .desc =
          "Calculates the covariance matrix describing the error due to uncertainties\n"
          "in the observation system.\n\nThe uncertainties of the observation system are\n"
          "described by *measurement_vector_error_covariance_matrix*, which must be set by the user to include the\n"
          "relevant contributions from the measurement and the forward model.\n"
          "\n"
          "Prerequisite for the calculation of ``measurement_vector_error_covariance_matrix_observation_system`` is a successful OEM\n"
          "computation where also the gain matrix has been computed.\n",
      .author = {"Simon Pfreundschuh"},
      .gout = {"measurement_vector_error_covariance_matrix_observation_system"},
      .gout_type = {"Matrix"},
      .gout_desc =
          {"Covariance matrix describing the retrieval error due to uncertainties of the observation system."},
      .in = {"measurement_gain_matrix",
             "measurement_vector_error_covariance_matrix"},
  };

  wsm_data["model_state_covariance_matrix_smoothing_errorCalc"] = {
      .desc =
          "Calculates the covariance matrix describing the error due to smoothing.\n"
          "\n"
          "The calculation of ``model_state_covariance_matrix_smoothing_error`` also requires the averaging kernel matrix *measurement_averaging_kernel*\n"
          "to be computed after a successful OEM calculation.\n",
      .author    = {"Simon Pfreundschuh"},
      .gout      = {"model_state_covariance_matrix_smoothing_error"},
      .gout_type = {"Matrix"},
      .gout_desc =
          {"Covariance matrix describing the retrieval error due to smoothing."},
      .in = {"measurement_averaging_kernel", "model_state_covariance_matrix"},
  };

  wsm_data["measurement_averaging_kernelCalc"] = {
      .desc =
          "Calculate the averaging kernel matrix.\n\nThis is done by describing the sensitivity of the\n"
          "OEM retrieval with respect to the true state of the system. A prerequisite\n"
          "for the calculation of the averaging kernel matrix is a successful OEM\n"
          "calculation in which the *measurement_jacobian* and the gain matrix *measurement_gain_matrix* have been calculated.\n",
      .author = {"Simon Pfreundschuh"},
      .out    = {"measurement_averaging_kernel"},
      .in     = {"measurement_gain_matrix", "measurement_jacobian"},
  };

  wsm_data["model_state_vector_aprioriFromState"] = {
      .desc =
          R"(Sets the a priori state of the model state vector to the current state.
)",
      .author = {"Richard Larsson"},
      .out    = {"model_state_vector_apriori"},
      .in     = {"model_state_vector"},
  };

  wsm_data["measurement_vector_fittedFromMeasurement"] = {
      .desc =
          R"(Sets the fitted measurement vector to the current measurement vector.
)",
      .author = {"Richard Larsson"},
      .out    = {"measurement_vector_fitted"},
      .in     = {"measurement_vector"},
  };

  wsm_data["model_state_covariance_matrixInit"] = {
      .desc =
          R"(Initialises the model state covariance matrix to the identity matrix.
)",
      .author = {"Richard Larsson"},
      .out    = {"model_state_covariance_matrix"},
  };

  wsm_data["model_state_covariance_matrixAddSpeciesVMR"] = {
      .desc =
          R"(Set a species model state covariance matrix element.
)",
      .author    = {"Richard Larsson"},
      .out       = {"model_state_covariance_matrix"},
      .in        = {"model_state_covariance_matrix", "jacobian_targets"},
      .gin       = {"species", "matrix", "inverse"},
      .gin_type  = {"SpeciesEnum", "BlockMatrix", "BlockMatrix"},
      .gin_value = {std::nullopt, std::nullopt, BlockMatrix{}},
      .gin_desc  = {"The species to set the covariance matrix for",
                    "The covariance diagoinal block matrix",
                    "The inverse covariance diagoinal block matrix"},
  };

  wsm_data["measurement_vector_error_covariance_matrixConstant"] = {
      .desc =
          R"(Sets a constant measurement vector error covariance matrix.
)",
      .author    = {"Richard Larsson"},
      .out       = {"measurement_vector_error_covariance_matrix"},
      .in        = {"measurement_sensor"},
      .gin       = {"value"},
      .gin_type  = {"Numeric"},
      .gin_value = {std::nullopt},
      .gin_desc  = {"The value of the covariance matrix diagonal"},
  };

  wsm_data["disort_settingsNoSun"] = {
      .desc   = R"(Turns off solar radiation in Disort calculations.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in     = {"disort_settings"},
  };

  wsm_data["disort_settingsSetSun"] = {
      .desc =
          R"--(Uses Set the FOV to the sun input for Disort calculations.
)--",
      .author    = {"Richard Larsson"},
      .out       = {"disort_settings"},
      .in        = {"disort_settings",
                    "frequency_grid",
                    "surface_field",
                    "sun",
                    "ray_path_point"},
  };

  wsm_data["disort_settingsNoLayerThermalEmission"] = {
      .desc   = R"(Turns off source radiation in Disort calculations.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in     = {"disort_settings"},
  };

  wsm_data["disort_settingsLayerThermalEmissionLinearInTau"] = {
      .desc =
          R"(Use a source function that changes linearly in optical thickness.

Note that you must have set the optical thickness before calling this.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in = {"disort_settings", "ray_path_atmospheric_point", "frequency_grid"},
  };

  wsm_data["disort_settingsNoSpaceEmission"] = {
      .desc =
          R"(Turns off boundary condition from space for Disort calculations.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in     = {"disort_settings"},
  };

  wsm_data["disort_settingsCosmicMicrowaveBackgroundRadiation"] = {
      .desc =
          R"(Space radiation into Disort is isotropic cosmic background radiation.
)",
      .author    = {"Richard Larsson"},
      .out       = {"disort_settings"},
      .in        = {"disort_settings", "frequency_grid"},
  };

  wsm_data["disort_settingsNoSurfaceEmission"] = {
      .desc = R"(Turns boundary condition from surface for Disort calculations.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in     = {"disort_settings"},
  };

  wsm_data["disort_settingsSurfaceEmissionByTemperature"] = {
      .desc =
          R"(Surface radiation into Disort is isotropic from surface temperature.
)",
      .author    = {"Richard Larsson"},
      .out       = {"disort_settings"},
      .in        = {"disort_settings",
                    "frequency_grid",
                    "ray_path_point",
                    "surface_field"},
  };

  wsm_data["disort_settingsNoLegendre"] = {
      .desc   = R"(Turns off Legendre coefficients in Disort calculations.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in     = {"disort_settings"},
  };

  wsm_data["disort_settingsNoFractionalScattering"] = {
      .desc   = R"(Turns off fractional scattering in Disort calculations.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in     = {"disort_settings"},
  };

  wsm_data["disort_settingsNoSingleScatteringAlbedo"] = {
      .desc   = R"(Turns off single albedo scattering in Disort calculations.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in     = {"disort_settings"},
  };

  wsm_data["disort_settingsOpticalThicknessFromPath"] = {
      .desc   = R"(Get optical thickness from path.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in     = {"disort_settings", "ray_path", "ray_path_propagation_matrix"},
  };

  wsm_data["disort_settingsNoSurfaceScattering"] = {
      .desc   = R"(Turns off BDRF in Disort calculations.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in     = {"disort_settings"},
  };

  wsm_data["disort_settingsSurfaceLambertian"] = {
      .desc      = R"(Turns off BDRF in Disort calculations.
)",
      .author    = {"Richard Larsson"},
      .out       = {"disort_settings"},
      .in        = {"disort_settings"},
      .gin       = {"value"},
      .gin_type  = {"Numeric"},
      .gin_value = {std::nullopt},
      .gin_desc  = {"The value of the BDRF in all directions"},
  };

  wsm_data["disort_settingsInit"] = {
      .desc   = R"(Perform Disort calculations for spectral flux.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_settings"},
      .in     = {"frequency_grid",
                 "ray_path",
                 "disort_quadrature_dimension",
                 "disort_fourier_mode_dimension",
                 "disort_legendre_polynomial_dimension"},
  };

  wsm_data["disort_spectral_radiance_fieldCalc"] = {
      .desc      = R"(Perform Disort calculations for spectral radiance.
)",
      .author    = {"Richard Larsson"},
      .out       = {"disort_spectral_radiance_field",
                    "disort_quadrature_angles",
                    "disort_quadrature_weights"},
      .in        = {"disort_settings"},
      .gin       = {"phis"},
      .gin_type  = {"Vector"},
      .gin_value = {Vector{0.0}},
      .gin_desc  = {"The azimuthal angles"},
  };

  wsm_data["disort_spectral_flux_fieldCalc"] = {
      .desc   = R"(Perform Disort calculations for spectral flux.
)",
      .author = {"Richard Larsson"},
      .out    = {"disort_spectral_flux_field"},
      .in     = {"disort_settings"},
  };

  wsm_data["SpectralFluxDisort"] = {
      .desc      = R"(Integrate Disort spectral radiance.
)",
      .author    = {"Richard Larsson"},
      .gout      = {"spectral_flux_field_up", "spectral_flux_field_down"},
      .gout_type = {"Matrix", "Matrix"},
      .gout_desc = {"Upward spectral flux field",
                    "Downward spectral flux field"},
      .in        = {"disort_spectral_flux_field"},
  };

  wsm_data["spectral_radianceIntegrateDisort"] = {
      .desc   = R"(Integrate Disort spectral radiance.
)",
      .author = {"Richard Larsson"},
      .out    = {"spectral_radiance"},
      .in     = {"disort_spectral_radiance_field",
                 "disort_quadrature_angles",
                 "disort_quadrature_weights"},
  };

  wsm_data["RetrievalInit"] = {
      .desc   = R"(Initialize the retrieval setup.
)",
      .author = {"Richard Larsson"},
      .out    = {"jacobian_targets",
                 "model_state_covariance_matrix",
                 "covariance_matrix_diagonal_blocks"},
  };

  wsm_data["RetrievalAddSurface"] = {
      .desc      = R"(Add surface property the retrieval setup.
)",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .in        = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .gin       = {"species", "matrix", "inverse", "d"},
      .gin_type  = {"SurfaceKey,SurfaceTypeTag,SurfacePropertyTag",
                    "BlockMatrix",
                    "BlockMatrix",
                    "Numeric"},
      .gin_value = {std::nullopt, std::nullopt, BlockMatrix{}, Numeric{0.1}},
      .gin_desc =
          {"The property added to the retrieval system",
           "The covariance diagonal block matrix",
           "The inverse covariance diagonal block matrix",
           "The delta of the value incase manual perturbation is needed"},
  };

  wsm_data["RetrievalAddAtmosphere"] = {
      .desc   = R"(Add atmospheric property the retrieval setup.
)",
      .author = {"Richard Larsson"},
      .out    = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .in     = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .gin    = {"species", "matrix", "inverse", "d"},
      .gin_type =
          {"AtmKey,SpeciesEnum,SpeciesIsotope,QuantumIdentifier",
           "BlockMatrix",
           "BlockMatrix",
           "Numeric"},
      .gin_value = {std::nullopt, std::nullopt, BlockMatrix{}, Numeric{0.1}},
      .gin_desc =
          {"The property added to the retrieval system",
           "The covariance diagonal block matrix",
           "The inverse covariance diagonal block matrix",
           "The delta of the value incase manual perturbation is needed"},
  };

  wsm_data["RetrievalAddSpeciesVMR"] = {
      .desc      = R"(Add species VMR to the retrieval setup.
)",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .in        = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .gin       = {"species", "matrix", "inverse", "d"},
      .gin_type  = {"SpeciesEnum", "BlockMatrix", "BlockMatrix", "Numeric"},
      .gin_value = {std::nullopt, std::nullopt, BlockMatrix{}, Numeric{0.1}},
      .gin_desc =
          {"The species added to the retrieval system",
           "The covariance diagonal block matrix",
           "The inverse covariance diagonal block matrix",
           "The delta of the value incase manual perturbation is needed"},
  };

  wsm_data["RetrievalAddSpeciesIsotopologueRatio"] = {
      .desc      = R"(Add species isotopologue ratio to the retrieval setup.
)",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .in        = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .gin       = {"species", "matrix", "inverse", "d"},
      .gin_type  = {"SpeciesIsotope", "BlockMatrix", "BlockMatrix", "Numeric"},
      .gin_value = {std::nullopt, std::nullopt, BlockMatrix{}, Numeric{0.1}},
      .gin_desc =
          {"The isotopologue ratio added to the retrieval system",
           "The covariance diagonal block matrix",
           "The inverse covariance diagonal block matrix",
           "The delta of the value incase manual perturbation is needed"},
  };

  wsm_data["RetrievalAddMagneticField"] = {
      .desc      = R"(Add magnetic field component to the retrieval setup.
)",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .in        = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .gin       = {"component", "matrix", "inverse", "d"},
      .gin_type  = {"String", "BlockMatrix", "BlockMatrix", "Numeric"},
      .gin_value = {std::nullopt, std::nullopt, BlockMatrix{}, Numeric{0.1}},
      .gin_desc =
          {"The magnetic field component added to the retrieval system",
           "The covariance diagonal block matrix",
           "The inverse covariance diagonal block matrix",
           "The delta of the value incase manual perturbation is needed"},
  };

  wsm_data["RetrievalAddWindField"] = {
      .desc      = R"(Add species VMR to the retrieval setup.
)",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .in        = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .gin       = {"component", "matrix", "inverse", "d"},
      .gin_type  = {"String", "BlockMatrix", "BlockMatrix", "Numeric"},
      .gin_value = {std::nullopt, std::nullopt, BlockMatrix{}, Numeric{0.1}},
      .gin_desc =
          {"The magnetic field component added to the retrieval system",
           "The covariance diagonal block matrix",
           "The inverse covariance diagonal block matrix",
           "The delta of the value incase manual perturbation is needed"},
  };

  wsm_data["RetrievalAddTemperature"] = {
      .desc      = R"(Add atmospheric temperature to the retrieval setup.
)",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .in        = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .gin       = {"matrix", "inverse", "d"},
      .gin_type  = {"BlockMatrix", "BlockMatrix", "Numeric"},
      .gin_value = {std::nullopt, BlockMatrix{}, Numeric{0.1}},
      .gin_desc =
          {"The covariance diagonal block matrix",
           "The inverse covariance diagonal block matrix",
           "The delta of the value incase manual perturbation is needed"},
  };

  wsm_data["RetrievalAddPressure"] = {
      .desc      = R"(Add atmospheric pressure to the retrieval setup.
)",
      .author    = {"Richard Larsson"},
      .out       = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .in        = {"jacobian_targets", "covariance_matrix_diagonal_blocks"},
      .gin       = {"matrix", "inverse", "d"},
      .gin_type  = {"BlockMatrix", "BlockMatrix", "Numeric"},
      .gin_value = {std::nullopt, BlockMatrix{}, Numeric{0.1}},
      .gin_desc =
          {"The covariance diagonal block matrix",
           "The inverse covariance diagonal block matrix",
           "The delta of the value incase manual perturbation is needed"},
  };

  wsm_data["RetrievalFinalizeDiagonal"] = {
      .desc   = R"(Add species VMR to the retrieval setup.
)",
      .author = {"Richard Larsson"},
      .out    = {"model_state_covariance_matrix", "jacobian_targets"},
      .in     = {"jacobian_targets",
                 "covariance_matrix_diagonal_blocks",
                 "atmospheric_field",
                 "surface_field",
                 "absorption_bands"},
  };

  /*
  LEAVE THIS LAST AS IT REQUIRES THE DATA ABOVE TO FUNCTION
  */
  for (auto& m : internal_meta_methods()) {
    wsm_data[m.name] = m.create(wsm_data);
  }

  return wsm_data;
} catch (std::exception& e) {
  throw std::runtime_error(
      var_string("Cannot create workspace methods:\n\n", e.what()));
}

const std::unordered_map<std::string, WorkspaceMethodInternalRecord>&
internal_workspace_methods() {
  static const auto wsm_data = internal_workspace_methods_create();
  return wsm_data;
}