# Changelog

## 0.7.1
- Bugfix CMake always includes tests

## 0.7.0
- Consistent spelling of 'load code'

## 0.6.1
- Update to CMakeModules 0.1.2

## 0.6.0
- Rename methods *Flash to *Zpp
- Exit takes multiple flags
- rx::Base::execute returns
- Methods which potentially mutate state are no longer const
  - rx::Base::writeCv
  - rx::Base::eraseZpp
  - rx::Base::writeZpp
  - rx::Base::exit

## 0.5.3
- Use CPM.cmake

## 0.5.2
- Communication not reliable without explicit switching between SPI and GPIO

## 0.5.1
- CV read/write uses address instead of index (conceptual)

## 0.5.0
- rx::Base does not explicitly switch between GPIO and SPI

## 0.4.0
- [Semantic versioning](https://semver.org)
- Renamed namespace receive to rx

## 0.3
- Removed CMake exports

## 0.2
- Added encrypt command

## 0.1
- First release