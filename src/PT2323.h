#pragma once

#include "Arduino.h"
#include "Wire.h"


#define PT2323_LIB_VERSION                   (F("1.0.1"))


class PT2323
{
public:
  PT2323(TwoWire *wire = &Wire);

  // Call Wire.begin() first. Use a longer delay if Cref requires it.
  bool    begin(uint32_t startupDelayMs = 300);
  bool    isConnected();
  uint8_t getAddress();

  //  CHANNEL = 0..3 or 6
  bool    setChannel(uint8_t channel = 0);
  uint8_t getChannel();

  
  bool    muteFrontLeft(bool mute);
  bool    muteFrontRight(bool mute);
  bool    muteCenter(bool mute);
  bool    muteSubWoofer(bool mute);
  bool    muteRearLeft(bool mute);
  bool    muteRearRight(bool mute);
  bool    muteAll(bool mute);

  
  bool    setSurround(bool enable);
  bool    getSurround();

  
  bool    setMixedChannel(bool six_db);
  bool    getMixedChannel();

  int     getLastError();

private:
  int     write(const uint8_t value);

  TwoWire  *_wire;
  uint8_t  _address   = 0x4A;

  uint8_t  _channel   = 0;
  bool     _surround  = false;
  bool     _mixedChannel = false;
  int      _error = 0;
};