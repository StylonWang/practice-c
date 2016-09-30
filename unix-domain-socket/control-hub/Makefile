
default:: server client

clean::
	rm -f server client

server: server.c
	gcc -Wall $< -o $@ -pthread

client: client.c
	gcc -Wall $< -o $@ -pthread
