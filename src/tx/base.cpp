// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Transmit base
///
/// \file   tx/base.cpp
/// \author Vincent Hamp
/// \date   21/03/2023

#include <cassert>
#include <climits>
#include <gsl/util>
#include "utility.hpp"
#include "zusi.hpp"

namespace zusi::tx {

/// Transmit entry sequence
void Base::enter() const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  gpioOutput();
  for (auto i{0uz}; i < 1'000'000uz / 10'000uz; ++i) {
    writeClock(true);
    writeData(i % 2uz);
    delayUs(5000u);
    writeClock(false);
    delayUs(5000u);
  }
  delayUs(resync_timeout_us);
}

/// Transmit bytes
///
/// \param  packet        Packet
/// \retval Response      Returned data (can be empty)
/// \retval std::nullopt  Error
std::optional<Response> Base::transmit(Packet const& packet) {
  return transmit({cbegin(packet), size(packet)});
}

/// Transmit bytes
///
/// \param  bytes         Bytes containing packet
/// \retval Response      Returned data (can be empty)
/// \retval std::nullopt  Error
std::optional<Response> Base::transmit(std::span<uint8_t const> bytes) {
  switch (std::bit_cast<Command>(bytes.front())) {
    case Command::CvRead:
      if (auto const cv{readCv(data2uint32(&bytes[addr_pos]))})
        return ztl::inplace_vector<uint8_t, 4uz>{*cv};
      break;
    case Command::CvWrite:
      if (writeCv(data2uint32(&bytes[addr_pos]), bytes[data_pos]))
        return ztl::inplace_vector<uint8_t, 4uz>{};
      break;
    case Command::ZppErase:
      if (eraseZpp()) return ztl::inplace_vector<uint8_t, 4uz>{};
      break;
    case Command::ZppWrite:
      if (writeZpp(data2uint32(&bytes[addr_pos]),
                   bytes.subspan(data_pos, bytes[data_cnt_pos] + 1uz)))
        return ztl::inplace_vector<uint8_t, 4uz>{};
      break;
    case Command::Features:
      if (auto const feats{features()})
        return ztl::inplace_vector<uint8_t, 4uz>{
          (*feats)[0uz], (*feats)[1uz], (*feats)[2uz], (*feats)[3uz]};
      break;
    case Command::Exit:
      if (exit(bytes[exit_flags_pos]))
        return ztl::inplace_vector<uint8_t, 4uz>{};
      break;
    case Command::ZppLcDcQuery: {
      if (auto const valid{lcDcQuery(bytes.subspan<1uz, 4uz>())})
        return ztl::inplace_vector<uint8_t, 4uz>{*valid};
      break;
    }
    default: break;
  }
  return std::nullopt;
}

/// Read CV
///
/// \param  addr          CV address
/// \retval std::nullopt  Error
std::optional<uint8_t> Base::readCv(uint32_t addr) const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 7uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::CvRead); // Command
  *it++ = 0u;                                  // Count
  it = uint32_2data(addr, it);                 // Address
  *it = crc8({cbegin(buf), size(buf) - 1uz});  // CRC8
  transmitBytes(buf, _mbps);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  auto const cv{receiveByte()};
  if (crc8(cv) == receiveByte()) return cv;
  return std::nullopt;
}

/// Write CV
///
/// \param  addr  CV address
/// \param  byte  CV value
/// \retval true  Success
/// \retval false Error
bool Base::writeCv(uint32_t addr, uint8_t byte) const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 8uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::CvWrite); // Command
  *it++ = 0u;                                   // Count
  it = uint32_2data(addr, it);                  // Address
  *it++ = byte;                                 // Value
  *it = crc8({cbegin(buf), size(buf) - 1uz});   // CRC8
  transmitBytes(buf, _mbps);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

/// Erase ZPP
///
/// \retval true  Success
/// \retval false Error
bool Base::eraseZpp() const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 4uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::ZppErase); // Command
  *it++ = 0x55u;                                 // Security byte
  *it++ = 0xAAu;                                 // Security byte
  *it = crc8({cbegin(buf), size(buf) - 1uz});    // CRC8
  transmitBytes(buf, _mbps);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

/// Write ZPP
///
/// \param  addr  Address
/// \param  bytes Bytes
/// \retval true  Success
/// \retval false Error
bool Base::writeZpp(uint32_t addr, std::span<uint8_t const> bytes) const {
  assert(size(bytes) <= 256uz);
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, ZUSI_MAX_PACKET_SIZE> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::ZppWrite);    // Command
  *it++ = static_cast<uint8_t>(size(bytes) - 1uz);  // Count
  it = uint32_2data(addr, it);                      // Address
  it = std::copy_n(cbegin(bytes), size(bytes), it); // Data
  *it = crc8({begin(buf), 6uz + size(bytes)});      // CRC8
  transmitBytes({cbegin(buf), 6uz + size(bytes) + 1uz}, _mbps);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

/// Features query
///
/// \retval Features      Feature bytes
/// \retval std::nullopt  Error
std::optional<Features> Base::features() {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 2uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::Features); // Command
  *it = crc8({cbegin(buf), size(buf) - 1uz});    // CRC8
  transmitBytes(buf, Mbps::_0_286);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  Features const features{
    receiveByte(), receiveByte(), receiveByte(), receiveByte()};
  if (!(features[0uz] & 0b100u)) _mbps = Mbps::_1_807;
  else if (!(features[0uz] & 0b010u)) _mbps = Mbps::_1_364;
  else if (!(features[0uz] & 0b001u)) _mbps = Mbps::_0_286;
  return features;
}

/// Exit
///
/// \param  flags Flags
/// \retval true  Success
/// \retval false Error
bool Base::exit(uint8_t flags) const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 5uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::Exit);  // Command
  *it++ = 0x55u;                              // Security byte
  *it++ = 0xAAu;                              // Security byte
  *it++ = flags;                              // Flags
  *it = crc8({cbegin(buf), size(buf) - 1uz}); // CRC8
  transmitBytes(buf, _mbps);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

/// LC-DC query
///
/// \param  developer_code  Developer code
/// \retval bool            Load code valid
/// \retval std::nullopt    Error
std::optional<bool>
Base::lcDcQuery(std::span<uint8_t const, 4uz> developer_code) const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 6uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::ZppLcDcQuery);                  // Command
  it = std::copy_n(cbegin(developer_code), size(developer_code), it); // Data
  *it = crc8({cbegin(buf), size(buf) - 1uz});                         // CRC8
  transmitBytes(buf, _mbps);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  auto const valid{receiveByte()};
  if (crc8(valid) == receiveByte()) return static_cast<bool>(valid);
  return std::nullopt;
}

/// Transmit resync byte
void Base::resync() const {
  delayUs(10u);
  transmitBytes({&resync_byte, 1uz}, Mbps::_0_1);
}

/// Checks if any decoder is still connected
///
/// \retval false Connection valid
/// \retval true  Connection lost
bool Base::ackValid() const { return !ack(); }

/// Read ack
///
/// \retval true  Ack high
/// \retval false Ack low
bool Base::ack() const {
  writeClock(true);
  delayUs(10u);
  auto const retval{readData()};
  writeClock(false);
  delayUs(20u);
  return retval;
}

/// Busy phase sequence
void Base::busy() const {
  writeClock(true);
  delayUs(10u);
  writeClock(false);
  delayUs(20u);
  while (!readData()); /// \todo timeout?
}

/// Receive byte
///
/// \return Received byte
uint8_t Base::receiveByte() const {
  uint8_t byte{};
  for (auto i{0uz}; i < CHAR_BIT; ++i) {
    writeClock(true);
    delayUs(10u);
    byte = static_cast<uint8_t>(byte | readData() << i);
    writeClock(false);
    delayUs(20u);
  }
  return byte;
}

} // namespace zusi::tx
