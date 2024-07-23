# Define the compiler
CC = gcc

# Define flags for compilation (including pthread library)
CFLAGS = -pthread -g -Wall -pedantic


# Define object files
SERVER_OBJ = server/server.o utils/queue.o utils/logger.o
CLIENT_OBJ = client/client.o utils/queue.o 

# Define the target executables
SERVER = server.out
CLIENT = client.out

# Default rule to build everything
all: clean $(SERVER) $(CLIENT)

# Rule to compile server.c
server.o: server/server.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Rule to compile client.c
client.o: client/client.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Rule to compile queue.c
queue.o: utils/queue.c utils/queue.h
	$(CC) -c $< -o $@ $(CFLAGS)

# Rule to link the server executable
$(SERVER): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o $@ $(CFLAGS)

# Rule to link the client executable
$(CLIENT): $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o $@ $(CFLAGS)

# Rule to clean up object files
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ)
# Rule to remove executables
delete:
	rm -f client/client server/server client.out server.out
