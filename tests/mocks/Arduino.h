#pragma once
#include <cstdint>
#include <cstddef>
#define F(value) value
extern unsigned long waitedMs;
inline void delay(unsigned long ms) { waitedMs += ms; }
