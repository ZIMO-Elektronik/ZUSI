#include "rx_test.hpp"

using namespace std::chrono_literals;

TEST_F(RxTest, cv_read) {
  zusi::Packet packet{
    std::to_underlying(zusi::Command::CvRead), 0u, 0u, 0u, 0u, 0u};

  EXPECT_CALL(_mock, receiveByte())
    .WillOnce(Return(packet[0uz]))
    .WillOnce(Return(packet[1uz]))
    .WillOnce(Return(packet[2uz]))
    .WillOnce(Return(packet[3uz]))
    .WillOnce(Return(packet[4uz]))
    .WillOnce(Return(packet[5uz]))
    .WillOnce(Return(zusi::crc8(packet)))
    .WillOnce(Return(zusi::resync_byte))
    .WillRepeatedly(Return(std::nullopt));
  EXPECT_CALL(_mock, gpio()).Times(1);
  EXPECT_CALL(_mock, waitClock(_)).WillRepeatedly(Return(true));
  EXPECT_CALL(_mock, readCv(0u));

  RunFor(100ms);
}