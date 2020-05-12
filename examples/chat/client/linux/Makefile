all:
	mkdir -p generated
	PYTHONPATH=../../../.. \
	    python3 -m messi generate_c_source \
	        -o generated -p linux ../../chat.proto
#	gcc -I build build/chat.c main.c -o client
