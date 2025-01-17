// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Response
///
/// \file   zusi/response.hpp
/// \author Vincent Hamp
/// \date   17/01/2025

#pragma once

#include <cstdint>
#include <ztl/inplace_vector.hpp>

namespace zusi {

using Response = ztl::inplace_vector<uint8_t, ZUSI_MAX_RESPONSE_SIZE>;

} // namespace zusi
