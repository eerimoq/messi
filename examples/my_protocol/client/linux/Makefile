SRC += main.c
SRC += build/my_protocol.c
SRC += build/my_protocol_client.c
SRC += ../../../../3pp/pbtools/lib/src/pbtools.c
SRC += ../../../../lib/src/messi.c
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -I build
CFLAGS += -I ../../../../lib/include
CFLAGS += -I ../../../../3pp/pbtools/lib/include

all:
	mkdir -p build
	PYTHONPATH=../../../.. \
	    python3 -m messi generate_c_source \
	        -o build -p linux ../../my_protocol.proto
	gcc $(CFLAGS) $(SRC) -o client
