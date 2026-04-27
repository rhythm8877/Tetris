/*
 * game/menu.h
 * Author : Rhythm
 * Date   : 2026-04-27
 *
 * Menu state machine — owns every non-gameplay screen: title menu,
 * name entry prompt, leaderboard view, pause overlay, game-over
 * overlay. Each screen is self-contained; main.c just drives the
 * outer state.
 */

#ifndef GAME_MENU_H
#define GAME_MENU_H

#include "score.h"

typedef enum {
    MENU_TITLE       = 0,
    MENU_NAME_ENTRY,
    MENU_LEADERBOARD,
    MENU_PAUSED,
    MENU_GAME_OVER,
    MENU_NONE                 /* gameplay state */
} MenuState;

char menu_show_title(void);
void menu_show_name_entry(char *out_name);
void menu_show_leaderboard(const ScoreTable *t);
void menu_render_pause_overlay(void);
void menu_render_game_over(int score, int lines);

#endif
