#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include "workspace_groups.h"

const static auto data = internal_workspace_groups();

std::vector<std::pair<std::string, std::vector<std::string>>> files() {
  std::vector<std::pair<std::string, std::vector<std::string>>> files;

  for (const auto& [group_name, group] : data) {
    auto ptr = std::find_if(files.begin(),
                            files.end(),
                            [f = group.file](auto& p) { return f == p.first; });
    if (ptr == files.end()) {
      files.emplace_back(group.file, std::vector<std::string>{group_name});
    } else {
      ptr->second.push_back(group_name);
    }
  }

  std::sort(files.begin(), files.end(), [](auto& a, auto& b) {
    return a.first < b.first;
  });
  for (auto& [file, groups] : files) {
    std::sort(groups.begin(), groups.end());
  }

  return files;
}

std::vector<std::string> groups() {
  std::vector<std::string> groups;
  groups.reserve(data.size());

  for (const auto& [group_name, group] : data) {
    groups.push_back(group_name);
  }

  std::sort(groups.begin(), groups.end());

  return groups;
}

void header(std::ostream& os) {
  os << R"--(#pragma once

//! auto-generated by make_auto_wsg.cpp

#include <memory>
#include <sstream>
#include <variant>

)--";

  for (const auto& [file, groups] : files()) {
    os << "// ";
    for (const auto& group : groups) {
      os << group << ", ";
    }
    os << "\n#include <" << file << ">\n\n";
  }

  os << "template <typename T>\nconcept WorkspaceGroup = false";
  for (auto& group : groups()) {
    os << "\n  || std::is_same_v<T, " << group << ">";
  }
  os << "\n;\n\n";
  os << "template <typename T>\nconcept QualifiedWorkspaceGroup = WorkspaceGroup<std::remove_cvref_t<T>>;\n\n";

  os << R"(
template <typename T>
concept WorkspaceGroupIsDefaultConstructible = requires(T) {
  T{};
};

template <typename T>
concept WorkspaceGroupIsCopyable = requires(T a) {
  T{a};
};

template <typename T>
concept WorkspaceGroupIsPrintable  = requires(T a) {
  std::ostringstream{} << a;
};

)";

  os << '\n';
  for (auto& group : groups()) {
    os << "void xml_read_from_stream(std::istream &, " << group
       << " &, bifstream *);\n";
    os << "void xml_write_to_stream(std::ostream &, const " << group
       << " &, bofstream *, const String &);\n";
  }

  os << R"(
template <typename T> struct WorkspaceGroupInfo {
  static constexpr std::string_view name = "<Unknown>";
  static constexpr std::string_view file = "<Unknown>";
  static constexpr std::string_view desc = "<Unknown>";
  static constexpr bool value_type = false;
};

)";
  for (auto& group : groups()) {
    os << "template <> struct WorkspaceGroupInfo<" << group << "> {\n";
    os << "  static constexpr std::string_view name = \"" << group << "\";\n";
    os << "  static constexpr std::string_view file = \"" << data.at(group).file
       << "\";\n";
    os << "  static constexpr std::string_view desc = R\"--("
       << data.at(group).desc << ")--\";\n";
    os << "  static constexpr bool value_type = " << data.at(group).value_type
       << ";\n";
    os << "};\n\n";
  }

  os << "using WsvValue = std::variant<";
  bool first = true;
  for (auto& group : groups()) {
    if (not first) os << ',';
    first = false;
    os << "\n  std::shared_ptr<" << group << ">";
  }
  os << ">;\n\n";

  os << "[[nodiscard]] std::string_view name_wsg(const WsvValue&);\n";
  os << "[[nodiscard]] bool valid_wsg(const std::string_view&);\n\n";

  os << R"--(
struct Wsv {
  WsvValue value;

  //! Move value into the workspace variable
  template <WorkspaceGroup T> Wsv(T&& x);

  //! Copy value into the workspace variable
  template <WorkspaceGroup T> Wsv(const T& x);

  //! Borrow value as workspace variable
  template <WorkspaceGroup T> Wsv(T* x);

  //! Share value as workspace variable
  template <WorkspaceGroup T> Wsv(std::shared_ptr<T>&& x);

  //! Must declare destructor to avoid incomplete type error
  Wsv();
  Wsv(const Wsv&);
  Wsv(Wsv&&) noexcept;
  Wsv& operator=(const Wsv&);
  Wsv& operator=(Wsv&&) noexcept;
  ~Wsv();

  [[nodiscard]] Wsv copy() const;

  [[nodiscard]] const std::string_view type_name() const;

  [[nodiscard]] std::size_t index() const;

  [[nodiscard]] bool holds_same(const Wsv& other) const;

  template <WorkspaceGroup T>
  [[nodiscard]] bool holds() const;

  template <WorkspaceGroup T>
  [[nodiscard]] const std::shared_ptr<T>& share_unsafe() const;

  template <WorkspaceGroup T>
  [[nodiscard]] const std::shared_ptr<T>& share() const;

  template <WorkspaceGroup T>
  [[nodiscard]] T& get_unsafe() const;

  template <WorkspaceGroup T>
  [[nodiscard]] T& get() const;

  static Wsv from_named_type(const std::string& type);
};
)--";
}

void implementation(std::ostream& os) {
  os << R"--(//! auto-generated by make_auto_wsg.cpp

#include <typeinfo>

#include "auto_wsg.h"

#include "workspace_agenda_class.h"
#include "workspace_method_class.h"

Wsv::Wsv() : value(std::make_shared<Any>()) {}
Wsv::Wsv(const Wsv&) = default;
Wsv::Wsv(Wsv&&) noexcept = default;
Wsv& Wsv::operator=(const Wsv&) = default;
Wsv& Wsv::operator=(Wsv&&) noexcept = default;
Wsv::~Wsv() = default;

)--";

  for (auto&& group : groups()) {
    os << "template <> Wsv::Wsv(" << group << "&& x) : value(std::make_shared<"
       << group << ">(std::move(x))) {}\n";
    os << "template <> Wsv::Wsv(const " << group
       << "& x) : value(std::make_shared<" << group << ">(x)) {}\n";
    os << "template <> Wsv::Wsv(" << group << "* x) : value(std::shared_ptr<"
       << group << ">(x, [](void*){})) {}\n";
    os << "template <> Wsv::Wsv(std::shared_ptr<" << group
       << ">&& x) : value(std::move(x)) {}\n\n";
  }

  os << "std::string_view name_wsg(const WsvValue& x) {\n  const static std::unordered_map<std::size_t, std::string> val {\n";
  for (auto& group : groups()) {
    os << "    {Wsv(" << group << "{}).index(), \"" << group << "\"}, \n";
  }
  os << R"--(  };

  return val.at(x.index());
}

)--";

  os << "bool valid_wsg(const std::string_view& x) {\n  constexpr static std::array val {\n";
  for (auto& group : groups()) {
    os << "    \"" << group << "\", \n";
  }

  os << R"--(  };
  return std::binary_search(val.begin(), val.end(), x);
}

std::size_t Wsv::index() const {return value.index();}

bool Wsv::holds_same(const Wsv& other) const {return index() == other.index();}

Wsv Wsv::copy() const {
  return std::visit([](const auto& val) -> Wsv {
    return Wsv{*val};
  }, value);
}

const std::string_view Wsv::type_name() const { return name_wsg(value); }

Wsv Wsv::from_named_type(const std::string& type) {
)--";

  for (auto& group : groups()) {
    os << "  if (type == \"" << group << "\") return " << group << "{};\n";
  }

  os << R"--( throw std::runtime_error(var_string("Unknown workspace group \"", type, '"'));
}
)--";

  for (auto& group : groups()) {
    os << "template <> bool Wsv::holds<" << group
       << ">() const { return std::holds_alternative<std::shared_ptr<" << group
       << ">>(value); }\n\n";

    os << "template <> const std::shared_ptr<" << group
       << ">& Wsv::share_unsafe<" << group
       << ">() const { return *std::get_if<std::shared_ptr<" << group
       << ">>(&value); }\n\n";

    os << "template <> const std::shared_ptr<" << group << ">& Wsv::share<"
       << group
       << ">() const {\n"
          "  if (not holds<"
       << group
       << ">()) {\n"
          "    throw std::runtime_error(var_string(\"Cannot use workspace variable of workspace group \\\"\",\n"
          "                                        type_name(),\n"
          "                                        \"\\\" as \\\""
       << group
       << "\\\"\"));\n"
          "  }\n"
          "\n"
          "  return share_unsafe<"
       << group
       << ">();\n"
          "}\n\n";

    os << "template <> " << group << "& Wsv::get_unsafe<" << group
       << ">() const { return *share_unsafe<" << group << ">(); }\n\n";

    os << "template <> " << group << "& Wsv::get<" << group
       << ">() const { return *share<" << group << ">(); }\n\n";
  }

  // Static asserts
  for (auto& group : groups()) {
    os << "static_assert(WorkspaceGroupIsDefaultConstructible<" << group
       << ">, \"Must be default constructible\");\n";
    os << "static_assert(WorkspaceGroupIsCopyable<" << group
       << ">, \"Must be copyable\");\n";
    os << "static_assert(WorkspaceGroupIsPrintable<" << group
       << ">, \"Must be printable\");\n";
    os << "static_assert(arts_formattable_or_value_type<" << group
       << ">, \"Must be formattable according to ARTS rules\");\n";
  }
}

void auto_workspace(std::ostream& os) {
  os << R"--(//! auto-generated by make_auto_wsg.cpp

#include "workspace_agenda_class.h"
#include "workspace_method_class.h"

#include "workspace_class.h"

)--";

  for (auto& group : groups()) {
    os << "template<> std::shared_ptr<" << group
       << "> Workspace::share_or<" << group
       << ">(const std::string& name) {\n"
          "  if (auto ptr = wsv.find(name); ptr not_eq wsv.end()) {\n"
          "    return ptr->second.template share<"
       << group
       << ">();\n"
          "  }\n"
          "\n";
    os << "  Wsv out = " << group;
    if (group == "Agenda") {
      os << "{name}";
    } else {
      os << "{}";
    }
    os << ";\n"
          "\n"
          "  set(name, out);\n"
          "  return out.share_unsafe<"
       << group
       << ">();\n"
          "}\n\n";

    os << "template <> " << group << "& Workspace::get_or<" << group
       << ">(const std::string& name) {\n  return *share_or<" << group
       << ">(name);\n}\n\n";
    os << "template <> " << group << "& Workspace::get<" << group
       << ">(const std::string& name) const try {\n  return wsv.at(name)."
          "get<"
       << group
       << ">();\n} catch (std::out_of_range&) {\n"
          "  throw std::runtime_error(var_string(\"Undefined workspace variable \", '\"', name, '\"'));\n"
          "} catch (std::exception& e) {\n"
          "  throw std::runtime_error(var_string(\"Error getting workspace variable \", '\"', name, '\"', \":\\n\", e.what()));\n"
          "}\n\n";
  }
}

void static_assert_for_vector_and_map(std::ostream& os) {
  os << R"--(
template <typename T>
concept has_value_type = requires { typename T::value_type; };

template <typename T>
concept value_type_is_wsg = QualifiedWorkspaceGroup<typename T::value_type>;

template <typename T>
concept mapped_type_is_wsg = QualifiedWorkspaceGroup<typename T::mapped_type>;

template <typename T>
concept python_requires_map_and_vector_subtype_are_groups =
    value_type_is_wsg<T> or mapped_type_is_wsg<T> or not has_value_type<T> or
    std::same_as<T, String>;

template <typename T>
concept internally_consistent =
    python_requires_map_and_vector_subtype_are_groups<T>;

)--";

  for (auto&& group : groups()) {
    os << "static_assert(internally_consistent<" << group
       << ">,\n     "
          R"--(R"-x-("
This fails one of the following:
  1) It has either a value_type or a mapped_type templated type that is not a WSG (e.g., Array<NotWSG>)
")-x-")--"
       << ");\n";
  }
}

int main() try {
  std::ofstream head("auto_wsg.h");
  std::ofstream impl("auto_wsg.cpp");
  header(head);
  implementation(impl);

  std::ofstream ws_os("auto_workspace.cpp");
  auto_workspace(ws_os);
  static_assert_for_vector_and_map(ws_os);
} catch (std::exception& e) {
  std::cerr << "Cannot create the automatic groups with error:\n\n"
            << e.what() << '\n';
  return 1;
}
