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

TEST(features, no_ACK_valid) {
  MockZUSI zusi{};
  {
    InSequence seq;
    EXPECT_CALL(zusi, transmitBytes(_, Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // not ACK valid
    EXPECT_CALL(zusi, spiMaster());
  }
  ASSERT_FALSE(zusi.features()) << "Should abort if not ACK valid";
}

TEST(features, NAK) {
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
  ASSERT_FALSE(zusi.features()) << "Should abort after NAK";
}

TEST(features, busy_wait) {
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
    EXPECT_CALL(zusi, readData()).Times(32);              // features
    EXPECT_CALL(zusi, spiMaster());
  }
  ASSERT_TRUE(zusi.features()) << "Should continue after no longer busy";
}

TEST(features, ACK) {
  NiceMock<MockZUSI> zusi{};
  {
    InSequence seq;
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(0x06u, 0xDDu), _0_286));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData());                        // ACK valid
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // ACK
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // Busy
    EXPECT_CALL(zusi, readData()).Times(32);              // features
    EXPECT_CALL(zusi, spiMaster());
  }
  auto tmp = zusi.features();
  ASSERT_TRUE(tmp) << "Should return true if command is correct";
}