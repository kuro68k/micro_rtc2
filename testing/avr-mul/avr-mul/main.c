/*
 * avr-mul.c
 *
 * Created: 25/09/2026 20:08:16
 * Author : kuro68k
 */

#include <stdbool.h>
#include <avr/io.h>

#define	SECONDS_PER_MINUTE			(60)
#define	SECONDS_PER_HOUR			(60 * 60)
#define	SECONDS_PER_DAY				(60L * 60 * 24)
#define	SECONDS_PER_NON_LEAP_YEAR	(SECONDS_PER_DAY * 365)
#define	SECONDS_PER_LEAP_YEAR		(SECONDS_PER_DAY * 366)
#define SECONDS_PER_LEAP_CENTURY	(3155760000)



uint32_t test(uint32_t epoch_seconds)
{
	//return (uint32_t)(((uint64_t)epoch_seconds * 3257812231ULL) >> 48);
	//return epoch_seconds / 8640;
	uint16_t year = 0;
	while (epoch_seconds > SECONDS_PER_NON_LEAP_YEAR)
	{
		epoch_seconds -= SECONDS_PER_NON_LEAP_YEAR;
		year++;
	}
	return year;
}

int main(void)
{
	TCC0.CNT = 0;
	TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
	volatile uint32_t t = test(1790363412);
	TCC0.CTRLA = 0;
	
	for (;;);
}

