#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <deque>
struct Transmission { uint8_t address; std::vector<uint8_t> bytes; };
class TwoWire {
public:
  uint32_t clock = 0;
  std::vector<Transmission> transmissions;
  std::deque<uint8_t> results;
  bool acceptWrite = true;
  void setClock(uint32_t hz) { clock = hz; }
  void beginTransmission(uint8_t address) { transmissions.push_back({address, {}}); }
  size_t write(uint8_t value) {
    if (!acceptWrite) return 0;
    transmissions.back().bytes.push_back(value); return 1;
  }
  uint8_t endTransmission() {
    if (results.empty()) return 0;
    auto result = results.front(); results.pop_front(); return result;
  }
};
extern TwoWire Wire;
