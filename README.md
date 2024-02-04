# ZUSI

<img src="data/images/logo.jpg" width="80" align="right"/>

ZUSI is a ZIMO specific protocol for the SUSI bus. It supports downloading soundprojects in the .zpp format to multiple decoders in parallel with up to 10MBaud. In addition, ZUSI supports fast reading and writing of CVs for individual decoders. Complete documentation of the protocol (German) can be found in the `/docs` folder.

## Beispiele
Currently there is just a single class intended for use. This class is
- zusi::rx::Base

The class relies on classical polymorphism and requires a set of virtual functions to be implemented.
```cpp
#include <zusi/zusi.hpp>

class ZppLoad : public zusi::rx::Base {
  // Receive a byte
  bool receiveByte(uint8_t* const dest) const final {}

  // Read a CV at address
  uint8_t readCv(uint32_t addr) const final {}

  // Write a CV at address
  void writeCv(uint32_t addr, uint8_t value) final {}

  // Delete entire soundflash
  void eraseZpp() final {}

  // Write soundflash
  void writeZpp(uint32_t addr, std::span<uint8_t const> bytes) final {}

  // Return value of feature request
  ::zusi::Features features() const final {}

  // Exit
  [[noreturn]] void exit(uint8_t flags) final {}

  // Check if the load code is valid
  bool loadCodeValid(std::span<uint8_t const, 4u> developer_code) const final {}

  // Check if the received address is valid
  bool addressValid(uint32_t addr) const final {}

  // Wait till clock pin equals state with a resync timeout
  bool waitClock(bool state) const final {}

  // Set or clear data pin
  void writeData(bool state) const final {}

  // Optional, blink front- and rearlights
  void toggleLights() const final {}

  /// Switch to SPI
  void spi() const final {}

  /// Switch to GPIO
  void gpio() const final {}
};
```