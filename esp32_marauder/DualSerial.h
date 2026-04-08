#pragma once

/*
 * DualSerial - wrapper that mirrors all output to USB Serial (UART0)
 * and Flipper GPIO Serial (UART2).
 *
 * This file is kept as a historical artifact for the article.
 * It is not used by the final firmware.
 */

#include <Arduino.h>

// Capture reference to the real Serial before any macro override.
static HardwareSerial& __realSerial __attribute__((unused)) = Serial;

class DualSerial : public Stream {
public:
  HardwareSerial& _usb;
  HardwareSerial _flipper;

  DualSerial() : _usb(__realSerial), _flipper(2) {}

  void begin(unsigned long baud) {
    _usb.begin(baud);
    _flipper.begin(baud, SERIAL_8N1, /*RX=*/36, /*TX=*/26);
  }

  void begin(unsigned long baud, uint32_t config) {
    _usb.begin(baud, config);
    _flipper.begin(baud, SERIAL_8N1, /*RX=*/36, /*TX=*/26);
  }

  explicit operator bool() const {
    return (bool)_usb || (bool)_flipper;
  }

  size_t write(uint8_t c) override {
    size_t n = _usb.write(c);
    _flipper.write(c);
    return n;
  }

  size_t write(const uint8_t* buf, size_t size) override {
    size_t n = _usb.write(buf, size);
    _flipper.write(buf, size);
    return n;
  }

  int available() override {
    int a = _flipper.available();
    if (a > 0) return a;
    return _usb.available();
  }

  int read() override {
    if (_flipper.available() > 0)
      return _flipper.read();
    if (_usb.available() > 0)
      return _usb.read();
    return -1;
  }

  int peek() override {
    if (_flipper.available() > 0)
      return _flipper.peek();
    if (_usb.available() > 0)
      return _usb.peek();
    return -1;
  }

  String readString() {
    if (_flipper.available() > 0)
      return _flipper.readString();
    return _usb.readString();
  }

  String readStringUntil(char terminator) {
    if (_flipper.available() > 0)
      return _flipper.readStringUntil(terminator);
    return _usb.readStringUntil(terminator);
  }

  void flush() override {
    _usb.flush();
    _flipper.flush();
  }

  void setTimeout(unsigned long timeout) {
    Stream::setTimeout(timeout);
    _usb.setTimeout(timeout);
    _flipper.setTimeout(timeout);
  }
};
