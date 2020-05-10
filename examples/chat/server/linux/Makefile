all:
	messager generate_c_source -o build -p linux ../../chat.proto
	gcc -I build build/chat.c main.c -o server
