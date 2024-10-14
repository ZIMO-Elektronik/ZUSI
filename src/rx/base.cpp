// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Receive base
///
/// \file   rx/base.cpp
/// \author Vincent Hamp
/// \date   21/03/2023

#include <climits>
#include "zusi.hpp"

namespace zusi::rx {

/// Execute
void Base::execute() {
  switch (state_) {
    case State::ReceiveCommand:
      toggleLights();
      state_ = receiveCommand();
      break;
    case State::ReceiveData: state_ = receiveData(); break;
    case State::ReceiveResynch: state_ = receiveResynch(); break;
    case State::TransmitAck: state_ = transmitAck(); break;
    case State::TransmitBusy: state_ = transmitBusy(); break;
    case State::TransmitData: state_ = transmitData(); break;
    case State::Error: state_ = reset(); break;
  }
}

/// Receive command
///
/// \return State
Base::State Base::receiveCommand() {
  if (!receiveBytes({&buf_[0uz], 1uz})) return State::Error;
  return is_valid_command(buf_[0uz]) ? State::ReceiveData : State::Error;
}

/// Receive data
///
/// \return State
Base::State Base::receiveData() {
  bool success{false};
  switch (static_cast<Command>(buf_[0uz])) {
    case Command::CvRead: success = receiveBytes({&buf_[1uz], 6uz}); break;
    case Command::CvWrite: [[fallthrough]];
    case Command::ZppWrite:
      if ((success = receiveBytes({&buf_[1uz], 1uz})))
        success = receiveBytes({&buf_[2uz], buf_[1uz] + 6uz});
      break;
    case Command::ZppErase: success = receiveBytes({&buf_[1uz], 3uz}); break;
    case Command::Features: success = receiveBytes({&buf_[1uz], 1uz}); break;
    case Command::Exit: success = receiveBytes({&buf_[1uz], 4uz}); break;
    case Command::Encrypt: success = receiveBytes({&buf_[1uz], 5uz}); break;
    default: break;
  }
  return success ? State::ReceiveResynch : State::Error;
}

/// Receive resynch byte
///
/// \return State
Base::State Base::receiveResynch() {
  ack_ = ackOrNack();
  if (uint8_t byte; !receiveByte(&byte)) return State::Error;
  else if (byte == resync_byte) {
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
  if (ack_ == true) writeData(true);
  if (!waitClock(false)) return State::Error;
  // Exit does not require us to carry on
  if (static_cast<Command>(buf_[0uz]) == Command::Exit) exit(buf_[3uz]);
  return ack_ == true ? State::TransmitBusy : State::Error;
}

/// Transmit busy
///
/// \return State
Base::State Base::transmitBusy() {
  if (!waitClock(true)) return State::Error;
  writeData(false);
  auto const retval{execute(static_cast<Command>(buf_[0uz]))};
  if (!waitClock(false)) return State::Error;
  writeData(true);
  if (retval == State::ReceiveCommand) spi();
  return retval;
}

/// Transmit data
///
/// \return State
Base::State Base::transmitData() {
  for (auto i{0uz}; i < bytes_count_; ++i)
    if (!transmitByte(buf_[i])) return State::Error;
  return reset();
}

/// Execute command
///
/// \param  cmd Command
/// \return State
Base::State Base::execute(Command cmd) {
  State retval{State::ReceiveCommand};
  uint32_t const addr{data2uint32(&buf_[2uz])};
  size_t const rx_count{buf_[1uz] + 1uz};
  switch (cmd) {
    case Command::CvRead:
      for (auto i{0uz}; i < rx_count; ++i) buf_[0uz + i] = readCv(addr + i);
      buf_[rx_count] = crc8({&buf_[0uz], rx_count});
      bytes_count_ = rx_count + 1uz;
      retval = State::TransmitData;
      break;
    case Command::CvWrite:
      for (auto i{0uz}; i < rx_count; ++i) writeCv(addr + i, buf_[6uz + i]);
      break;
    case Command::ZppErase: eraseZpp(); break;
    case Command::ZppWrite: writeZpp(addr, {&buf_[6uz], rx_count}); break;
    case Command::Features: {
      auto const feature_bytes{features()};
      std::copy(cbegin(feature_bytes), cend(feature_bytes), begin(buf_));
      bytes_count_ = size(feature_bytes);
      retval = State::TransmitData;
      break;
    }
    case Command::Encrypt: {
      std::span<uint8_t const, 4u> developer_code{&buf_[1uz], 4uz};
      buf_[1uz] = loadCodeValid(developer_code);
      buf_[2uz] = crc8(buf_[1uz]);
      bytes_count_ = 2uz;
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
  buf_.fill(0u);
  bytes_count_ = crc_ = 0uz;
  ack_ = false;
  return State::ReceiveCommand;
}

/// Receive bytes
///
/// \param  dest  Span to receive to
/// \return true  Success
/// \return false Failure
bool Base::receiveBytes(std::span<uint8_t> dest) {
  for (auto& byte : dest) {
    if (!receiveByte(&byte)) return false;
    crc_ = crc8(static_cast<uint8_t>(byte ^ crc_));
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
  bool const crc_ok{!crc_};
  crc_ = 0u;
  switch (static_cast<Command>(buf_[0uz])) {
    // Requires only CRC
    case Command::CvRead: [[fallthrough]];
    case Command::CvWrite: [[fallthrough]];
    case Command::Features: [[fallthrough]];
    case Command::Encrypt: return crc_ok ? true : false;
    // Requires CRC and address validation by decryption
    case Command::ZppWrite:
      return crc_ok && addressValid(data2uint32(&buf_[2uz])) ? true : false;
    // Requires CRC and safety bytes
    case Command::ZppErase: [[fallthrough]];
    case Command::Exit:
      return crc_ok && buf_[1uz] == 0x55u && buf_[2uz] == 0xAAu ? true : false;
    default: break;
  }
  return false;
}

}  // namespace zusi::rx
