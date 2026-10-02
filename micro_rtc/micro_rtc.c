// micro_rtc.c

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include "iso_8601_date.h"
#include "micro_rtc.h"

const uint8_t days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const uint16_t cumulative_days_at_start_of_month[12] =
	{ 0, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
volatile uint32_t RTC_seconds_since_epoch = 0;


#define	START_DAY_OF_2100	29220
#define	END_DAY_OF_2100		29584


static inline uint8_t mod7_u16(uint16_t n)
{
#ifndef RTC_8BIT_OPTIMIZATION
	#ifndef RTC_AVOID_DIVISION
		return n % 7;
	#else
		return (uint8_t)(n - ((uint32_t)(((uint64_t)n * 0x24924925ULL) >> 32) * 7U));
	#endif
#else
	// Fold using 6-bit chunks (64 = 8^2 = 1 mod 7)
	n = (n >> 6) + (n & 0x3FU);
	n = (n >> 6) + (n & 0x3FU);
	n = (n >> 3) + (n & 0x07U);
	n = (n >> 3) + (n & 0x07U);
	if (n >= 7)
		n -= 7;
	return (uint8_t)n;
#endif
}

static inline void div86400_u32(uint32_t n, uint16_t* div, uint32_t* mod)
{
#ifndef RTC_8BIT_OPTIMIZATION
	#ifndef RTC_AVOID_DIVISION
		uint16_t d = n / SECONDS_PER_DAY;
	#else
		uint16_t d = (uint32_t)(((uint64_t)n * 3257812231ULL) >> 48);
	#endif
	uint32_t s = n - (d * SECONDS_PER_DAY);
#else
	uint16_t d = (uint16_t)(((uint64_t)n * 3257812231ULL) >> 48);
	uint32_t s = n - ((uint32_t)d * 86400UL);
#endif
	if (div != NULL)
		*div = d;
	if (mod != NULL)
		*mod = s;
}

static inline void divmod1461_u16(uint16_t n, uint16_t *div, uint16_t *mod)
{
#ifndef RTC_AVOID_DIVISION
	if (div != NULL)
		*div = n / 1461;
	if (mod != NULL)
		*mod = n % 1461;
#else
	uint16_t t = ((uint32_t)n * 11484) >> 24;
	if (div != NULL)
		*div = t;
	if (mod != NULL)
		*mod = n - (t * 1461);
#endif
}

/*****************************************************************************
* Check if year is a leap year. Only needs to work for 2020-2156.
*/
bool RTC_is_leap_year_bd(uint16_t year)
{
	if (((year & 0b11) == 0) && (year != 2100))
		return true;
	return false;
}

bool RTC_is_leap_year(uint32_t seconds_since_epoch)
{
	uint16_t days;
	div86400_u32(seconds_since_epoch, &days, NULL);

	if (days >= START_DAY_OF_2100 && days <= END_DAY_OF_2100)
		return false;	// 2100 is not a leap year
	if (days > END_DAY_OF_2100)
		days++;			// compensate for 2100's skipped leap day

	// find day offset within 4-year era (1,461 days)
	uint16_t era_day;
	divmod1461_u16(days, NULL, &era_day);
	return era_day < 366;	// year zero of era is leap year
}

/*****************************************************************************
* Return the number of days in the month. January is month 1. Returns 0 on
* error.
*/
uint8_t RTC_days_in_month_bd(uint8_t month, uint16_t year)
{
	if ((month == 0) || (month > 12))
		return 0;

	if (month == 2)
	{
		if (RTC_is_leap_year_bd(year))
			return 29;
		return 28;
	}
	return days_in_month[month-1];
}

uint8_t RTC_days_in_month(uint32_t seconds_since_epoch)
{
	bool is_leap_year;
	uint16_t day = RTC_day_of_year(seconds_since_epoch, &is_leap_year);

	if (day < days_in_month[0])
		return days_in_month[0];
	uint16_t acc = days_in_month[0] + days_in_month[1];
	if (day < acc + (is_leap_year ? 1 : 0))
		return days_in_month[1] + (is_leap_year ? 1 : 0);

	for (uint8_t m = 2; m < 11; m++)
	{
		acc += days_in_month[m];
		if (day < acc)
			return days_in_month[m];
	}

	return days_in_month[11];
}

/*****************************************************************************
* Get the day of the week. 0 = Monday.
* Month = 1-12, day = 1-31.
*/
uint8_t RTC_day_of_week_bd(const RTC_TIME_BD_t *bdt)
{
	// Sakamoto Tomohiko's algorithm
	static const uint8_t t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	uint8_t month = bdt->month;
	uint16_t year = bdt->year;

	// If January or February, treat as months 13 and 14 of the previous year
	if (month < 3) {
		year--;
	}

	uint16_t leap_shift = year >> 2;

	// Century adjustment: 6 for 2019..2099, 5 for 2100..2156
	uint8_t century_adj = (year > 2099) ? RTC_FRIDAY : RTC_SATURDAY;

	// Total day offset
	uint16_t sum = year + leap_shift + century_adj + t[month - 1] + bdt->day;

	return mod7_u16(sum);
}

uint8_t RTC_day_of_week(uint32_t seconds_since_epoch)
{
	uint16_t days;
	div86400_u32(seconds_since_epoch, &days, NULL);
	return mod7_u16(days + RTC_WEDNESDAY);
}

/*****************************************************************************
* Get the day of the year. 0 = January 1st.
* Month = 1-12, day = 1-31.
*/
uint16_t RTC_day_of_year_bd(const RTC_TIME_BD_t *bdt)
{
	uint8_t	doy = bdt->day;

	uint8_t m = 1;
	while (m < bdt->month)
	{
		doy += days_in_month[m - 1];
		if ((m == 2) && RTC_is_leap_year_bd(bdt->year))
			doy++;
		m++;
	}

	return doy;
}

uint16_t RTC_day_of_year(uint32_t seconds_since_epoch, bool *is_leap_year)
{
	uint16_t days;
	div86400_u32(seconds_since_epoch, &days, NULL);

	if (days > 29584)
		days++;			// compensate for 2100's skipped leap day

	// Find day offset within 4-year era (1,461 days)
	uint16_t era, era_day;
	divmod1461_u16(days, &era, &era_day);

	if (is_leap_year != NULL)
	{
		if ((days < 29220) || (days > 29584))
			*is_leap_year = era_day < 366;
		else
			*is_leap_year = false;
	}

	// era day to year day
	if (era_day > 366)
		era_day -= 366;
	if (era_day > 365)
		era_day -= 365;
	if (era_day > 365)
		era_day -= 365;

	return era_day;
}

/*****************************************************************************
* Return the number of days since the epoch
*/
uint16_t RTC_days_since_epoch_bd(const RTC_TIME_BD_t *bdt)
{
	uint16_t days = bdt->day - 1;
	for (int8_t i = 0; i < bdt->month - 1; i++)
		days += days_in_month[i];

	days += (bdt->year - RTC_EPOCH_YEAR) * 365;
	uint16_t leaps = (bdt->year - 2017) >> 2;
	leaps -= (bdt->year > 2100);
	days += leaps;

	if (RTC_is_leap_year_bd(bdt->year) && (bdt->month > 2))
		days++;

	return days;
}

uint16_t RTC_days_since_epoch(uint32_t seconds_since_epoch)
{
	uint16_t days;
	div86400_u32(seconds_since_epoch, &days, NULL);
	return days;
}

/*****************************************************************************
* Year that seconds_since_epoch lies within
*/
uint16_t RTC_year(uint32_t seconds_since_epoch)
{
	uint16_t days;
	div86400_u32(seconds_since_epoch, &days, NULL);
	if (days >= START_DAY_OF_2100)
		days++;		// 2100 is not a leap year

	// Find day offset within 4-year era (1,461 days)
	uint16_t era, era_day;
	divmod1461_u16(days, &era, &era_day);

	// Year within era (Year 0 is a 366-day leap year)
	uint16_t year = 2020 + (era << 2);
	if (era_day < 366)
		return year;
	if (era_day < (366 + 365))
		return year + 1;
	if (era_day < (366 + 365 + 365))
		return year + 2;
	return year + 3;
}

/*****************************************************************************
* Broken down time to seconds since epoch
* Note that month is 1-12, day is 1-31.
*/
uint32_t RTC_bd_to_seconds_since_epoch(const RTC_TIME_BD_t *bdt)
{
	uint32_t seconds_since_epoch = bdt->second;
	seconds_since_epoch += bdt->minute * SECONDS_PER_MINUTE;
	seconds_since_epoch += bdt->hour * SECONDS_PER_HOUR;
	seconds_since_epoch += (RTC_days_since_epoch_bd(bdt) * SECONDS_PER_DAY);
	return seconds_since_epoch;
}

static void seconds_in_day_to_bd_time_of_day(
	uint32_t sec_in_day, RTC_TIME_BD_t *bdt)
{
#ifndef RTC_AVOID_DIVISION
	uint8_t hour = sec_in_day / 3600;
	uint16_t sec_in_hour = sec_in_day % 3600;
	uint8_t minute = sec_in_hour / 60;
	uint8_t second = sec_in_hour % 60;
#else
	uint8_t hour = (uint8_t)(((uint32_t)sec_in_day * 37283) >> 27);
	uint16_t sec_in_hour = (uint16_t)(sec_in_day - ((uint32_t)hour * 3600));
	uint8_t minute = (uint8_t)(((uint32_t)sec_in_hour * 4370) >> 18);
	uint8_t second = (uint8_t)(sec_in_hour - ((uint16_t)minute * 60));
#endif

	bdt->hour = hour;
	bdt->minute = minute;
	bdt->second = second;
}

static void seconds_since_epoch_to_bd_date(uint16_t days, RTC_TIME_BD_t* bdt)
{
	// shift epoch origin back to 2016-03-01 (1401 days prior to 2020-01-01),
	// so year 4 of a 4 year cycle (3 when numbered 0-3) is the leap year
	uint32_t d = days + 1401U;

	if (d >= 30680U)	// compensate for 2100 not being a leap year
		d++;

	// 4 year era
	uint16_t era, d_era;
	divmod1461_u16(d, &era, &d_era);

#ifndef RTC_AVOID_DIVISION
	// year within the era (0-3)
	uint32_t d_era_clamp = (d_era == 1460U) ? 1459U : d_era; // leap day boundary
	uint32_t y_in_era = d_era_clamp / 365;
	uint32_t d_year = d_era - (y_in_era * 365);
	uint32_t m = (5 * d_year + 2) / 153;			// March-based year (0 = March)
	uint32_t day = d_year - (153 * m + 2) / 5 + 1;	// day of month (1 to 31)
#else
	// year within the era (0-3)
	uint32_t d_era_clamp = (d_era == 1460U) ? 1459U : d_era; // leap day boundary
	uint32_t y_in_era = (d_era_clamp * 45966U) >> 24;
	uint32_t d_year = d_era - (y_in_era * 365U);
	uint32_t m = ((5U * d_year + 2U) * 429U) >> 16;	// March-based year (0 = March)
	uint32_t day = d_year - ((153U * m + 2U) / 5U) + 1U; // day of month (1 to 31)
#endif

	// convert back to standard Gregorian calendar
	uint32_t year = 2016U + (era * 4U) + y_in_era + (m >= 10U ? 1U : 0U);
	uint32_t month = (m < 10U) ? (m + 3U) : (m - 9U);

	bdt->year = (uint16_t)year;
	bdt->month = (uint8_t)month;
	bdt->day = (uint8_t)day;
}

/*****************************************************************************
* Seconds since epoch to broken down time
*/
void RTC_seconds_since_epoch_to_bd(
	uint32_t seconds_since_epoch,
	RTC_TIME_BD_t* bdt)
{
	uint32_t sec_in_day;
	uint16_t days;
	div86400_u32(seconds_since_epoch, &days, &sec_in_day);
	seconds_in_day_to_bd_time_of_day(sec_in_day, bdt);
	seconds_since_epoch_to_bd_date(days, bdt);
}

/*****************************************************************************
* Seconds since epoch to broken down time, time of day only
*/
void RTC_seconds_since_epoch_to_bd_time_of_day(
	uint32_t seconds_since_epoch,
	RTC_TIME_BD_t* bdt)
{
	uint32_t sec_in_day;
	div86400_u32(seconds_since_epoch, NULL, &sec_in_day);
	seconds_in_day_to_bd_time_of_day(sec_in_day, bdt);
}

/*****************************************************************************
* Seconds since epoch to broken down time, date only
*/
void RTC_seconds_since_epoch_to_bd_date(
	uint32_t seconds_since_epoch,
	RTC_TIME_BD_t* bdt)
{
	uint16_t days;
	div86400_u32(seconds_since_epoch, &days, NULL);
	seconds_since_epoch_to_bd_date(days, bdt);
}

/*****************************************************************************
* Calculate DST start and end days using the EU scheme.
* Start is last Sunday in March.
* End is last Sunday in October.
* Start and end times are 01:00 on this day.
* Only valid from year 2000 onwards.
*/
uint8_t RTC_eu_dst_start_day_of_month_bd(uint16_t year)
{
#ifndef RTC_AVOID_DIVISION
	return 31 - ((((5 * (uint32_t)year) / 4) + 4) % 7);
#else
	return 31 - mod7_u16(year + (year >> 2) + 4);
#endif
}

uint8_t RTC_eu_dst_end_day_of_month_bd(uint16_t year)
{
#ifndef RTC_AVOID_DIVISION
	return 31 - ((((5 * (uint32_t)year) / 4) + 1) % 7);
#else
	return 31 - mod7_u16(year + (year >> 2) + 1);
#endif
}

/*****************************************************************************
* Calculate DST start and end times using the EU scheme. Times are UTC.
*/
void RTC_eu_dst_times(uint32_t seconds_since_epoch,
					  uint32_t *dst_start_seconds_since_epoch,
					  uint32_t *dst_end_seconds_since_epoch,
					  bool *is_leap_year)
{
	uint16_t year = RTC_year(seconds_since_epoch);

	// days prior to 01-01
	uint16_t y_off = year - 2020;
	uint16_t leaps = (year - 2017) >> 2;
	if (year > 2100)
		leaps--; // 2100 is not a leap year
	uint32_t year_start_days = ((uint32_t)y_off * 365) + leaps;

	// leap year flag for current year
	bool is_leap = ((year & 3U) == 0U) && (year != 2100U);
	if (is_leap_year)
		*is_leap_year = is_leap;

	uint8_t day_of_month = RTC_eu_dst_start_day_of_month_bd(year);
	uint32_t start_days =	year_start_days +
							(is_leap ? 1 : 0) +
							cumulative_days_at_start_of_month[2] +
							day_of_month - 1;
	if (dst_end_seconds_since_epoch)
		*dst_start_seconds_since_epoch = (start_days * 86400UL) + 3600UL;

	day_of_month = RTC_eu_dst_end_day_of_month_bd(year);
	uint32_t end_days =	year_start_days +
						(is_leap ? 1 : 0) +
						cumulative_days_at_start_of_month[9] +
						day_of_month - 1;
	if (dst_end_seconds_since_epoch)
		*dst_end_seconds_since_epoch = (end_days * 86400UL) + 3600UL;
}

/*****************************************************************************
* Calculate DST start and end times using the EU scheme. Times are UTC.
* Set year in output structs.
*/
void RTC_eu_dst_times_bd(RTC_TIME_BD_t* start_bdt, RTC_TIME_BD_t* end_bdt)
{
	if (start_bdt)
	{
		start_bdt->month = 3;
		start_bdt->day = RTC_eu_dst_start_day_of_month_bd(start_bdt->year);
		start_bdt->hour = 1;
		start_bdt->minute = 0;
		start_bdt->second = 0;
	}

	if (end_bdt)
	{
		end_bdt->month = 10;
		end_bdt->day = RTC_eu_dst_end_day_of_month_bd(end_bdt->year);
		end_bdt->hour = 1;
		end_bdt->minute = 0;
		end_bdt->second = 0;
	}
}

/*****************************************************************************
* Check if time is inside EU DST.
* leap_year can be NULL if not used.
* Calls RTC_eu_dst_times(), so if you already called it you can do the
* calculation yourself to save re-calculating.
*/
bool RTC_is_in_eu_dst(uint32_t seconds_since_epoch, bool *leap_year)
{
	uint32_t start_time, end_time;
	RTC_eu_dst_times(seconds_since_epoch, &start_time, &end_time, leap_year);
	return ((seconds_since_epoch >= start_time) && (seconds_since_epoch < end_time));
}

/*****************************************************************************
* Compare broken down times. Returns
* 0		times are equal
* 1		a after b
* -1	a before b
*/
int8_t RTC_bd_compare(const RTC_TIME_BD_t* a, const RTC_TIME_BD_t* b)
{
	if (a->year < b->year)
		return -1;
	if (a->year > b->year)
		return 1;
	// year is equal
	if (a->month < b->month)
		return -1;
	if (a->month > b->month)
		return 1;
	// year and month are equal
	if (a->day < b->day)
		return -1;
	if (a->day > b->day)
		return 1;
	// year, month, day are equal
	if (a->hour < b->hour)
		return -1;
	if (a->hour > b->hour)
		return 1;
	// ymd and hour are equal
	if (a->minute < b->minute)
		return -1;
	if (a->minute > b->minute)
		return 1;
	// ymd, hm are equal
	if (a->second < b->second)
		return -1;
	if (a->second > b->second)
		return 1;
	// times are equal
	return 0;
}

/*****************************************************************************
* Check if broken down time is inside EU DST.
*/
bool RTC_is_in_eu_dst_bdt(const RTC_TIME_BD_t* bdt)
{
	RTC_TIME_BD_t start, end;
	start.year = bdt->year;
	end.year = bdt->year;
	RTC_eu_dst_times_bd(&start, &end);
	return (RTC_bd_compare(bdt, &start) >= 0) &&
		   (RTC_bd_compare(bdt, &end) < 0);
}

/*****************************************************************************
* Add seconds. Seconds can be negative to subtract time. On underflow,
* returns 0. No overflow check is made.
*/
uint32_t RTC_add_seconds(uint32_t seconds_since_epoch, int32_t addend)
{
	if (addend < 0)
	{
		if (seconds_since_epoch < abs(addend))
			seconds_since_epoch = 0;
		else
			seconds_since_epoch += addend;
	}
	else
		seconds_since_epoch += addend;
	return seconds_since_epoch;
}
