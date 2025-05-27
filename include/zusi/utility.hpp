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
#include <concepts>
#include <cstdint>
#include <iterator>
#include <utility>
#include "command.hpp"
#include "crc8.hpp"
#include "packet.hpp"

namespace zusi {

// Byte positions in frame
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
                           std::to_underlying(Command::ZppLcDcQuery));
}

/// Data to uint32_t
///
/// \tparam RandomIt  std::random_access_iterator
/// \param  first     Beginning of the source range
/// \return uint32_t
template<std::random_access_iterator RandomIt>
requires(sizeof(std::iter_value_t<RandomIt>) == 1uz)
constexpr auto data2uint32(RandomIt first) {
  return static_cast<uint32_t>(first[0uz] << 24u | first[1uz] << 16u |
                               first[2uz] << 8u | first[3uz] << 0u);
}

/// uint32_t to data
///
/// \tparam OutputIt  std::output_iterator
/// \param  word      Word to convert
/// \param  out       Beginning of the destination range
/// \return Output iterator one past the last element copied
template<std::output_iterator<uint8_t> OutputIt>
inline constexpr auto uint32_2data(uint32_t word, OutputIt out) {
  *out++ = static_cast<uint8_t>((word & 0xFF00'0000u) >> 24u);
  *out++ = static_cast<uint8_t>((word & 0x00FF'0000u) >> 16u);
  *out++ = static_cast<uint8_t>((word & 0x0000'FF00u) >> 8u);
  *out++ = static_cast<uint8_t>((word & 0x0000'00FFu) >> 0u);
  return out;
}

/// Make CV-Read packet
///
/// \param  count    CV count - 1
/// \param  address  First CV address
/// \return Packet
inline constexpr Packet make_cv_read_packet(uint8_t count, uint32_t address) {
  Packet packet{};
  auto it{std::back_inserter(packet)};
  *it++ = std::to_underlying(Command::CvRead); // Command
  *it++ = count;                               // Count
  uint32_2data(address, it);                   // Address
  *it++ = crc8(packet);                        // CRC8
  return packet;
}

/// Make CV-Write packet
///
/// \param  count    CV count - 1
/// \param  address  First CV address
/// \param  values   CV values
/// \return Packet
inline constexpr Packet make_cv_write_packet(uint8_t count,
                                             uint32_t address,
                                             std::span<uint8_t const> values) {
  // Count must match value list
  assert((count - 1uz) == size(values));

  Packet packet{};
  auto it{std::back_inserter(packet)};
  *it++ = std::to_underlying(Command::CvWrite); // Command
  *it++ = count;                                // Count
  uint32_2data(address, it);                    // Address
  std::ranges::copy(values, it);                // Values
  *it++ = crc8(packet);                         // CRC8
  return packet;
}

/// Make ZPP-Erase packet
///
/// \return Packet
inline constexpr Packet make_zpp_erase_packet() {
  Packet packet{};
  auto it{std::back_inserter(packet)};
  *it++ = std::to_underlying(Command::ZppErase); // Command
  *it++ = 0x55u;                                 // Security byte
  *it++ = 0xAAu;                                 // Security byte
  *it++ = crc8(packet);                          // CRC8
  return packet;
}

/// Make ZPP-Write packet
///
/// \param  size    Chunk size - 1
/// \param  address Chunk address
/// \param  bytes   Chunk
/// \return Packet
inline constexpr Packet make_zpp_write_packet(uint8_t size,
                                              uint32_t address,
                                              std::span<uint8_t const> bytes) {
  Packet packet{};
  auto it{std::back_inserter(packet)};
  *it++ = std::to_underlying(Command::ZppWrite); // Command
  *it++ = size;                                  // Size
  uint32_2data(address, it);                     // Address
  std::ranges::copy(bytes, it);                  // Flash data
  *it++ = crc8(packet);                          // CRC8
  return packet;
}

/// Make Features packet
///
/// \return Packet
inline constexpr Packet make_features_packet() {
  Packet packet{};
  auto it{std::back_inserter(packet)};
  *it++ = std::to_underlying(Command::Features); // Command
  *it++ = crc8(packet);                          // CRC8
  return packet;
}

/// Make Exit packet
///
/// \param  option  Exit option
/// \return Packet
inline constexpr Packet make_exit_packet(uint8_t option) {
  Packet packet{};
  auto it{std::back_inserter(packet)};
  *it++ = std::to_underlying(Command::Exit); // Command
  *it++ = 0x55u;                             // Security byte
  *it++ = 0xAAu;                             // Security byte
  *it++ = option;                            // Option
  *it++ = crc8(packet);                      // CRC8
  return packet;
}

/// Make ZPP LC DC Query packet
///
/// \param  developer_code  Developer code
/// \return Packet
inline constexpr Packet make_zpp_lc_dc_query_packet(uint32_t developer_code) {
  Packet packet{};
  auto it{std::back_inserter(packet)};
  *it++ = std::to_underlying(Command::ZppLcDcQuery); // Command
  uint32_2data(developer_code, it);                  // Developer Code
  *it++ = crc8(packet);                              // CRC8
  return packet;
}

} // namespace zusi
