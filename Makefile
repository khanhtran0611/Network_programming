CC = gcc
CFLAGS = -Wall -g -std=c11
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)
SQLITE_LIBS = -lsqlite3

# Server
SERVER_SRC = server.c database.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)
SERVER_TARGET = server

# Client Group
CLIENT_GROUP_SRC = client_group.c
CLIENT_GROUP_OBJ = $(CLIENT_GROUP_SRC:.c=.o)
CLIENT_GROUP_TARGET = client_group

# Original Client
CLIENT_SRC = client.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
CLIENT_TARGET = client

.PHONY: all clean server client_group client test

all: server client_group client

server: $(SERVER_TARGET)

$(SERVER_TARGET): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(SQLITE_LIBS)

client_group: $(CLIENT_GROUP_TARGET)

$(CLIENT_GROUP_TARGET): $(CLIENT_GROUP_OBJ)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $^ $(GTK_LIBS)

client: $(CLIENT_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $^ $(GTK_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_GROUP_OBJ) $(CLIENT_OBJ)
	rm -f $(SERVER_TARGET) $(CLIENT_GROUP_TARGET) $(CLIENT_TARGET)
	rm -f groups.db

test: server client_group
	@echo "Compilation successful! Now you can run:"
	@echo "  Terminal 1: ./server"
	@echo "  Terminal 2: ./client_group"

