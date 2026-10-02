#pragma once

/******************************************************************************
* Configuration
*/
//#define	RTC_AVOID_DIVISION		// use multiplication instead of division
//#define	RTC_8BIT_OPTIMIZATION	// use 8 bit optimizations

// fill these in with code to disable/enable the RTC tick interrupt

// for bare metal STM32
//#define	RTC_ENABLE_INTERRUPT	do{ __enable_irq(); } while(0)
//#define	RTC_DISABLE_INTERRUPT	do{ __disable_irq(); } while(0)
// or rely on 32 bit access being atomic for the CPU
//#define	RTC_ENABLE_INTERRUPT	do{ } while(0)
//#define	RTC_DISABLE_INTERRUPT	do{ } while(0)

// for AVR
//#include <avr/interrupt.h>
//#define	RTC_ENABLE_INTERRUPT	do{ __enable_irq(); } while(0)
//#define	RTC_DISABLE_INTERRUPT	do{ __disable_irq(); } while(0)


#ifndef RTC_ENABLE_INTERRUPT
#error "RTC_ENABLE_INTERRUPT not configured for this platform"
#endif
#ifndef RTC_DISABLE_INTERRUPT
#error "RTC_DISABLE_INTERRUPT not configured for this platform"
#endif

/******************************************************************************
* Public definitions, enums and typedefs
*/
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#define	SECONDS_PER_MINUTE			(60)
#define	SECONDS_PER_HOUR			(60 * 60)
#define	SECONDS_PER_DAY				(60 * 60 * 24)
#define	SECONDS_PER_NON_LEAP_YEAR	(SECONDS_PER_DAY * 365)
#define	SECONDS_PER_LEAP_YEAR		(SECONDS_PER_DAY * 366)
#define SECONDS_PER_LEAP_CENTURY	(3155760000)

#define	RTC_EPOCH_YEAR	(2020)
#define RTC_UNIX_TIMESTAMP_AT_EPOCH	(1577836800)

// convert to seconds
// useful for calculating timezone offsets
#define	HMS_TO_SECONDS(hours, minutes, seconds) \
			((hours*SECONDS_PER_HOUR)+(minutes*SECONDS_PER_MINUTE)+seconds)
#define	DHMS_TO_SECONDS(days, hours, minutes, seconds) \
			((days*SECONDS_PER_DAY)+(hours*SECONDS_PER_HOUR)+(minutes*SECONDS_PER_MINUTE)+seconds)


typedef enum 
{
	RTC_DEPTH_Y			= 0,
	RTC_DEPTH_YM		= 1,
	RTC_DEPTH_YMD		= 2,
	RTC_DEPTH_YMDH		= 3,
	RTC_DEPTH_YMDHMS	= 4,
} RTC_DEPTH_e;

enum {
	RTC_MONDAY = 0,
	RTC_TUESDAY, RTC_WEDNESDAY, RTC_THURSDAY, RTC_FRIDAY, RTC_SATURDAY, RTC_SUNDAY
};

typedef struct {
	uint16_t	year;
	uint8_t		month, day;
	uint8_t		hour, minute, second;
} RTC_TIME_BD_t;


/******************************************************************************
* Public variables
*/
extern const uint8_t days_in_month[12];
extern volatile uint32_t RTC_seconds_since_epoch;

/******************************************************************************
* Public functions
*/

// for atomic access to RTC_seconds_since_epoch
static inline uint32_t RTC_get_time(void)
{
	RTC_DISABLE_INTERRUPT;
	uint32_t t = RTC_seconds_since_epoch;
	RTC_ENABLE_INTERRUPT;
	return t;
}
static inline void RTC_set_time(uint32_t time)
{
	RTC_DISABLE_INTERRUPT;
	RTC_seconds_since_epoch = time;
	RTC_ENABLE_INTERRUPT;
}
static inline void RTC_tick(void)
{
	RTC_DISABLE_INTERRUPT;
	RTC_seconds_since_epoch++;
	RTC_ENABLE_INTERRUPT;
}

// date handling
extern bool RTC_is_leap_year_bd(uint16_t year);
extern bool RTC_is_leap_year(uint32_t seconds_since_epoch);

extern uint8_t RTC_days_in_month_bd(uint8_t month, uint16_t year);
extern uint8_t RTC_days_in_month(uint32_t seconds_since_epoch);

extern uint8_t RTC_day_of_week_bd(const RTC_TIME_BD_t *split);
extern uint8_t RTC_day_of_week(uint32_t seconds_since_epoch);

extern uint16_t RTC_day_of_year_bd(const RTC_TIME_BD_t* split);
extern uint16_t RTC_day_of_year(uint32_t seconds_since_epoch, bool* is_leap_year);

extern uint16_t RTC_days_since_epoch_bd(const RTC_TIME_BD_t* bdt);
extern uint16_t RTC_days_since_epoch(uint32_t seconds_since_epoch);

extern uint16_t RTC_year(uint32_t seconds_since_epoch);

// conversion between time formats
extern uint32_t RTC_bd_to_seconds_since_epoch(const RTC_TIME_BD_t* bdt);
extern void RTC_seconds_since_epoch_to_bd(uint32_t seconds_since_epoch, RTC_TIME_BD_t *bdt);
extern void RTC_seconds_since_epoch_to_bd_time_of_day(uint32_t seconds_since_epoch, RTC_TIME_BD_t* bdt);
extern void RTC_seconds_since_epoch_to_bd_date(uint32_t seconds_since_epoch, RTC_TIME_BD_t* bdt);

// daylight saving time
extern uint8_t RTC_eu_dst_start_day_of_month_bd(uint16_t year);
extern uint8_t RTC_eu_dst_end_day_of_month_bd(uint16_t year);
extern void RTC_eu_dst_times(uint32_t seconds_since_epoch, uint32_t* dst_start_seconds_since_epoch, uint32_t* dst_end_seconds_since_epoch, bool* is_leap_year);
extern void RTC_eu_dst_times_bd(RTC_TIME_BD_t* start_bdt, RTC_TIME_BD_t* end_bdt);
extern bool RTC_is_in_eu_dst(uint32_t seconds_since_epoch, bool* leap_year);
extern bool RTC_is_in_eu_dst_bdt(const RTC_TIME_BD_t* bdt);

// misc
extern int8_t RTC_bd_compare(const RTC_TIME_BD_t* a, const RTC_TIME_BD_t* b);
extern uint32_t RTC_add_seconds(uint32_t seconds_since_epoch, int32_t addend);
