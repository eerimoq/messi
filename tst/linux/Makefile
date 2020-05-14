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
SRC += pbtools.c
INC += chat
INC += imported

default: messi-generate
	$(MAKE) all

messi-generate:
	@echo "Generating Messi files."
	PYTHONPATH=../.. \
	    python3 -m messi -d generate_c_source \
	        -o chat -p linux \
	        ../../tests/files/chat/chat.proto
	mv chat/pbtools.* .
	PYTHONPATH=../.. \
	    python3 -m messi -d generate_c_source \
	        -o imported -p linux -I ../../tests/files/imported \
	        ../../tests/files/imported/types_not_package_name.proto \
	        ../../tests/files/imported/imported.proto

include ../test.mk
