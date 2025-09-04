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
/// \param  packet                      Packet
/// \return Feedback                    Returned data (can contain error)
/// \retval std::errc::connection_reset No response
/// \retval std::errc::protocol_error   NAK
/// \retval std::errc::bad_message      CRC error
/// \retval std::errc::invalid_argument Unknown command
Feedback Base::transmit(Packet const& packet) {
  return transmit({cbegin(packet), size(packet)});
}

/// Transmit bytes
///
/// \param  bytes                       Bytes containing packet
/// \return Feedback                    Returned data (can contain error)
/// \retval std::errc::connection_reset No response
/// \retval std::errc::protocol_error   NAK
/// \retval std::errc::bad_message      CRC error
/// \retval std::errc::invalid_argument Unknown command
Feedback Base::transmit(std::span<uint8_t const> bytes) {
  switch (std::bit_cast<Command>(bytes.front())) {
    case Command::CvRead:
      if (auto const cv{readCv(data2uint32(&bytes[addr_pos]))})
        return Feedback::value_type{*cv};
      else return std::unexpected(cv.error());
      break;
    case Command::CvWrite:
      if (auto const result{
            writeCv(data2uint32(&bytes[addr_pos]), bytes[data_pos])})
        return Feedback::value_type{};
      else return std::unexpected(result.error());
      break;
    case Command::ZppErase:
      if (auto const result{eraseZpp()}) return Feedback::value_type{};
      else return std::unexpected(result.error());
      break;
    case Command::ZppWrite:
      if (auto const result{
            writeZpp(data2uint32(&bytes[addr_pos]),
                     bytes.subspan(data_pos, bytes[data_cnt_pos] + 1uz))})
        return Feedback::value_type{};
      else return std::unexpected(result.error());
      break;
    case Command::Features:
      if (auto const feats{features()})
        return Feedback::value_type{
          (*feats)[0uz], (*feats)[1uz], (*feats)[2uz], (*feats)[3uz]};
      else return std::unexpected(feats.error());
      break;
    case Command::Exit:
      if (auto const result{exit(bytes[exit_flags_pos])})
        return Feedback::value_type{};
      else return std::unexpected(result.error());
      break;
    case Command::ZppLcDcQuery: {
      if (auto const valid{lcDcQuery(bytes.subspan<1uz, 4uz>())})
        return Feedback::value_type{*valid};
      else return std::unexpected(valid.error());
      break;
    }
    default: break;
  }
  return std::unexpected(std::errc::invalid_argument);
}

/// Read CV
///
/// \param  addr                        CV address
/// \retval uint8_t                     CV value
/// \retval std::errc::connection_reset No response
/// \retval std::errc::protocol_error   NAK
/// \retval std::errc::bad_message      CRC error
std::expected<uint8_t, std::errc> Base::readCv(uint32_t addr) const {
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
  if (auto const err{ack()}; err != std::errc{}) return std::unexpected{err};
  busy();
  auto const cv{receiveByte()};
  if (crc8(cv) == receiveByte()) return cv;
  else return std::unexpected{std::errc::bad_message};
}

/// Write CV
///
/// \param  addr  CV address
/// \param  byte  CV value
/// \retval true  Success
/// \retval std::errc::connection_reset No response
/// \retval std::errc::protocol_error   NAK
std::expected<bool, std::errc> Base::writeCv(uint32_t addr,
                                             uint8_t byte) const {
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
  if (auto const err{ack()}; err != std::errc{}) return std::unexpected{err};
  busy();
  return true;
}

/// Erase ZPP
///
/// \retval true                        Success
/// \retval std::errc::connection_reset No response
/// \retval std::errc::protocol_error   NAK
std::expected<bool, std::errc> Base::eraseZpp() const {
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
  if (auto const err{ack()}; err != std::errc{}) return std::unexpected{err};
  busy();
  return true;
}

/// Write ZPP
///
/// \param  addr                        Address
/// \param  bytes                       Bytes
/// \retval true                        Success
/// \retval std::errc::connection_reset No response
/// \retval std::errc::protocol_error   NAK
std::expected<bool, std::errc>
Base::writeZpp(uint32_t addr, std::span<uint8_t const> bytes) const {
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
  if (auto const err{ack()}; err != std::errc{}) return std::unexpected{err};
  busy();
  return true;
}

/// Features query
///
/// \retval Features                    Feature bytes
/// \retval std::errc::connection_reset No response
/// \retval std::errc::protocol_error   NAK
std::expected<Features, std::errc> Base::features() {
  gsl::final_action spi_master{[this] { spiMaster(); }};
  std::array<uint8_t, 2uz> buf;
  auto it{begin(buf)};
  *it++ = std::to_underlying(Command::Features); // Command
  *it = crc8({cbegin(buf), size(buf) - 1uz});    // CRC8
  transmitBytes(buf, Mbps::_0_286);
  resync();
  gpioInput();
  if (auto const err{ack()}; err != std::errc{}) return std::unexpected{err};
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
/// \param  flags                       Flags
/// \retval true                        Success
/// \retval std::errc::connection_reset No response
/// \retval std::errc::protocol_error   NAK
std::expected<bool, std::errc> Base::exit(uint8_t flags) const {
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
  if (auto const err{ack()}; err != std::errc{}) return std::unexpected{err};
  busy();
  return true;
}

/// LC-DC query
///
/// \param  developer_code              Developer code
/// \retval bool                        Load code valid
/// \retval std::errc::connection_reset No response
/// \retval std::errc::protocol_error   NAK
/// \retval std::errc::bad_message      CRC error
std::expected<bool, std::errc>
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
  if (auto const err{ack()}; err != std::errc{}) return std::unexpected{err};
  busy();
  auto const valid{receiveByte()};
  if (crc8(valid) == receiveByte()) return static_cast<bool>(valid);
  else return std::unexpected{std::errc::bad_message};
}

/// Transmit resync byte
void Base::resync() const {
  delayUs(10u);
  transmitBytes({&resync_byte, 1uz}, Mbps::_0_1);
}

/// ACK phase
///
/// \return std::errc
std::errc Base::ack() const {
  // ACK valid
  if (auto const ack_valid{receiveAck()}, ack{receiveAck()}; ack_valid)
    return std::errc::connection_reset;
  // ACK
  else if (!ack) return std::errc::protocol_error;
  // Success
  else return {};
}

/// Busy phase sequence
void Base::busy() const {
  writeClock(true);
  delayUs(10u);
  writeClock(false);
  delayUs(20u);
  while (!readData()); /// \todo timeout?
}

/// Receive ACK
///
/// \return Received ACK
bool Base::receiveAck() const {
  writeClock(true);
  delayUs(10u);
  auto const retval{readData()};
  writeClock(false);
  delayUs(20u);
  return retval;
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
