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

inline constexpr uint32_t mock_addr{0x000000FFu};
inline constexpr uint8_t mock_val{0x0F};

TEST_F(TxTest, cv_write_no_ack_valid) {
  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // not ACK valid
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_FALSE(_mock.writeCv(mock_addr, mock_val))
    << "Should abort if not ACK valid";
}

TEST_F(TxTest, cv_write_nak) {
  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData()); // ACK valid
  EXPECT_CALL(_mock, readData()); // NAK
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_FALSE(_mock.writeCv(mock_addr, mock_val)) << "Should abort after NAK";
}

TEST_F(TxTest, cv_write_busy_wait) {
  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData());                        // ACK valid
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // ACK
  EXPECT_CALL(_mock, readData()).Times(5);               // Busy
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // Busy End
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_TRUE(_mock.writeCv(mock_addr, mock_val))
    << "Should continue after no longer busy";
}

TEST_F(TxTest, cv_write_ack) {
  InSequence seq;
  EXPECT_CALL(
    _mock,
    transmitBytes(
      ElementsAre(0x02u, 0x00u, 0x00u, 0x00u, 0x00u, 0xFFu, 0x0Fu, 0xBAu),
      Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData());                        // ACK valid
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // ACK
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // Busy
  EXPECT_CALL(_mock, spiMaster());

  auto tmp = _mock.writeCv(mock_addr, mock_val);
  ASSERT_TRUE(tmp) << "Should return true if command is correct";
}
