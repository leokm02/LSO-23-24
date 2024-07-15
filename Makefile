# Define the compiler
CC = gcc

# Define flags for compilation (including pthread library)
CFLAGS = -pthread -g
ASAN_CFLAGS = -pthread -fsanitize=address -g

# Define object files
SERVER_OBJ = server.o queue.o
CLIENT_OBJ = client.o

# Define the target executables
SERVER = server
CLIENT = client
SERVER_ASAN = server_asan
CLIENT_ASAN = client_asan
SERVER_VALGRIND = server_valgrind
CLIENT_VALGRIND = client_valgrind

# Default rule to build everything
all: clean $(SERVER) $(CLIENT)

# Rule to compile server.c
server.o: server.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Rule to compile client.c
client.o: client.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Rule to compile queue.c
queue.o: queue.c queue.h
	$(CC) -c $< -o $@ $(CFLAGS)

# Rule to link the server executable
$(SERVER): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) -o $@ $(CFLAGS)

# Rule to link the client executable
$(CLIENT): $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) -o $@ $(CFLAGS)

# Rule to compile server with AddressSanitizer
$(SERVER_ASAN): server.c queue.c queue.h
	$(CC) server.c queue.c -o $@ $(ASAN_CFLAGS)

# Rule to compile client with AddressSanitizer
$(CLIENT_ASAN): client.c
	$(CC) client.c -o $@ $(ASAN_CFLAGS)

# Rule to compile server for Valgrind
$(SERVER_VALGRIND): server.c queue.c queue.h
	$(CC) server.c queue.c -o $@ $(CFLAGS)

# Rule to compile client for Valgrind
$(CLIENT_VALGRIND): client.c
	$(CC) client.c -o $@ $(CFLAGS)

# Rule to clean up object files
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER_ASAN) $(CLIENT_ASAN) $(SERVER_VALGRIND) $(CLIENT_VALGRIND)

# Rule to remove executables
delete:
	rm -f client server $(SERVER_ASAN) $(CLIENT_ASAN) $(SERVER_VALGRIND) $(CLIENT_VALGRIND)

# Rule to build everything with AddressSanitizer
asan: clean $(SERVER_ASAN) $(CLIENT_ASAN)

# Rule to build everything for Valgrind
valgrind: clean $(SERVER_VALGRIND) $(CLIENT_VALGRIND)