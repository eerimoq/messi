TESTS += test_chat_server.c
TESTS += test_chat_client.c
SRC += chat/chat.c
SRC += chat/chat_server.c
SRC += chat/chat_client.c
SRC += chat/chat_common.c
SRC += pbtools.c
INC += chat

default: messi-generate
	$(MAKE) all

messi-generate:
	@echo "Generating Messi files."
	PYTHONPATH=../.. \
	    python3 -m messi generate_c_source \
		-o chat -p linux ../../tests/files/chat/chat.proto
	mv chat/pbtools.* .

include ../test.mk
