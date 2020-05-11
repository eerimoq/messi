SRC += main.c
SRC += build/chat.c
CFLAGS += -Wall
CFLAGS += -I build

all:
	mkdir -p generated
	PYTHONPAH=../../../.. \
	    python3 -m messager generate_c_source \
	        -o generated -p linux ../../chat.proto
	gcc $(CFLAGS) $(SRC) -o server
