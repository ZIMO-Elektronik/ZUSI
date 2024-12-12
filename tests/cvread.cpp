#include <gtest/gtest.h>
#include <zusi/command.hpp> // Needed so utility wont complain about missing Command declaration
#include <zusi/utility.hpp>
#include <zusi/zusi.hpp>
#include "mock_zusi.hpp"

using ::testing::AtLeast;
using ::testing::InSequence;
using ::testing::Return;

using ::Mbps::_0_1;
using ::Mbps::_0_286;
using ::Mbps::_1_364;
using ::Mbps::_1_807;
using ::zusi::resync_byte;

TEST(test, no_ACK_valid) {
  MockZUSI zusi{};
  EXPECT_CALL(zusi, gpioInput()).Times(AtLeast(1));
  ASSERT_FALSE(zusi.readCv(0x000000FF));
}

TEST(test, NAK) {
  MockZUSI zusi{};
  std::span<uint8_t const> span{&resync_byte, 1uz};
  {
    InSequence seq;
    EXPECT_CALL(zusi, transmitBytes(span, _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData()).WillRepeatedly(Return(false));
  }
}

TEST(test, ACK) {}
