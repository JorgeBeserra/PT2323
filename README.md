# PT2323

ESP32/Arduino library for the Princeton Technology PT2323 audio selector, based on datasheet V1.3 (October 2005), pages 6-9.

## Initialization

```cpp
#include <Wire.h>
#include <PT2323.h>
PT2323 audio(&Wire);

void setup() {
  Wire.begin(); // On ESP32, select the SDA/SCL pins for your board here.
  if (!audio.begin()) {
    // Connection or initialization failed. Inspect audio.getLastError().
  }
}
void loop() {}
```

`begin()` waits 300 ms, sets the shared I2C bus to 100 kHz, probes address `0x4A`, mutes all outputs, selects stereo input group 1, disables surround, selects 0 dB mixed gain, explicitly clears all six individual mutes in one batch, then unmutes all outputs. Any failed transaction returns false and stops initialization; a failure during configuration leaves outputs muted. Initialize Wire before calling begin. The delay is relative to the begin call, so the PT2323 supply must already be stable. For Cref above 10 uF, provide an appropriately longer delay: `audio.begin(500)`. Passing a shorter delay is only valid if the required power stabilization time has already elapsed. The 100 kHz clock setting affects every device sharing this Wire bus; keep it at or below 100 kHz afterward.

## Commands

- `setChannel(0..3)`: stereo input groups 1..4. `setChannel(6)`: direct 6-channel input.
- `setSurround(true/false)`: enable/disable enhanced surround (`0xD0` / `0xD1`). The feature has no effect on direct 6-channel input.
- `setMixedChannel(false/true)`: select 0 dB / +6 dB (`0x90` / `0x91`).
- `muteFrontLeft`, `muteFrontRight`, `muteCenter`, `muteSubWoofer`, `muteRearLeft`, `muteRearRight`, `muteAll`: true mutes; false unmutes.
- Getters return the last successfully requested state, not a hardware readback.
- `getLastError()` returns and clears the last Wire transmission error (0 = success). Null Wire reports 4; a rejected buffered byte reports 1 if the transaction otherwise succeeded.

`0x94` in the datasheet is the complete write address byte; Arduino Wire takes its 7-bit address, `0x4A`.

## Validation

The `tests` directory exercises the real library source with a mocked Arduino/Wire interface against the datasheet command table, startup sequence and transmission failures. Hardware testing remains necessary.

Run the tests with a native C++ compiler (GCC, Clang or Zig):

```text
python tests/run_tests.py --compiler path/to/zig.exe
```

On Windows, Zig bundles the headers and runtime needed by these host tests. Native GCC/Clang can be selected with the same option when installed. Tests use a mock Wire bus and do not access connected hardware.

## Batched channel mutes (1.0.2)

`setMuteMask(mask, updateMask=0x3F)` sends all selected individual mute commands consecutively in one I2C transaction. Bits 0..5 correspond to front left, front right, center, subwoofer, rear left and rear right. A set mask bit means muted. updateMask selects the channels to write; zero is a no-op. Invalid bits are rejected. An ESP32 TX-buffer failure discards the incomplete batch before transmission.

The Tasmota Driver130 controller uses these individual commands to implement master mute while preserving each channel's mute preference.
