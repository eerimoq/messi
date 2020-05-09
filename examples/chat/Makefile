all:
	pbtools generate_c_source chat.proto
	gcc -Wall server.c -o server
	gcc -Wall client.c -o client
