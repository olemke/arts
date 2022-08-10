#include <cstdlib>
#include <fstream>
#include <variant>

#include "global_data.h"

int main() {
  std::ofstream file_var_h("tokval_variant.h");
  std::ofstream file_h("tokval.h");
  std::ofstream file_cc("tokval.cc");

  define_wsv_groups();

  file_h << R"--(// auto-generated tokval interface

#ifndef auto_tokval_h
#define auto_tokval_h

#include "ppath_struct.h"
#include "absorptionlines.h"
#include "artstime.h"
#include "callback.h"
#include "cia.h"
#include "covariance_matrix.h"
#include "energylevelmap.h"
#include "gas_abs_lookup.h"
#include "linemixing.h"
#include "linemixing_hitran.h"
#include "matpackII.h"
#include "matpackVII.h"
#include "mc_antenna.h"
#include "optproperties.h"
#include "star.h"
#include "supergeneric.h"
#include "telsem.h"
#include "tessem.h"
#include "timer_struct.h"
#include "transmissionmatrix.h"
#include "xsec_fit.h"

#include <predefined/predef_data.h>

template <class base> class Array;
class Agenda;
using ArrayOfAgenda = Array<Agenda>;

class TokVal {
  void * ptr{nullptr};
public:

)--";

  for (auto& group : global_data::wsv_groups) {
    file_h << "  TokVal(" << group << " in);\n"
           << "  TokVal& operator=(" << group << " in);\n"
           << "  [[nodiscard]] bool holds" << group << "() const;\n"
           << "  operator " << group << "() const;\n"
           << '\n';
  }

  file_h << R"--(  TokVal(const char * const c);
    
  TokVal();
  TokVal(const TokVal& v);
  TokVal& operator=(const TokVal& v);

  ~TokVal();

  [[nodiscard]] std::shared_ptr<void> copy_value() const;

  //! Only for cases when you also have to include tokval_variant.h manually (or via, e.g., a printer)
  [[nodiscard]] void * data() const {return ptr;}
};

#endif
)--";

file_var_h <<  R"--(// auto-generated tokval implementation

#include "tokval.h"

using TokValType = std::variant<
)--";

  bool first = true;
  for (auto& group : global_data::wsv_groups) {
    if (not first) file_var_h << ",\n";
    first = false;
    file_var_h << "    std::unique_ptr<" << group << '>';
  }

  file_var_h << R"--(>;

inline TokValType* tokval_type(void * ptr) {
  return static_cast<TokValType*>(ptr);
}
)--";

file_cc << R"--(// auto-generated tokval implementation

#include "tokval.h"
#include "tokval_variant.h"
#include <agenda_class.h>

)--";

  for (auto& group : global_data::wsv_groups) {
    file_cc << "TokVal::TokVal(" << group
            << " in) : ptr(static_cast<void*>(new TokValType{std::make_unique<Any>()})) {*tokval_type(ptr) = std::make_unique<" << group
            << ">(std::move(in));}\n";
  }

  file_cc << '\n';

  for (auto& group : global_data::wsv_groups) {
    file_cc << "TokVal& TokVal::operator=(" << group
            << " in) {*tokval_type(ptr) = std::make_unique<" << group
            << ">(std::move(in)); return *this;}\n";
  }

  file_cc << '\n';

  for (auto& group : global_data::wsv_groups) {
    file_cc << "bool TokVal::holds" << group << "() const {return std::holds_alternative<std::unique_ptr<"<<group<<">>(*tokval_type(ptr));}\n";
  }

  file_cc << '\n';

  for (auto& group : global_data::wsv_groups) {
    file_cc << "TokVal::operator " << group << "() const {\n"
    "   ARTS_USER_ERROR_IF(not holds"<<group<<"(), \"Wrong type\")\n"
    "  return **std::get_if<std::unique_ptr<"<<group<<">>(tokval_type(ptr));\n}\n";
  }

  file_cc << R"--(
TokVal::TokVal(const char * const c) : TokVal(String(c)) {}
    
TokVal::TokVal() : TokVal(Any{}) {}
TokVal::TokVal(const TokVal& v) : TokVal(Any{}) { std::visit([&](auto&& in) {*this = *in;}, *tokval_type(v.ptr)); }
TokVal& TokVal::operator=(const TokVal& v) {std::visit([&](auto&& in) {*this = *in;}, *tokval_type(v.ptr)); return *this; }

TokVal::~TokVal() {delete tokval_type(ptr);}

std::shared_ptr<void> TokVal::copy_value() const {
  return std::visit(
    [](auto&& val) -> std::shared_ptr<void> {
      using value_type =
          std::remove_cv_t<std::remove_pointer_t<decltype(val.get())>>;
      return std::make_shared<value_type>(*val);
    },
    *tokval_type(ptr));
}
)--";

  return EXIT_SUCCESS;
}