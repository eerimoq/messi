SRC += main.c
SRC += build/chat.c
SRC += build/chat_client.c
SRC += build/chat_common.c
SRC += build/pbtools.c
SRC += build/messi.c
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -I build

all:
	mkdir -p build
	PYTHONPATH=../../../.. \
	    python3 -m messi generate_c_source \
	        -o build -p linux ../../chat.proto
	gcc $(CFLAGS) $(SRC) -o client
