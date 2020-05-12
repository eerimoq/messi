all:
	mkdir -p generated
	PYTHONPATH=../../../.. \
	    python3 -m messager generate_c_source \
	        -o generated -p async ../../chat.proto
#	gcc -I build build/chat.c client.c main.c -o client
