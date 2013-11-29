#ifndef __FRAMELIB_H
#define __FRAMELIB_H
#include <stdint.h>

#include "DataFrame.pb-c.h"

/** init a data frame with specific values
 * the frame copies the pointer to source, not the string itself.
 * caller is responsible for copying first if they need
 */
void data_frame_init_numeric(DataFrame *frame,
		char *source, uint64_t timestamp, uint64_t value_numeric);

void data_frame_init_real(DataFrame *frame,
		char *source, uint64_t timestamp, double value_measurement);

void data_frame_init_text(DataFrame *frame,
		char *source, uint64_t timestamp, char * value_textual);

/**
 * write out a frame to a stream
 *
 * return bytes written to stream iff transmitted successfully
 * or 0 on error
 */
int write_frame(FILE *fp, DataFrame *frame);

/**
 * build and output a frame to a stream, preceeded by a length prelude
 *
 * The length prelude is a network byte ordered uint32 that contains the
 * amount of bytes to read for the successive frame
 *
 * return bytes written to stream iff transmitted successfully
 * or 0 on error
 */
int emit_uint64(FILE *stream, uint64_t timestamp, char *source, uint64_t value);

#endif
