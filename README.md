# GBA RTC Test

This testrom tests the RTC present in various GBA cartridges (including Pokemon games).

The RTC is actually a stock Seiko S-3511A, communicated with cartridge GPIO. Since GPIO effectively forces the GBA to do raw bit-banging with this RTC, various internal quirks can be exposed and tested.

This testrom currently only features 18 "basic" tests, only covering the RTC state machine. Only status read/writes must be implemented. "Advanced" tests requiring an actual time implementation will come later.
