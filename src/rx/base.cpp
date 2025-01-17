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

/// Receive
void Base::receive() {
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
  _packet.clear();
  if (!receiveBytes(1uz)) return State::Error;
  return is_valid_command(_packet[0uz]) ? State::ReceiveData : State::Error;
}

/// Receive data
///
/// \return State
Base::State Base::receiveData() {
  bool success{};
  switch (static_cast<Command>(_packet[0uz])) {
    case Command::CvRead: success = receiveBytes(6uz); break;
    case Command::CvWrite: [[fallthrough]];
    case Command::ZppWrite:
      if ((success = receiveBytes(1uz)))
        success = receiveBytes(_packet[1uz] + 6uz);
      break;
    case Command::ZppErase: success = receiveBytes(3uz); break;
    case Command::Features: success = receiveBytes(1uz); break;
    case Command::Exit: success = receiveBytes(4uz); break;
    case Command::ZppLcDcQuery: success = receiveBytes(5uz); break;
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
    gpioOutput();
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
  if (static_cast<Command>(_packet[0uz]) == Command::Exit) exit(_packet[3uz]);
  return _ack == true ? State::TransmitBusy : State::Error;
}

/// Transmit busy
///
/// \return State
Base::State Base::transmitBusy() {
  if (!waitClock(true)) return State::Error;
  writeData(false);
  auto const retval{execute(static_cast<Command>(_packet[0uz]))};
  if (!waitClock(false)) return State::Error;
  writeData(true);
  if (retval == State::ReceiveCommand) spiSlave();
  return retval;
}

/// Transmit data
///
/// \return State
Base::State Base::transmitData() {
  for (auto byte : _packet)
    if (!transmitByte(byte)) return State::Error;
  return reset();
}

/// Execute command
///
/// \param  cmd Command
/// \return State
Base::State Base::execute(Command cmd) {
  State retval{State::ReceiveCommand};
  uint32_t const addr{data2uint32(&_packet[2uz])};
  switch (cmd) {
    case Command::CvRead:
      _packet[0uz] = readCv(addr);
      _packet[1uz] = crc8(_packet[0uz]);
      _packet.resize(2uz);
      retval = State::TransmitData;
      break;
    case Command::CvWrite: writeCv(addr, _packet[6uz]); break;
    case Command::ZppErase: eraseZpp(); break;
    case Command::ZppWrite: {
      size_t const count{_packet[1uz] + 1uz};
      writeZpp(addr, {&_packet[6uz], count});
      break;
    }
    case Command::Features: {
      auto const feature_bytes{features()};
      std::copy(cbegin(feature_bytes), cend(feature_bytes), begin(_packet));
      _packet.resize(size(feature_bytes));
      retval = State::TransmitData;
      break;
    }
    case Command::ZppLcDcQuery: {
      std::span<uint8_t const, 4uz> developer_code{&_packet[1uz], 4uz};
      _packet[0uz] = loadCodeValid(developer_code);
      _packet[1uz] = crc8(_packet[0uz]);
      _packet.resize(2uz);
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
  spiSlave();
  _crc = 0u;
  _ack = false;
  return State::ReceiveCommand;
}

/// Receive bytes
///
/// \param  count Number of bytes to receive
/// \retval true  Success
/// \retval false Failure
bool Base::receiveBytes(size_t count) {
  for (auto i{0uz}; i < count; ++i)
    if (auto const byte{receiveByte()}; !byte) return false;
    else {
      _packet.push_back(*byte);
      _crc = crc8(static_cast<uint8_t>(*byte ^ _crc));
    }
  return true;
}

/// Transmit byte
///
/// \param  byte  Byte to send
/// \retval true  Success
/// \retval false Failure
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
/// \retval true  Acknowledge
/// \retval false Not acknowledge
bool Base::ackOrNack() {
  gsl::final_action clear_crc{[this] { _crc = 0u; }};
  switch (static_cast<Command>(_packet[0uz])) {
    // Requires only CRC
    case Command::CvRead: [[fallthrough]];
    case Command::CvWrite: [[fallthrough]];
    case Command::Features: [[fallthrough]];
    case Command::ZppLcDcQuery: return !_crc ? true : false;
    // Requires CRC and address validation by decryption
    case Command::ZppWrite:
      return !_crc && addressValid(data2uint32(&_packet[2uz])) ? true : false;
    // Requires CRC and safety bytes
    case Command::ZppErase: [[fallthrough]];
    case Command::Exit:
      return !_crc && _packet[1uz] == 0x55u && _packet[2uz] == 0xAAu ? true
                                                                     : false;
    default: break;
  }
  return false;
}

} // namespace zusi::rx
