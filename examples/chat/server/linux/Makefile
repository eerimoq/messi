SRC += main.c
SRC += generated/chat.c
SRC += generated/chat_server.c
SRC += generated/chat_common.c
SRC += generated/pbtools.c
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -I generated

all:
	mkdir -p generated
	PYTHONPATH=../../../.. \
	    python3 -m messi generate_c_source \
	        -o generated -p linux ../../chat.proto
	gcc $(CFLAGS) $(SRC) -o server
