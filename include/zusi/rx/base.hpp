// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Receive base
///
/// \file   zusi/rx/base.hpp
/// \author Vincent Hamp
/// \date   21/03/2023

#pragma once

#include <climits>
#include <cstddef>
#include <utility>
#include "../command.hpp"
#include "../crc8.hpp"
#include "../features.hpp"
#include "../packet.hpp"
#include "../utility.hpp"

namespace zusi::rx {

/// Implements the bare necessities for loading sound
class Base {
public:
  /// Dtor
  virtual constexpr ~Base() = default;

  void execute();

private:
  /// Read CV
  ///
  /// \param  addr  CV address
  /// \return CV value
  virtual uint8_t readCv(uint32_t addr) const = 0;

  /// Write CV
  ///
  /// \param  addr  CV address
  /// \param  value CV value
  virtual void writeCv(uint32_t addr, uint8_t value) = 0;

  /// Erase ZPP
  virtual void eraseZpp() = 0;

  /// Write ZPP
  ///
  /// \param  addr  Address
  /// \param  bytes Bytes
  /// \retval true  Success
  /// \retval false Error
  virtual void writeZpp(uint32_t addr, std::span<uint8_t const> bytes) = 0;

  /// Get features
  ///
  /// \return Features
  virtual Features features() const = 0;

  /// Exit
  ///
  /// \param  flags Flags
  [[noreturn]] virtual void exit(uint8_t flags) = 0;

  /// Check if load code is valid
  ///
  /// \param  developer_code  Developer code
  /// \retval true            Load code is valid
  /// \retval false           Load code is not valid
  virtual bool
  loadCodeValid(std::span<uint8_t const, 4uz> developer_code) const = 0;

  /// Check if address is valid
  ///
  /// \param  addr  Address
  /// \retval true  Address valid
  /// \retval false Address not valid
  virtual bool addressValid(uint32_t addr) const = 0;

  /// Receive byte
  ///
  /// \retval uint8_t       Received byte
  /// \retval std::nullopt  No byte received
  virtual std::optional<uint8_t> receiveByte() const = 0;

  /// Wait for the clock to equal state
  ///
  /// \param  state State
  /// \retval true  Clock equals state
  /// \retval false Timeout occurred
  virtual bool waitClock(bool state) const = 0;

  /// Set data to state
  ///
  /// \param  state State
  virtual void writeData(bool state) const = 0;

  /// Switch to SPI
  virtual void spi() const = 0;

  /// Switch to GPIO
  virtual void gpio() const = 0;

  /// Toggle front- and rear lights
  virtual void toggleLights() const {}

  enum class State : uint8_t {
    ReceiveCommand,
    ReceiveData,
    ReceiveResync,
    TransmitAck,
    TransmitBusy,
    TransmitData,
    Error
  };

  State receiveCommand();
  State receiveData();
  State receiveResync();
  State transmitAck();
  State transmitBusy();
  State transmitData();

  State execute(Command cmd);
  State reset();
  bool receiveBytes(size_t count);
  bool transmitByte(uint8_t byte) const;
  bool ackOrNack();

  Packet _packet{}; ///< Receive/transmit
  uint8_t _crc{};   ///< CRC8
  State _state{};   ///< State
  bool _ack{};      ///< Ack/nak
};

} // namespace zusi::rx
