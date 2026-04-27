/*
 * game/menu.c
 * Author : Rhythm
 * Date   : 2026-04-27
 *
 * Menu state machine — owns the non-gameplay screens.
 *
 * Layout convention:
 *   - scr_* coordinates are 0-indexed (matches the rest of the project).
 *   - Direct ANSI cursor positioning (used only around kb_readline) is
 *     1-indexed; cursor_show_at() handles the conversion.
 *   - Pause / game-over overlays cover the playfield rectangle drawn
 *     by main.c at top=1, left=1, width = 2*BOARD_W + 2 (= 22).
 */

#include "menu.h"
#include "score.h"
#include "board.h"

#include "../libs/screen.h"
#include "../libs/keyboard.h"
#include "../libs/string.h"
#include "../libs/math.h"
#include "../libs/safe.h"

#include <stdio.h>
#include <unistd.h>

/* --------------------------------------------------------------------- */
/*  Internal helpers                                                      */
/* --------------------------------------------------------------------- */

static void center_puts(int row, const char *s)
{
    int len;
    int col;

    len = my_strlen(s);
    col = my_div(SCR_COLS - len, 2);
    if (col < 0)
        col = 0;
    scr_puts(row, col, s);
}

/* Reveal the cursor at (row, col) so kb_readline's cooked-mode echo
 * appears in the right place. We bypass scr_* here because the screen
 * library hides the cursor in scr_init() and never exposes it back. */
static void cursor_show_at(int row, int col)
{
    printf("\033[%d;%dH", row + 1, col + 1);
    printf("\033[?25h");
    fflush(stdout);
}

static void cursor_hide(void)
{
    printf("\033[?25l");
    fflush(stdout);
}

/* Draw a dotted rectangle border (used by both overlays) and clear
 * the interior so the existing playfield content does not bleed
 * through behind the message. */
static void draw_overlay_box(int top, int bot, int left, int width)
{
    int right;
    int r;
    int c;

    right = left + width - 1;

    for (c = left; c <= right; c++) {
        scr_putchar(top, c, '-');
        scr_putchar(bot, c, '-');
    }
    for (r = top + 1; r <= bot - 1; r++) {
        scr_putchar(r, left,  '|');
        scr_putchar(r, right, '|');
        for (c = left + 1; c <= right - 1; c++)
            scr_putchar(r, c, ' ');
    }
}

static void center_in_box(int row, int left, int width, const char *s)
{
    int slen;
    int col;

    slen = my_strlen(s);
    col  = left + my_div(width - slen, 2);
    if (col < left + 1)
        col = left + 1;
    scr_puts(row, col, s);
}

/* Title art: 33-col x 5-row block letters built from ASCII. Centred at
 * runtime so an SCR_COLS change does not break it. */
static const char *TITLE_ART[5] = {
    "TTTTT EEEEE TTTTT RRRR  III SSSSS",
    "  T   E       T   R   R  I  S    ",
    "  T   EEE     T   RRRR   I  SSSS ",
    "  T   E       T   R  R   I      S",
    "  T   EEEEE   T   R   R III SSSSS"
};

/* --------------------------------------------------------------------- */
/*  Public API                                                            */
/* --------------------------------------------------------------------- */

char menu_show_title(void)
{
    int i;
    int k;

    scr_clear();
    for (i = 0; i < 5; i++)
        center_puts(2 + i, TITLE_ART[i]);

    center_puts(10, "[N] New Game");
    center_puts(11, "[L] Leaderboard");
    center_puts(12, "[Q] Quit");

    center_puts(16, "Authors: Rhythm  Amod  Sai Kiran");
    scr_present();

    for (;;) {
        k = kb_pressed();
        if (k == 'n' || k == 'N') return 'n';
        if (k == 'l' || k == 'L') return 'l';
        if (k == 'q' || k == 'Q') return 'q';
        usleep(16000);
    }
}

void menu_show_name_entry(char *out_name)
{
    char buf[SCORE_NAME_MAX];
    int  prompt_col;
    int  k;

    safe_assert(out_name != 0);

    scr_clear();
    center_puts(4, "Enter name (1-12 chars, A-Z 0-9 _ -):");
    scr_present();

    /* Cursor goes one row below the prompt, lined up with its leftmost
     * column so the typed name reads as a continuation. */
    prompt_col = my_div(SCR_COLS - 12, 2);
    cursor_show_at(6, prompt_col);

    kb_readline(buf, SCORE_NAME_MAX);
    safe_sanitize_name(buf, SCORE_NAME_MAX);
    my_strncpy(out_name, buf, SCORE_NAME_MAX);

    cursor_hide();

    /* Confirmation. We have no sprintf/strcat, so we paint the line
     * in three pieces and advance col by my_strlen each time. */
    scr_clear();
    {
        const char *pre  = "Welcome, ";
        const char *suf  = "! Press ENTER to start.";
        int         row  = 8;
        int         pre_len  = my_strlen(pre);
        int         name_len = my_strlen(out_name);
        int         suf_len  = my_strlen(suf);
        int         total    = pre_len + name_len + suf_len;
        int         col      = my_div(SCR_COLS - total, 2);

        if (col < 0)
            col = 0;
        scr_puts(row, col,                          pre);
        scr_puts(row, col + pre_len,                out_name);
        scr_puts(row, col + pre_len + name_len,     suf);
    }
    scr_present();

    for (;;) {
        k = kb_pressed();
        if (k == KEY_ENTER)
            break;
        usleep(16000);
    }
}

void menu_show_leaderboard(const ScoreTable *t)
{
    int k;

    safe_assert(t != 0);

    scr_clear();
    center_puts(1, "T E T R I S - Leaderboard");
    score_render(t, 4, 5);
    center_puts(SCR_ROWS - 2, "Press B to go back");
    scr_present();

    for (;;) {
        k = kb_pressed();
        if (k == 'b' || k == 'B')
            return;
        usleep(16000);
    }
}

void menu_render_pause_overlay(void)
{
    const int top   = 8;
    const int bot   = 12;
    const int left  = 1;
    const int width = 2 * BOARD_W + 2;       /* 22 */

    draw_overlay_box(top, bot, left, width);
    center_in_box(my_div(top + bot, 2), left, width, "PAUSED - P TO RESUME");
}

void menu_render_game_over(int score, int lines)
{
    char       sbuf[12];
    char       lbuf[12];
    const int  top   = 8;
    const int  bot   = 12;
    const int  left  = 1;
    const int  width = 2 * BOARD_W + 2;      /* 22 */
    int        pos;
    int        col;
    int        total;
    int        sp_len;
    int        lp_len;
    int        sbuf_len;
    int        lbuf_len;

    draw_overlay_box(top, bot, left, width);

    /* Row 1 of the box: "GAME OVER" */
    center_in_box(top + 1, left, width, "GAME OVER");

    /* Row 2: compact "S:<score> L:<lines>" — full word labels do not
     * fit in a 22-column overlay alongside the numbers. */
    my_itoa(score, sbuf);
    my_itoa(lines, lbuf);
    {
        const char *sp = "S:";
        const char *lp = "  L:";
        sp_len   = my_strlen(sp);
        lp_len   = my_strlen(lp);
        sbuf_len = my_strlen(sbuf);
        lbuf_len = my_strlen(lbuf);
        total    = sp_len + sbuf_len + lp_len + lbuf_len;
        col      = left + my_div(width - total, 2);
        if (col < left + 1)
            col = left + 1;

        pos = col;
        scr_puts(top + 2, pos, sp);   pos += sp_len;
        scr_puts(top + 2, pos, sbuf); pos += sbuf_len;
        scr_puts(top + 2, pos, lp);   pos += lp_len;
        scr_puts(top + 2, pos, lbuf);
    }

    /* Row 3 (= bot - 1): prompt to continue. */
    center_in_box(bot - 1, left, width, "ENTER to continue");
}
