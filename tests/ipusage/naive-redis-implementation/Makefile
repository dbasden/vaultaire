PROTOBUF_DIR=../../../protobuf

DataFrame_pb2.py:
	protoc -I=$(PROTOBUF_DIR) --python_out=. $(PROTOBUF_DIR)/DataFrame.proto
clean:
	rm dataframe.pyc DataFrame_pb2.py DataFrame_pb2.pyc

