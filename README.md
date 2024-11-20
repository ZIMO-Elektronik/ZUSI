# :construction: ZUSI :construction:

[![build](https://github.com/ZIMO-Elektronik/ZUSI/actions/workflows/build.yml/badge.svg)](https://github.com/ZIMO-Elektronik/ZUSI/actions/workflows/build.yml) [![tests](https://github.com/ZIMO-Elektronik/ZUSI/actions/workflows/tests.yml/badge.svg)](https://github.com/ZIMO-Elektronik/ZUSI/actions/workflows/tests.yml) [![license](https://img.shields.io/github/license/ZIMO-Elektronik/ZUSI)](https://github.com/ZIMO-Elektronik/ZUSI/raw/master/LICENSE)

<img src="data/images/logo.jpg" width="80" align="right"/>

ZUSI is a ZIMO specific protocol for the SUSI bus. It supports downloading soundprojects in the .zpp format to multiple decoders in parallel with up to 10MBaud. In addition, ZUSI supports fast reading and writing of CVs for individual decoders. Complete documentation of the protocol (German) can be found in the `/docs` folder.

## ZUSI-Frame

ZUSI uses a command specific frame structure. The first byte of each frame marks the used command, all subsequent bytes will be sent according to frame description. 

After each frame sent by the host, the connected decoders have an answer window. 

### CV Read
Reads CV values from a decoder. Up to 256 continuous CVs can be read in one call. 

|Length|Name         |Value / Limits|Desctiption|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x01          |Command code|
|1 Byte|Count - 1    |0 - 255 (=N-1)|Count of requested CVs - 1 -> up to 256 CVs per call|
|4 Byte|CV Adress    |0 - 1024      |Adress of the first CV. Currently this is restricted to a value between 0 - 1024 (May 2014)|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation before the Ack, with a 10µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|
|N Byte|CV values    |---           |Values of requested CVs in order (1 Byte each)|
|1 Byte|CRC          |---           |CRC8 Checksum|

### CV Write
Writes CV values to a decoder. Up to 256 continuous CVs can be written in one call

|Length|Name         |Value / Limits|Desctiption|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x02          |Command code|
|1 Byte|Count - 1    |0 - 255       |Count of CVs to write -> up to 256 CVs per call|
|4 Byte|Count - 1    |0 - 1024      |Adress of the first CV. Currently this is restricted to a value between 0 - 1024 (May 2014)|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation before the Ack, with a 10µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|

### Request flash block size
Requests the flash block size of the decoder. This will give a size limit for data packages targetting this decoder.

|Length|Name         |Value / Limits|Desctiption|
|:----:|:---------   |:------------:|:----------|
|1 Byte|Command      |0x03          |Command code|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation befor the Ack, with a 10 µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|
|1 Byte|Block size   |---           |Block size of the decoder flash|

### Delete flash
Deletes the flash memory of the decoder. 

|Length|Name         |Value / Limits|Desctiption|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x04          |Command code|
|1 Byte|Security byte|0x55          ||
|1 Byte|Security byte|0xAA          ||
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation befor the Ack, with a 10 µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|

### Write flash
Writes data directly to the flash of the decoder

|Length|Name         |Value / Limits|Desctiption|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x05          |Command code|
|1 Byte|Size         |---           |Size of the data block to write into the decoder flash|
|4 Byte|Adress       |---           |Absolute flash adress of the first byte of sent flash block|
|x Byte|Data         |up to 256 Byte|Block data to be written to decoder flash|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation befor the Ack, with a 10 µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|

### Feature request
Requests the transmission capabilities of connected decoders. Feature bits (see Baudrate and Multiple) will be pulled to 0 when at least one decoder does not support the corresponding feature. 

|Length|Name         |Value / Limits|Desctiption|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x06          |Command code|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation befor the Ack, with a 10 µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|
|1 Byte|Baudrate     |7 -> 1<br>6 -> 1<br>5 -> 1<br>4 -> 1<br>3 -> 1<br>2 -> 0<br>_ -> 1<br>1 -> 0<br>_ -> 1<br>0 -> 0<br>_ -> 1|<br><br><br><br><br>0,5533µs timing - supported<br>_______________ - unsuported<br>0,733µs timing - supported<br>______________ - unsuported<br>3,5µs timing (fallback) - supported<br>____________________ - unsupported|
|1 Byte|N/A          |0xFF           ||
|1 Byte|N/A          |0xFF           ||
|1 Byte|Multiple     |7 -> 0<br>_ -> 1<br>6 -> 1<br>5 -> 1<br>4 -> 1<br>3 -> 1<br>2 -> 1<br>1 -> 1<br>0 -> 1|Multiple devices are not suppeorted<br>Multiple devices are supported<br><br><br><br><br><br><br><br>

### Exit
This will exit the ZUSI mode and resume normal DCC operation

|Length|Name         |Value / Limits|Desctiption|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x07          |Command code|
|1 Byte|Security Byte|0x55          ||
|1 Byte|Security Byte|0xFF          ||
|1 Byte|Option       |---           |Will be masked with 0xF8:<br>0 -> 0 --> Reboot<br>0 -> 1 --> No reboot<br>1 -> 0 --> CV8 reset<br>1 -> 1 --> CV8 no reset|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation befor the Ack, with a 10 µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|


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