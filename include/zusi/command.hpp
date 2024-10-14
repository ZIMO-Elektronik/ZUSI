// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Commands
///
/// \file   zusi/command.hpp
/// \author Vincent Hamp
/// \date   21/03/2023

#pragma once

#include <cstdint>

namespace zusi {

enum class Command : uint8_t {
  None = 0u,      ///< No instruction
  CvRead = 1u,    ///< Read CV (ack->busy->data)
  CvWrite = 2u,   ///< Write CV (ack->busy)
  ZppErase = 4u,  ///< ZPP erase (ack->busy)
  ZppWrite = 5u,  ///< ZPP write (ack->busy)
  Features = 6u,  ///< Feature request (ack->busy->data)
  Exit = 7u,      ///< Exit (ack)
  Encrypt = 13u   ///< Ask if encryption is supported
};

}  // namespace zusi
