#include <gtest/gtest.h>
#include <zusi/zusi.hpp>
#include "mock_zusi.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::Ne;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnArg;

using ::Mbps::_0_1;
using ::Mbps::_0_286;
using ::Mbps::_1_364;
using ::Mbps::_1_807;
using ::zusi::resync_byte;

TEST(erasezpp, no_ACK_valid) {
  MockZUSI zusi{};
  {
    InSequence seq;
    EXPECT_CALL(zusi, transmitBytes(_, Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // not ACK valid
    EXPECT_CALL(zusi, spiMaster());
  }
  ASSERT_FALSE(zusi.eraseZpp()) << "Should abort if not ACK valid";
}

TEST(erasezpp, NAK) {
  NiceMock<MockZUSI> zusi{};
  {
    InSequence seq;
    EXPECT_CALL(zusi, transmitBytes(_, Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData()); // ACK valid
    EXPECT_CALL(zusi, readData()); // NAK
    EXPECT_CALL(zusi, spiMaster());
  }
  ASSERT_FALSE(zusi.eraseZpp()) << "Should abort after NAK";
}

TEST(erasezpp, busy_wait) {
  NiceMock<MockZUSI> zusi{};
  {
    InSequence seq;
    EXPECT_CALL(zusi, transmitBytes(_, Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData());                        // ACK valid
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // ACK
    EXPECT_CALL(zusi, readData()).Times(5);               // Busy
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // Busy End
    EXPECT_CALL(zusi, spiMaster());
  }
  ASSERT_TRUE(zusi.eraseZpp()) << "Should continue after no longer busy";
}

TEST(erasezpp, ACK) {
  NiceMock<MockZUSI> zusi{};
  {
    InSequence seq;
    EXPECT_CALL(
      zusi, transmitBytes(ElementsAre(0x04u, 0x55u, 0xAAu, 0xC7u), Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData());                        // ACK valid
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // ACK
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // Busy
    EXPECT_CALL(zusi, spiMaster());
  }
  auto tmp = zusi.eraseZpp();
  ASSERT_TRUE(tmp) << "Should return true if command is correct";
}