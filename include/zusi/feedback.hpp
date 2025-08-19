// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/// Feedback
///
/// \file   zusi/feedback.hpp
/// \author Vincent Hamp
/// \date   17/01/2025

#pragma once

#include <cstdint>
#include <expected>
#include <system_error>
#include <ztl/inplace_vector.hpp>

namespace zusi {

/// Feedback from decoders
///
/// Either empty (`std::nullopt`) on error, or vector with up to 4 bytes.
using Feedback =
  std::expected<ztl::inplace_vector<uint8_t, ZUSI_MAX_FEEDBACK_SIZE>,
                std::errc>;

} // namespace zusi
