#include "PT2323.h"

PT2323::PT2323(TwoWire *wire)
{
  _wire = wire;
}

bool PT2323::begin(uint32_t startupDelayMs)
{
  if (_wire == nullptr) { _error = 4; return false; }
  // Datasheet: at least 300 ms after power-up for Cref = 10 uF.
  delay(startupDelayMs);
  _wire->setClock(100000);
  if (!isConnected()) return false;
  // Keep outputs muted while selecting deterministic startup settings.
  if (!muteAll(true)) return false;
  if (!setChannel(0)) return false;
  if (!setSurround(false)) return false;
  if (!setMixedChannel(false)) return false;
  if (!setMuteMask(0)) return false;
  return muteAll(false);
}

uint8_t PT2323::getAddress()
{
  return _address;
}

bool PT2323::isConnected()
{
  if (_wire == nullptr) { _error = 4; return false; }
  _wire->beginTransmission(_address);
  _error = _wire->endTransmission();
  return _error == 0;
}

//
//  CHANNEL
//
bool PT2323::setChannel(uint8_t channel)
{
  if (channel > 3 && channel != 6)
  {
    _error = -1;
    return false;
  }

  uint8_t cmd = 0;
  if      (channel == 6) cmd = 0xC7;
  else if (channel == 3) cmd = 0xC8;
  else if (channel == 2) cmd = 0xC9;
  else if (channel == 1) cmd = 0xCA;
  else if (channel == 0) cmd = 0xCB;

  if (write(cmd) == 0)
  {
    _channel = channel;
    return true;
  }
  return false;
}

uint8_t PT2323::getChannel()
{
  return _channel;
}

bool PT2323::muteFrontLeft(bool mute)
{
  uint8_t cmd = 0xF0;
  if (mute) cmd |= 0x01;
  return write(cmd) == 0;
}

bool PT2323::muteFrontRight(bool mute)
{
  uint8_t cmd = 0xF2;
  if (mute) cmd |= 0x01;
  return write(cmd) == 0;
}

bool PT2323::muteCenter(bool mute)
{
  uint8_t cmd = 0xF4;
  if (mute) cmd |= 0x01;
  return write(cmd) == 0;
}

bool PT2323::muteSubWoofer(bool mute)
{
  uint8_t cmd = 0xF6;
  if (mute) cmd |= 0x01;
  return write(cmd) == 0;
}

bool PT2323::muteRearLeft(bool mute)
{
  uint8_t cmd = 0xF8;
  if (mute) cmd |= 0x01;
  return write(cmd) == 0;
}

bool PT2323::muteRearRight(bool mute)
{
  uint8_t cmd = 0xFA;
  if (mute) cmd |= 0x01;
  return write(cmd) == 0;
}

bool PT2323::muteAll(bool mute)
{
  uint8_t cmd = 0xFE;
  if (mute) cmd |= 0x01;
  return write(cmd) == 0;
}

bool PT2323::setSurround(bool enable)
{
  uint8_t cmd = 0xD0;
  if (!enable) cmd |= 0x01;
  if (write(cmd) == 0)
  {
    _surround = enable;
    return true;
  }
  return false;
}

bool PT2323::getSurround()
{
  return _surround;
}

bool PT2323::setMixedChannel(bool six_db)
{
  uint8_t cmd = 0x90;
  if (six_db) cmd |= 0x01;
  if (write(cmd) == 0)
  {
    _mixedChannel = six_db;
    return true;
  }
  return false;
}

bool PT2323::getMixedChannel()
{
  return _mixedChannel;
}

int PT2323::getLastError()
{
  int e = _error;
  _error = 0;
  return e;
}

bool PT2323::setMuteMask(uint8_t muteMask, uint8_t updateMask)
{
  if ((muteMask & 0xC0) || (updateMask & 0xC0)) { _error = -1; return false; }
  uint8_t bytes[6];
  size_t count = 0;
  for (uint8_t channel = 0; channel < 6; ++channel) {
    if (updateMask & (1U << channel)) {
      bytes[count++] = 0xF0 | (channel << 1) | ((muteMask >> channel) & 1);
    }
  }
  return writeBytes(bytes, count) == 0;
}

int PT2323::write(uint8_t cmd)
{
  return writeBytes(&cmd, 1);
}

int PT2323::writeBytes(const uint8_t *values, size_t count)
{
  if (_wire == nullptr) { _error = 4; return _error; }
  if (!count) { _error = 0; return 0; }
  _wire->beginTransmission(_address);
  bool buffered = true;
  for (size_t i = 0; i < count; ++i) {
    if (_wire->write(values[i]) != 1) { buffered = false; break; }
  }
  if (!buffered) _wire->flush(); // ESP32: discard an incomplete TX batch.
  _error = _wire->endTransmission();
  if (!buffered && _error == 0) _error = 1;
  return _error;
}
