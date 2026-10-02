// main() with tests for micro_rtc
//
// TODO:	RTC_seconds_since_epoch_is_in_dst_eu()
//			RTC_local_time_split()
//			RTC_local_time_seconds_since_epoch
//			RTC_get_time()
//			RTC_get_time_seconds_since_epoch()
//			RTC_get_time_split()
//
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include "iso_8601_date.h"
#include "micro_rtc.h"
#include "build.h"

#define	ARRAY_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

/****************************************************************************/

uint32_t test_leap_years[] = {
	2020, 2024, 2028, 2032, 2036, 2040, 2044,
	2048, 2052, 2056, 2060, 2064, 2068, 2072, 2076, 2080, 2084, 2088, 2092,
	2096, 2104, 2108, 2112, 2116, 2120, 2124, 2128, 2132, 2136, 2140, 2144,
	2148, 2152, 2156
};
#define	TEST_LEAP_YEARS_START_YEAR	2020
#define	TEST_LEAP_YEARS_END_YEAR	2156

struct {
	uint32_t days_in_month;
	RTC_TIME_BD_t bdt;
} test_days_in_month[] = {
	{ 31, { 2020, 1, 1, 0, 0, 0 } },
	{ 29, { 2020, 2, 16, 5, 34, 12 } },
	{ 31, { 2020, 12, 16, 5, 34, 12 } },
	{ 28, { 2021, 2, 16, 5, 34, 12 } },
	{ 31, { 2154, 1, 1, 0, 0, 0 } },
};

struct {
	uint8_t dow;
	RTC_TIME_BD_t bdt;
} test_days_of_week[] = {
	{ RTC_WEDNESDAY,{ 2020,  1,  1 } },	// 0
	{ RTC_THURSDAY,	{ 2020,  1,  2 } },	// 1
	{ RTC_SATURDAY,	{ 2020,  2, 29 } },	// 2
	{ RTC_THURSDAY,	{ 2020, 12, 31 } },	// 3
	{ RTC_FRIDAY,	{ 2021,  1,  1 } },	// 4
	{ RTC_MONDAY,	{ 2044,  2, 29 } },	// 5
	{ RTC_THURSDAY,	{ 2099, 12, 31 } },	// 6
	{ RTC_WEDNESDAY,{ 2155, 12, 31 } },	// 7
};

struct {
	uint32_t seconds_since_epoch;
	RTC_TIME_BD_t bdt;
	bool is_leap_year;
} test_bd_to_seconds_since_epoch[] = {
	{ 0, { 2020, 1, 1, 0, 0, 0 }, true },					// 0
	{ 1, { 2020, 1, 1, 0, 0, 1 }, true },					// 1
	{ 86399, { 2020, 1, 1, 23, 59, 59 }, true },			// 2
	{ 86400, { 2020, 1, 2, 0, 0, 0 }, true },				// 3
	{ 5157326, { 2020, 2, 29, 16, 35, 26 }, true },			// 4
	{ 31622399, { 2020, 12, 31, 23, 59, 59 }, true },		// 5
	{ 31622400, { 2021, 1, 1, 0, 0, 0 }, false },			// 6
	{ 3172408526, { 2120, 7, 12, 16, 35, 26 }, false },		// 7
	{ 4294967295, { 2156, 2, 7, 6, 28, 15 }, true },		// 8
};

struct {
	uint32_t days_since_epoch;
	RTC_TIME_BD_t split;
} test_ymd_to_days_since_epoch[] = {
	{ 0, { 0 + RTC_EPOCH_YEAR, 1, 1 } },			// 0
	{ 1, { 0 + RTC_EPOCH_YEAR, 1, 2 } },			// 1
	{ 59, { 0 + RTC_EPOCH_YEAR, 2, 29 } },			// 2
	{ 60, { 0 + RTC_EPOCH_YEAR, 3, 1 } },			// 3
	{ 365, { 0 + RTC_EPOCH_YEAR, 12, 31 } },		// 4
	{ 366, { 1 + RTC_EPOCH_YEAR, 1, 1 } },			// 5
	{ 8139, { 22 + RTC_EPOCH_YEAR, 4, 14 } },		// 6
	{ 36524, { 99 + RTC_EPOCH_YEAR, 12, 31 } },		// 7
	{ 36525, { 100 + RTC_EPOCH_YEAR, 1, 1 } },		// 8
	{ 40177, { 110 + RTC_EPOCH_YEAR, 1, 1 } },		// 9
	{ 43828, { 119 + RTC_EPOCH_YEAR, 12, 31 } },	// 10
	{ 43829, { 120 + RTC_EPOCH_YEAR, 1, 1 } },		// 11
	{ 49709, { 136 + RTC_EPOCH_YEAR, 2, 6 } },		// 12
	{ 49710, { 136 + RTC_EPOCH_YEAR, 2, 7 } },		// 13
#ifndef RTC_BITS_8
	{ 146097, { 400 + RTC_EPOCH_YEAR, 1, 1 } },		// 14
#endif
};

#include "dst_dates.h"

/****************************************************************************/

struct tm make_tm_bdt(const RTC_TIME_BD_t* bdt)
{
	return (struct tm) {
		.tm_year = bdt->year - 1900,
			.tm_mon = bdt->month - 1,
			.tm_mday = bdt->day,
			.tm_hour = bdt->hour,
			.tm_min = bdt->minute,
			.tm_sec = bdt->second,
	};
}

uint32_t make_time(const RTC_TIME_BD_t* bdt)
{
	struct tm t = make_tm_bdt(bdt);
	time_t time = mktime(&t);
	return (uint32_t)time - RTC_UNIX_TIMESTAMP_AT_EPOCH;
}

int main(void)
{
	int errors = 0;
	
	printf("__DATE__ \"%s\"\n", __DATE__);
	printf("__TIME__ \"%s\"\n", __TIME__);
	printf("ISO 8601 date: \"%s\"\n", ISO_8601_DATE);
	printf("Build timestamp: \"%s\"\n", build_timestamp);
	printf("Build number: \"%s\"\n", build_number);

	// leap years
	printf("RTC_is_leap_year_bd()\n");
	for (uint16_t year = TEST_LEAP_YEARS_START_YEAR; year < TEST_LEAP_YEARS_END_YEAR; year++)
	{
		bool found = false;
		for (uint32_t i = 0; i < ARRAY_COUNT(test_leap_years); i++)
		{
			if (test_leap_years[i] == year)
				found = true;
		}
		bool res = RTC_is_leap_year_bd(year);
		if (found != res)
		{
			printf("Year %" PRIu32 " returned %s\n", year, res ? "true" : "false");
			errors++;
		}
	}

	printf("RTC_is_leap_year()\n");
	uint8_t month = 0;
	uint8_t day = 1;
	for (uint16_t year = TEST_LEAP_YEARS_START_YEAR; year < TEST_LEAP_YEARS_END_YEAR; year++)
	{
		bool found = false;
		for (uint32_t i = 0; i < ARRAY_COUNT(test_leap_years); i++)
		{
			if (test_leap_years[i] == year)
				found = true;
		}

		struct tm brokentime;
		memset(&brokentime, 0, sizeof(brokentime));
		brokentime.tm_year = year - 1900;
		brokentime.tm_mon = month++;
		if (month > 11) month = 0;
		brokentime.tm_mday = day++;
		if (day > 28) day = 1;
		time_t t = mktime(&brokentime);
		t -= RTC_UNIX_TIMESTAMP_AT_EPOCH;

		bool res = RTC_is_leap_year((uint32_t)t);
		if (found != res)
		{
			printf("Year %" PRIu16 " returned %s\n", year, res ? "true" : "false");
			errors++;
		}
	}

	// days in month
	printf("RTC_days_in_month_bd()\n");
	for (uint32_t i = 0; i < ARRAY_COUNT(test_days_in_month); i++)
	{
		//struct tm t = make_tm_bdt(&test_days_in_month[i].bdt);
		uint8_t dow = RTC_days_in_month_bd(test_days_in_month[i].bdt.month, test_days_in_month[i].bdt.year);
		if (dow != test_days_in_month[i].days_in_month)
		{
			printf(
				"Error line %" PRIu32 ", want %" PRIu8 ", "
				"days in month = %" PRIu8 ", %04" PRIu16 "/%02" PRIu8 "\n",
				i, test_days_in_month[i].days_in_month, dow,
				test_days_in_month[i].bdt.year,
				test_days_in_month[i].bdt.month);
			errors++;
		}
	}

	printf("RTC_days_in_month()\n");
	for (uint32_t i = 0; i < ARRAY_COUNT(test_days_in_month); i++)
	{
		uint32_t time = make_time(&test_days_in_month[i].bdt);
		uint8_t dow = RTC_days_in_month(time);
		if (dow != test_days_in_month[i].days_in_month)
		{
			printf(
				"Error line %" PRIu32 ", want %" PRIu8 ", "
				"days in month = %" PRIu8 ", %04" PRIu16 "/%02" PRIu8 "\n",
				i, test_days_in_month[i].days_in_month, dow,
				test_days_in_month[i].bdt.year,
				test_days_in_month[i].bdt.month);
			errors++;
		}
	}

	// day of week
	printf("RTC_day_of_week_bd()\n");
	for (uint32_t i = 0; i < ARRAY_COUNT(test_days_of_week); i++)
	{
		uint8_t dow = RTC_day_of_week_bd(&test_days_of_week[i].bdt);
		if (dow != test_days_of_week[i].dow)
		{
			printf(
				"Error line %" PRIu32 ", want %" PRIu8 ", "
				"DOW=%" PRIu8 ", %04" PRIu16 "/%02" PRIu8 "/%02" PRIu8 "\n",
				i, test_days_of_week[i].dow, dow,
				test_days_of_week[i].bdt.year,
				test_days_of_week[i].bdt.month,
				test_days_of_week[i].bdt.day);
			errors++;
		}
	}

	printf("RTC_day_of_week()\n");
	for (uint32_t i = 0; i < ARRAY_COUNT(test_days_of_week); i++)
	{
		struct tm brokentime;
		memset(&brokentime, 0, sizeof(brokentime));
		brokentime.tm_year = test_days_of_week[i].bdt.year - 1900;
		brokentime.tm_mon = test_days_of_week[i].bdt.month - 1;
		brokentime.tm_mday = test_days_of_week[i].bdt.day;
		time_t t = mktime(&brokentime);
		t -= RTC_UNIX_TIMESTAMP_AT_EPOCH;

		uint8_t dow = RTC_day_of_week(t);
		if (dow != test_days_of_week[i].dow)
		{
			printf(
				"Error line %" PRIu32 ", want %" PRIu8 ", "
				"DOW=%" PRIu8 ", %04" PRIu16 "/%02" PRIu8 "/%02" PRIu8 "\n",
				i, test_days_of_week[i].dow, dow,
				test_days_of_week[i].bdt.year,
				test_days_of_week[i].bdt.month,
				test_days_of_week[i].bdt.day);
			errors++;
		}
	}

	// TODO day of year

	// TODO days since epoch

	// split to seconds since epoch
	printf("RTC_bd_to_seconds_since_epoch()\n");
	for (uint32_t i = 0; i < ARRAY_COUNT(test_bd_to_seconds_since_epoch); i++)
	{
		uint32_t seconds_since_epoch = RTC_bd_to_seconds_since_epoch(
			&test_bd_to_seconds_since_epoch[i].bdt);
		if (seconds_since_epoch != test_bd_to_seconds_since_epoch[i].seconds_since_epoch)
		{
			printf("Error line %" PRIu32 ", want %" PRIu32 ", got %" PRIu32 "\n",
				i,
				test_bd_to_seconds_since_epoch[i].seconds_since_epoch,
				seconds_since_epoch);
			errors++;
		}
	}

	// seconds since epoch to split
	printf("RTC_seconds_since_epoch_to_bd()\n");
	for (uint32_t i = 0; i < ARRAY_COUNT(test_bd_to_seconds_since_epoch); i++)
	{
		RTC_TIME_BD_t bdt;
		RTC_seconds_since_epoch_to_bd(
			test_bd_to_seconds_since_epoch[i].seconds_since_epoch, &bdt);
		if (RTC_bd_compare(&bdt, &test_bd_to_seconds_since_epoch[i].bdt) != 0)
		{
			printf("Error line %" PRIu32 ", want "
				"%04" PRIu16 "/%02" PRIu8 "/%02" PRIu8 " "
				"%02" PRIu8 ":%02" PRIu8 ":%02" PRIu8 ", got "
				"%04" PRIu16 "/%02" PRIu8 "/%02" PRIu8 " "
				"%02" PRIu8 ":%02" PRIu8 ":%02" PRIu8 "\n",
				i,
				test_bd_to_seconds_since_epoch[i].bdt.year,
				test_bd_to_seconds_since_epoch[i].bdt.month,
				test_bd_to_seconds_since_epoch[i].bdt.day,
				test_bd_to_seconds_since_epoch[i].bdt.hour,
				test_bd_to_seconds_since_epoch[i].bdt.minute,
				test_bd_to_seconds_since_epoch[i].bdt.second,
				bdt.year, bdt.month, bdt.day, bdt.hour, bdt.minute, bdt.second);
			errors++;
		}
	}

	// DST
	printf("RTC_eu_dst_start_day_of_month_bd(), RTC_eu_dst_end_day_of_month_bd()\n");
	uint16_t year = RTC_EPOCH_YEAR;
	for (uint32_t i = 20*2; i < ARRAY_COUNT(test_eu_dst_dates);)
	{
		uint8_t startd = test_eu_dst_dates[i++];
		uint8_t endd = test_eu_dst_dates[i++];
		if ((startd != RTC_eu_dst_start_day_of_month_bd(year)) ||
			(endd != RTC_eu_dst_end_day_of_month_bd(year)))
		{
			printf("Year %" PRIu16 ", Got %" PRIu8 "-%" PRIu8 ", wanted %" PRIu8 "-%" PRIu8 "\n",
				year, startd, endd,
				RTC_eu_dst_start_day_of_month_bd(year), RTC_eu_dst_end_day_of_month_bd(year));
			errors++;
		}
		year++;
	}

	printf("RTC_eu_dst_times()\n");
	year = RTC_EPOCH_YEAR;
	for (uint32_t i = 20 * 2; i < ARRAY_COUNT(test_eu_dst_dates);)
	{
		uint32_t startd;
		uint32_t endd;
		bool is_leap_year;
		RTC_TIME_BD_t bdt = {
			.year = year,
			.month = 1,
			.day = 1,
		};
		uint32_t time = make_time(&bdt);
		RTC_eu_dst_times(time, &startd, &endd, &is_leap_year);
		RTC_TIME_BD_t startbdt, endbdt;
		RTC_seconds_since_epoch_to_bd(startd, &startbdt);
		RTC_seconds_since_epoch_to_bd(endd, &endbdt);
		RTC_TIME_BD_t want_startbdt = {
			.year = year, .month = 3, .day = test_eu_dst_dates[i++],
			.hour = 1, .minute = 0, .second = 0,
		};
		RTC_TIME_BD_t want_endbdt = {
			.year = year, .month = 10, .day = test_eu_dst_dates[i++],
			.hour = 1, .minute = 0, .second = 0,
		};

		if ((RTC_bd_compare(&startbdt, &want_startbdt) != 0) ||
			(RTC_bd_compare(&endbdt, &want_endbdt) != 0))
		{
			uint32_t want_startt = RTC_bd_to_seconds_since_epoch(&want_startbdt);
			uint32_t want_endt = RTC_bd_to_seconds_since_epoch(&want_endbdt);
			printf("Year %" PRIu16 ", Got %" PRIu32 "-%" PRIu32 ", wanted %" PRIu32 "-%" PRIu32 "\n",
				year, startd, endd,
				want_startt, want_endt);
			printf("(%03"PRIu16"/%02"PRIu8"/%02"PRIu8" %02"PRIu8":%02"PRIu8":%02"PRIu8" - "
					"%03"PRIu16"/%02"PRIu8"/%02"PRIu8" %02"PRIu8":%02"PRIu8":%02"PRIu8
					", want "
					"%03"PRIu16"/%02"PRIu8"/%02"PRIu8" %02"PRIu8":%02"PRIu8":%02"PRIu8" - "
					"%03"PRIu16"/%02"PRIu8"/%02"PRIu8" %02"PRIu8":%02"PRIu8":%02"PRIu8
					")\n",
				startbdt.year, startbdt.month, startbdt.day, startbdt.hour, startbdt.minute, startbdt.second,
				endbdt.year, endbdt.month, endbdt.day, endbdt.hour, endbdt.minute, endbdt.second,
				want_startbdt.year, want_startbdt.month, want_startbdt.day, want_startbdt.hour, want_startbdt.minute, want_startbdt.second,
				want_endbdt.year, want_endbdt.month, want_endbdt.day, want_endbdt.hour, want_endbdt.minute, want_endbdt.second);
				errors++;
		}
		year++;
	}

	return errors;
}
