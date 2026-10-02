// build.c
//
// Build timestamp and time derived build number strings

#define MONTH_CH0 (\
	  __DATE__ [2] == 't' ? '1' \
	: __DATE__ [2] == 'v' ? '1' \
	: __DATE__ [2] == 'c' ? '1' \
	: '0')

#define MONTH_CH1 (\
	  __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? '1' : '6') \
	: __DATE__ [2] == 'b' ? '2' \
	: __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? '3' : '4') \
	: __DATE__ [2] == 'y' ? '5' \
	: __DATE__ [2] == 'l' ? '7' \
	: __DATE__ [2] == 'g' ? '8' \
	: __DATE__ [2] == 'p' ? '9' \
	: __DATE__ [2] == 't' ? '0' \
	: __DATE__ [2] == 'v' ? '1' \
	: '2')

#define	DAY_CH0		( __DATE__ [4] == ' ' ? '0' : __DATE__ [4] )

const char build_timestamp[] = {
	__DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10],	// year
	'-', MONTH_CH0, MONTH_CH1,
	'-', DAY_CH0, __DATE__[5],								// day
	' ', __TIME__[0], __TIME__[1], __TIME__[2],
	__TIME__[3], __TIME__[4], __TIME__[5], __TIME__[6], __TIME__[7],
	'\0'
};

const char build_number[] = {
	__DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10],	// year
	MONTH_CH0, MONTH_CH1,
	DAY_CH0, __DATE__[5],									// day
	__TIME__[0], __TIME__[1], __TIME__[3], __TIME__[4], __TIME__[6], __TIME__[7],
	'\0'
};

#undef MONTH_CH0
#undef MONTH_CH1
#undef DAY_CH0
