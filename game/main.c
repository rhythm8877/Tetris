#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>

#include "../libs/math.h"
#include "../libs/string.h"
#include "../libs/memory.h"
#include "../libs/screen.h"
#include "../libs/keyboard.h"

#define BOARD_W 10
#define BOARD_H 20

typedef struct {
    int row;
    int col;
    int type;
} Piece;

static Piece *current = 0;

static void cleanup(void)
{
    if (current) {
        my_dealloc(current);
        current = 0;
    }
    kb_restore();
    scr_clear();
}

static void sigint_handler(int sig)
{
    (void)sig;
    exit(0);
}

int main(void)
{
    int key;
    int r;
    int c;
    setlocale(LC_CTYPE, "en_US.UTF-8");
    mem_init();
    kb_init();
    atexit(cleanup);
    signal(SIGINT, sigint_handler);
    scr_clear();

    current       = (Piece *)my_alloc(sizeof(Piece));
    current->row  = 0;
    current->col  = my_div(BOARD_W, 2);
    current->type = 0;

    while (1) {
        key = kb_pressed();

        if (key == 'q')
            break;
        if (key == KEY_LEFT)
            current->col--;
        if (key == KEY_RIGHT)
            current->col++;
        if (key == KEY_DOWN)
            current->row++;

        current->col = my_clamp(current->col, 1, BOARD_W - 4);
        current->row = my_clamp(current->row, 0, BOARD_H - 1);

        scr_clear();
        scr_draw_border(0, 0, BOARD_H + 2, BOARD_W + 2);

        for (r = 0; r < 2; r++) {
            for (c = 0; c < 2; c++) {
                scr_puts(current->row + 1 + r, current->col + 1 + c * 2, "◻️");
            }
        }

        scr_puts(1, BOARD_W + 4, "SCORE:");
        scr_render_int(1, BOARD_W + 11, 0);
        scr_puts(2, BOARD_W + 4, "LEVEL:");
        scr_render_int(2, BOARD_W + 11, 1);

        fflush(stdout);
        usleep(50000);
    }

    cleanup();
    printf("Game exited cleanly.\n");
    return 0;
}
