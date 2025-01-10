// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Receive base
///
/// \file   rx/base.cpp
/// \author Vincent Hamp
/// \date   21/03/2023

#include <climits>
#include <gsl/util>
#include "zusi.hpp"

namespace zusi::rx {

/// Execute
void Base::execute() {
  switch (_state) {
    case State::ReceiveCommand:
      toggleLights();
      _state = receiveCommand();
      break;
    case State::ReceiveData: _state = receiveData(); break;
    case State::ReceiveResync: _state = receiveResync(); break;
    case State::TransmitAck: _state = transmitAck(); break;
    case State::TransmitBusy: _state = transmitBusy(); break;
    case State::TransmitData: _state = transmitData(); break;
    case State::Error: _state = reset(); break;
  }
}

/// Receive command
///
/// \return State
Base::State Base::receiveCommand() {
  if (!receiveBytes({&_buf[0uz], 1uz})) return State::Error;
  return is_valid_command(_buf[0uz]) ? State::ReceiveData : State::Error;
}

/// Receive data
///
/// \return State
Base::State Base::receiveData() {
  bool success{};
  switch (static_cast<Command>(_buf[0uz])) {
    case Command::CvRead: success = receiveBytes({&_buf[1uz], 6uz}); break;
    case Command::CvWrite: [[fallthrough]];
    case Command::ZppWrite:
      if ((success = receiveBytes({&_buf[1uz], 1uz})))
        success = receiveBytes({&_buf[2uz], _buf[1uz] + 6uz});
      break;
    case Command::ZppErase: success = receiveBytes({&_buf[1uz], 3uz}); break;
    case Command::Features: success = receiveBytes({&_buf[1uz], 1uz}); break;
    case Command::Exit: success = receiveBytes({&_buf[1uz], 4uz}); break;
    case Command::Encrypt: success = receiveBytes({&_buf[1uz], 5uz}); break;
    default: break;
  }
  return success ? State::ReceiveResync : State::Error;
}

/// Receive resync byte
///
/// \return State
Base::State Base::receiveResync() {
  _ack = ackOrNack();
  if (auto const retval{receiveByte()}; !retval) return State::Error;
  else if (*retval == resync_byte) {
    gpio();
    return State::TransmitAck;
  } else return State::Error;
}

/// Transmit acknowledge
///
/// \return State
Base::State Base::transmitAck() {
  if (!waitClock(true)) return State::Error;
  writeData(false);
  if (!waitClock(false)) return State::Error;
  if (!waitClock(true)) return State::Error;
  if (_ack == true) writeData(true);
  if (!waitClock(false)) return State::Error;
  // Exit does not require us to carry on
  if (static_cast<Command>(_buf[0uz]) == Command::Exit) exit(_buf[3uz]);
  return _ack == true ? State::TransmitBusy : State::Error;
}

/// Transmit busy
///
/// \return State
Base::State Base::transmitBusy() {
  if (!waitClock(true)) return State::Error;
  writeData(false);
  auto const retval{execute(static_cast<Command>(_buf[0uz]))};
  if (!waitClock(false)) return State::Error;
  writeData(true);
  if (retval == State::ReceiveCommand) spi();
  return retval;
}

/// Transmit data
///
/// \return State
Base::State Base::transmitData() {
  for (auto i{0uz}; i < _bytes_count; ++i)
    if (!transmitByte(_buf[i])) return State::Error;
  return reset();
}

/// Execute command
///
/// \param  cmd Command
/// \return State
Base::State Base::execute(Command cmd) {
  State retval{State::ReceiveCommand};
  uint32_t const addr{data2uint32(&_buf[2uz])};
  size_t const rx_count{_buf[1uz] + 1uz};
  switch (cmd) {
    case Command::CvRead:
      for (auto i{0uz}; i < rx_count; ++i) _buf[0uz + i] = readCv(addr + i);
      _buf[rx_count] = crc8({&_buf[0uz], rx_count});
      _bytes_count = rx_count + 1uz;
      retval = State::TransmitData;
      break;
    case Command::CvWrite:
      for (auto i{0uz}; i < rx_count; ++i) writeCv(addr + i, _buf[6uz + i]);
      break;
    case Command::ZppErase: eraseZpp(); break;
    case Command::ZppWrite: writeZpp(addr, {&_buf[6uz], rx_count}); break;
    case Command::Features: {
      auto const feature_bytes{features()};
      std::copy(cbegin(feature_bytes), cend(feature_bytes), begin(_buf));
      _bytes_count = size(feature_bytes);
      retval = State::TransmitData;
      break;
    }
    case Command::Encrypt: {
      std::span<uint8_t const, 4uz> developer_code{&_buf[1uz], 4uz};
      _buf[1uz] = loadCodeValid(developer_code);
      _buf[2uz] = crc8(_buf[1uz]);
      _bytes_count = 2uz;
      retval = State::TransmitData;
      break;
    }
    default: break;
  }
  return retval;
}

/// Reset data and state
///
/// \return State
Base::State Base::reset() {
  spi();
  _buf.fill(0u);
  _bytes_count = _crc = 0uz;
  _ack = false;
  return State::ReceiveCommand;
}

/// Receive bytes
///
/// \param  dest  Span to receive to
/// \return true  Success
/// \return false Failure
bool Base::receiveBytes(std::span<uint8_t> dest) {
  for (auto& byte : dest)
    if (auto const retval{receiveByte()}; !retval) return false;
    else {
      byte = *retval;
      _crc = crc8(static_cast<uint8_t>(byte ^ _crc));
    }
  return true;
}

/// Transmit byte
///
/// \param  byte  Byte to send
/// \return true  Success
/// \return false Failure
bool Base::transmitByte(uint8_t byte) const {
  for (auto i{0uz}; i < CHAR_BIT; ++i) {
    if (!waitClock(true)) return false;
    writeData(byte & 1u << i);
    if (!waitClock(false)) return false;
  }
  return true;
}

/// Decide whether to acknowledge or not acknowledge
///
/// \return true  Acknowledge
/// \return false Not acknowledge
bool Base::ackOrNack() {
  gsl::final_action clear_crc{[this] { _crc = 0u; }};
  switch (static_cast<Command>(_buf[0uz])) {
    // Requires only CRC
    case Command::CvRead: [[fallthrough]];
    case Command::CvWrite: [[fallthrough]];
    case Command::Features: [[fallthrough]];
    case Command::Encrypt: return !_crc ? true : false;
    // Requires CRC and address validation by decryption
    case Command::ZppWrite:
      return !_crc && addressValid(data2uint32(&_buf[2uz])) ? true : false;
    // Requires CRC and safety bytes
    case Command::ZppErase: [[fallthrough]];
    case Command::Exit:
      return !_crc && _buf[1uz] == 0x55u && _buf[2uz] == 0xAAu ? true : false;
    default: break;
  }
  return false;
}

} // namespace zusi::rx
