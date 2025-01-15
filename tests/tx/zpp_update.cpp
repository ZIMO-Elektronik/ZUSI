#include "tx_test.hpp"

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

inline constexpr uint32_t _addr{0x000000FFu};
inline constexpr uint8_t mock_val{0x0Fu};

TEST_F(TxTest, zpp_update_no_ack_valid) {
  std::span<uint8_t const> mock_vals{&mock_val, 1u};

  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // not ACK valid
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_FALSE(_mock.writeZpp(_addr, mock_vals))
    << "Should abort if not ACK valid";
}

TEST_F(TxTest, zpp_update_nak) {
  std::span<uint8_t const> mock_vals{&mock_val, 1u};

  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData()); // ACK valid
  EXPECT_CALL(_mock, readData()); // NAK
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_FALSE(_mock.writeZpp(_addr, mock_vals)) << "Should abort after NAK";
}

TEST_F(TxTest, zpp_update_busy_wait) {
  std::span<uint8_t const> mock_vals{&mock_val, 1u};

  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData());                        // ACK valid
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // ACK
  EXPECT_CALL(_mock, readData()).Times(5);               // Busy
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // Busy End
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_TRUE(_mock.writeZpp(_addr, mock_vals))
    << "Should continue after no longer busy";
}

TEST_F(TxTest, zpp_update_ack) {
  std::span<uint8_t const> mock_vals{&mock_val, 1u};

  InSequence seq;
  EXPECT_CALL(
    _mock,
    transmitBytes(
      ElementsAre(0x05u, 0x00u, 0x00u, 0x00u, 0x00u, 0xFFu, 0x0Fu, 0x09u),
      Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData());                        // ACK valid
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // ACK
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // Busy
  EXPECT_CALL(_mock, spiMaster());

  auto tmp{_mock.writeZpp(_addr, mock_vals)};
  ASSERT_TRUE(tmp) << "Should return true if command is correct";
}