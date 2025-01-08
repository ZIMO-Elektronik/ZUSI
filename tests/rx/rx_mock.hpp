#pragma once

#include <gmock/gmock.h>
#include <zusi/zusi.hpp>

class RxMock : public zusi::rx::Base {
public:
  MOCK_METHOD(uint8_t, readCv, (uint32_t), (const, override));
  MOCK_METHOD(void, writeCv, (uint32_t, uint8_t), (override));
  MOCK_METHOD(void, eraseZpp, (), (override));
  MOCK_METHOD(void, writeZpp, (uint32_t, std::span<uint8_t const>), (override));
  MOCK_METHOD(zusi::Features, features, (), (const, override));
  MOCK_METHOD(void, exit, (uint8_t), (override));
  MOCK_METHOD(bool,
              loadCodeValid,
              ((std::span<uint8_t const, 4uz>)),
              (const, override));
  MOCK_METHOD(bool, addressValid, (uint32_t), (const, override));
  MOCK_METHOD(std::optional<uint8_t>, receiveByte, (), (const, override));
  MOCK_METHOD(bool, waitClock, (bool), (const, override));
  MOCK_METHOD(void, writeData, (bool), (const, override));
  MOCK_METHOD(void, toggleLights, (), (const, override));
  MOCK_METHOD(void, spi, (), (const, override));
  MOCK_METHOD(void, gpio, (), (const, override));
};