// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Utilities
///
/// \file   zusi/utility.hpp
/// \author Vincent Hamp
/// \date   21/03/2023

#pragma once

#include <algorithm>
#include <cstdint>
#include <utility>

namespace zusi {

/// Resync byte
inline constexpr uint8_t resync_byte{0x80u};

/// Resync timeout in ms
inline constexpr auto resync_timeout_ms{20u};

/// Resync timeout in us
inline constexpr auto resync_timeout_us{20'000u};

/// Check if byte is entry byte
///
/// \param  byte  Byte to check
/// \return true  Byte is entry byte
/// \return false Byte is not entry byte
constexpr bool is_entry_byte(uint8_t byte) {
  return byte == 0x55u || byte == 0xAAu;
}

/// Check if command is valid
///
/// \param  cmd   Command
/// \return true  Command is valid
/// \return false Command is not valid
constexpr bool is_valid_command(uint8_t cmd) {
  return cmd == std::clamp(cmd,
                           std::to_underlying(Command::CvRead),
                           std::to_underlying(Command::Encrypt));
}

/// Data to uint32_t
///
/// \param  data  Pointer to data
/// \return uint32_t from data
constexpr auto data2uint32(uint8_t const* data) {
  return static_cast<uint32_t>(data[0uz] << 24u | data[1uz] << 16u |
                               data[2uz] << 8u | data[3uz] << 0u);
}

}  // namespace zusi
