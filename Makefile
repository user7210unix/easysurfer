CC = gcc
CFLAGS = -Wall -g `pkg-config --cflags gtk+-3.0 libsoup-2.4`
LIBS = `pkg-config --libs gtk+-3.0 libsoup-2.4`

SRC = src/main.c src/browser.c src/bookmarks.c src/quickmarks.c src/sessions.c
OBJ = $(SRC:.c=.o)
TARGET = easysurfer

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
