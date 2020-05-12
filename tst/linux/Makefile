TESTS += test_chat_server.c

default: build/chat.h
	$(MAKE) all

build/chat.h: ../../tests/files/chat.proto
	mkdir -p build
	PYTHONPATH=../.. \
	    python3 -m messager generate_c_source \
		-o build -p linux ../../tests/files/chat.proto

include ../test.mk
