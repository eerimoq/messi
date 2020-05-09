all:
	mkdir -p build
	messager generate_c_source -o build hello_world.proto
	gcc -Wall server.c -o server
	gcc -Wall client.c -o client
