CC=gcc
CFLAGS=-g -Wall -O2 -std=c99 -D_POSIX_C_SOURCE -D_GNU_SOURCE
LDFLAGS=-lprotobuf-c -lprofiler
PROTOCC=protoc-c
PROTO_PATH=../../../protobuf/
PROTO_SRC=${PROTO_PATH}DataFrame.proto

.PHONY: default
default: all

DataFrame.pb-c.c: ${PROTO_SRC}
	${PROTOCC} --proto_path=${PROTO_PATH} ${PROTO_SRC} --c_out .

frames2iptimetable: DataFrame.pb-c.c frames2iptimetable.c

.PHONY: clean
clean:
	rm -f test frames2iptimetable DataFrame.pb-c.[coh]

.PHONY: all
all: test frames2iptimetable

.PHONY: run_test
run_test: test
	./test
