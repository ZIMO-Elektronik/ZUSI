#include <gmock/gmock.h>
#include <zusi/zusi.hpp>

using ::zusi::Mbps;
using ::zusi::tx::Base;

class TxMock : public Base {
public:
  MOCK_METHOD(void,
              transmitBytes,
              (std::span<uint8_t const> bytes, Mbps mbps),
              (const, override));
  MOCK_METHOD(void, spiMaster, (), (const, override));
  MOCK_METHOD(void, gpioInput, (), (const, override));
  MOCK_METHOD(void, gpioOutput, (), (const, override));
  MOCK_METHOD(void, writeClock, (bool state), (const, override));
  MOCK_METHOD(void, writeData, (bool state), (const, override));
  MOCK_METHOD(bool, readData, (), (const, override));
  MOCK_METHOD(void, delayUs, (uint32_t us), (const, override));
};
