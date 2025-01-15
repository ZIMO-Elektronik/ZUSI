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

TEST_F(TxTest, cv_read_no_ack_valid) {
  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // not ACK valid
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_FALSE(_mock.readCv(_addr)) << "Should abort if not ACK valid";
}

TEST_F(TxTest, cv_read_nak) {
  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData()); // ACK valid
  EXPECT_CALL(_mock, readData()); // NAK
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_FALSE(_mock.readCv(_addr)) << "Should abort after NAK";
}

TEST_F(TxTest, cv_read_busy_wait) {
  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData());                        // ACK valid
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // ACK
  EXPECT_CALL(_mock, readData()).Times(5);               // Busy
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // Busy End
  EXPECT_CALL(_mock, readData()).Times(8);               // Receive data
  EXPECT_CALL(_mock, readData()).Times(8);               // Receive crc8
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_TRUE(_mock.readCv(_addr)) << "Should continue after no longer busy";
}

TEST_F(TxTest, cv_read_crc8_answer) {
  InSequence seq;
  EXPECT_CALL(_mock, transmitBytes(_, Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData());                        // ACK valid
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // ACK
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // Busy
  EXPECT_CALL(_mock, readData())                         //
    .Times(4)                                            //
    .WillRepeatedly(Return(true));                       // Receive 4 bits 1
  EXPECT_CALL(_mock, readData()).Times(4);               // Receive 4 bits 0
  EXPECT_CALL(_mock, readData()).Times(8);               // Receive crc8
  EXPECT_CALL(_mock, spiMaster());

  ASSERT_FALSE(_mock.readCv(_addr)) << "Should abort if CRC error occurs";
}

TEST_F(TxTest, cv_read_ack) {
  InSequence seq;
  EXPECT_CALL(
    _mock,
    transmitBytes(ElementsAre(0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0xFFu, 0x02u),
                  Ne(_0_1)));
  EXPECT_CALL(_mock, transmitBytes(ElementsAre(resync_byte), _0_1));
  EXPECT_CALL(_mock, gpioInput());
  EXPECT_CALL(_mock, readData());                        // ACK valid
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // ACK
  EXPECT_CALL(_mock, readData()).WillOnce(Return(true)); // Busy
  EXPECT_CALL(_mock, readData()).Times(8);               // Receive data
  EXPECT_CALL(_mock, readData()).Times(8);               // CRC8
  EXPECT_CALL(_mock, spiMaster());

  auto tmp{_mock.readCv(_addr)};
  ASSERT_TRUE(tmp) << "Should return CV value if command is correct";
  ASSERT_EQ(*tmp, 0x00)
    << "Should return data the decoder sent (see Receive data)";
}
