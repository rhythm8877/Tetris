/*
 * game/score.c
 * Author : Rhythm
 * Date   : 2026-04-27
 *
 * Leaderboard state and persistence. Owns:
 *   - load: read data/scores.dat, validate per-record CRC, sort.
 *   - save: serialise back via fio_write_atomic (atomic via rename(2)).
 *   - insert: sorted insert with truncate-to-N.
 *   - render: scr_puts/scr_render_int into the screen buffer.
 *
 * Sort key (descending, "higher is better"):
 *     score, then level, then lines.
 */

#include "score.h"

#include "../libs/memory.h"
#include "../libs/string.h"
#include "../libs/screen.h"
#include "../libs/fileio.h"
#include "../libs/safe.h"

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/* CRC over the four payload fields only. We compute the byte distance
 * from the start of the struct to the `crc` member at runtime, so we
 * are immune to any compiler-inserted padding between `lines` and `crc`. */
static unsigned int score_compute_crc(const ScoreRecord *r)
{
    int payload_bytes;

    payload_bytes = (int)((const unsigned char *)&r->crc
                          - (const unsigned char *)r);
    return fio_crc32(r, payload_bytes);
}

/* Returns >0 if a ranks higher than b, <0 if lower, 0 if equal.
 * Higher score → higher rank; ties broken by level, then lines. */
static int score_compare(const ScoreRecord *a, const ScoreRecord *b)
{
    if (a->score != b->score)
        return (a->score > b->score) ? 1 : -1;
    if (a->level != b->level)
        return (a->level > b->level) ? 1 : -1;
    if (a->lines != b->lines)
        return (a->lines > b->lines) ? 1 : -1;
    return 0;
}

/* In-place insertion sort, descending rank (best at index 0). */
static void score_sort(ScoreTable *t)
{
    int         i;
    int         j;
    ScoreRecord key;

    for (i = 1; i < t->count; i++) {
        key = t->records[i];
        j   = i - 1;
        while (j >= 0 && score_compare(&t->records[j], &key) < 0) {
            t->records[j + 1] = t->records[j];
            j--;
        }
        t->records[j + 1] = key;
    }
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

ScoreTable *score_load(const char *path)
{
    ScoreTable  *t;
    int          max_bytes;
    int          n;
    int          slots;
    int          i;
    int          valid;
    int          corrupted;
    unsigned int got;
    unsigned int want;

    fio_ensure_parent_dir(path);

    t = (ScoreTable *)my_alloc((int)sizeof(ScoreTable));
    safe_assert(t != 0);

    t->records  = (ScoreRecord *)my_alloc((int)sizeof(ScoreRecord) * SCORE_TOP_N);
    safe_assert(t->records != 0);

    t->count    = 0;
    t->capacity = SCORE_TOP_N;

    if (!fio_exists(path))
        return t;

    max_bytes = (int)sizeof(ScoreRecord) * SCORE_TOP_N;
    n = fio_read_all(path, t->records, max_bytes);
    if (n <= 0)
        return t;

    /* Compact valid records in-place; tally corrupted ones. */
    slots     = n / (int)sizeof(ScoreRecord);
    valid     = 0;
    corrupted = 0;
    for (i = 0; i < slots; i++) {
        got  = t->records[i].crc;
        want = score_compute_crc(&t->records[i]);
        if (got == want) {
            if (valid != i)
                t->records[valid] = t->records[i];
            valid++;
        } else {
            corrupted++;
        }
    }
    t->count = valid;

    if (corrupted > 0) {
        /* Side-note for the player. Assumes scr_init() ran before
         * score_load(); main.c arranges that ordering. */
        scr_puts(0, 0, "leaderboard: some records corrupted, dropped");
    }

    score_sort(t);
    return t;
}

int score_save(const ScoreTable *t, const char *path)
{
    int bytes;

    safe_assert(t != 0);

    bytes = (int)sizeof(ScoreRecord) * t->count;
    return fio_write_atomic(path, t->records, bytes);
}

void score_free(ScoreTable *t)
{
    if (t == 0)
        return;
    if (t->records != 0)
        my_dealloc(t->records);
    my_dealloc(t);
}

int score_qualifies(const ScoreTable *t, int score)
{
    safe_assert(t != 0);

    if (t->count < SCORE_TOP_N)
        return 1;
    /* Table is full and sorted DESC; last record is the lowest score. */
    return (score > t->records[t->count - 1].score) ? 1 : 0;
}

int score_insert(ScoreTable *t,
                 const char *name,
                 int score,
                 int level,
                 int lines)
{
    char        sanitised[SCORE_NAME_MAX];
    ScoreRecord candidate;
    int         idx;
    int         i;
    char       *cp;

    safe_assert(t != 0);
    safe_assert(name != 0);

    /* Zero the entire candidate first. my_strncpy null-terminates but
     * does not pad — without zeroing, name[strlen+1 .. 15] keeps stack
     * garbage that gets serialised to scores.dat and confuses every
     * downstream consumer (CRC, debug dumps, hex tools). */
    cp = (char *)&candidate;
    for (i = 0; i < (int)sizeof(ScoreRecord); i++)
        cp[i] = 0;

    /* Build the sanitised name in a local buffer so we never trust the
     * caller-supplied string to be safe to write into the file. */
    my_strncpy(sanitised, name, SCORE_NAME_MAX);
    safe_sanitize_name(sanitised, SCORE_NAME_MAX);

    my_strncpy(candidate.name, sanitised, SCORE_NAME_MAX);
    candidate.score = score;
    candidate.level = level;
    candidate.lines = lines;
    candidate.crc   = 0;

    /* Find first slot whose ranking is below the candidate. */
    idx = t->count;
    for (i = 0; i < t->count; i++) {
        if (score_compare(&candidate, &t->records[i]) > 0) {
            idx = i;
            break;
        }
    }

    /* Full table + candidate ranks at the bottom → reject silently.
     * Without this guard we would drop the legitimate last-place entry
     * and replace it with a worse one. */
    if (t->count == t->capacity && idx == t->capacity)
        return -1;

    if (t->count == t->capacity)
        t->count--;

    /* Shift right to open a slot at idx. */
    for (i = t->count; i > idx; i--)
        t->records[i] = t->records[i - 1];

    t->records[idx] = candidate;
    t->records[idx].crc = score_compute_crc(&t->records[idx]);

    t->count++;
    return idx;
}

void score_render(const ScoreTable *t, int top_row, int left_col)
{
    int i;

    safe_assert(t != 0);

    /* Column layout (offsets relative to left_col):
     *      0..3   RANK    (1-2 digits)
     *      6..21  NAME    (up to SCORE_NAME_MAX chars)
     *     24..31  SCORE   (up to ~10 digits)
     *     33..36  LV
     *     38..41  LN
     */

    scr_puts(top_row, left_col +  0, "RANK");
    scr_puts(top_row, left_col +  6, "NAME");
    scr_puts(top_row, left_col + 24, "SCORE");
    scr_puts(top_row, left_col + 33, "LV");
    scr_puts(top_row, left_col + 38, "LN");

    for (i = 0; i < t->count; i++) {
        char buf[12];
        int  row = top_row + 1 + i;

        my_itoa(i + 1, buf);
        scr_puts(row, left_col +  0, buf);
        scr_puts(row, left_col +  6, t->records[i].name);
        scr_render_int(row, left_col + 24, t->records[i].score);
        scr_render_int(row, left_col + 33, t->records[i].level);
        scr_render_int(row, left_col + 38, t->records[i].lines);
    }
}
