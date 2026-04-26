/*
 * game/main.c
 * Author : Amod
 * Desc   : Phase 1 Tetris — placeholder 2x2 piece, keyboard movement.
 * Date   : {{DATE}}
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

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
static int did_cleanup = 0;
static volatile sig_atomic_t g_quit = 0;

static void cleanup(void)
{
    if (did_cleanup)
        return;
    did_cleanup = 1;

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
    g_quit = 1;
}

int main(void)
{
    int key;
    int r;
    int c;
    struct sigaction sa;

    mem_init();
    kb_init();
    atexit(cleanup);

    sa.sa_handler = sigint_handler;
    sa.sa_flags   = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, 0);
    sigaction(SIGTERM, &sa, 0);

    scr_clear();

    current       = (Piece *)my_alloc(sizeof(Piece));
    current->row  = 0;
    current->col  = my_div(BOARD_W, 2);
    current->type = 0;

    while (!g_quit) {
        key = kb_pressed();

        if (key == 'q')
            break;
        if (key == KEY_LEFT)
            current->col--;
        if (key == KEY_RIGHT)
            current->col++;
        if (key == KEY_DOWN)
            current->row++;

        current->col = my_clamp(current->col, 0, BOARD_W - 2);
        current->row = my_clamp(current->row, 0, BOARD_H - 1);

        scr_clear();
        scr_draw_border(0, 0, BOARD_H + 2, 2 * BOARD_W + 2);

        for (r = 0; r < 2; r++) {
            for (c = 0; c < 2; c++) {
                scr_putchar(current->row + 1 + r,
                            (current->col + c) * 2 + 1, '[');
                scr_putchar(current->row + 1 + r,
                            (current->col + c) * 2 + 2, ']');
            }
        }

        scr_puts(1, 2 * BOARD_W + 4, "SCORE:");
        scr_render_int(1, 2 * BOARD_W + 11, 0);
        scr_puts(2, 2 * BOARD_W + 4, "LEVEL:");
        scr_render_int(2, 2 * BOARD_W + 11, 1);

        fflush(stdout);
        usleep(50000);
    }

    printf("Game exited cleanly.\n");
    return 0;
}
