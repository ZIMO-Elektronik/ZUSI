// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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

} // namespace zusi
