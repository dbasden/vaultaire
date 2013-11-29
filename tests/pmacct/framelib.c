/**
 * really quick and dirty helper library to build and send frames
 * out a stream.
 *
 * More a prototype than production ready
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "DataFrame.pb-c.h"

/** init a data frame with specific values
 * the frame copies the pointer to source, not the string itself.
 * caller is responsible for copying first if they need
 */
void data_frame_init_numeric(DataFrame *frame,
		char *source, uint64_t timestamp, uint64_t value_numeric) {
	data_frame__init(frame);
	frame->source = source;
	frame->timestamp = timestamp;
	frame->payload = DATA_FRAME__TYPE__NUMBER;
	frame->value_numeric = value_numeric;
	frame->has_value_numeric = 1;
	frame->has_value_measurement = 0;
	frame->has_value_blob = 0;
}
void data_frame_init_real(DataFrame *frame,
		char *source, uint64_t timestamp, double value_measurement) {
	data_frame__init(frame);
	frame->source = source;
	frame->timestamp = timestamp;
	frame->payload = DATA_FRAME__TYPE__REAL;
	frame->value_measurement = value_measurement;
	frame->has_value_numeric = 0;
	frame->has_value_measurement = 1;
	frame->has_value_blob = 0;
}
void data_frame_init_text(DataFrame *frame,
		char *source, uint64_t timestamp, char * value_textual) {
	data_frame__init(frame);
	frame->source = source;
	frame->timestamp = timestamp;
	frame->payload = DATA_FRAME__TYPE__TEXT;
	frame->value_textual = value_textual;
	frame->has_value_numeric = 0;
	frame->has_value_measurement = 0;
	frame->has_value_blob = 0;
}


/** write out the length of the frame as a network byte ordered uint32_t
 *
 * returns number of bytes written or <= 0 if the write failed
 */
static size_t write_frame_length(FILE *fp, DataFrame *frame) {
	size_t packed_size;
	uint32_t prelude;

	packed_size = data_frame__get_packed_size(frame);

	prelude = htonl((uint32_t)(packed_size & 0xffffffff));
	if (fwrite(&prelude,sizeof(prelude), 1, fp) != 1)
		return 0;

	return sizeof(prelude);
}

/** pack and write a frame out to a stream
 */
size_t write_frame(FILE *fp, DataFrame *frame) {
	uint8_t buf[BUFSIZ];
	size_t packed_size;

	packed_size = data_frame__get_packed_size(frame);
	if (packed_size > BUFSIZ) { perror("Packed frame > BUFSIZ"); return 0; }
	data_frame__pack(frame, buf);

	if (fwrite(buf, packed_size, 1, fp) != 1)
		return 0;

	return packed_size;
}

/**
 * output an integer frame including a frame length header
 *
 * return bytes written to the output stream if transmitted successfully
 * otherwise returns 0
 *
 * Note: It is possible that even if the frame is not written successfully to the
 * stream that the header will have written.
 */
size_t emit_uint64(FILE *stream, uint64_t timestamp, char *source, uint64_t value) {
	DataFrame frame;
	size_t written_head;
	size_t written_frame;

	data_frame_init_numeric(&frame, source, timestamp, value);

	written_head = write_frame_length(stream, &frame);
	if (! written_head) 
		return 0;

	written_frame = write_frame(stream,&frame);

	return written_frame ? written_head + written_frame : 0;
}

