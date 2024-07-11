# Define the compiler
CC = gcc

# Define flags for server compilation (including pthread library)
SERVER_CFLAGS = -lpthread

# Define object files
SERVER_OBJ = server.o
CLIENT_OBJ = client.o

# Define the target executables
SERVER = server
CLIENT = client

# Rule to compile server.c 
server.o: server.c
	$(CC) -c $< -o $@

# Rule to compile client.c
client.o: client.c
	$(CC) -c $< -o $@

# Rule to link the server executable
$(SERVER): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o $@ $(SERVER_CFLAGS)

# Rule to link the client executable
$(CLIENT): $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o $@

# Rule to clean up object files
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ)

