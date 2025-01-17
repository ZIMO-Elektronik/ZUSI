#pragma once

#include <gtest/gtest.h>
#include "rx_mock.hpp"

using namespace ::testing;

// Receive test fixture
struct RxTest : ::testing::Test {
protected:
  RxTest();
  virtual ~RxTest();

  template<typename Rep, typename Period>
  void RunFor(std::chrono::duration<Rep, Period> duration,
              std::function<void()> f = nullptr) {
    auto const then{std::chrono::system_clock::now() + duration};
    while (std::chrono::system_clock::now() < then) f ? f() : _mock.receive();
  }

  NiceMock<RxMock> _mock;
};