#include <gtest/gtest.h>
#include <zusi/zusi.hpp>

TEST(crc8, data) {
  constexpr std::array<uint8_t, 12u> data{0x0Bu,
                                          0x0Au,
                                          0x00u,
                                          0x00u,
                                          0x8Eu,
                                          0x40u,
                                          0x00u,
                                          0x0Du,
                                          0x67u,
                                          0x00u,
                                          0x01u,
                                          0x00u};
  EXPECT_EQ(zusi::crc8(data), 0x4Cu);
}

TEST(crc8, string) {
  std::string const str{"Hello World!"};
  std::vector<uint8_t> v;
  std::ranges::copy(str, std::back_inserter(v));
  EXPECT_EQ(zusi::crc8(v), 0x9Eu);
}
