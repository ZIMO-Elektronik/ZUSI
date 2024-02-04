/// Mbps
///
/// \file   zusi/mbps.hpp
/// \author Vincent Hamp
/// \date   20/06/2023

#pragma once

#include <cstdint>

namespace zusi {

enum class Mbps : uint8_t {
  _0_1,    // 10µs ~ 0.1Mbps
  _0_286,  // 3.5µs ~ 0.286Mbps
  _1_364,  // 0.733µs ~ 1.364Mbps
  _1_807,  // 0.5533µs ~ 1.807Mbps
};

}  // namespace zusi
