#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <zusi/features.hpp>
#include <zusi/zusi.hpp>

#include <span>

class MockZUSI : public zusi::rx::Base {
public:
  MOCK_METHOD(uint8_t, readCv, (uint32_t));
  MOCK_METHOD(void, writeCv, (uint32_t, uint8_t));
  MOCK_METHOD(void, eraseZpp, ());
  MOCK_METHOD(void, writeZpp, (uint32_t, uint8_t));
  MOCK_METHOD(zusi::Features, features, ());
  MOCK_METHOD(void, exit, (uint8_t));
  MOCK_METHOD(bool, loadCodeValid, ((std::span<uint8_t, 4uz>)));
  MOCK_METHOD(bool, adressValid, (uint32_t));
  MOCK_METHOD(bool, receiveByte, (uint8_t*));
  MOCK_METHOD(bool, waitClock, ());
  MOCK_METHOD(void, writeData, (bool));
  MOCK_METHOD(void, toggleLights, ());
  MOCK_METHOD(void, spi, ());
  MOCK_METHOD(void, gpio, ());
};