#pragma once

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>


namespace btc_arb {
namespace utils {

// Code below is a modified version of
// codereview.stackexchange.com/questions/14309/conversion-between-enum-and-string-in-c-class-header

// This is the type that will hold all the strings.
// Each enumerate type will declare its own specialization.
// Any enum that does not have a specialization will generate a compiler error
// indicating that there is no definition of this variable (as there should be
// be no definition of a generic version).
template<typename T>
struct EnumStrings {
  static char const* names[];
};

// This is a utility type.
// Creted automatically. Should not be used directly.
template<typename T>
struct EnumRefHolder {
  T& enumVal;
  EnumRefHolder(T& enumVal): enumVal(enumVal) {}
};
template<typename T>
struct EnumConstRefHolder {
  T const& enumVal;
  EnumConstRefHolder(T const& enumVal): enumVal(enumVal) {}
};

// The next too functions do the actual work of reading/writtin an
// enum as a string.
template<typename T>
std::ostream& operator<<(std::ostream& str, EnumConstRefHolder<T> const& data) {
  return str << EnumStrings<T>::names[static_cast<int>(data.enumVal)];
}

template<typename T>
std::istream& operator>>(std::istream& str, EnumRefHolder<T> const& data) {
  static auto begin = std::begin(EnumStrings<T>::names);
  static auto end = std::end(EnumStrings<T>::names);

  std::string value;
  str >> value;
  auto find = std::find(begin, end, value);
  if (find != end)
  {
    data.enumVal = static_cast<T>(std::distance(begin, find));
    return str;
  } else {
    throw std::runtime_error("unknown endpoint type '" + value  + "'");
  }
}

// This is the public interface:
// use the ability of function to deuce their template type without
// being explicitly told to create the correct type of enumRefHolder<T>
template<typename T>
EnumConstRefHolder<T>  enum_to_str(T const& e) {return EnumConstRefHolder<T>(e);}

template<typename T>
EnumRefHolder<T>       enum_from_str(T& e)     {return EnumRefHolder<T>(e);}

}}  // namespace btc_arb::utils
