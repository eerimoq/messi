all:
	pbtools generate_c_source hello_world.proto
	gcc -Wall server.c -o server
	gcc -Wall client.c -o client
