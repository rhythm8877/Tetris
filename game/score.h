/*
 * game/score.h
 * Author : Rhythm
 * Date   : 2026-04-27
 *
 * Top-N leaderboard stored in data/scores.dat. Records are validated
 * via per-record CRC32 (computed by libs/fileio.c) so a partially-
 * written file or a manually-edited entry is dropped silently rather
 * than crashing the load path.
 */

#ifndef MY_SCORE_H
#define MY_SCORE_H

#define SCORE_NAME_MAX 16
#define SCORE_TOP_N    10

typedef struct {
    char         name[SCORE_NAME_MAX];   /* null-terminated, sanitised */
    int          score;
    int          level;
    int          lines;
    unsigned int crc;                    /* crc32 over name+score+level+lines */
} ScoreRecord;

typedef struct {
    ScoreRecord *records;                /* my_alloc'd, length = capacity */
    int          count;
    int          capacity;               /* always SCORE_TOP_N */
} ScoreTable;

ScoreTable *score_load(const char *path);
int         score_save(const ScoreTable *t, const char *path);
void        score_free(ScoreTable *t);

int         score_qualifies(const ScoreTable *t, int score);
int         score_insert(ScoreTable *t,
                         const char *name,
                         int score,
                         int level,
                         int lines);

void        score_render(const ScoreTable *t, int top_row, int left_col);

#endif
