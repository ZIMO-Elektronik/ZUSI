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

/// ZUSI connection sequence
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

/// Dispatcher for all supported ZUSI frame types
///
/// \param frame ZUSI frame to compute
///
/// \retval `true, std::nullopt´                      Successful transmission,
///                                                   no data returned
/// \retval `true, ztl::inplace_vector<uint8_t, 4>´   Successful transmission,
///                                                   data returned
/// \retval `false, <IGNORE>´                         Transmission error
std::pair<bool, std::optional<ztl::inplace_vector<uint8_t, 4>>>
Base::execute(std::span<uint8_t const> frame) {
  std::pair<bool, std::optional<ztl::inplace_vector<uint8_t, 4>>> ret{
    false, std::nullopt};

  switch (std::bit_cast<Command>(frame.front())) {
    case Command::CvRead: {
      auto val{readCv(data2uint32(&frame[addr_pos]))};
      if (val) ret = std::make_pair(val.has_value(), *val);
      else ret = std::make_pair(val.has_value(), std::nullopt);
      break;
    }
    case Command::CvWrite:
      ret = std::make_pair(
        writeCv(data2uint32(&frame[addr_pos]), frame[data_pos]), std::nullopt);
      break;
    case Command::ZppErase:
      ret = std::make_pair(eraseZpp(), std::nullopt);
      break;
    case Command::ZppWrite:
      ret = std::make_pair(
        writeZpp(data2uint32(&frame[addr_pos]),
                 frame.subspan(data_pos, frame[data_cnt_pos] + 1)),
        std::nullopt);
      break;
    case Command::Features: {
      auto ans{features()};
      if (ans) {
        ztl::inplace_vector<uint8_t, 4> vec{};
        std::copy(&ans.value()[0], &ans.value()[3], std::back_inserter(vec));
        ret = std::make_pair(true, vec);
      } else ret = std::make_pair(false, std::nullopt);
      break;
    }
    case Command::Exit:
      ret = std::make_pair(exit(frame[exit_flags_pos]), std::nullopt);
      break;
    case Command::None: [[fallthrough]];
    case Command::Encrypt: [[fallthrough]];
    default: break;
  }

  return ret;
}

/// Transmit readCv command
///
/// \param addr CV adress
///
/// \retval CV adress     if successful
/// \retval std::nullopt  Transmission error
std::optional<uint8_t> Base::readCv(uint32_t addr) const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 7uz> buf;
  std::optional<uint8_t> ret = std::nullopt;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::CvRead); // Command
  *it++ = 0u;                                  // Count
  it = uint32_2data(addr, it);                 // Address
  *it = crc8({cbegin(buf), size(buf) - 1uz});  // CRC8
  transmitBytes(buf, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  ret = receiveByte();
  if (!(crc8(ret.value()) == receiveByte())) ret = std::nullopt;
  return ret;
}

/// Transmit writeCv command
///
/// \param addr   CV adress
/// \param value  new CV value
///
/// \retval true  if successful
/// \retval false Transmission error
bool Base::writeCv(uint32_t addr, uint8_t value) const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 8uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::CvWrite); // Command
  *it++ = 0u;                                   // Count
  it = uint32_2data(addr, it);                  // Address
  *it++ = value;                                // Value
  *it = crc8({cbegin(buf), size(buf) - 1uz});   // CRC8
  transmitBytes(buf, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

/// Transmit eraseZpp command
///
/// \retval true  if successful
/// \retval false Transmission error
bool Base::eraseZpp() const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 4uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::ZppErase); // Command
  *it++ = 0x55u;                                 // Security byte
  *it++ = 0xAAu;                                 // Security byte
  *it = crc8({cbegin(buf), size(buf) - 1uz});    // CRC8
  transmitBytes(buf, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

/// Transmit writeZpp command
///
/// \param addr   Flash adress
/// \param bytes  Raw flash data
///
/// \retval true if successful
/// \retval`false Transmission error
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
  transmitBytes({cbegin(buf), 6uz + size(bytes) + 1uz}, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

/// Transmit feature request
///
/// \retval Features      if successful
/// \retval std::nullopt  Transmission error
std::optional<Features> Base::features() {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 2uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::Features); // Command
  *it = crc8({cbegin(buf), size(buf) - 1uz});    // CRC8
  transmitBytes(buf, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  Features const features{
    receiveByte(), receiveByte(), receiveByte(), receiveByte()};
  if (!(features[0uz] & 0b100u)) mbps_ = Mbps::_1_807;
  else if (!(features[0uz] & 0b010u)) mbps_ = Mbps::_1_364;
  else if (!(features[0uz] & 0b001u)) mbps_ = Mbps::_0_286;
  return features;
}

/// Transmit exit command
///
/// \param flags Exit flags
///
/// \retval true if successful
/// \retval false Transmission error
bool Base::exit(uint8_t flags) const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 5uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::Exit);  // Command
  *it++ = 0x55u;                              // Security byte
  *it++ = 0xAAu;                              // Security byte
  *it++ = flags;                              // Flags
  *it = crc8({cbegin(buf), size(buf) - 1uz}); // CRC8
  transmitBytes(buf, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

/// Resync phase sequence
void Base::resync() const {
  delayUs(10u);
  transmitBytes({&resync_byte, 1uz}, Mbps::_0_1);
}

/// Checks if any decoder is still connected
///
/// \retval false connection valid
/// \retval true  connection lost
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
  while (!readData()); // TODO timeout?
}

/// Receive data phase sequence
///
/// \retval DATA decoder data received
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
