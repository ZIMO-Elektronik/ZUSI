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
#include "command.hpp"

namespace zusi {

// Byte positions in ZUSI frame
inline constexpr size_t cmd_pos{0uz};
inline constexpr size_t data_cnt_pos{1uz};
inline constexpr size_t addr_pos{2uz};
inline constexpr size_t data_pos{6uz};
inline constexpr size_t sec_bytes_pos{1uz};
inline constexpr size_t exit_flags_pos{3uz};

/// Resync byte
inline constexpr uint8_t resync_byte{0x80u};

/// Resync timeout in ms
inline constexpr auto resync_timeout_ms{10u};

/// Resync timeout in us
inline constexpr auto resync_timeout_us{resync_timeout_ms * 1000u};

/// Check if byte is entry byte
///
/// \param  byte  Byte to check
/// \retval true  Byte is entry byte
/// \retval false Byte is not entry byte
constexpr bool is_entry_byte(uint8_t byte) {
  return byte == 0x55u || byte == 0xAAu;
}

/// Check if command is valid
///
/// \param  cmd   Command
/// \retval true  Command is valid
/// \retval false Command is not valid
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

/// uint32_t to data
///
/// \param  word  Word to convert
/// \param  data  Pointer to write to
/// \return Pointer after last element
constexpr auto uint32_2data(uint32_t word, uint8_t* data) {
  *data++ = static_cast<uint8_t>((word & 0xFF00'0000u) >> 24u);
  *data++ = static_cast<uint8_t>((word & 0x00FF'0000u) >> 16u);
  *data++ = static_cast<uint8_t>((word & 0x0000'FF00u) >> 8u);
  *data++ = static_cast<uint8_t>((word & 0x0000'00FFu) >> 0u);
  return data;
}

} // namespace zusi
