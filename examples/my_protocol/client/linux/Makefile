SRC += main.c
SRC += build/my_protocol.c
SRC += build/my_protocol_client.c
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
	        -o build -p linux ../../my_protocol.proto
	gcc $(CFLAGS) $(SRC) -o client
