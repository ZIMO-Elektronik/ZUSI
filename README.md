# ZUSI

[![build](https://github.com/ZIMO-Elektronik/ZUSI/actions/workflows/build.yml/badge.svg)](https://github.com/ZIMO-Elektronik/ZUSI/actions/workflows/build.yml) [![tests](https://github.com/ZIMO-Elektronik/ZUSI/actions/workflows/tests.yml/badge.svg)](https://github.com/ZIMO-Elektronik/ZUSI/actions/workflows/tests.yml) [![license](https://img.shields.io/github/license/ZIMO-Elektronik/ZUSI)](https://github.com/ZIMO-Elektronik/ZUSI/raw/master/LICENSE)

<img src="data/images/logo.jpg" width="80" align="right"/>

ZUSI is a ZIMO specific protocol for the [SUSI](https://normen.railcommunity.de/RCN-600.pdf) bus. It supports [ZPP](https://github.com/ZIMO-Elektronik/ZPP) updates on multiple decoders in parallel with up to 1.8MBaud. In addition, it also supports fast reading and writing of CVs for individual decoders. The protocol is currently supported by the following products:
- Command stations
  - [ZIMO MXULF](https://www.zimo.at/web2010/products/InfMXULF_EN.htm)
- Decoders
  - [ZIMO MN decoders](http://www.zimo.at/web2010/products/mn-nicht-sound-decoder_EN.htm)
  - [ZIMO small-](http://www.zimo.at/web2010/products/ms-sound-decoder_EN.htm) and [large-scale MS decoders](http://www.zimo.at/web2010/products/ms-sound-decoder-grossbahn_EN.htm)
  - [ZIMO small-](http://www.zimo.at/web2010/products/lokdecoder_EN.htm) and [large-scale MX decoders](http://www.zimo.at/web2010/products/lokdecodergrosse_EN.htm)

<details>
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#protocol">Protocol</a></li>
      <ul>
        <li><a href="#electrical-specification">Electrical Specification</a></li>
        <li><a href="#peripheral-configuration">Peripheral Configuration</a></li>
        <li><a href="#establishing-a-connection">Establishing a Connection</a></li>
        <li><a href="#general-data-transfer">General Data Transfer</a></li>
        <li><a href="#commands">Commands</a></li>
      </ul>
    <li><a href="#getting-started">Getting Started</a></li>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
        <li><a href="#build">Build</a></li>
      </ul>
    <li><a href="#usage">Usage</a></li>
  </ol>
</details>

## Protocol
### Electrical Specification
The ZUSI uses a combination of peripheral functions. Standard SPI (Mode1) is used for data transmission, GPIO I/O is used for decoder answer and busy phase. 

### Peripheral Configuration
For ZUSI to function correctly, the SPI peripheral needs to be configured correctly. For ZUSI, the SPI uses SPIMode 1 (CPOL 0, CPHA 1) and the transmissions will be SPI TX-only. The SPI needs to operate in LSB first mode

Also, the protocol requires the use of four different SPI clock frequencies: 

| Clock Period [µs] | Bit Rate [Mbps] | Description                                         |
| ----------------- | --------------- | --------------------------------------------------- |
| 10                | 0.1             | Resync timing                                       |
| 3.5               | 0.286           | Fallback timing, slowest used for data transmission |
| 0.733             | 1.364           | 2. fastest possible                                 |
| 0.5533            | 1.807           | Fastest possible                                    |

These timings do not need to be matched precisely, it is acceptable to achieve a slightly slower data-rate.

In addition to the SPI configuration, all used pins on either host or slave need to be connected with pull-up resistors. 

### Establishing a Connection
To connect devices to the host, the host needs to send 0x55 (or 0xAA) for at least a second with a clock period fixed at 10ms. This is necessary to allow the decoder to evaluate the signal during its normal operation. 

### General Data Transfer
The ZUSI clock is given by the host with variable period (see SPI frequency table). The Protocol allows the connected devices to return an answer in a specified time window (see ZUSI frame tables). In accordance to the SPI mode, data is clocked out on a rising clock edge and read on a falling clock edge. 

The speed of transmission can be increased if the transmission stays stable to maximize data rate, but must be reduced if the transmissions become too fast for the connected devices. 

To simplify finding the correct transmission speed, the FeatureRequest command was implemented to ask the maximum supported transmission speed of the devices. Details can be found in the Feature request frame table.

#### Resynchronisation Phase
To avoid problems with the MX644, a resynchronisation phase was introduced between host transmission and device answer. This consists of a 10µs delay, with a following transmission of 0x80. The clock period for the transmission is 10µs, which results in a frequency of 0.1Mbps (or 12.5kBaud).

To catch asynchronous behavior, the state-machine of a decoder will be reset after 10ms of no activity on ZUSI clock. This can be used to resync all decoders. 

#### Answer Phase
After the last bit of the host transmission (after the resynchronisation phase), the host switches the ZUSI data line to input (with pull-up). The decoder will send a two bit answer, one ACK valid and one ACK (in this order). 

ACK valid is low. This verifies that at least one decoder is still connected and received the transmission. If the ACK valid is high, no decoder was able to receive the transmission. (Wired-OR) 

ACK is high (NAK is low), if all decoders were able to complete the command. If a decoder was unable to complete the command, this will be pulled to low for all connected devices (Wired-AND)

To integrate the MX644, the answer phase is asymmetric. The low period of clock is defined with 20µs, the high period with 10µs. Both ACK valid and ACK/NAK will be set by the decoder on a rising edge on clock and can be read on the falling edge. 

#### Busy Phase
Some commands (e.g. DeleteFlash or WriteFlash) have built-in busy to signal the command still being processed on at least one decoder. This is achieved through the decoder pulling the data line to logical low and holding this state. When the decoder is finished, it needs to release the data line again, but only after the clock line is high again (signaling that the answer phase is complete). This results in a "Wired-AND", which will result in a logical high only if all decoders are finished with the command. The host clock is suspended during the busy phase. 

### Commands
ZUSI uses a command specific frame structure. The first byte of each frame marks the used command, all subsequent bytes will be sent according to frame description. 

After each frame sent by the host, the connected decoders have an answer window. 

#### CV Read
Reads CV values from a decoder. The ZUSI specification permits up to 256 CVs to be read in one command, however this library only admits one CV at a time. 

|Length|Name         |Value / Limits|Description|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x01          |Command code|
|1 Byte|Count - 1    |0 - 255 (=N-1)|Count of requested CVs - 1 -> up to 256 CVs per call|
|4 Byte|CV Address    |0 - 1024      |Address of the first CV. Currently this is restricted to a value between 0 - 1024 (May 2014)|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation before the ACK, with a 10µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|
|N Byte|CV values    |---           |Values of requested CVs in order (1 Byte each)|
|1 Byte|CRC          |---           |CRC8 Checksum|

#### CV Write
Writes CV values to a decoder. The ZUSI specification permits up to 256 CVs to be read in one command, however this library only admits one CV at a time. 

|Length|Name         |Value / Limits|Description|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x02          |Command code|
|1 Byte|Count - 1    |0 - 255       |Count of CVs to write -> up to 256 CVs per call|
|4 Byte|CV Address    |0 - 1024      |Address of the first CV. Currently this is restricted to a value between 0 - 1024 (May 2014)|
|N Byte|Values (N)   |---           |Values to be written in actual order. N must be same as Count|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation before the ACK, with a 10µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|

#### Request Flash Block Size
Requests the flash block size of the decoder. This will give a size limit for data packages targetting this decoder.

|Length|Name         |Value / Limits|Description|
|:----:|:---------   |:------------:|:----------|
|1 Byte|Command      |0x03          |Command code|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation before the ACK, with a 10 µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|
|1 Byte|Block size   |---           |Block size of the decoder flash|

#### Delete Flash
Deletes the flash memory of the decoder. 

|Length|Name         |Value / Limits|Description|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x04          |Command code|
|1 Byte|Security byte|0x55          ||
|1 Byte|Security byte|0xAA          ||
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation before the ACK, with a 10 µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|

#### Write Flash
Writes data directly to the flash of the decoder

|Length|Name         |Value / Limits|Description|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x05          |Command code|
|1 Byte|Size         |---           |Size of the data block to write into the decoder flash|
|4 Byte|Address       |---           |Absolute flash adress of the first byte of sent flash block|
|x Byte|Data         |up to 256 Byte|Block data to be written to decoder flash|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation before the ACK, with a 10 µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|

#### Feature Request
Requests the transmission capabilities of connected decoders. Feature bits (see Baudrate and Multiple) will be pulled to 0 when at least one decoder does not support the corresponding feature. 

|Length|Name         |Value / Limits|Description|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x06          |Command code|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation before the ACK, with a 10 µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|
|1 Byte|Baudrate     |7 -> 1<br>6 -> 1<br>5 -> 1<br>4 -> 1<br>3 -> 1<br>2 -> 0<br>_ -> 1<br>1 -> 0<br>_ -> 1<br>0 -> 0<br>_ -> 1|<br><br><br><br><br>0,5533µs timing - supported<br>_______________ - unsuported<br>0,733µs timing - supported<br>______________ - unsuported<br>3,5µs timing (fallback) - supported<br>____________________ - unsupported|
|1 Byte|N/A          |0xFF           ||
|1 Byte|N/A          |0xFF           ||
|1 Byte|Multiple     |7 -> 0<br>_ -> 1<br>6 -> 1<br>5 -> 1<br>4 -> 1<br>3 -> 1<br>2 -> 1<br>1 -> 1<br>0 -> 1|Multiple devices are not suppeorted<br>Multiple devices are supported<br><br><br><br><br><br><br><br>

#### Exit
This will exit the ZUSI mode and resume normal DCC operation

|Length|Name         |Value / Limits|Description|
|:----:|:------------|:------------:|:----------|
|1 Byte|Command      |0x07          |Command code|
|1 Byte|Security Byte|0x55          ||
|1 Byte|Security Byte|0xAA          ||
|1 Byte|Option       |---           |Will be masked with 0xF8:<br>0 -> 0 --> Reboot<br>0 -> 1 --> No reboot<br>1 -> 0 --> CV8 reset<br>1 -> 1 --> CV8 no reset|
|1 Byte|CRC          |---           |CRC8 Checksum|
|1 Byte|Resync       |0x80          |Byte used for resynchronisation before the ACK, with a 10 µs delay|
|||||
|1 Bit |ACK valid    |---           |1 = ACK - 0 = NACK|
|1 Bit |ACK          |---           |1 = ACK - 0 = NACK|
|x Bits|Busy         |---           |0 while device is still busy|

#### Encrypt
:construction:

### Typical processes
#### ZPP Update
:construction:

## Getting Started
### Prerequisites
:construction:

### Installation
:construction:

### Build
:construction:

## Usage
To use the ZUSI library, a number of virtual functions must be implemented. 

### Receiver
In case of the receiving side it is necessary to derive from `zusi::rx::Base`.

```cpp
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
```

### Transmitter
:construction: