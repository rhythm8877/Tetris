/*
 * game/main.c
 * Author : Amod
 * Desc   : Phase 2 Tetris — full game wiring (menu, gameplay, scoring, hold).
 * Date   : {{DATE}}
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "../libs/math.h"
#include "../libs/string.h"
#include "../libs/memory.h"
#include "../libs/screen.h"
#include "../libs/keyboard.h"
#include "../libs/safe.h"
#include "../libs/timer.h"
#include "../libs/rng.h"
#include "../libs/fileio.h"

#include "piece.h"
#include "board.h"
#include "score.h"
#include "menu.h"

#define SCORES_PATH      "data/scores.dat"
#define FRAME_SLEEP_US   16000
#define TICK_MS          50
#define BOARD_TOP        1
#define BOARD_LEFT       1
#define SIDE_COL         (BOARD_LEFT + CELL_W * BOARD_W + 5)

/* ------------------------------------------------------------------ */
/*  Globals — only flags touched by signal handlers                    */
/* ------------------------------------------------------------------ */

/* g_quit is referenced from menu.c so the menu polling loops can break
 * out on SIGINT/SIGTERM. Keep file-scope but with external linkage. */
volatile sig_atomic_t        g_quit      = 0;
static volatile sig_atomic_t g_winch     = 0;
static int                   did_cleanup = 0;

/* ------------------------------------------------------------------ */
/*  Signal handlers — async-signal-safe (flag-only)                    */
/* ------------------------------------------------------------------ */

static void on_quit(int sig)
{
    (void)sig;
    g_quit = 1;
}

static void on_winch(int sig)
{
    (void)sig;
    g_winch = 1;
}

static void install_signals(void)
{
    struct sigaction sa;

    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = on_quit;
    sigaction(SIGINT,  &sa, 0);
    sigaction(SIGTERM, &sa, 0);

    sa.sa_handler = on_winch;
    sigaction(SIGWINCH, &sa, 0);
}

/* ------------------------------------------------------------------ */
/*  Cleanup                                                            */
/* ------------------------------------------------------------------ */

static void cleanup(void)
{
    if (did_cleanup)
        return;
    did_cleanup = 1;

    tm_stop();
    kb_restore();
    scr_shutdown();
    fprintf(stderr, "Game exited cleanly.\n");
}

/* ------------------------------------------------------------------ */
/*  Rendering helpers                                                  */
/* ------------------------------------------------------------------ */

static void render_cell_block(int row, int col, int color_id)
{
    /* Three-char cell: "[ ]" — left bracket, space, right bracket.
     * The brackets are coloured so the cell looks like a wide block. */
    scr_set_color(color_id);
    scr_putchar(row, col,     '[');
    scr_putchar(row, col + 1, ' ');
    scr_putchar(row, col + 2, ']');
    scr_set_color(0);
}

static void render_empty_block(int row, int col)
{
    scr_putchar(row, col,     ' ');
    scr_putchar(row, col + 1, ' ');
    scr_putchar(row, col + 2, ' ');
}

static void render_preview(int top, int left, const Piece *p)
{
    int dr, dc;

    for (dr = 0; dr < PIECE_BOX; dr++) {
        for (dc = 0; dc < PIECE_BOX; dc++) {
            int row = top + dr;
            int col = left + my_mul(dc, CELL_W);
            if (p && piece_cell(p->type, p->rot, dr, dc))
                render_cell_block(row, col, p->color_id);
            else
                render_empty_block(row, col);
        }
    }
}

static void render_frame(const Board *b, const Piece *cur, const Piece *nxt,
                         const Piece *hold, const char *name)
{
    int r, c, dr, dc;

    if (g_winch) {
        g_winch = 0;
        scr_force_full_redraw();
    }

    scr_clear();

    /* Playfield border */
    scr_draw_border(BOARD_TOP, BOARD_LEFT,
                    BOARD_H + 2,
                    my_mul(CELL_W, BOARD_W) + 2);

    /* Locked cells */
    for (r = 0; r < BOARD_H; r++) {
        for (c = 0; c < BOARD_W; c++) {
            int color = b->cells[my_mul(r, BOARD_W) + c];
            int row   = BOARD_TOP + 1 + r;
            int col   = BOARD_LEFT + 1 + my_mul(c, CELL_W);
            if (color != 0)
                render_cell_block(row, col, color);
            else
                render_empty_block(row, col);
        }
    }

    /* Active piece */
    for (dr = 0; dr < PIECE_BOX; dr++) {
        for (dc = 0; dc < PIECE_BOX; dc++) {
            if (piece_cell(cur->type, cur->rot, dr, dc)) {
                int rr = cur->row + dr;
                int cc = cur->col + dc;
                if (rr >= 0 && rr < BOARD_H && cc >= 0 && cc < BOARD_W) {
                    int row = BOARD_TOP + 1 + rr;
                    int col = BOARD_LEFT + 1 + my_mul(cc, CELL_W);
                    render_cell_block(row, col, cur->color_id);
                }
            }
        }
    }

    /* Side panel */
    scr_puts(BOARD_TOP,      SIDE_COL, "NEXT:");
    render_preview(BOARD_TOP + 1, SIDE_COL, nxt);

    scr_puts(BOARD_TOP + 6,  SIDE_COL, "HOLD:");
    if (hold)
        render_preview(BOARD_TOP + 7, SIDE_COL, hold);
    else
        scr_puts(BOARD_TOP + 7, SIDE_COL, "-");

    scr_puts(BOARD_TOP + 12, SIDE_COL, "NAME:");
    scr_puts(BOARD_TOP + 12, SIDE_COL + 6, name);

    scr_puts(BOARD_TOP + 14, SIDE_COL, "SCORE:");
    scr_render_int(BOARD_TOP + 14, SIDE_COL + 7, b->score);

    scr_puts(BOARD_TOP + 15, SIDE_COL, "LEVEL:");
    scr_render_int(BOARD_TOP + 15, SIDE_COL + 7, b->level);

    scr_puts(BOARD_TOP + 16, SIDE_COL, "LINES:");
    scr_render_int(BOARD_TOP + 16, SIDE_COL + 7, b->lines);

    scr_present();
}

/* ------------------------------------------------------------------ */
/*  Gameplay                                                           */
/* ------------------------------------------------------------------ */

static void play_one_game(const char *name, ScoreTable *table)
{
    Board *b;
    Piece *cur;
    Piece *nxt;
    Piece *hold            = (Piece *)0;
    int    hold_used       = 0;
    int    gravity_counter = 0;

    b   = board_new();
    cur = piece_new(rng_bag_next());
    nxt = piece_new(rng_bag_next());

    /* Force a full redraw on the first frame so any leftover content
     * from the welcome screen (including terminal-echoed typed name
     * that bypassed our back buffer) is wiped before we paint the
     * game. */
    scr_force_full_redraw();

    /* Drop any SIGALRM ticks that accumulated while the user was on
     * the menu / name-entry screens. Without this, the menu phase's
     * 5–30 seconds of pending ticks all fire on the first gameplay
     * iteration, instantly gravity-dropping the first piece (and
     * sometimes locking it before the first frame even renders). */
    tm_reset_ticks();

    while (!g_quit) {
        int k;

        /* Drain pending gravity ticks */
        while (tm_consume_tick()) {
            gravity_counter++;
            if (my_mul(gravity_counter, TICK_MS) >= tm_fall_period_ms(b->level)) {
                gravity_counter = 0;
                if (board_can_place(b, cur, +1, 0, 0)) {
                    cur->row++;
                } else {
                    int n;

                    board_lock(b, cur);
                    n = board_clear_lines(b);
                    if (n > 0)
                        board_apply_score(b, n);
                    b->lines += n;
                    board_update_level(b);

                    piece_free(cur);
                    cur = nxt;
                    nxt = piece_new(rng_bag_next());
                    hold_used = 0;

                    if (!board_can_place(b, cur, 0, 0, 0)) {
                        menu_render_game_over(b->score, b->lines);
                        scr_present();
                        while (!g_quit) {
                            int kk = kb_pressed();
                            if (kk == KEY_ENTER)
                                break;
                            usleep(FRAME_SLEEP_US);
                        }
                        goto end_game;
                    }
                }
            }
        }

        /* Input */
        k = kb_pressed();
        if (k == 'q')
            goto end_game;

        if (k == 'p') {
            tm_stop();
            menu_render_pause_overlay();
            scr_present();
            while (!g_quit) {
                int kk = kb_pressed();
                if (kk == 'p' || kk == 'P')
                    break;
                usleep(FRAME_SLEEP_US);
            }
            tm_resume();
            scr_force_full_redraw();
            continue;
        }

        if (k == KEY_LEFT  && board_can_place(b, cur, 0, -1, 0))
            cur->col--;
        if (k == KEY_RIGHT && board_can_place(b, cur, 0, +1, 0))
            cur->col++;
        if (k == KEY_DOWN  && board_can_place(b, cur, +1, 0, 0)) {
            cur->row++;
            b->score++;
        }

        if (k == KEY_UP) {
            int i;
            int n_kicks = piece_kick_count(cur->type);
            for (i = 0; i < n_kicks; i++) {
                int dr, dc;
                piece_kick_offset(cur->type, i, &dr, &dc);
                if (board_can_place(b, cur, dr, dc, +1)) {
                    cur->row += dr;
                    cur->col += dc;
                    piece_rotate_cw(cur);
                    break;
                }
            }
        }

        if (k == KEY_SPACE) {
            int drop = 0;
            while (board_can_place(b, cur, drop + 1, 0, 0))
                drop++;
            cur->row += drop;
            b->score += my_mul(drop, 2);
            gravity_counter = 999999;     /* force lock on next tick */
        }

        if (k == 'c' && !hold_used) {
            hold_used = 1;
            if (hold == (Piece *)0) {
                hold = cur;
                cur  = nxt;
                nxt  = piece_new(rng_bag_next());
            } else {
                Piece *tmp = hold;
                hold = cur;
                cur  = tmp;
                cur->row = 0;
                cur->col = my_div(BOARD_W, 2) - 2;
                cur->rot = 0;
            }
        }

        render_frame(b, cur, nxt, hold, name);
        usleep(FRAME_SLEEP_US);
    }

end_game:
    if (score_qualifies(table, b->score)) {
        score_insert(table, name, b->score, b->level, b->lines);
        score_save(table, SCORES_PATH);
    }

    piece_free(cur);
    piece_free(nxt);
    if (hold)
        piece_free(hold);
    board_free(b);
}

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */

int main(void)
{
    ScoreTable *table;

    mem_init();
    scr_init();
    kb_init();
    tm_init();
    atexit(cleanup);

    install_signals();
    rng_seed(rng_seed_from_clock());

    table = score_load(SCORES_PATH);

    while (!g_quit) {
        /* Always start each menu transition with a known-clean canvas.
         * After play_one_game returns, front buffer holds the game-over
         * frame; without this the title's diff-emission would leave
         * gameplay cells visible at unwritten positions. */
        scr_force_full_redraw();
        char choice = menu_show_title();
        if (choice == 'q')
            break;
        if (choice == 'l') {
            menu_show_leaderboard(table);
            continue;
        }
        if (choice == 'n') {
            char name[SCORE_NAME_MAX];
            menu_show_name_entry(name);
            play_one_game(name, table);
        }
    }

    score_free(table);
    return 0;
}
