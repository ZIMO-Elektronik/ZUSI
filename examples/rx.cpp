#include <zusi/zusi.hpp>

class ZppLoad : public zusi::rx::Base {
  // Receive a byte
  std::optional<uint8_t> receiveByte() const final { return 0u; }

  // Read a CV at address
  uint8_t readCv(uint32_t addr) const final { return 0u; }

  // Write a CV at address
  void writeCv(uint32_t addr, uint8_t value) final {}

  // Erase ZPP
  void eraseZpp() final {}

  // Write ZPP
  void writeZpp(uint32_t addr, std::span<uint8_t const> bytes) final {}

  // Return value of feature request
  zusi::Features features() const final { return {}; }

  // Exit
  [[noreturn]] void exit(uint8_t flags) final {}

  // Check if the load code is valid
  bool loadCodeValid(std::span<uint8_t const, 4uz> developer_code) const final {
    return true;
  }

  // Check if the received address is valid
  bool addressValid(uint32_t addr) const final { return true; }

  // Wait till clock pin equals state with a resync timeout
  bool waitClock(bool state) const final { return true; }

  // Set or clear data pin
  void writeData(bool state) const final {}

  // Optional, blink front- and rear lights
  void toggleLights() const final {}

  /// Switch to SPI
  void spi() const final {}

  /// Switch to GPIO
  void gpio() const final {}
};

int main() {
  ZppLoad zpp_load{};
  zpp_load.execute();
}