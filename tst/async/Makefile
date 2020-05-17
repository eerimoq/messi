TESTS += test_chat_client.c
SRC += chat/chat.c
SRC += chat/chat_server.c
SRC += chat/chat_client.c
SRC += my_protocol/my_protocol.c
SRC += my_protocol/my_protocol_server.c
SRC += my_protocol/my_protocol_client.c
SRC += pbtools.c
SRC += ../../messi/c_source/messi.c
INC += chat
INC += imported
INC += ../../messi/c_source
INC += ../../3pp/async/include
INC += ../../3pp/async/3pp/mbedtls/include

default: messi-generate
	$(MAKE) all

messi-generate:
	@echo "Generating Messi files."
	for protocol in "chat" "my_protocol" ; do \
	    mkdir -p $$protocol && \
	    PYTHONPATH=../.. \
		python3 -m messi -d generate_c_source \
		    -o $$protocol -p async \
		    ../../tests/files/$$protocol/$$protocol.proto && \
	    mv $$protocol/pbtools.* . && \
	    rm $$protocol/messi.* ; \
	done

include ../test.mk
