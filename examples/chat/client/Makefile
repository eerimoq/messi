all:
	messager generate_c_source -o build -p async ../chat.proto
	gcc -I build build/chat.c client.c main.c -o client
