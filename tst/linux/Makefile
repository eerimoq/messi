TESTS += test_chat_server.c
SRC += build/chat.c
SRC += build/chat_server.c
SRC += build/chat_common.c
SRC += build/pbtools.c

default: build/chat.h
	$(MAKE) all

build/chat.h: ../../tests/files/chat/chat.proto
	mkdir -p build
	PYTHONPATH=../.. \
	    python3 -m messi generate_c_source \
		-o build -p linux ../../tests/files/chat/chat.proto

include ../test.mk
