#include "rx_test.hpp"

using namespace std::chrono_literals;

TEST_F(RxTest, zpp_lc_dc_query) {
  zusi::Packet packet{
    std::to_underlying(zusi::Command::ZppLcDcQuery), 1u, 2u, 3u, 4u};

  EXPECT_CALL(_mock, receiveByte())
    .WillOnce(Return(packet[0uz]))
    .WillOnce(Return(packet[1uz]))
    .WillOnce(Return(packet[2uz]))
    .WillOnce(Return(packet[3uz]))
    .WillOnce(Return(packet[4uz]))
    .WillOnce(Return(zusi::crc8(packet)))
    .WillOnce(Return(zusi::resync_byte))
    .WillRepeatedly(Return(std::nullopt));
  EXPECT_CALL(_mock, gpioOutput()).Times(1);
  EXPECT_CALL(_mock, waitClock(_)).WillRepeatedly(Return(true));
  EXPECT_CALL(_mock, loadCodeValid(ElementsAre(1u, 2u, 3u, 4u)))
    .WillOnce(Return(true));

  InSequence seq;

  EXPECT_CALL(_mock, writeData(_))
    .Times(1 + // ack_valid
           1 + // ack
           1 + // busy
           1)  // busy
    .RetiresOnSaturation();

  // Load code valid
  EXPECT_CALL(_mock, writeData(true)).Times(1).RetiresOnSaturation();  // Bit 0
  EXPECT_CALL(_mock, writeData(false)).Times(1).RetiresOnSaturation(); // Bit 1
  EXPECT_CALL(_mock, writeData(false)).Times(1).RetiresOnSaturation(); // Bit 2
  EXPECT_CALL(_mock, writeData(false)).Times(1).RetiresOnSaturation(); // Bit 3
  EXPECT_CALL(_mock, writeData(false)).Times(1).RetiresOnSaturation(); // Bit 4
  EXPECT_CALL(_mock, writeData(false)).Times(1).RetiresOnSaturation(); // Bit 5
  EXPECT_CALL(_mock, writeData(false)).Times(1).RetiresOnSaturation(); // Bit 6
  EXPECT_CALL(_mock, writeData(false)).Times(1).RetiresOnSaturation(); // Bit 7

  EXPECT_CALL(_mock, writeData(_)).Times(CHAR_BIT).RetiresOnSaturation(); // CRC

  RunFor(100ms);
}