# Makefile for Llama Shell (lsh)

CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lreadline

TARGET = lsh
INSTALL_DIR = /usr/local/bin

all: $(TARGET)

$(TARGET): lsh.c
	$(CC) $(CFLAGS) -o $(TARGET) lsh.c $(LDFLAGS)

install: $(TARGET)
	install -m 755 $(TARGET) $(INSTALL_DIR)/

clean:
	rm -f $(TARGET)

.PHONY: all install clean