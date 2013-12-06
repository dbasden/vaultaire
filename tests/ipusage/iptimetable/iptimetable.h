#ifndef _IPTIMETABLE_H
#define _IPTIMETABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if __WORDSIZE < 64
#error "64 bit code of evil. Get some more bits."
#endif

#define SECONDS_PER_BIN	600LLU

#define IPSPACE	8192LLU
#define TENMINUTE	(600LLU / SECONDS_PER_BIN)
#define HOUR	(3600LLU / SECONDS_PER_BIN)
#define DAY	( HOUR * 24LLU )
#define WEEK	( DAY * 7LLU )
#define YEAR	( DAY * 366LLU )

#define START_OF_YEAR	1356958800LLU

#define IP_ROW(_IP)		((_IP) * YEAR)
#define JULIANCOL(_JULIAN)	((_JULIAN) * DAY)
#define TIMESTAMPCOL(_TIMESTAMP)	(((_TIMESTAMP)-START_OF_YEAR)/SECONDS_PER_BIN)

#define IP_WITHIN_BOUNDS(_IP)	((_IP) >= 0 && (_IP) < IPSPACE)
#define TIMESTAMP_WITHIN_BOUNDS(_TS)	(((_TS) >= START_OF_YEAR) && (_TS) < (START_OF_YEAR + (SECONDS_PER_BIN * YEAR)))

#define IPTIMETABLECELLS	(YEAR * IPSPACE)
#define IPTIMETABLESIZE		(IPTIMETABLECELLS * sizeof(uint64_t))
#define WITHIN_BOUNDS(_INDEX)	((_INDEX) >= 0 && (_INDEX) < (IPTIMETABLECELLS))

static __inline__ uint64_t sum(uint64_t * iptimetable, uint64_t start, uint64_t end) {
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

#endif
