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
#include <ulf/susiv2.hpp>
#include "buffer.hpp"
#include "zusi.hpp"

namespace zusi::tx {

namespace {

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

}  // namespace

///
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

///
std::pair<bool, std::optional<ztl::inplace_vector<uint8_t, 4>>>
Base::execute(std::span<uint8_t const> data) {
  Command cmd = std::bit_cast<Command>(data.front());
  std::pair<bool, std::optional<ztl::inplace_vector<uint8_t, 4>>> ret{
    false, std::nullopt};

  switch (cmd) {
    case Command::CvRead: {
      auto val = this->readCv(**ulf::susiv2::get_adress(data));
      if (val) ret = std::make_pair(val.has_value(), val.value());
      else ret = std::make_pair(val.has_value(), std::nullopt);
      break;
    }
    case Command::CvWrite: {
      ret =
        std::make_pair(this->writeCv(**ulf::susiv2::get_adress(data),
                                     (**ulf::susiv2::get_values(data)).front()),
                       std::nullopt);
      break;
    }
    case Command::ZppErase: {
      ret = std::make_pair(this->eraseZpp(), std::nullopt);
      break;
    }
    case Command::ZppWrite: {
      ret = std::make_pair(this->writeZpp(**ulf::susiv2::get_adress(data),
                                          **ulf::susiv2::get_data(data)),
                           std::nullopt);
      break;
    }
    case Command::Features: {
      ret = std::make_pair(this->features().has_value(), std::nullopt);
      break;
    }
    case Command::Exit: {
      ret = std::make_pair(this->exit(**ulf::susiv2::get_exit_flags(data)),
                           std::nullopt);
      break;
    }
    case Command::None: [[fallthough]];
    case Command::Encrypt: [[fallthrough]];
    default: break;
  }
  return ret;
}

///
std::optional<uint8_t> Base::readCv(uint32_t addr) const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  Buffer<7uz> buf;
  std::optional<uint8_t> ret = std::nullopt;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::CvRead);  // Command
  *it++ = 0u;                                   // Count
  it = uint32_2data(addr, it);                  // Address
  *it = crc8({cbegin(buf), size(buf) - 1uz});   // CRC8
  transmitBytes(buf, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  ret = receiveByte();
  if (!(crc8(ret.value()) == receiveByte())) ret = std::nullopt;
  return ret;
}

///
bool Base::writeCv(uint32_t addr, uint8_t value) const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  Buffer<8uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::CvWrite);  // Command
  *it++ = 0u;                                    // Count
  it = uint32_2data(addr, it);                   // Address
  *it++ = value;                                 // Value
  *it = crc8({cbegin(buf), size(buf) - 1uz});    // CRC8
  transmitBytes(buf, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

///
bool Base::eraseZpp() const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  Buffer<4uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::ZppErase);  // Command
  *it++ = 0x55u;                                  // Security byte
  *it++ = 0xAAu;                                  // Security byte
  *it = crc8({cbegin(buf), size(buf) - 1uz});     // CRC8
  transmitBytes(buf, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

///
bool Base::writeZpp(uint32_t addr, std::span<uint8_t const> bytes) const {
  assert(size(bytes) <= 256uz);
  gsl::final_action spi_master{[this] { spiMaster(); }};
  Buffer buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::ZppWrite);     // Command
  *it++ = size(bytes) - 1uz;                         // Count
  it = uint32_2data(addr, it);                       // Address
  it = std::copy_n(cbegin(bytes), size(bytes), it);  // Data
  *it = crc8({begin(buf), 6uz + size(bytes)});       // CRC8
  transmitBytes({cbegin(buf), 6uz + size(bytes) + 1uz}, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

///
std::optional<Features> Base::features() {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  Buffer<2uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::Features);  // Command
  *it = crc8({cbegin(buf), size(buf) - 1uz});     // CRC8
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

///
bool Base::exit(uint8_t flags) const {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  Buffer<5uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::Exit);   // Command
  *it++ = 0x55u;                               // Security byte
  *it++ = 0xAAu;                               // Security byte
  *it++ = flags;                               // Flags
  *it = crc8({cbegin(buf), size(buf) - 1uz});  // CRC8
  transmitBytes(buf, mbps_);
  resync();
  gpioInput();
  if (!ackValid() || !ack()) return {};
  busy();
  return true;
}

///
void Base::resync() const {
  delayUs(10u);
  transmitBytes({&resync_byte, 1uz}, Mbps::_0_1);
}

///
bool Base::ackValid() const { return !ack(); }

/// Read ack
///
/// \return true  Ack high
/// \return false Ack low
bool Base::ack() const {
  writeClock(true);
  delayUs(10u);
  auto const retval{readData()};
  writeClock(false);
  delayUs(20u);
  return retval;
}

///
void Base::busy() const {
  writeClock(true);
  delayUs(10u);
  writeClock(false);
  delayUs(20u);
  while (!readData());  // TODO timeout?
}

///
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

}  // namespace zusi::tx
