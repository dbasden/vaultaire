/*
 * framecat: output data from frames, one line per frame
 *
 * This is pretty simplistic; We don't bother checking for whitespace
 * in any of the data
 *
 * format is "source timestamp value"
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <search.h>
#include <arpa/inet.h>

#include "DataFrame.pb-c.h"
#include "iptimetable.h"


#define BUFFER_SIZE	16384

/* Really blunt way to stop invalid data from protobuf-c libraries
 * letting us overrun our buffer.
 */
#define MAX_STRING_LEN	8000

#define INBOUND		'r'
#define OUTBOUND	'w'

/* Make sure that the strings in the frame are null
 * terminated (at least up to MAX_STRING
 */
int check_frame_bounds(DataFrame *frame){
	if (strnlen(frame->source, MAX_STRING_LEN) >= MAX_STRING_LEN)
		return 1;
	if (frame->payload == DATA_FRAME__TYPE__TEXT && 
		strnlen(frame->value_textual, MAX_STRING_LEN) >= MAX_STRING_LEN)
		return 1;
	return 0;
}

/* read the next frame from a stream
 *
 * returns a pointer to DataFrame on success, otherwise NULL
 * caller must call data_frame__free_unpacked on the returned DataFrame
 * pointer when finished with it
 */
DataFrame * read_next_frame(FILE *fp) {
	uint8_t buf[BUFFER_SIZE];
	DataFrame *frame;
	size_t b;


	/* network ordered uint32_t leads saying how many bytes to read
	 * for the next frame
	 */
	uint32_t prelude;
	b = fread(&prelude, sizeof(prelude), 1, stdin);
	if (b < 1) 
		return NULL;

	prelude = ntohl(prelude);
	if (prelude > BUFSIZ) {
		fprintf(stderr, "header said frame was %u bytes, but our buffer is only %u bytes. Bailing\n",prelude, BUFSIZ);
		return NULL;
	}
	b = fread(buf, prelude, 1, stdin);
	if (b < 1) { perror("fread didn't return frame"); return NULL;}

	frame = data_frame__unpack(NULL,prelude,buf);
	if (frame == NULL) { perror("data_frame__unpack"); return NULL; }

	if (check_frame_bounds(frame)) { 
		perror("frame string overflow"); 
		data_frame__free_unpacked(frame,NULL); return NULL; 
	}

	return frame;
}


/* string matches ip_capture:traffic:*:rx_bytes
 * returns char * to ip address string if matched otherwise NULL
 */
char * match_source(char *source, char *direction) {
	char * ret;
	char ignore;
	int k;
	k = sscanf(source, "ip_capture:traffic:%m[^:]:%cx_byte%c", &ret, direction, &ignore);
	if (k == 3 && (*direction == 'r' || *direction == 't'))
		return ret;
	if (k > 0)
		free(ret);
	return NULL;
}

/* One label per entry in the iptimetable */
char *labels[IPSPACE];
char *text_labels[IPSPACE];
size_t next_label = 0;

/*** hash table index for ip -> row index */

/* return the entry number for a label */
size_t get_entry_number(char *label) {
	ENTRY e, *result;
	struct in_addr in;
	char *desc_label=label;

	/* If we are going to use hsearch with it's painfully slow strcmp, we can
	 * at least make the search quick and use a uint32 to store any ipv4 addresses
	 *
	 * Falls back to strings for a huge performance hit (but supports non-ipv4 addresses)
	 */
	union {
		char fakestr[8];
		struct v4addr {
			uint32_t s_addr;
			uint32_t zero;
		} v4addr;
	} v4;
	if (inet_aton(label, &in)) {
		v4.v4addr =  ((struct v4addr){ .s_addr = in.s_addr, .zero = 0 });
		label = v4.fakestr;
	}

	e.key = label;
	e.data = NULL;
	result = hsearch(e, FIND);
	if (result == NULL) {
		if (next_label >= IPSPACE) {
			fprintf(stderr, "run out of entries to add to table\n");
			return -1;
		}

		fprintf(stderr, "adding label %s as entry %lu\n", desc_label, next_label);
		text_labels[next_label] = strdup(desc_label);

		e.key = strdup(label);
		e.data = (void *) ((size_t)next_label);
		labels[next_label] = e.key;

		result = hsearch(e, ENTER);
		if (result == NULL) { free(e.key); perror("hsearch(ENTER, ...)"); return -1; }

		next_label++;
	}
	return (size_t) result->data;
}

/* free up memory from the ip addresses */
void cleanup_labels() {
	int i;
	hdestroy(); 
	for (i=0; i < next_label; ++i) {
		free(labels[i]);
		free(text_labels[i]);
	}
}

int main(int argc, char **argv) {
	int exitcode = 0;
	uint64_t *inbound_table;
	uint64_t *outbound_table;

	/* hash table */
	if (!hcreate(IPSPACE * 4))  { perror("hcreate"); return 1; }

	/* Malloc tables of unusual size */
	inbound_table = calloc(1,IPTIMETABLESIZE);
	if (inbound_table == NULL) {
	       	perror("calloc"); return 1; 
	}
	outbound_table = calloc(1,IPTIMETABLESIZE);
	if (outbound_table == NULL) { 
		free(inbound_table); 
		perror("calloc"); return 1; 
	}

	while (1) {
		DataFrame *frame;

		frame = read_next_frame(stdin);
		if (frame == NULL)  {
			if (!feof(stdin))
				exitcode = 1;
			break;
		}

		/* skip anything that isn't a number */
		if (frame->payload != DATA_FRAME__TYPE__NUMBER) {
			data_frame__free_unpacked(frame,NULL);
			continue;
		}
		
		/* get the ip and flow direction from the header
		 * otherwise skip the frame
		 */
		char *ipstr;
		char direction;

		ipstr = match_source(frame->source, &direction);
		if (ipstr == NULL) {
			data_frame__free_unpacked(frame,NULL);
			continue;
		}

		/* Pull timestamp into seconds and bounds check */
		frame->timestamp /= 1000000000LLU;
		if (! TIMESTAMP_WITHIN_BOUNDS(frame->timestamp)) {
			fprintf(stderr, "Timestamp %lu isn't within the table dimensions. Bailing\n", frame->timestamp);
			data_frame__free_unpacked(frame,NULL);
			exitcode = 2;
			break;
		}

		/* Find out what row in the table the IP goes to */
		size_t row;
		row = get_entry_number(ipstr);
		if (row == -1) {
			data_frame__free_unpacked(frame,NULL);
			exitcode = 3;
			break;
		}

		size_t cell = IP_ROW(row) + TIMESTAMPCOL(frame->timestamp);
		assert( WITHIN_BOUNDS(cell) );

		if (direction == INBOUND)
			inbound_table[cell] += frame->value_numeric;
		else if (direction == OUTBOUND)
			outbound_table[cell] += frame->value_numeric;

		free(ipstr);
		data_frame__free_unpacked(frame,NULL);
	}

	if (exitcode == 0) {
		int ret;
		/* so far so good. write out the inbound table */
		ret = fwrite(inbound_table, IPTIMETABLESIZE, 1, stdout);
		if (!ret) { perror("fwrite"); exitcode = 0; }
	}
	if (exitcode == 0) {
		int ret;
		/* now the outbound table */
		ret = fwrite(outbound_table, IPTIMETABLESIZE, 1, stdout);
		if (!ret) { perror("fwrite"); exitcode = 0; }
	}
	if (exitcode == 0) {
		/* and finally, the addresses */
		int i;
		for (i=0; i<next_label; ++i)
			fwrite(text_labels[i], strlen(text_labels[i])+1, 1, stdout);
		for (; i<IPSPACE; ++i)
			fwrite("",1,1,stdout);
	}

	free(outbound_table);
	free(inbound_table);
	cleanup_labels();
	return exitcode || !feof(stdin);
}
