#include "rx_test.hpp"

using namespace std::chrono_literals;

// https://github.com/ZIMO-Elektronik/ZUSI/issues/10
TEST_F(RxTest, exit_ignored_security_bytes_and_crc) {
  zusi::Packet packet{std::to_underlying(zusi::Command::Exit), 1u, 2u, 0xFFu};

  EXPECT_CALL(_mock, receiveByte())
    .WillOnce(Return(packet[0uz]))
    .WillOnce(Return(packet[1uz]))
    .WillOnce(Return(packet[2uz]))
    .WillOnce(Return(packet[3uz]))
    .WillOnce(Return(zusi::crc8(packet)))
    .WillOnce(Return(zusi::resync_byte))
    .WillRepeatedly(Return(std::nullopt));
  EXPECT_CALL(_mock, gpioOutput()).Times(1);
  EXPECT_CALL(_mock, waitClock(_)).WillRepeatedly(Return(true));
  EXPECT_CALL(_mock, exit(0xFFu)).Times(0);

  RunFor(100ms);
}

TEST_F(RxTest, exit) {
  zusi::Packet packet{
    std::to_underlying(zusi::Command::Exit), 0x55u, 0xAAu, 0xFFu};

  EXPECT_CALL(_mock, receiveByte())
    .WillOnce(Return(packet[0uz]))
    .WillOnce(Return(packet[1uz]))
    .WillOnce(Return(packet[2uz]))
    .WillOnce(Return(packet[3uz]))
    .WillOnce(Return(zusi::crc8(packet)))
    .WillOnce(Return(zusi::resync_byte))
    .WillRepeatedly(Return(std::nullopt));
  EXPECT_CALL(_mock, gpioOutput()).Times(1);
  EXPECT_CALL(_mock, waitClock(_)).WillRepeatedly(Return(true));
  EXPECT_CALL(_mock, exit(0xFFu)).Times(1);

  RunFor(100ms);
}