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
