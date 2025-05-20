#pragma once

#include <gtest/gtest.h>
#include "tx_mock.hpp"

using namespace ::testing;

// Transmit test fixture
struct TxTest : ::testing::Test {
protected:
  TxTest();
  virtual ~TxTest();

  static constexpr uint32_t _addr{0x0000'00FFu};
  static constexpr uint8_t _cv{0x0Fu};
  static constexpr uint8_t _exit_flags{0x02u};

  NiceMock<TxMock> _mock;
};
