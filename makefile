CC = gcc

all : client server

client :client_dir/client.c
	$(CC) -o client_dir/client client_dir/client.c

server :server_dir/server.c
	$(CC) -o server_dir/server server_dir/server.c

clean :
	rm client
	rm server

