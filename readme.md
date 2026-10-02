# µRTC2 - an RTC for embedded systems

µRTC2 (Micro RTC 2) is an RTC for embedded systems with the following features:

* Optimized for architectures without hardware division
* Optimized for 8 and 32 bit systems, including AVR
* `2020/01/01 00:00:00 UTC` epoch, avoiding leap seconds
* 32 bit second counter, good until 2099 or 2156 (configurable)
* 1 second resolution
* EU DST calculation
* Time handling utility functions
* Build time and date utility functions
* GPLv3 licence

## Design
The goal is to be simple and robust, but also offer as good as possible performance. Converting between seconds since the epoch and broken down time (human format, i.e. years, months, days, hours, minutes, seconds) is expensive, but simplifies implementation. The general philosophy is to only convert when needed for input and output.

Data types are minimized, and rely on the compiler to optimize them.

Time should always be stored as UTC. Convert it to the local timezone and DST just before the conversion to broken down time.

## Leap seconds
Leap seconds are not implemented. As of the time of writing, since 2020 there have been no leap seconds, and it looks likely that there won't be any for the foreseeable future. This simplifies things considerably, and was one of the motivations behind selecting 2020 as the epoch.

## Avoiding division
Some RTC functions require division, but not all architectures have hardware division support. For improved performance where only a hardware multiplier is available, `#define RTC_AVOID_DIVISION` in `micro_rtc.h`. Instead of division, multiplication and shifting is used.

## 8 bit optimizations
For 8 bit platforms, `#define RTC_8BIT_OPTIMIZATION` in `micro_rtc.h`. This avoids some multiplications, particularly of 32 bit values, by shifting instead. It is particularly focused on AVR, but should help on any 8 bit architecture.

## Atomic 32 bit time access
Because the time is typically incremented in an interrupt, RTC_seconds_since_epoch should be accessed atomically. Typically that means disabling interrupts to access it, but beware of DMA also reading and writing it. On 32 bit systems, all CPU accesses to 32 bit values are probably atomic anyway.

See `micro_rtc.h` to define the necessary macros.

## Tick
Once per second, call RTC_tick() to increment the clock.

## Build time/date

`build.c` contains code to create strings that can be used to timestamp each build. To use them, make sure that your build process touches `build.c` so that the time and date are updated. The strings are built by the preprocessor, so have no runtime overhead.

`build_timestamp` contains the built timestamp in human readable format, basically ISO 8601 but with a space instead of the T separate date and time.

`build_number` is similar but contains only digits.

Be aware that these strings use the `__DATE__` and `__TIME__` macros, which are usually local time. If you want `build_number` to be unique you will need to account for DST changeover times, e.g. by avoiding building at that time or by setting the machine/build process to use UTC. A similar issue can happen if the build machine's clock is changed, although with periodic NTP sync it rarely gets more than a few seconds out.
