#include <zusi/zusi.hpp>

class Receiver : public zusi::rx::Base {
  // Receive a byte
  std::optional<uint8_t> receiveByte() const final { return 0u; }

  // Read a CV at address
  uint8_t readCv(uint32_t addr) const final { return 0u; }

  // Write a CV at address
  void writeCv(uint32_t addr, uint8_t byte) final {}

  // Erase ZPP
  void eraseZpp() final {}

  // Write ZPP
  void writeZpp(uint32_t addr, std::span<uint8_t const> bytes) final {}

  // Return value of features query
  zusi::Features features() const final { return {}; }

  // Exit
  void exit(uint8_t flags) final {}

  // Check if the load code is valid
  bool loadCodeValid(std::span<uint8_t const, 4uz> developer_code) const final {
    return true;
  }

  // Check if the received address is valid
  bool addressValid(uint32_t addr) const final { return true; }

  // Wait till clock pin equals state with a resync timeout
  bool waitClock(bool state) const final { return true; }

  // Write data line
  void writeData(bool state) const final {}

  // Switch to SPI slave
  void spiSlave() const final {}

  // Switch to GPIO output
  void gpioOutput() const final {}

  // Optional, blink front- and rear lights
  void toggleLights() const final {}
};

class Transmitter : public zusi::tx::Base {
  /// Transmit byte at specific transmission speed
  void transmitBytes(std::span<uint8_t const> bytes,
                     zusi::Mbps mbps) const final {}

  // Switch to SPI master
  void spiMaster() const final {}

  // Switch to GPIO input
  void gpioInput() const final {}

  // Switch to GPIO output
  void gpioOutput() const final {}

  // Write clock line
  void writeClock(bool state) const final {}

  // Write data line
  void writeData(bool state) const final {}

  // Read data line
  bool readData() const final { return true; }

  // Delay microseconds
  void delayUs(uint32_t us) const final {}

  /// Busy phase
  ///
  /// \note Default impl. will block until done
  virtual void busy() const;
};

int main() {
  Receiver rx{};
  rx.receive();

  Transmitter tx{};
  tx.transmit(zusi::Packet{});
}