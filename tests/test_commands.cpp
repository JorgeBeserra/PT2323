#include "PT2323.h"
#include <iostream>
#include <stdexcept>
#include <functional>
TwoWire Wire;
unsigned long waitedMs = 0;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
void packet(TwoWire &bus, uint8_t value) {
  check(!bus.transmissions.empty(), "missing transmission");
  const auto &tx = bus.transmissions.back();
  check(tx.address == 0x4A, "incorrect 7-bit address");
  check(tx.bytes == std::vector<uint8_t>{value}, "incorrect command byte");
}
int main() {
  int failures = 0;
  auto test = [&](const char *name, std::function<void()> run) {
    try { run(); std::cout << "PASS " << name << "\n"; }
    catch (const std::exception &e) { ++failures; std::cout << "FAIL " << name << ": " << e.what() << "\n"; }
  };
  test("address", [] { TwoWire w; PT2323 p(&w); check(p.getAddress() == 0x4A, "must use 0x94 >> 1"); });
  test("input selection", [] {
    TwoWire w; PT2323 p(&w);
    const uint8_t channels[] = {0, 1, 2, 3, 6}; const uint8_t bytes[] = {0xCB, 0xCA, 0xC9, 0xC8, 0xC7};
    for (int i=0; i<5; ++i) { check(p.setChannel(channels[i]), "selection failed"); packet(w, bytes[i]); check(p.getChannel()==channels[i], "wrong cached channel"); }
    auto count=w.transmissions.size(); check(!p.setChannel(4) && !p.setChannel(5) && !p.setChannel(255), "invalid channel accepted"); check(w.transmissions.size()==count, "invalid input sent to chip");
  });
  test("all mute commands", [] {
    TwoWire w; PT2323 p(&w);
    bool (PT2323::*methods[])(bool) = {&PT2323::muteFrontLeft,&PT2323::muteFrontRight,&PT2323::muteCenter,&PT2323::muteSubWoofer,&PT2323::muteRearLeft,&PT2323::muteRearRight,&PT2323::muteAll};
    const uint8_t bytes[]={0xF0,0xF2,0xF4,0xF6,0xF8,0xFA,0xFE};
    for(int i=0;i<7;++i) { check((p.*methods[i])(false), "unmute failed"); packet(w,bytes[i]); check((p.*methods[i])(true), "mute failed");packet(w,bytes[i]|1); }
  });
  test("surround polarity", [] { TwoWire w; PT2323 p(&w); check(p.setSurround(true),"enable failed"); packet(w,0xD0);check(p.getSurround(),"cache false");check(p.setSurround(false),"disable failed");packet(w,0xD1);check(!p.getSurround(),"cache true"); });
  test("mixed gain does not mute", [] { TwoWire w;PT2323 p(&w);check(p.setMixedChannel(false),"0dB failed");packet(w,0x90);check(p.setMixedChannel(true),"6dB failed");packet(w,0x91);check(p.getMixedChannel(),"cache false"); });
  test("begin defaults and timing", [] {
    TwoWire w;PT2323 p(&w);waitedMs=0;check(p.begin(),"begin failed");check(waitedMs>=300,"power stabilization delay missing");check(w.clock==100000,"100kHz limit missing");
    const std::vector<std::vector<uint8_t>> expected={{},{0xFF},{0xCB},{0xD1},{0x90},{0xF0,0xF2,0xF4,0xF6,0xF8,0xFA},{0xFE}};
    check(w.transmissions.size()==expected.size(),"wrong startup transaction count");for(size_t i=0;i<expected.size();++i){check(w.transmissions[i].address==0x4A,"wrong startup address");check(w.transmissions[i].bytes==expected[i],"wrong startup defaults");}
    check(p.getChannel()==0 && !p.getSurround() && !p.getMixedChannel(),"incorrect startup cache");
  });
  test("begin propagates every NACK", [] {
    for(int failAt=0;failAt<7;++failAt) { TwoWire w;PT2323 p(&w);for(int i=0;i<failAt;++i)w.results.push_back(0);w.results.push_back(3);check(!p.begin(),"begin falsely succeeded");check(w.transmissions.size()==size_t(failAt+1),"continued after failed command");check(p.getLastError()==3,"NACK lost");check(p.getLastError()==0,"error not cleared"); }
  });
  test("failed writes preserve cached state", [] {
    TwoWire w;PT2323 p(&w);check(p.setChannel(2),"setup failed");check(p.setSurround(true),"setup failed");check(p.setMixedChannel(true),"setup failed");
    w.results={2};check(!p.setChannel(3)&&p.getChannel()==2,"channel cache corrupted");w.results={2};check(!p.setSurround(false)&&p.getSurround(),"surround cache corrupted");w.results={2};check(!p.setMixedChannel(false)&&p.getMixedChannel(),"gain cache corrupted");
  });
  test("buffer failure", [] { TwoWire w; PT2323 p(&w); w.acceptWrite=false;check(!p.setChannel(1),"buffer failure accepted");check(p.getLastError()==1,"buffer failure not reported");check(p.getChannel()==0,"failed write changed state"); });
  test("null bus", [] { PT2323 p(nullptr);check(!p.begin(),"null begin accepted");check(p.getLastError()==4,"null error missing");check(!p.isConnected(),"null probe accepted");check(!p.muteAll(true),"null write accepted"); });
  test("configurable startup delay", [] { TwoWire w;PT2323 p(&w);waitedMs=0;check(p.begin(500),"begin failed");check(waitedMs==500,"longer Cref delay ignored"); });
  test("all 64 mute masks in one transaction", [] {TwoWire w;PT2323 p(&w);for(int mask=0;mask<64;++mask){auto count=w.transmissions.size();check(p.setMuteMask(mask),"mask failed");check(w.transmissions.size()==count+1,"batch split");const auto &tx=w.transmissions.back();check(tx.address==0x4A&&tx.bytes.size()==6,"bad batch");for(int i=0;i<6;++i)check(tx.bytes[i]==uint8_t(0xF0|(i<<1)|((mask>>i)&1)),"bad channel mute");}});
  test("partial and invalid mute mask", [] {TwoWire w;PT2323 p(&w);check(p.setMuteMask(5,5),"partial update failed");check(w.transmissions.back().bytes==std::vector<uint8_t>{0xF1,0xF5},"wrong selected channels");auto count=w.transmissions.size();check(p.setMuteMask(0,0),"empty update failed");check(w.transmissions.size()==count,"empty update sent");check(!p.setMuteMask(64)&&!p.setMuteMask(0,128),"invalid mask accepted");check(w.transmissions.size()==count,"invalid mask sent");});
  test("partial mute buffer is discarded", [] {TwoWire w;PT2323 p(&w);w.maxBytes=3;check(!p.setMuteMask(63),"partial batch accepted");check(w.transmissions.back().bytes.empty(),"partial channel commands sent");check(p.getLastError()==1,"buffer error missing");});
  std::cout << failures << " failing test groups\n";return failures ? 1 : 0;
}
