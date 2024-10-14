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
#include "../features.hpp"
#include "../mbps.hpp"

namespace zusi::tx {

class Base {
public:
  /// Dtor
  virtual constexpr ~Base() = default;

  void enter() const;

  /// \param  index CV index
  std::optional<uint8_t> readCv(uint32_t addr) const;

  /// \param  addr  CV address
  /// \param  value CV value
  bool writeCv(uint32_t addr, uint8_t value) const;

  ///
  bool eraseZpp() const;

  ///
  bool writeZpp(uint32_t addr, std::span<uint8_t const> bytes) const;

  ///
  std::optional<Features> features();

  /// uint8_t flags
  bool exit(uint8_t flags) const;

private:
  ///
  virtual void transmitBytes(std::span<uint8_t const> bytes,
                             Mbps mbps) const = 0;

  ///
  virtual void spiMaster() const = 0;

  ///
  virtual void gpioInput() const = 0;

  ///
  virtual void gpioOutput() const = 0;

  ///
  virtual void writeClock(bool state) const = 0;

  ///
  virtual void writeData(bool state) const = 0;

  ///
  virtual bool readData() const = 0;

  ///
  virtual void delayUs(uint32_t us) const = 0;

  void resync() const;
  bool ackValid() const;
  bool ack() const;
  void busy() const;
  uint8_t receiveByte() const;

  Mbps mbps_{Mbps::_0_286};
};

}  // namespace zusi::tx
