# MAKEFILE for the C server

# compiler and flags
CC = gcc
CFLAGS = -Wall -lpthread

# server binary name
TARGET = server

# source files
SRC = server.c

# object files
OBJ = $(SRC:.c=.o)

# dt: compile the server
all: $(TARGET)

# lk the object files -> create the executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# compile the C source files -> object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean object files and binary
clean:
	rm -f $(OBJ) $(TARGET)

# rebuild everything
rebuild: clean all

# run the server!!!
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean rebuild run
