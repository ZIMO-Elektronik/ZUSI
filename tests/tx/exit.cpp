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

TEST_F(TxTest, exit_no_ack_valid) {
  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // not ACK valid
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // invalid ACK
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_FALSE(_mock.exit(_exit_flags)) << "Should abort if not ACK valid";
}

TEST_F(TxTest, exit_nak) {
  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData()); // ACK valid
  EXPECT_CALL(_mock, readData()); // NAK
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_FALSE(_mock.exit(_exit_flags)) << "Should abort after NAK";
}

TEST_F(TxTest, exit_busy_wait) {
  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData());                        // ACK valid
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // ACK
  EXPECT_CALL(_mock, readData()).Times(5);               // Busy
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // Busy End
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_TRUE(_mock.exit(_exit_flags))
    << "Should continue after no longer busy";
}

TEST_F(TxTest, exit_ack) {
  InSequence seq;
  EXPECT_CALL(
    _mock,
    transmitBytes(ElementsAre(0x07u, 0x55u, 0xAAu, 0x02u, 0x7Du), _0_286));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData());                        // ACK valid
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // ACK
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // Busy
  EXPECT_CALL(_mock, spiMaster());

  auto tmp{_mock.exit(_exit_flags)};
  ASSERT_TRUE(tmp) << "Should return true if command is correct";
}
