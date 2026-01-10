CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Wextra $(FEATURES)
AR = ar
PREFIX = /usr/local

# see `sclexer.c` for more
FEATURES = -DENABLE_MSG_COLOR

HEADER_DIR = $(PREFIX)/include
TARGET_DIR = $(PREFIX)/lib

HEADER = sclexer.h
TARGET = libsclexer.a

SRC = sclexer.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean debug install uninstall
all: $(TARGET) $(HEADER)
debug: all main.c
	$(CC) -o sclexer main.c -g $(CFLAGS) -L. -lsclexer

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(AR) -rcs $@ $(OBJ)

clean:
	rm -f $(OBJ) $(TARGET) sclexer

install: all
	mkdir -p $(HEADER_DIR) $(TARGET_DIR)
	cp -f $(HEADER) $(HEADER_DIR)/$(HEADER)
	cp -f $(TARGET) $(TARGET_DIR)/$(TARGET)

uninstall:
	rm -f $(HEADER_DIR)/$(HEADER) $(TARGET_DIR)/$(TARGET)
