CC=gcc
LD=ld
CFLAGS=-O2 -Wall
#
LDLIBPROTO=-lprotobuf-c -lrt
LDSTATICLIBPROTO=-Wl,-Bstatic -lprotobuf-c -Wl,-Bdynamic -lrt
LDSTATICALL=-static -lprotobuf-c -lrt
#
PROTOCC=protoc-c
PROTO_PATH=../../protobuf/
PROTO_SRC=$(PROTO_PATH)DataFrame.proto

ALLBINS=pmacct2vault pmacct2vault.static pmacct2vault.staticlibproto

.PHONY: default
default: all

DataFrame.pb-c.c: $(PROTO_SRC)
	$(PROTOCC) --proto_path=$(PROTO_PATH) $(PROTO_SRC) --c_out .

framelib.o: DataFrame.pb-c.c DataFrame.pb-c.h

unpack_frames: DataFrame.pb-c.o pmacct2vault.o

pmacct2vault: DataFrame.pb-c.o framelib.o pmacct2vault.o
	$(CC) -o $@ $^ $(LDLIBPROTO)

pmacct2vault.staticlibproto: DataFrame.pb-c.o framelib.o pmacct2vault.o
	# statically link protobuf-c but dynamically link everything else
	$(CC) -o $@ $^ $(LDSTATICLIBPROTO)

pmacct2vault.static: DataFrame.pb-c.o framelib.o pmacct2vault.o
	$(CC) -o $@ $^ $(LDSTATICALL)

pmacct2vault.o: framelib.o

strip: $(ALLBINS)
	strip $^

tags:
	ctags .

.PHONY: clean
clean:
	rm -f DataFrame.pb-c.[coh] pmacct2vault.o framelib.o $(ALLBINS)

.PHONY: all
all: pmacct2vault
