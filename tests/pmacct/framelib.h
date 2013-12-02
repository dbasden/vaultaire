#ifndef __FRAMELIB_H
#define __FRAMELIB_H
#include <stdint.h>

#include "DataFrame.pb-c.h"

/*
 * helpers to init data frames for various types
 * for any char * arguments, the function copies the pointer, not the underlying data.
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
size_t write_frame(FILE *fp, DataFrame *frame);

/**
 * output a frame including a frame length header
 *
 * return bytes written to the output stream if transmitted successfully
 * otherwise returns 0
 *
 * Note: It is possible that even if the frame is not written successfully to the
 * stream that the header will have written.
 */
size_t emit_frame(FILE *stream, DataFrame *framep);

/*
 * Wrapper functions to init then emit frames
 * 
 * return bytes written to the output stream if transmitted successfully
 * otherwise returns 0
 *
 * Note: It is possible that even if the frame is not written successfully to the
 * stream that the header will have written.
 */
size_t emit_uint64(FILE *stream, uint64_t timestamp, char *source, uint64_t value);
size_t emit_real(FILE *stream, uint64_t timestamp, char *source, double value);
size_t emit_text(FILE *stream, uint64_t timestamp, char *source, char * value);

#endif
