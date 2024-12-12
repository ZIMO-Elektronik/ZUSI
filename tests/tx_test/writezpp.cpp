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
inline constexpr uint8_t mock_val{0x0Fu};

TEST(writezpp, no_ACK_valid) {
  MockZUSI zusi{};
  std::span<uint8_t const> mock_vals{&mock_val, 1u};
  {
    InSequence seq;
    EXPECT_CALL(zusi, transmitBytes(_, Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // not ACK valid
    EXPECT_CALL(zusi, spiMaster());
  }
  ASSERT_FALSE(zusi.writeZpp(mock_addr, mock_vals))
    << "Should abort if not ACK valid";
}

TEST(writezpp, NAK) {
  NiceMock<MockZUSI> zusi{};
  std::span<uint8_t const> mock_vals{&mock_val, 1u};
  {
    InSequence seq;
    EXPECT_CALL(zusi, transmitBytes(_, Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData()); // ACK valid
    EXPECT_CALL(zusi, readData()); // NAK
    EXPECT_CALL(zusi, spiMaster());
  }
  ASSERT_FALSE(zusi.writeZpp(mock_addr, mock_vals)) << "Should abort after NAK";
}

TEST(writezpp, busy_wait) {
  NiceMock<MockZUSI> zusi{};
  std::span<uint8_t const> mock_vals{&mock_val, 1u};
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
  ASSERT_TRUE(zusi.writeZpp(mock_addr, mock_vals))
    << "Should continue after no longer busy";
}

TEST(writezpp, ACK) {
  NiceMock<MockZUSI> zusi{};
  std::span<uint8_t const> mock_vals{&mock_val, 1u};
  {
    InSequence seq;
    EXPECT_CALL(
      zusi,
      transmitBytes(
        ElementsAre(0x05u, 0x00u, 0x00u, 0x00u, 0x00u, 0xFFu, 0x0Fu, 0x09u),
        Ne(_0_1)));
    EXPECT_CALL(zusi, transmitBytes(ElementsAre(resync_byte), _0_1));
    EXPECT_CALL(zusi, gpioInput());
    EXPECT_CALL(zusi, readData());                        // ACK valid
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // ACK
    EXPECT_CALL(zusi, readData()).WillOnce(Return(true)); // Busy
    EXPECT_CALL(zusi, spiMaster());
  }
  auto tmp = zusi.writeZpp(mock_addr, mock_vals);
  ASSERT_TRUE(tmp) << "Should return true if command is correct";
}