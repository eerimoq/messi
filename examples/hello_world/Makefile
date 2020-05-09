all:
	pbtools generate_c_source greeting.proto
	gcc -Wall server.c -o server
	gcc -Wall client.c -o client
