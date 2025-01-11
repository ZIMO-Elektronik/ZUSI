// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Packet
///
/// \file   zusi/packet.hpp
/// \author Vincent Hamp
/// \date   03/01/2025

#pragma once

#include <cstdint>
#include <ztl/inplace_vector.hpp>

namespace zusi {

using Packet = ztl::inplace_vector<uint8_t, ZUSI_MAX_PACKET_SIZE>;

} // namespace zusi
