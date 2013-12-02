#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "framelib.h"

/**
 * This is only going to fly if we are getting data in on the fly
 * and we have no other timestamp source
 */
uint64_t timestamp_now() {
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts)) { perror("clock_gettime"); exit(2); }
	return (ts.tv_sec*1000000000) + ts.tv_nsec;
}

/*
 * parse a pmacct record line.
 *
 * *source_ip and *dest_ip are allocated by libc
 * caller must free() *source_ip and *dest_ip iff the call was successful
 * (as per sscanf)
 *
 * pre: instring is zero terminated
 *
 * returns >= 1 on success
 */
int parse_pmacct_record(char *cs, char **source_ip, char **dest_ip, uint64_t *bytes) {
	/* Format (with more whitespace in actual input):
	 *
	 * ID CLASS SRC_MAC DST_MAC VLAN SRC_AS DST_AS SRC_IP DST_IP SRC_PORT DST_PORT TCP_FLAGS PROTOCOL TOS PACKETS FLOWS BYTES
	 * 0 unknown 00:00:00:00:00:00 00:00:00:00:00:00 0 0 0 202.4.228.250 180.76.5.15 0 0 0 ip 0 24 0 34954
	 *
	 * We ignore everything other than source IP, destination IP, and bytes
	 */

	/* Not needed by POSIX, but works around eglibc passing these pointers 
	 * to realloc(3) before checking them
	 */
	//*source_ip = NULL; 
	//*dest_ip = NULL;
	return sscanf(cs, 
		"%*s%*s%*s%*s%*s%*s%*s"
#if _POSIX_C_SOURCE >= 200809L
		"%ms%ms"
#elif defined(__GLIBC__) && (__GLIBC__ >= 2)
		"%as%as"
#else
#error "Please let us have POSIX.1-2008, glibc, or a puppy"
#endif
		"%*s%*s%*s%*s%*s%*s%*s"
		"%lu",
		source_ip, dest_ip, bytes
		) == 3;
}

int main(int argc, char **argv) {
	FILE *infile;
	FILE *outfile;
	char buf[BUFSIZ];

	char *source_ip;
	char *dest_ip;
	uint64_t bytes;
	uint64_t timestamp;
	uint64_t last_timestamp;
	last_timestamp = timestamp_now();

	infile = stdin; /* slack */
	outfile = stdout; /* slack */

	while ( fgets(buf, BUFSIZ, infile) == buf ) {
		/* Ignore any line that doesn't start with a numeric
		 * ID. This gets around pmacct's stupid logging of 
		 * totally unimportant warnings to stdout
		 */
		if (buf[0] < '0' || buf[0] > '9')  continue;

		/* Keep timestamp the same for all items based on the same entry
		 * in case we need to cross correlate them later
		 */
		timestamp = timestamp_now();

		/* the 2tuple of source,timestamp must be unique for each frame
		 *
		 * Check to make sure that if we have a really course clock that we
		 * are still maintaining that invariant
		 */
		if (timestamp < last_timestamp)
			timestamp = last_timestamp + 1;   /* Great. NTP skew. My lucky day */
		if (timestamp == last_timestamp)
			timestamp++;
		last_timestamp = timestamp;

		buf[BUFSIZ-1] = 0;
		if (! parse_pmacct_record(buf, &source_ip, &dest_ip, &bytes))
			continue; /* Doesn't look like it's actually a record */

		/* emit a frame for both parties */
		if ( snprintf(buf, BUFSIZ, "ip_capture:traffic:%s:tx_bytes", source_ip) <10 ) {
			perror("snprintf"); return 2; }
		if (! emit_uint64(outfile, timestamp, buf, bytes)) {
			perror("emit_uint64"); return 1; }

		if ( snprintf(buf, BUFSIZ, "ip_capture:traffic:%s:rx_bytes", dest_ip) < 10)  {
			perror("snprintf"); return 3; }
		if (! emit_uint64(outfile, timestamp, buf, bytes)) {
			perror("emit_uint64"); return 1; }

		/* and who they were talking to */
		if ( snprintf(buf, BUFSIZ, "ip_capture:traffic:%s:dest_ip", source_ip) <10 ) {
			perror("snprintf"); return 2; }
		if (! emit_text(outfile, timestamp, buf, dest_ip)) {
			perror("emit_text"); return 1; }
		if ( snprintf(buf, BUFSIZ, "ip_capture:traffic:%s:src_ip", dest_ip) <10 ) {
			perror("snprintf"); return 2; }
		if (! emit_text(outfile, timestamp, buf, source_ip)) {
			perror("emit_text"); return 1; }


		free(source_ip);
		free(dest_ip);
	}

	return 0;
}
