/// Feature bytes
///
/// \file   zusi/features.hpp
/// \author Vincent Hamp
/// \date   21/03/2023

#pragma once

#include <array>
#include <cstdint>

namespace zusi {

/// Feature bytes used by Command::Features
using Features = std::array<uint8_t, 4uz>;

}  // namespace zusi
