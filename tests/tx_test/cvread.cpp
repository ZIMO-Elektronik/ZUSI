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

inline constexpr uint32_t mock_addr{0x000000FFu};

TEST(cvread, no_ACK_valid) {
  MockZUSI zusi{};
  {
    InSequence seq;
    EXPECT_CALL(zusi, transmitBytes(_, Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // not ACK valid
    EXPECT_CALL(zusi, spiMaster());
  }
  ASSERT_FALSE(zusi.readCv(mock_addr)) << "Should abort if not ACK valid";
}

TEST(cvread, NAK) {
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
  ASSERT_FALSE(zusi.readCv(mock_addr)) << "Should abort after NAK";
}

TEST(cvread, busy_wait) {
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
    EXPECT_CALL(zusi, readData()).Times(8);               // Receive data
    EXPECT_CALL(zusi, readData()).Times(8);               // Receive crc8
    EXPECT_CALL(zusi, spiMaster());
  }
  ASSERT_TRUE(zusi.readCv(mock_addr)) << "Should continue after no longer busy";
}

TEST(cvread, crc8_answer) {
  NiceMock<MockZUSI> zusi{};
  {
    InSequence seq;
    EXPECT_CALL(zusi, transmitBytes(_, Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData());                        // ACK valid
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // ACK
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // Busy
    EXPECT_CALL(zusi, readData())                         //
      .Times(4)                                           //
      .WillRepeatedly(Return(true));                      // Receive 4 bits 1
    EXPECT_CALL(zusi, readData()).Times(4);               // Receive 4 bits 0
    EXPECT_CALL(zusi, readData()).Times(8);               // Receive crc8
    EXPECT_CALL(zusi, spiMaster());
  }
  ASSERT_FALSE(zusi.readCv(mock_addr)) << "Should abort if CRC error occurs";
}

TEST(cvread, ACK) {
  NiceMock<MockZUSI> zusi{};
  {
    InSequence seq;
    EXPECT_CALL(zusi,
                transmitBytes(
                  ElementsAre(0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0xFFu, 0x02u),
                  Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData());                        // ACK valid
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // ACK
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // Busy
    EXPECT_CALL(zusi, readData()).Times(8);               // Receive data
    EXPECT_CALL(zusi, readData()).Times(8);               // CRC8
    EXPECT_CALL(zusi, spiMaster());
  }
  auto tmp = zusi.readCv(mock_addr);
  ASSERT_TRUE(tmp) << "Should return CV value if command is correct";
  ASSERT_EQ(*tmp, 0x00)
    << "Should return data the decoder sent (see Receive data)";
}
