#include <lineshape.h>
#include <lineshapemodel.h>
#include <py_auto_interface.h>
#include <pybind11/cast.h>
#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <zeemandata.h>

#include <memory>
#include <string>
#include <utility>

#include "absorptionlines.h"
#include "debug.h"
#include "py_macros.h"
#include "quantum_numbers.h"
#include "species_tags.h"

namespace Python {
void internal_spectroscopy(py::module_& m) {
  m.doc() = "For internal functionality dealing with spectroscopy";

  py::class_<AbsorptionMirroringTagTypeStatus>(
      m, "AbsorptionMirroringTagTypeStatus")
      .def(py::init([](ArrayOfArrayOfAbsorptionLines& a) {
        return new AbsorptionMirroringTagTypeStatus{a};
      }))
      .def_readwrite("None", &AbsorptionMirroringTagTypeStatus::None)
      .def_readwrite("Lorentz", &AbsorptionMirroringTagTypeStatus::Lorentz)
      .def_readwrite("SameAsLineShape",
           &AbsorptionMirroringTagTypeStatus::SameAsLineShape)
      .def_readwrite("Manual", &AbsorptionMirroringTagTypeStatus::Manual)
      .PythonInterfaceBasicRepresentation(AbsorptionMirroringTagTypeStatus);

  py::class_<AbsorptionNormalizationTagTypeStatus>(
      m, "AbsorptionNormalizationTagTypeStatus")
      .def(py::init([](ArrayOfArrayOfAbsorptionLines& a) {
        return new AbsorptionNormalizationTagTypeStatus{a};
      }))
      .def_readwrite("None", &AbsorptionNormalizationTagTypeStatus::None)
      .def_readwrite("VVH", &AbsorptionNormalizationTagTypeStatus::VVH)
      .def_readwrite("VVW", &AbsorptionNormalizationTagTypeStatus::VVW)
      .def_readwrite("RQ", &AbsorptionNormalizationTagTypeStatus::RQ)
      .def_readwrite("SFS", &AbsorptionNormalizationTagTypeStatus::SFS)
      .PythonInterfaceBasicRepresentation(AbsorptionNormalizationTagTypeStatus);

  py::class_<AbsorptionPopulationTagTypeStatus>(
      m, "AbsorptionPopulationTagTypeStatus")
      .def(py::init([](ArrayOfArrayOfAbsorptionLines& a) {
        return new AbsorptionPopulationTagTypeStatus{a};
      }))
      .def_readwrite("LTE", &AbsorptionPopulationTagTypeStatus::LTE)
      .def_readwrite("NLTE", &AbsorptionPopulationTagTypeStatus::NLTE)
      .def_readwrite("VibTemps", &AbsorptionPopulationTagTypeStatus::VibTemps)
      .def_readwrite("ByHITRANRosenkranzRelmat",
           &AbsorptionPopulationTagTypeStatus::ByHITRANRosenkranzRelmat)
      .def_readwrite("ByHITRANFullRelmat",
           &AbsorptionPopulationTagTypeStatus::ByHITRANFullRelmat)
      .def_readwrite("ByMakarovFullRelmat",
           &AbsorptionPopulationTagTypeStatus::ByMakarovFullRelmat)
      .def_readwrite("ByRovibLinearDipoleLineMixing",
           &AbsorptionPopulationTagTypeStatus::ByRovibLinearDipoleLineMixing)
      .PythonInterfaceBasicRepresentation(AbsorptionPopulationTagTypeStatus);

  py::class_<AbsorptionCutoffTagTypeStatus>(m, "AbsorptionCutoffTagTypeStatus")
      .def(py::init([](ArrayOfArrayOfAbsorptionLines& a) {
        return new AbsorptionCutoffTagTypeStatus{a};
      }))
      .def_readwrite("None", &AbsorptionCutoffTagTypeStatus::None)
      .def_readwrite("ByLine", &AbsorptionCutoffTagTypeStatus::ByLine)
      .PythonInterfaceBasicRepresentation(AbsorptionCutoffTagTypeStatus);

  py::class_<AbsorptionLineShapeTagTypeStatus>(
      m, "AbsorptionLineShapeTagTypeStatus")
      .def(py::init([](ArrayOfArrayOfAbsorptionLines& a) {
        return new AbsorptionLineShapeTagTypeStatus{a};
      }))
      .def_readwrite("DP", &AbsorptionLineShapeTagTypeStatus::DP)
      .def_readwrite("LP", &AbsorptionLineShapeTagTypeStatus::LP)
      .def_readwrite("VP", &AbsorptionLineShapeTagTypeStatus::VP)
      .def_readwrite("SDVP", &AbsorptionLineShapeTagTypeStatus::SDVP)
      .def_readwrite("HTP", &AbsorptionLineShapeTagTypeStatus::HTP)
      .PythonInterfaceBasicRepresentation(AbsorptionLineShapeTagTypeStatus);

  py::class_<AbsorptionTagTypesStatus>(m, "AbsorptionTagTypesStatus")
      .def(py::init([](ArrayOfArrayOfAbsorptionLines& a) {
        return new AbsorptionTagTypesStatus{a};
      }))
      .def_readwrite("mirroring", &AbsorptionTagTypesStatus::mirroring)
      .def_readwrite("normalization", &AbsorptionTagTypesStatus::normalization)
      .def_readwrite("population", &AbsorptionTagTypesStatus::population)
      .def_readwrite("cutoff", &AbsorptionTagTypesStatus::cutoff)
      .def_readwrite("lineshapetype", &AbsorptionTagTypesStatus::lineshapetype)
      .PythonInterfaceBasicRepresentation(AbsorptionTagTypesStatus);
}

void py_spectroscopy(py::module_& m) {
  py::class_<LineShape::TemperatureModel>(m, "LineShapeTemperatureModel")
      .def(py::init([]() { return new LineShape::TemperatureModel{}; }))
      .def(py::init([](const std::string& c) {
        return LineShape::toTemperatureModelOrThrow(c);
      }))
      .PythonInterfaceCopyValue(LineShape::TemperatureModel)
      .PythonInterfaceBasicRepresentation(LineShape::TemperatureModel)
      .def(py::pickle(
          [](const LineShape::TemperatureModel& t) {
            return py::make_tuple(std::string(LineShape::toString(t)));
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 1, "Invalid state!")
            return new LineShape::TemperatureModel{
                LineShape::toTemperatureModel(t[0].cast<std::string>())};
          }));
  py::implicitly_convertible<std::string, LineShape::TemperatureModel>();

  static_assert(LineShapeModelParameters::N == 4);
  py::class_<LineShapeModelParameters>(m, "LineShapeModelParameters")
      .def(py::init([](LineShape::TemperatureModel a,
                       Numeric b,
                       Numeric c,
                       Numeric d,
                       Numeric e) {
             return new LineShapeModelParameters(a, b, c, d, e);
           }),
           py::arg("type") = LineShape::TemperatureModel::None,
           py::arg("X0") = 0,
           py::arg("X1") = 0,
           py::arg("X2") = 0,
           py::arg("X3") = 0)
      .PythonInterfaceCopyValue(LineShapeModelParameters)
      .def_readwrite("type", &LineShapeModelParameters::type)
      .def_readwrite("X0", &LineShapeModelParameters::X0)
      .def_readwrite("X1", &LineShapeModelParameters::X1)
      .def_readwrite("X2", &LineShapeModelParameters::X2)
      .def_readwrite("X3", &LineShapeModelParameters::X3)
      .PythonInterfaceBasicRepresentation(LineShapeModelParameters)
      .def(py::pickle(
          [](const LineShapeModelParameters& t) {
            return py::make_tuple(t.type, t.X0, t.X1, t.X2, t.X3);
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 5, "Invalid state!")
            return new LineShapeModelParameters{
                t[0].cast<LineShape::TemperatureModel>(),
                t[1].cast<Numeric>(),
                t[2].cast<Numeric>(),
                t[3].cast<Numeric>(),
                t[4].cast<Numeric>()};
          }));

  py::class_<AbsorptionCutoffType>(m, "AbsorptionCutoffType")
      .def(py::init([]() { return new AbsorptionCutoffType{}; }))
      .def(py::init([](const std::string& c) {
             return Absorption::toCutoffTypeOrThrow(c);
           }),
           py::arg("str").none(false))
      .PythonInterfaceCopyValue(AbsorptionCutoffType)
      .PythonInterfaceBasicRepresentation(AbsorptionCutoffType)
      .def(py::pickle(
          [](const AbsorptionCutoffType& t) {
            return py::make_tuple(std::string(Absorption::toString(t)));
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 1, "Invalid state!")
            return new AbsorptionCutoffType{
                Absorption::toCutoffType(t[0].cast<std::string>())};
          }));
  py::implicitly_convertible<std::string, AbsorptionCutoffType>();

  auto ZeemanPolarizationStringGetter =
      [](Zeeman::Polarization c) -> std::string {
    if (c == Zeeman::Polarization::SigmaMinus) return "SigmaMinus";
    if (c == Zeeman::Polarization::Pi) return "Pi";
    if (c == Zeeman::Polarization::SigmaPlus) return "SigmaPlus";
    if (c == Zeeman::Polarization::None) return "None";
    ARTS_USER_ERROR("Bad enum state")
  };
  auto ZeemanPolarizationEnumGetter =
      [](const std::string& c) -> Zeeman::Polarization {
    if (c == "SigmaMinus") return Zeeman::Polarization::SigmaMinus;
    if (c == "Pi") return Zeeman::Polarization::Pi;
    if (c == "SigmaPlus") return Zeeman::Polarization::SigmaPlus;
    if (c == "None") return Zeeman::Polarization::None;
    ARTS_USER_ERROR("Bad enum value ", c);
  };
  py::class_<Zeeman::Polarization>(m, "ZeemanPolarization")
      .def(py::init([]() { return new Zeeman::Polarization{}; }))
      .def(py::init([ZeemanPolarizationEnumGetter](const std::string& c) {
             return ZeemanPolarizationEnumGetter(c);
           }),
           py::arg("str"))
      .PythonInterfaceCopyValue(Zeeman::Polarization)
      .def("__repr__",
           [ZeemanPolarizationStringGetter](Zeeman::Polarization c) {
             return ZeemanPolarizationStringGetter(c);
           })
      .def("__str__",
           [ZeemanPolarizationStringGetter](Zeeman::Polarization c) {
             return ZeemanPolarizationStringGetter(c);
           })
      .def(py::pickle(
          [ZeemanPolarizationStringGetter](const Zeeman::Polarization& c) {
            return py::make_tuple(ZeemanPolarizationStringGetter(c));
          },
          [ZeemanPolarizationEnumGetter](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 1, "Invalid state!")
            return ZeemanPolarizationEnumGetter(t[0].cast<std::string>());
          }));
  py::implicitly_convertible<std::string, Zeeman::Polarization>();

  py::class_<AbsorptionMirroringType>(m, "AbsorptionMirroringType")
      .def(py::init([]() { return new AbsorptionMirroringType{}; }))
      .def(py::init([](const std::string& c) {
             return Absorption::toMirroringTypeOrThrow(c);
           }),
           py::arg("str").none(false))
      .PythonInterfaceCopyValue(AbsorptionMirroringType)
      .PythonInterfaceBasicRepresentation(AbsorptionMirroringType)
      .def(py::pickle(
          [](const AbsorptionMirroringType& t) {
            return py::make_tuple(std::string(Absorption::toString(t)));
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 1, "Invalid state!")
            return new AbsorptionMirroringType{
                Absorption::toMirroringType(t[0].cast<std::string>())};
          }));
  py::implicitly_convertible<std::string, AbsorptionMirroringType>();

  py::class_<AbsorptionPopulationType>(m, "AbsorptionPopulationType")
      .def(py::init([]() { return new AbsorptionPopulationType{}; }))
      .def(py::init([](const std::string& c) {
             return Absorption::toPopulationTypeOrThrow(c);
           }),
           py::arg("str").none(false))
      .PythonInterfaceCopyValue(AbsorptionPopulationType)
      .PythonInterfaceBasicRepresentation(AbsorptionPopulationType)
      .def(py::pickle(
          [](const AbsorptionPopulationType& t) {
            return py::make_tuple(std::string(Absorption::toString(t)));
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 1, "Invalid state!")
            return new AbsorptionPopulationType{
                Absorption::toPopulationType(t[0].cast<std::string>())};
          }));
  py::implicitly_convertible<std::string, AbsorptionPopulationType>();

  py::class_<AbsorptionNormalizationType>(m, "AbsorptionNormalizationType")
      .def(py::init([]() { return new AbsorptionNormalizationType{}; }))
      .def(py::init([](const std::string& c) {
             return Absorption::toNormalizationTypeOrThrow(c);
           }),
           py::arg("str").none(false))
      .PythonInterfaceCopyValue(AbsorptionNormalizationType)
      .PythonInterfaceBasicRepresentation(AbsorptionNormalizationType)
      .def(py::pickle(
          [](const AbsorptionNormalizationType& t) {
            return py::make_tuple(std::string(Absorption::toString(t)));
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 1, "Invalid state!")
            return new AbsorptionNormalizationType{
                Absorption::toNormalizationType(t[0].cast<std::string>())};
          }));
  py::implicitly_convertible<std::string, AbsorptionNormalizationType>();

  py::class_<LineShapeType>(m, "LineShapeType")
      .def(py::init([]() { return new LineShapeType{}; }))
      .def(py::init([](const std::string& c) {
             return LineShape::toTypeOrThrow(c);
           }),
           py::arg("str").none(false))
      .PythonInterfaceCopyValue(LineShapeType)
      .PythonInterfaceBasicRepresentation(LineShapeType)
      .def(py::pickle(
          [](const LineShapeType& t) {
            return py::make_tuple(std::string(LineShape::toString(t)));
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 1, "Invalid state!")
            return new LineShapeType{
                LineShape::toType(t[0].cast<std::string>())};
          }));
  py::implicitly_convertible<std::string, LineShapeType>();

  py::class_<Zeeman::Model>(m, "ZeemanModel")
      .def(py::init([]() { return new Zeeman::Model{}; }))
      .def(py::init([](Numeric a, Numeric b) {
             return new Zeeman::Model{a, b};
           }),
           py::arg("gu"),
           py::arg("gl"))
      .PythonInterfaceCopyValue(Zeeman::Model)
      .PythonInterfaceBasicRepresentation(Zeeman::Model)
      .def_property("gu", &Zeeman::Model::gu, &Zeeman::Model::gu)
      .def_property("gl", &Zeeman::Model::gl, &Zeeman::Model::gl)
      .def(py::pickle(
          [](const Zeeman::Model& t) { return py::make_tuple(t.gu(), t.gl()); },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 2, "Invalid state!")
            return new Zeeman::Model{t[0].cast<Numeric>(),
                                     t[1].cast<Numeric>()};
          }));

  py::class_<LineShape::Output>(m, "LineShapeOutput")
      .def(py::init([](Numeric a,
                       Numeric b,
                       Numeric c,
                       Numeric d,
                       Numeric e,
                       Numeric f,
                       Numeric g,
                       Numeric h,
                       Numeric i) {
             return new LineShape::Output{a, b, c, d, e, f, g, h, i};
           }),
           py::arg("g0") = 0,
           py::arg("d0") = 0,
           py::arg("g2") = 0,
           py::arg("d2") = 0,
           py::arg("fvc") = 0,
           py::arg("eta") = 0,
           py::arg("y") = 0,
           py::arg("g") = 0,
           py::arg("dv") = 0)
      .PythonInterfaceCopyValue(LineShape::Output)
      .PythonInterfaceReadWriteData(LineShape::Output, G0)
      .PythonInterfaceReadWriteData(LineShape::Output, D0)
      .PythonInterfaceReadWriteData(LineShape::Output, G2)
      .PythonInterfaceReadWriteData(LineShape::Output, D2)
      .PythonInterfaceReadWriteData(LineShape::Output, ETA)
      .PythonInterfaceReadWriteData(LineShape::Output, FVC)
      .PythonInterfaceReadWriteData(LineShape::Output, Y)
      .PythonInterfaceReadWriteData(LineShape::Output, G)
      .PythonInterfaceReadWriteData(LineShape::Output, DV)
      .PythonInterfaceBasicRepresentation(LineShape::Output)
      .def(py::pickle(
          [](const LineShape::Output& t) {
            return py::make_tuple(
                t.G0, t.D0, t.G2, t.D2, t.FVC, t.ETA, t.Y, t.G, t.DV);
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 9, "Invalid state!")
            return new LineShape::Output{t[0].cast<Numeric>(),
                                         t[1].cast<Numeric>(),
                                         t[2].cast<Numeric>(),
                                         t[3].cast<Numeric>(),
                                         t[4].cast<Numeric>(),
                                         t[5].cast<Numeric>(),
                                         t[6].cast<Numeric>(),
                                         t[7].cast<Numeric>(),
                                         t[8].cast<Numeric>()};
          }));

  py::class_<LineShapeSingleSpeciesModel>(m, "LineShapeSingleSpeciesModel")
      .def(py::init([](LineShape::ModelParameters a,
                       LineShape::ModelParameters b,
                       LineShape::ModelParameters c,
                       LineShape::ModelParameters d,
                       LineShape::ModelParameters e,
                       LineShape::ModelParameters f,
                       LineShape::ModelParameters g,
                       LineShape::ModelParameters h,
                       LineShape::ModelParameters i) {
             return new LineShapeSingleSpeciesModel{a, b, c, d, e, f, g, h, i};
           }),
           py::arg("G0") = LineShape::ModelParameters{},
           py::arg("D0") = LineShape::ModelParameters{},
           py::arg("G2") = LineShape::ModelParameters{},
           py::arg("D2") = LineShape::ModelParameters{},
           py::arg("FVC") = LineShape::ModelParameters{},
           py::arg("ETA") = LineShape::ModelParameters{},
           py::arg("Y") = LineShape::ModelParameters{},
           py::arg("G") = LineShape::ModelParameters{},
           py::arg("DV") = LineShape::ModelParameters{})
      .PythonInterfaceCopyValue(LineShapeSingleSpeciesModel)
      .def_property(
          "G0",
          [](const LineShapeSingleSpeciesModel& x) { return x.G0(); },
          [](LineShapeSingleSpeciesModel& x, LineShapeModelParameters y) {
            return x.G0() = y;
          })
      .def_property(
          "D0",
          [](const LineShapeSingleSpeciesModel& x) { return x.D0(); },
          [](LineShapeSingleSpeciesModel& x, LineShapeModelParameters y) {
            return x.D0() = y;
          })
      .def_property(
          "G2",
          [](const LineShapeSingleSpeciesModel& x) { return x.G2(); },
          [](LineShapeSingleSpeciesModel& x, LineShapeModelParameters y) {
            return x.G2() = y;
          })
      .def_property(
          "D2",
          [](const LineShapeSingleSpeciesModel& x) { return x.D2(); },
          [](LineShapeSingleSpeciesModel& x, LineShapeModelParameters y) {
            return x.D2() = y;
          })
      .def_property(
          "FVC",
          [](const LineShapeSingleSpeciesModel& x) { return x.FVC(); },
          [](LineShapeSingleSpeciesModel& x, LineShapeModelParameters y) {
            return x.FVC() = y;
          })
      .def_property(
          "ETA",
          [](const LineShapeSingleSpeciesModel& x) { return x.ETA(); },
          [](LineShapeSingleSpeciesModel& x, LineShapeModelParameters y) {
            return x.ETA() = y;
          })
      .def_property(
          "Y",
          [](const LineShapeSingleSpeciesModel& x) { return x.Y(); },
          [](LineShapeSingleSpeciesModel& x, LineShapeModelParameters y) {
            return x.Y() = y;
          })
      .def_property(
          "G",
          [](const LineShapeSingleSpeciesModel& x) { return x.G(); },
          [](LineShapeSingleSpeciesModel& x, LineShapeModelParameters y) {
            return x.G() = y;
          })
      .def_property(
          "DV",
          [](const LineShapeSingleSpeciesModel& x) { return x.DV(); },
          [](LineShapeSingleSpeciesModel& x, LineShapeModelParameters y) {
            return x.DV() = y;
          })
      .def(py::pickle(
          [](const LineShapeSingleSpeciesModel& t) {
            return py::make_tuple(t.G0(),
                                  t.D0(),
                                  t.G2(),
                                  t.D2(),
                                  t.FVC(),
                                  t.ETA(),
                                  t.Y(),
                                  t.G(),
                                  t.DV());
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 9, "Invalid state!")
            return new LineShapeSingleSpeciesModel{
                t[0].cast<LineShape::ModelParameters>(),
                t[1].cast<LineShape::ModelParameters>(),
                t[2].cast<LineShape::ModelParameters>(),
                t[3].cast<LineShape::ModelParameters>(),
                t[4].cast<LineShape::ModelParameters>(),
                t[5].cast<LineShape::ModelParameters>(),
                t[6].cast<LineShape::ModelParameters>(),
                t[7].cast<LineShape::ModelParameters>(),
                t[8].cast<LineShape::ModelParameters>()};
          }));

  py::class_<LineShapeModel>(m, "LineShapeModel")
      .def(py::init([]() { return new LineShapeModel{}; }))
      .def(py::init([](const std::vector<LineShapeSingleSpeciesModel>& v) {
        return new LineShapeModel{v};
      }))
      .PythonInterfaceCopyValue(LineShapeModel)
      .PythonInterfaceBasicRepresentation(LineShapeModel)
      .PythonInterfaceIndexItemAccess(LineShapeModel)
      .def_property(
          "data",
          [](const LineShapeModel& x) { return x.Data(); },
          [](LineShapeModel& x, std::vector<LineShapeSingleSpeciesModel> y) {
            x.Data() = std::move(y);
          })
      .def(py::pickle(
          [](const LineShapeModel& t) { return py::make_tuple(t.Data()); },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 1, "Invalid state!")
            return new LineShapeModel{
                t[0].cast<std::vector<LineShapeSingleSpeciesModel>>()};
          }));
  py::implicitly_convertible<std::vector<LineShapeSingleSpeciesModel>,
                             LineShapeModel>();

  py::class_<AbsorptionSingleLine>(m, "AbsorptionSingleLine")
      .def(py::init([](Numeric a,
                       Numeric b,
                       Numeric c,
                       Numeric d,
                       Numeric e,
                       Numeric f,
                       ZeemanModel g,
                       LineShapeModel h,
                       Quantum::Number::LocalState i) {
             return new AbsorptionSingleLine{
                 a, b, c, d, e, f, g, std::move(h), std::move(i)};
           }),
           py::arg("F0") = 0,
           py::arg("I0") = 0,
           py::arg("E0") = 0,
           py::arg("glow") = 0,
           py::arg("gupp") = 0,
           py::arg("A") = 0,
           py::arg("zeeman") = Zeeman::Model(),
           py::arg("lineshape") = LineShape::Model(),
           py::arg("localquanta") = Quantum::Number::LocalState{})
      .PythonInterfaceCopyValue(AbsorptionSingleLine)
      .PythonInterfaceBasicRepresentation(AbsorptionSingleLine)
      .PythonInterfaceReadWriteData(AbsorptionSingleLine, F0)
      .PythonInterfaceReadWriteData(AbsorptionSingleLine, I0)
      .PythonInterfaceReadWriteData(AbsorptionSingleLine, E0)
      .PythonInterfaceReadWriteData(AbsorptionSingleLine, glow)
      .PythonInterfaceReadWriteData(AbsorptionSingleLine, gupp)
      .PythonInterfaceReadWriteData(AbsorptionSingleLine, A)
      .PythonInterfaceReadWriteData(AbsorptionSingleLine, zeeman)
      .PythonInterfaceReadWriteData(AbsorptionSingleLine, lineshape)
      .PythonInterfaceReadWriteData(AbsorptionSingleLine, localquanta)
      .def(py::pickle(
          [](const AbsorptionSingleLine& t) {
            return py::make_tuple(t.F0,
                                  t.I0,
                                  t.E0,
                                  t.glow,
                                  t.gupp,
                                  t.A,
                                  t.zeeman,
                                  t.lineshape,
                                  t.localquanta);
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 9, "Invalid state!")
            return new AbsorptionSingleLine{
                t[0].cast<Numeric>(),
                t[1].cast<Numeric>(),
                t[2].cast<Numeric>(),
                t[3].cast<Numeric>(),
                t[4].cast<Numeric>(),
                t[5].cast<Numeric>(),
                t[6].cast<Zeeman::Model>(),
                t[7].cast<LineShape::Model>(),
                t[8].cast<Quantum::Number::LocalState>()};
          }));

  py::class_<Array<AbsorptionSingleLine>>(m, "ArrayOfAbsorptionSingleLine")
      .PythonInterfaceBasicRepresentation(Array<AbsorptionSingleLine>)
      .PythonInterfaceArrayDefault(AbsorptionSingleLine)
      .doc() = "The Arts ArrayOfAbsorptionSingleLine class";
  py::implicitly_convertible<std::vector<AbsorptionSingleLine>,
                             Array<AbsorptionSingleLine>>();

  py::class_<AbsorptionLines>(m, "AbsorptionLines")
      .def(py::init([](bool a,
                       bool b,
                       AbsorptionCutoffType c,
                       AbsorptionMirroringType d,
                       AbsorptionPopulationType e,
                       AbsorptionNormalizationType f,
                       LineShape::Type g,
                       Numeric h,
                       Numeric i,
                       Numeric j,
                       QuantumIdentifier k,
                       ArrayOfSpecies l,
                       Array<AbsorptionSingleLine> n) {
             return new AbsorptionLines{a,
                                        b,
                                        c,
                                        d,
                                        e,
                                        f,
                                        g,
                                        h,
                                        i,
                                        j,
                                        std::move(k),
                                        std::move(l),
                                        std::move(n)};
           }),
           py::arg("selfbroadening") = false,
           py::arg("bathbroadening") = false,
           py::arg("cutoff") = AbsorptionCutoffType::None,
           py::arg("mirroring") = AbsorptionMirroringType::None,
           py::arg("population") = AbsorptionPopulationType::LTE,
           py::arg("normalization") = AbsorptionNormalizationType::None,
           py::arg("lineshapetype") = LineShape::Type::DP,
           py::arg("T0") = 296,
           py::arg("cutofffreq") = -1,
           py::arg("linemixinglimit") = -1,
           py::arg("quantumidentity") = QuantumIdentifier(),
           py::arg("broadeningspecies") = ArrayOfSpecies{},
           py::arg("lines") = Array<AbsorptionSingleLine>{})
      .PythonInterfaceCopyValue(AbsorptionLines)
      .PythonInterfaceWorkspaceVariableConversion(AbsorptionLines)
      .PythonInterfaceBasicRepresentation(AbsorptionLines)
      .PythonInterfaceFileIO(AbsorptionLines)
      .PythonInterfaceReadWriteData(AbsorptionLines, selfbroadening)
      .PythonInterfaceReadWriteData(AbsorptionLines, bathbroadening)
      .PythonInterfaceReadWriteData(AbsorptionLines, cutoff)
      .PythonInterfaceReadWriteData(AbsorptionLines, mirroring)
      .PythonInterfaceReadWriteData(AbsorptionLines, population)
      .PythonInterfaceReadWriteData(AbsorptionLines, normalization)
      .PythonInterfaceReadWriteData(AbsorptionLines, lineshapetype)
      .PythonInterfaceReadWriteData(AbsorptionLines, T0)
      .PythonInterfaceReadWriteData(AbsorptionLines, cutofffreq)
      .PythonInterfaceReadWriteData(AbsorptionLines, linemixinglimit)
      .PythonInterfaceReadWriteData(AbsorptionLines, quantumidentity)
      .PythonInterfaceReadWriteData(AbsorptionLines, broadeningspecies)
      .PythonInterfaceReadWriteData(AbsorptionLines, lines)
      .def_property_readonly(
          "ok",
          &AbsorptionLines::OK,
          py::doc("If false, the catalog cannot be used for any calculations"))
      .def_property_readonly("meta_data",
                             &AbsorptionLines::MetaData,
                             py::doc("Catalog meta data string"))
      .def(
          "LineShapeOutput",
          [](AbsorptionLines& band,
             Index line,
             Numeric T,
             Numeric P,
             const Vector& VMR) {
            ARTS_USER_ERROR_IF(not band.OK(), "Band in bad shape")
            ARTS_USER_ERROR_IF(not(T > 0) or not(P >= 0),
                               "Bad atmospheric state (T P): ",
                               Vector{T, P})
            ARTS_USER_ERROR_IF(
                VMR.size() not_eq band.broadeningspecies.nelem(),
                "Mismatch between VMRs and broadening species.\nVMR: ",
                VMR,
                "\nSpecies: ",
                band.broadeningspecies)
            band.lines.at(line);

            return band.ShapeParameters(line, T, P, VMR);
          },
          py::doc(
              R"--(Computes the line shape paramters for the given atmospheric state
Note that the normalization assumes sum(VMR) is 1 for good results but does not enforce it)--"))
      .def(py::pickle(
          [](const AbsorptionLines& t) {
            return py::make_tuple(t.selfbroadening,
                                  t.bathbroadening,
                                  t.cutoff,
                                  t.mirroring,
                                  t.population,
                                  t.normalization,
                                  t.lineshapetype,
                                  t.T0,
                                  t.cutofffreq,
                                  t.linemixinglimit,
                                  t.quantumidentity,
                                  t.broadeningspecies,
                                  t.lines);
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 13, "Invalid state!")
            return new AbsorptionLines{
                t[0].cast<bool>(),
                t[1].cast<bool>(),
                t[2].cast<AbsorptionCutoffType>(),
                t[3].cast<AbsorptionMirroringType>(),
                t[4].cast<AbsorptionPopulationType>(),
                t[5].cast<AbsorptionNormalizationType>(),
                t[6].cast<LineShapeType>(),
                t[7].cast<Numeric>(),
                t[8].cast<Numeric>(),
                t[9].cast<Numeric>(),
                t[10].cast<QuantumIdentifier>(),
                t[11].cast<ArrayOfSpecies>(),
                t[12].cast<Array<AbsorptionSingleLine>>()};
          }))
      .PythonInterfaceWorkspaceDocumentation(AbsorptionLines);

  PythonInterfaceWorkspaceArray(AbsorptionLines);
  PythonInterfaceWorkspaceArray(ArrayOfAbsorptionLines);

  py::class_<LineShape::Calculator>(m, "LineShapeCalculator")
      .def(py::init([](AbsorptionLines& band,
                       Index line,
                       Numeric T,
                       Numeric P,
                       const Vector& VMR,
                       Zeeman::Polarization zeeman,
                       Numeric H,
                       Index iz) {
             ARTS_USER_ERROR_IF(not band.OK(), "Band in bad shape")
             ARTS_USER_ERROR_IF(not(T > 0) or not(P >= 0) or not(H >= 0),
                                "Bad atmospheric state (T P H): ",
                                Vector{T, P, H})
             ARTS_USER_ERROR_IF(
                 VMR.size() not_eq band.broadeningspecies.nelem(),
                 "Mismatch between VMRs and broadening species.\nVMR: ",
                 VMR,
                 "\nSpecies: ",
                 band.broadeningspecies)
             auto F0 = band.lines.at(line).F0;
             auto DC = band.DopplerConstant(T);
             auto mirror = band.mirroring;
             auto type = band.lineshapetype;
             auto X = band.ShapeParameters(line, T, P, VMR);
             auto DZ = band.ZeemanSplitting(line, zeeman, iz) * H;

             if (mirror == AbsorptionMirroringType::Manual)
               return LineShape::Calculator(mirror, type, F0, X, DC, DZ);
             return LineShape::Calculator(type, F0, X, DC, DZ, false);
           }),
           py::arg("band"),
           py::arg("line"),
           py::arg("T"),
           py::arg("P"),
           py::arg("VMR"),
           py::arg("zeeman") = Zeeman::Polarization::None,
           py::arg("H") = 0,
           py::arg("iz") = 0)
      .PythonInterfaceCopyValue(LineShape::Calculator)
      .def("dFdT",
           [](LineShape::Calculator& LS,
              const LineShape::Output& dXdT,
              Numeric T) { return LS.dFdT(dXdT, T); })
      .def("dFdf", [](LineShape::Calculator& LS) { return LS.dFdf(); })
      .def("dFdF0", [](LineShape::Calculator& LS) { return LS.dFdF0(); })
      .def(
          "dFdH",
          [](LineShape::Calculator& LS, Numeric dfdH) { return LS.dFdH(dfdH); })
      .def("dFdVMR",
           [](LineShape::Calculator& LS, const LineShape::Output& dXdVMR) {
             return LS.dFdVMR(dXdVMR);
           })
      .def("dFdFVC",
           [](LineShape::Calculator& LS, Numeric d) { return LS.dFdFVC(d); })
      .def("dFdETA",
           [](LineShape::Calculator& LS, Numeric d) { return LS.dFdETA(d); })
      .def("dFdDV",
           [](LineShape::Calculator& LS, Numeric d) { return LS.dFdDV(d); })
      .def("dFdD0",
           [](LineShape::Calculator& LS, Numeric d) { return LS.dFdD0(d); })
      .def("dFdG0",
           [](LineShape::Calculator& LS, Numeric d) { return LS.dFdG0(d); })
      .def("dFdD2",
           [](LineShape::Calculator& LS, Numeric d) { return LS.dFdD2(d); })
      .def("dFdG2",
           [](LineShape::Calculator& LS, Numeric d) { return LS.dFdG2(d); })
      .def("F", [](LineShape::Calculator& LS) { return LS.F(); })
      .def("F", [](LineShape::Calculator& LS, Numeric f) { return LS(f); })
      .doc() =
      "Class to compute the line shape\n\nNote that the normalization assumes sum(VMR) is 1 for good results but does not enforce it";

  py::class_<SpeciesErrorCorrectedSuddenData>(m,
                                              "SpeciesErrorCorrectedSuddenData")
      .def(py::init([]() { return new SpeciesErrorCorrectedSuddenData{}; }))
      .PythonInterfaceCopyValue(SpeciesErrorCorrectedSuddenData)
      .PythonInterfaceBasicRepresentation(SpeciesErrorCorrectedSuddenData)
      .def_readwrite("spec", &SpeciesErrorCorrectedSuddenData::spec)
      .def_readwrite("scaling", &SpeciesErrorCorrectedSuddenData::scaling)
      .def_readwrite("beta", &SpeciesErrorCorrectedSuddenData::beta)
      .def_readwrite("lambda", &SpeciesErrorCorrectedSuddenData::lambda)
      .def_readwrite("collisional_distance",
                     &SpeciesErrorCorrectedSuddenData::collisional_distance)
      .def_readwrite("mass", &SpeciesErrorCorrectedSuddenData::mass)
      .def(py::pickle(
          [](const SpeciesErrorCorrectedSuddenData& t) {
            return py::make_tuple(t.spec,
                                  t.scaling,
                                  t.beta,
                                  t.lambda,
                                  t.collisional_distance,
                                  t.mass);
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 6, "Invalid state!")
            auto* out = new SpeciesErrorCorrectedSuddenData{};
            out->spec = t[0].cast<Species::Species>();
            out->scaling = t[1].cast<LineShapeModelParameters>();
            out->beta = t[2].cast<LineShapeModelParameters>();
            out->lambda = t[3].cast<LineShapeModelParameters>();
            out->collisional_distance = t[4].cast<LineShapeModelParameters>();
            out->mass = t[5].cast<Numeric>();
            return out;
          }));

  py::class_<Array<SpeciesErrorCorrectedSuddenData>>(
      m, "ArrayOfSpeciesErrorCorrectedSuddenData")
      .PythonInterfaceBasicRepresentation(
          Array<SpeciesErrorCorrectedSuddenData>)
      .PythonInterfaceArrayDefault(SpeciesErrorCorrectedSuddenData);

  py::class_<ErrorCorrectedSuddenData>(m, "ErrorCorrectedSuddenData")
      .def(py::init([]() { return new ErrorCorrectedSuddenData{}; }))
      .PythonInterfaceCopyValue(ErrorCorrectedSuddenData)
      .PythonInterfaceBasicRepresentation(ErrorCorrectedSuddenData)
      .def(
          "__getitem__",
          [](ErrorCorrectedSuddenData& x, Species::Species& y)
              -> SpeciesErrorCorrectedSuddenData& { return x[y]; },
          py::return_value_policy::reference_internal)
      .def(
          "__setitem__",
          [](ErrorCorrectedSuddenData& x,
             Species::Species& y,
             SpeciesErrorCorrectedSuddenData& z) { x[y] = z; },
          py::return_value_policy::reference_internal)
      .def(py::pickle(
          [](const ErrorCorrectedSuddenData& t) {
            return py::make_tuple(t.id, t.data);
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 2, "Invalid state!")
            auto* out = new ErrorCorrectedSuddenData{};
            out->id = t[0].cast<QuantumIdentifier>();
            out->data = t[1].cast<Array<SpeciesErrorCorrectedSuddenData>>();
            return out;
          }));

  py::class_<Array<ErrorCorrectedSuddenData>>(m,
                                              "ArrayOfErrorCorrectedSuddenData")
      .PythonInterfaceBasicRepresentation(Array<ErrorCorrectedSuddenData>)
      .PythonInterfaceArrayDefault(ErrorCorrectedSuddenData);

  py::class_<MapOfErrorCorrectedSuddenData, Array<ErrorCorrectedSuddenData>>(
      m, "MapOfErrorCorrectedSuddenData")
      .def(py::init([]() { return new MapOfErrorCorrectedSuddenData{}; }))
      .PythonInterfaceCopyValue(MapOfErrorCorrectedSuddenData)
      .PythonInterfaceWorkspaceVariableConversion(MapOfErrorCorrectedSuddenData)
      .PythonInterfaceBasicRepresentation(MapOfErrorCorrectedSuddenData)
      .PythonInterfaceFileIO(MapOfErrorCorrectedSuddenData)
      .def(
          "__getitem__",
          [](MapOfErrorCorrectedSuddenData& x, QuantumIdentifier& y)
              -> ErrorCorrectedSuddenData& { return x[y]; },
          py::return_value_policy::reference_internal)
      .def(
          "__setitem__",
          [](MapOfErrorCorrectedSuddenData& x,
             QuantumIdentifier& y,
             ErrorCorrectedSuddenData& z) { x[y] = z; },
          py::return_value_policy::reference_internal)
      .def(py::pickle(
          [](const MapOfErrorCorrectedSuddenData& v) {
            auto n = v.size();
            Array<ErrorCorrectedSuddenData> out(n);
            std::copy(v.begin(), v.end(), out.begin());
            return py::make_tuple(std::move(out));
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 1, "Invalid state!")
            auto x = t[0].cast<Array<ErrorCorrectedSuddenData>>();
            auto* out = new MapOfErrorCorrectedSuddenData{};
            for (auto& b : x) out->operator[](b.id) = b;
            return out;
          }))
      .PythonInterfaceWorkspaceDocumentation(MapOfErrorCorrectedSuddenData);

  py::class_<CIARecord>(m, "CIARecord")
      .def(py::init([]() { return new CIARecord{}; }))
      .PythonInterfaceCopyValue(CIARecord)
      .PythonInterfaceWorkspaceVariableConversion(CIARecord)
      .PythonInterfaceBasicRepresentation(CIARecord)
      .PythonInterfaceFileIO(CIARecord)
      .def(py::pickle(
          [](const CIARecord& t) {
            return py::make_tuple(t.Data(), t.TwoSpecies());
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 2, "Invalid state!")
            auto* out = new CIARecord{};
            out->Data() = t[0].cast<ArrayOfGriddedField2>();
            out->TwoSpecies() = t[1].cast<std::array<Species::Species, 2>>();
            return out;
          }))
      .PythonInterfaceWorkspaceDocumentation(CIARecord);

  PythonInterfaceWorkspaceArray(CIARecord);

  py::class_<HitranRelaxationMatrixData>(m, "HitranRelaxationMatrixData")
      .def(py::init([]() { return new HitranRelaxationMatrixData{}; }))
      .PythonInterfaceCopyValue(HitranRelaxationMatrixData)
      .PythonInterfaceWorkspaceVariableConversion(HitranRelaxationMatrixData)
      .PythonInterfaceBasicRepresentation(HitranRelaxationMatrixData)
      .PythonInterfaceFileIO(HitranRelaxationMatrixData)
      .def_readwrite("W0pp", &HitranRelaxationMatrixData::W0pp)
      .def_readwrite("B0pp", &HitranRelaxationMatrixData::B0pp)
      .def_readwrite("W0rp", &HitranRelaxationMatrixData::W0rp)
      .def_readwrite("B0rp", &HitranRelaxationMatrixData::B0rp)
      .def_readwrite("W0qp", &HitranRelaxationMatrixData::W0qp)
      .def_readwrite("B0qp", &HitranRelaxationMatrixData::B0qp)
      .def_readwrite("W0pr", &HitranRelaxationMatrixData::W0pr)
      .def_readwrite("B0pr", &HitranRelaxationMatrixData::B0pr)
      .def_readwrite("W0rr", &HitranRelaxationMatrixData::W0rr)
      .def_readwrite("B0rr", &HitranRelaxationMatrixData::B0rr)
      .def_readwrite("W0qr", &HitranRelaxationMatrixData::W0qr)
      .def_readwrite("B0qr", &HitranRelaxationMatrixData::B0qr)
      .def_readwrite("W0pq", &HitranRelaxationMatrixData::W0pq)
      .def_readwrite("B0pq", &HitranRelaxationMatrixData::B0pq)
      .def_readwrite("W0rq", &HitranRelaxationMatrixData::W0rq)
      .def_readwrite("B0rq", &HitranRelaxationMatrixData::B0rq)
      .def_readwrite("W0qq", &HitranRelaxationMatrixData::W0qq)
      .def_readwrite("B0qq", &HitranRelaxationMatrixData::B0qq)
      .def(py::pickle(
          [](const HitranRelaxationMatrixData& t) {
            return py::make_tuple(t.W0pp,
                                  t.B0pp,
                                  t.W0rp,
                                  t.B0rp,
                                  t.W0pr,
                                  t.B0pr,
                                  t.W0rr,
                                  t.B0rr,
                                  t.W0qr,
                                  t.B0qr,
                                  t.W0pq,
                                  t.B0pq,
                                  t.W0rq,
                                  t.B0rq,
                                  t.W0qq,
                                  t.B0qq);
          },
          [](const py::tuple& t) {
            ARTS_USER_ERROR_IF(t.size() != 16, "Invalid state!")
            auto* out = new HitranRelaxationMatrixData{};
            out->W0pp = t[0].cast<Tensor4>();
            out->B0pp = t[1].cast<Tensor4>();
            out->W0rp = t[2].cast<Tensor4>();
            out->B0rp = t[3].cast<Tensor4>();
            out->W0pr = t[4].cast<Tensor4>();
            out->B0pr = t[5].cast<Tensor4>();
            out->W0rr = t[6].cast<Tensor4>();
            out->B0rr = t[7].cast<Tensor4>();
            out->W0qr = t[8].cast<Tensor4>();
            out->B0qr = t[9].cast<Tensor4>();
            out->W0pq = t[10].cast<Tensor4>();
            out->B0pq = t[11].cast<Tensor4>();
            out->W0rq = t[12].cast<Tensor4>();
            out->B0rq = t[13].cast<Tensor4>();
            out->W0qq = t[14].cast<Tensor4>();
            out->B0qq = t[15].cast<Tensor4>();
            return out;
          }))
      .PythonInterfaceWorkspaceDocumentation(HitranRelaxationMatrixData);

  auto spec = m.def_submodule("spectroscopy");
  internal_spectroscopy(spec);
}
}  // namespace Python