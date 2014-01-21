#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if __WORDSIZE < 64
#error "64 bit code of evil. Get some more bits."
#endif

#define IPSPACE	8192LLU
#define TENMINUTE	1LLU
#define HOUR	6LLU
#define DAY	144LLU
#define WEEK	( DAY * 7LLU )
#define YEAR	( DAY * 366LLU )

#define START_OF_YEAR	1356958800LLU

#define IP_ROW(_IP)		((_IP) * YEAR)
#define JULIANCOL(_JULIAN)	(_JULIAN)
#define TIMESTAMPCOL(_TIMESTAMP)	(((_TIMESTAMP)-START_OF_YEAR)/)

#define IP_WITHIN_BOUNDS(_IP)	((_IP) >= 0 && (_IP) < IPSPACE)
#define TIMESTAMP_WITHIN_BOUNDS(_TS)	(((_TS) >= START_OF_YEAR) && (_TS) < (START_OF_YEAR + (10 * YEAR)))

#define IPTIMETABLESIZE		(YEAR * IPSPACE)
#define WITHIN_BOUNDS(_INDEX)	((_INDEX) >= 0 && (_INDEX) < (IPTIMETABLESIZE))

uint64_t sum(uint64_t * iptimetable, uint64_t start, uint64_t end) {
	uint64_t i;
	uint64_t tot=0;
	for (i=start; i < end; ++i )
		tot += iptimetable[i];
	return tot;
}

#define sum_for_hour(_iptt,_st)		sum((_iptt),(_st),(_st)+HOUR)
#define sum_for_day(_iptt,_st)		sum((_iptt),(_st),(_st)+DAY)
#define sum_for_week(_iptt,_st)		sum((_iptt),(_st),(_st)+WEEK)
#define sum_for_year(_iptt,_st)		sum((_iptt),(_st),(_st)+YEAR)


int main() {
	uint64_t *iptimetable;

	printf("Attempting to malloc %llu bytes\n", YEAR * IPSPACE * sizeof(uint64_t));

	iptimetable = calloc(1, IPTIMETABLESIZE * sizeof(uint64_t));
	if (iptimetable == NULL) {
		perror("calloc()");
		return 1;
	}

	printf("filling with some fake data\n");
	uint64_t i;
	for (i=0; i<IPTIMETABLESIZE;++i)
		iptimetable[i] = random();

	printf("Daily totals for ip 4242\n");
	for (i=IP_ROW(4242); i<IP_ROW(4242)+YEAR; i += DAY)
		printf("%lu\n", sum_for_day(iptimetable, i));

	printf("Weekly totals for ip 4242\n");
	for (i=IP_ROW(4242); i<IP_ROW(4242)+YEAR; i += WEEK)
		printf("%lu\n", sum_for_week(iptimetable, i));

	printf("Yearly total for ip 4242\n");
	printf("%lu\n", sum_for_year(iptimetable, i));

	printf("Yearly totals for each IP\n");
	for (i=0; i<IPTIMETABLESIZE; i += YEAR)
		printf("%lu\n", sum_for_year(iptimetable, i));

	uint64_t tot = 0;
	printf("Total yearly value for entire IP space\n");
	for (i=0; i<IPTIMETABLESIZE; i++)
		tot += iptimetable[i];
	printf("%lu\n", tot);

	free(iptimetable);
	return 0;
}
