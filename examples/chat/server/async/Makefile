all:
	mkdir -p generated
	PYTHONPATH=../../../.. \
	    python3 -m messi generate_c_source \
	        -o generated -p async ../../chat.proto
#	gcc -I build build/chat.c server.c main.c -o server
