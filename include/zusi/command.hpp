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

enum struct Command : uint8_t {
  None = 0x00u,
  CvRead = 0x01u,
  CvWrite = 0x02u,
  ZppErase = 0x04u,
  ZppWrite = 0x05u,
  Features = 0x06u,
  Exit = 0x07u,
  ZppLcDcQuery = 0x0D
};

} // namespace zusi
