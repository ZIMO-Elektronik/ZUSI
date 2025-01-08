#pragma once

#include <gtest/gtest.h>
#include "rx_mock.hpp"

using namespace ::testing;

// Receive test fixture
struct RxTest : ::testing::Test {
protected:
  RxTest();
  virtual ~RxTest();

  NiceMock<RxMock> _mock;
};