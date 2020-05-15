TESTS += test_chat_server.c
TESTS += test_chat_client.c
SRC += chat/chat.c
SRC += chat/chat_server.c
SRC += chat/chat_client.c
SRC += chat/chat_common.c
SRC += imported/imported.c
SRC += imported/imported_server.c
SRC += imported/imported_client.c
SRC += imported/imported_common.c
SRC += imported/types_not_package_name.c
SRC += my_protocol/my_protocol.c
SRC += my_protocol/my_protocol_server.c
SRC += my_protocol/my_protocol_client.c
SRC += my_protocol/my_protocol_common.c
SRC += pbtools.c
SRC += ../../messi/c_source/messi.c
INC += chat
INC += imported
INC += ../../messi/c_source

default: messi-generate
	$(MAKE) all

messi-generate:
	@echo "Generating Messi files."
	for protocol in "chat" "my_protocol" ; do \
	    PYTHONPATH=../.. \
		python3 -m messi -d generate_c_source \
		    -o $$protocol -p linux \
		    ../../tests/files/$$protocol/$$protocol.proto && \
	    mv $$protocol/pbtools.* . && \
	    rm $$protocol/messi.* ; \
	done
	PYTHONPATH=../.. \
	    python3 -m messi -d generate_c_source \
		-o imported -p linux -I ../../tests/files/imported \
		../../tests/files/imported/types_not_package_name.proto \
		../../tests/files/imported/imported.proto
	rm imported/pbtools.* imported/messi.*

include ../test.mk
