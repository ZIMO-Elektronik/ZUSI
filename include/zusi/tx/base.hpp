// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Transmit base
///
/// \file   zusi/tx/base.hpp
/// \author Vincent Hamp
/// \date   21/03/2023

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <ztl/inplace_vector.hpp>
#include "../features.hpp"
#include "../feedback.hpp"
#include "../mbps.hpp"

namespace zusi::tx {

class Base {
public:
  /// Dtor
  virtual constexpr ~Base() = default;

  /// Transmit entry sequence
  void enter() const;

  /// Transmit packet
  ///
  /// \param  packet    Packet
  /// \return Feedback  Returned data (can be empty)
  Feedback transmit(Packet const& packet);

  /// Transmit bytes
  ///
  /// \param  bytes     Bytes containing ZUSI packet
  /// \return Feedback  Returned data (can be empty)
  Feedback transmit(std::span<uint8_t const> bytes);

  /// Read CV
  ///
  /// \param  addr          CV address
  /// \retval std::nullopt  Error
  std::optional<uint8_t> readCv(uint32_t addr) const;

  /// Write CV
  ///
  /// \param  addr  CV address
  /// \param  byte  CV value
  /// \retval true  Success
  /// \retval false Error
  bool writeCv(uint32_t addr, uint8_t byte) const;

  /// Erase ZPP
  ///
  /// \retval true  Success
  /// \retval false Error
  bool eraseZpp() const;

  /// Write ZPP
  ///
  /// \param  addr  Address
  /// \param  bytes Bytes
  /// \retval true  Success
  /// \retval false Error
  bool writeZpp(uint32_t addr, std::span<uint8_t const> bytes) const;

  /// Features query
  ///
  /// \retval Features      Feature bytes
  /// \retval std::nullopt  Error
  std::optional<Features> features();

  /// Exit
  ///
  /// \param  flags Flags
  /// \retval true  Success
  /// \retval false Error
  bool exit(uint8_t flags) const;

  /// LC-DC query
  ///
  /// \param  developer_code  Developer code
  /// \retval bool            Load code valid
  /// \retval std::nullopt    Error
  std::optional<bool>
  lcDcQuery(std::span<uint8_t const, 4uz> developer_code) const;

private:
  /// Transmit bytes
  virtual void transmitBytes(std::span<uint8_t const> bytes,
                             Mbps mbps) const = 0;

  /// Switch to SPI master
  virtual void spiMaster() const = 0;

  /// Switch to GPIO input
  virtual void gpioInput() const = 0;

  /// Switch to GPIO output
  virtual void gpioOutput() const = 0;

  /// Write clock line
  virtual void writeClock(bool state) const = 0;

  /// Write data line
  virtual void writeData(bool state) const = 0;

  /// Read data line
  virtual bool readData() const = 0;

  /// Delay microseconds
  virtual void delayUs(uint32_t us) const = 0;

  void resync() const;
  bool ackValid() const;
  bool ack() const;
  void busy() const;
  uint8_t receiveByte() const;

  Mbps _mbps{Mbps::_0_286};
};

} // namespace zusi::tx
