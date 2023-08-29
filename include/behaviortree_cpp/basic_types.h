#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <typeinfo>
#include <functional>
#include <chrono>
#include <memory>
#include <string_view>
#include <variant>
#include <optional>

#include "behaviortree_cpp/utils/safe_any.hpp"
#include "behaviortree_cpp/exceptions.h"
#include "behaviortree_cpp/contrib/expected.hpp"

namespace BT
{
/// Enumerates the possible types of nodes
enum class NodeType
{
  UNDEFINED = 0,
  ACTION,
  CONDITION,
  CONTROL,
  DECORATOR,
  SUBTREE
};

/// Enumerates the states every node can be in after execution during a particular
/// time step.
/// IMPORTANT: Your custom nodes should NEVER return IDLE.
enum class NodeStatus
{
  IDLE = 0,
  RUNNING = 1,
  SUCCESS = 2,
  FAILURE = 3,
  SKIPPED = 4,
};

inline bool isStatusActive(const NodeStatus& status)
{
  return status != NodeStatus::IDLE && status != NodeStatus::SKIPPED;
}

inline bool isStatusCompleted(const NodeStatus& status)
{
  return status == NodeStatus::SUCCESS || status == NodeStatus::FAILURE;
}

enum class PortDirection
{
  INPUT,
  OUTPUT,
  INOUT
};

using StringView = std::string_view;

/**
 * convertFromString is used to convert a string into a custom type.
 *
 * This function is invoked under the hood by TreeNode::getInput(), but only when the
 * input port contains a string.
 *
 * If you have a custom type, you need to implement the corresponding template specialization.
 */
template <typename T> [[nodiscard]]
inline T convertFromString(StringView /*str*/)
{
  auto type_name = BT::demangle(typeid(T));

  std::cerr << "You (maybe indirectly) called BT::convertFromString() for type ["
            << type_name << "], but I can't find the template specialization.\n"
            << std::endl;

  throw LogicError(std::string("You didn't implement the template specialization of "
                               "convertFromString for this type: ") +
                   type_name);
}

template <> [[nodiscard]]
std::string convertFromString<std::string>(StringView str);

template <> [[nodiscard]]
const char* convertFromString<const char*>(StringView str);

template <> [[nodiscard]]
int convertFromString<int>(StringView str);

template <> [[nodiscard]]
unsigned convertFromString<unsigned>(StringView str);

template <> [[nodiscard]]
long convertFromString<long>(StringView str);

template <> [[nodiscard]]
unsigned long convertFromString<unsigned long>(StringView str);

template <> [[nodiscard]]
float convertFromString<float>(StringView str);

template <> [[nodiscard]]
double convertFromString<double>(StringView str);

// Integer numbers separated by the character ";"
template <> [[nodiscard]]
std::vector<int> convertFromString<std::vector<int>>(StringView str);

// Real numbers separated by the character ";"
template <> [[nodiscard]]
std::vector<double> convertFromString<std::vector<double>>(StringView str);

// This recognizes either 0/1, true/false, TRUE/FALSE
template <> [[nodiscard]]
bool convertFromString<bool>(StringView str);

// Names with all capital letters
template <> [[nodiscard]]
NodeStatus convertFromString<NodeStatus>(StringView str);

// Names with all capital letters
template <> [[nodiscard]]
NodeType convertFromString<NodeType>(StringView str);

template <> [[nodiscard]]
PortDirection convertFromString<PortDirection>(StringView str);

using StringConverter = std::function<Any(StringView)>;

using StringConvertersMap = std::unordered_map<const std::type_info*, StringConverter>;

// helper function
template <typename T> [[nodiscard]]
inline StringConverter GetAnyFromStringFunctor()
{
  if constexpr(std::is_constructible_v<StringView, T>)
  {
    return [](StringView str) { return Any(str); };
  }
  else {
    return [](StringView str) { return Any(convertFromString<T>(str)); };
  }
}

template <> [[nodiscard]]
inline StringConverter GetAnyFromStringFunctor<void>()
{
  return {};
}

//------------------------------------------------------------------

template<typename T> [[nodiscard]]
std::string toStr(const T& value)
{
  if constexpr(!std::is_arithmetic_v<T>)
  {
    throw LogicError(
        StrCat("Function BT::toStr<T>() not specialized for type [",
               BT::demangle(typeid(T)), "],",
               "Implement it consistently with BT::convertFromString<T>(), "
               "or provide at dummy version that returns an empty string.")
      );
  } else {
    return std::to_string(value);
  }
}

template <> [[nodiscard]]
std::string toStr<bool>(const bool& value);

template <> [[nodiscard]]
std::string toStr<std::string>(const std::string& value);

template <> [[nodiscard]]
std::string toStr<BT::NodeStatus>(const BT::NodeStatus& status);

/**
 * @brief toStr converts NodeStatus to string. Optionally colored.
 */
[[nodiscard]]
std::string toStr(BT::NodeStatus status, bool colored);

std::ostream& operator<<(std::ostream& os, const BT::NodeStatus& status);

template <> [[nodiscard]]
std::string toStr<BT::NodeType>(const BT::NodeType& type);

std::ostream& operator<<(std::ostream& os, const BT::NodeType& type);

template <> [[nodiscard]]
std::string toStr<BT::PortDirection>(const BT::PortDirection& direction);

std::ostream& operator<<(std::ostream& os, const BT::PortDirection& type);

// Small utility, unless you want to use <boost/algorithm/string.hpp>
[[nodiscard]]
std::vector<StringView> splitString(const StringView& strToSplit, char delimeter);

template <typename Predicate>
using enable_if = typename std::enable_if<Predicate::value>::type*;

template <typename Predicate>
using enable_if_not = typename std::enable_if<!Predicate::value>::type*;

/** Usage: given a function/method like this:
 *
 *     Expected<double> getAnswer();
 *
 * User code can check result and error message like this:
 *
 *     auto res = getAnswer();
 *     if( res )
 *     {
 *         std::cout << "answer was: " << res.value() << std::endl;
 *     }
 *     else{
 *         std::cerr << "failed to get the answer: " << res.error() << std::endl;
 *     }
 *
 * */
template <typename T>
using Expected = nonstd::expected<T, std::string>;

#ifdef USE_BTCPP3_OLD_NAMES
// note: we also use the name Optional instead of expected because it is more intuitive
// for users that are not up to date with "modern" C++
template <typename T>
using Optional = nonstd::expected<T, std::string>;
#endif

/** Usage: given a function/method like:
 *
 *     Result DoSomething();
 *
 * User code can check result and error message like this:
 *
 *     auto res = DoSomething();
 *     if( res )
 *     {
 *         std::cout << "DoSomething() done " << std::endl;
 *     }
 *     else{
 *         std::cerr << "DoSomething() failed with message: " << res.error() << std::endl;
 *     }
 *
 * */
using Result = Expected<std::monostate>;

[[nodiscard]]
bool IsAllowedPortName(StringView str);

class PortInfo
{
public:
  struct AnyTypeAllowed
  {
  };

  PortInfo(PortDirection direction = PortDirection::INOUT) :
    type_(direction), type_info_(typeid(AnyTypeAllowed)),
    type_str_("AnyTypeAllowed")
  {}

  PortInfo(PortDirection direction, std::type_index type_info, StringConverter conv) :
    type_(direction), type_info_(type_info), converter_(conv), 
    type_str_(BT::demangle(type_info))
  {}

  [[nodiscard]] PortDirection direction() const;

  [[nodiscard]] const std::type_index& type() const;

  [[nodiscard]] const std::string& typeName() const;

  [[nodiscard]] Any parseString(const char* str) const;

  [[nodiscard]] Any parseString(const std::string& str) const;

  template <typename T> [[nodiscard]]
  Any parseString(const T&) const
  {
    // avoid compilation errors
    return {};
  }

  void setDescription(StringView description);

  template <typename T>
  void setDefaultValue(const T& default_value) {
    default_value_ = Any(default_value);
    try{
      default_value_str_ = BT::toStr(default_value);
    }
    catch(LogicError&) {}
  }

  [[nodiscard]] const std::string& description() const;

  [[nodiscard]] const Any& defaultValue() const;

  [[nodiscard]] const std::string& defaultValueString() const;

  [[nodiscard]] bool isStronglyTyped() const
  {
    return type_info_ != typeid(AnyTypeAllowed);
  }

  [[nodiscard]] const StringConverter& converter() const
  {
    return converter_;
  }

private:
  PortDirection type_;
  std::type_index type_info_;
  StringConverter converter_;
  std::string description_;
  Any default_value_;
  std::string default_value_str_;
  std::string type_str_;
};

template <typename T = PortInfo::AnyTypeAllowed> [[nodiscard]]
std::pair<std::string, PortInfo> CreatePort(PortDirection direction,
                                            StringView name,
                                            StringView description = {})
{
  auto sname = static_cast<std::string>(name);
  if (!IsAllowedPortName(sname))
  {
    throw RuntimeError("The name of a port must not be `name` or `ID` "
                       "and must start with an alphabetic character. "
                       "Underscore is reserved.");
  }

  std::pair<std::string, PortInfo> out;

  if (std::is_same<T, void>::value)
  {
    out = {sname, PortInfo(direction)};
  }
  else
  {
    out = {sname, PortInfo(direction, typeid(T), GetAnyFromStringFunctor<T>())};
  }
  if (!description.empty())
  {
    out.second.setDescription(description);
  }
  return out;
}

//----------
template <typename T = void> [[nodiscard]]
inline std::pair<std::string, PortInfo> InputPort(StringView name,
                                                  StringView description = {})
{
  return CreatePort<T>(PortDirection::INPUT, name, description);
}

template <typename T = void> [[nodiscard]]
inline std::pair<std::string, PortInfo> OutputPort(StringView name,
                                                   StringView description = {})
{
  return CreatePort<T>(PortDirection::OUTPUT, name, description);
}

template <typename T = void> [[nodiscard]]
inline std::pair<std::string, PortInfo> BidirectionalPort(StringView name,
                                                          StringView description = {})
{
  return CreatePort<T>(PortDirection::INOUT, name, description);
}
//----------
template <typename T = void> [[nodiscard]]
inline std::pair<std::string, PortInfo> InputPort(StringView name, const T& default_value,
                                                  StringView description)
{
  auto out = CreatePort<T>(PortDirection::INPUT, name, description);
  out.second.setDefaultValue(default_value);
  return out;
}

template <typename T = void> [[nodiscard]]
inline std::pair<std::string, PortInfo> BidirectionalPort(StringView name,
                                                          const T& default_value,
                                                          StringView description)
{
  auto out = CreatePort<T>(PortDirection::INOUT, name, description);
  out.second.setDefaultValue(default_value);
  return out;
}
//----------

using PortsList = std::unordered_map<std::string, PortInfo>;

template <typename T, typename = void>
struct has_static_method_providedPorts : std::false_type
{
};

template <typename T>
struct has_static_method_providedPorts<
    T, typename std::enable_if<
           std::is_same<decltype(T::providedPorts()), PortsList>::value>::type>
  : std::true_type
{
};

template <typename T, typename = void>
struct has_static_method_description : std::false_type
{
};

template <typename T>
struct has_static_method_description<
    T, typename std::enable_if<
           std::is_same<decltype(T::description()), std::string>::value>::type>
  : std::true_type
{
};

template <typename T> [[nodiscard]]
inline PortsList getProvidedPorts(enable_if<has_static_method_providedPorts<T>> = nullptr)
{
  return T::providedPorts();
}

template <typename T> [[nodiscard]]
inline PortsList
    getProvidedPorts(enable_if_not<has_static_method_providedPorts<T>> = nullptr)
{
  return {};
}

using TimePoint = std::chrono::high_resolution_clock::time_point;
using Duration = std::chrono::high_resolution_clock::duration;

}   // namespace BT

