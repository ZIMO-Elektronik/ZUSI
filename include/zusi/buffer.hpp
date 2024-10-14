// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Buffer
///
/// \file   zusi/buffer.hpp
/// \author Vincent Hamp
/// \date   29/03/2023

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace zusi {

template<size_t I = 1uz +    // Command
                    1uz +    // Length
                    4uz +    // Address
                    256uz +  // Data
                    1uz +    // CRC
                    1uz>
using Buffer = std::array<uint8_t, I>;  // Resync

}  // namespace zusi
