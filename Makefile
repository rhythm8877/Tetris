CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=c99 -Ilibs

LIB_SRCS  = libs/math.c \
            libs/string.c \
            libs/memory.c \
            libs/screen.c \
            libs/keyboard.c \
            libs/safe.c \
            libs/rng.c \
            libs/timer.c \
            libs/fileio.c

GAME_SRCS = game/main.c \
            game/board.c \
            game/piece.c \
            game/score.c \
            game/menu.c

SRCS      = $(LIB_SRCS) $(GAME_SRCS)
OBJS      = $(SRCS:.c=.o)
TARGET    = tetris

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean run
