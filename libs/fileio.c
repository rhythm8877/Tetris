/*
 * libs/fileio.c
 * Author : Sai Kiran
 * Date   : 2026-04-26
 *
 * Atomic file I/O and CRC32 for the persistent leaderboard.
 * Only translation unit that touches fopen/fread/fwrite/fclose
 * (File Systems rubric).  All string ops route through libs/string.h.
 */

#include "fileio.h"
#include "string.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/* ------------------------------------------------------------------ */
/*  fio_exists                                                         */
/* ------------------------------------------------------------------ */

int fio_exists(const char *path)
{
    struct stat st;

    return (stat(path, &st) == 0) ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/*  fio_ensure_parent_dir                                              */
/* ------------------------------------------------------------------ */

int fio_ensure_parent_dir(const char *path)
{
    char dir[256];
    int  len;
    int  last_slash;
    int  i;
    struct stat st;

    len        = my_strlen(path);
    last_slash = -1;

    for (i = 0; i < len; i++)
    {
        if (path[i] == '/')
            last_slash = i;
    }

    if (last_slash <= 0)
        return 0;

    my_strncpy(dir, path, last_slash + 1);

    if (stat(dir, &st) == 0)
        return 0;

    if (mkdir(dir, 0755) == 0 || errno == EEXIST)
        return 0;

    return -1;
}

/* ------------------------------------------------------------------ */
/*  fio_read_all                                                       */
/* ------------------------------------------------------------------ */

int fio_read_all(const char *path, void *buf, int max_bytes)
{
    FILE *fp;
    int   pos;
    int   n;

    if (!fio_exists(path))
        return 0;

    fp = fopen(path, "rb");
    if (!fp)
        return -1;

    pos = 0;
    while (pos < max_bytes)
    {
        n = (int)fread((char *)buf + pos, 1, (size_t)(max_bytes - pos), fp);
        if (n <= 0)
            break;
        pos += n;
    }

    fclose(fp);
    return pos;
}

/* ------------------------------------------------------------------ */
/*  fio_write_atomic                                                   */
/* ------------------------------------------------------------------ */

int fio_write_atomic(const char *path, const void *buf, int bytes)
{
    char tmp_path[256];
    int  len;
    int  written;
    int  fd;
    FILE *fp;

    len = my_strlen(path);
    if (len + 4 >= 256)
        return -1;

    my_strcpy(tmp_path, path);
    my_strncpy(tmp_path + len, ".tmp", 5);

    fp = fopen(tmp_path, "wb");
    if (!fp)
        return -1;

    written = (int)fwrite(buf, 1, (size_t)bytes, fp);
    if (written != bytes)
    {
        fclose(fp);
        unlink(tmp_path);
        return -1;
    }

    fflush(fp);
    fd = fileno(fp);
    fsync(fd);
    fclose(fp);

    if (rename(tmp_path, path) != 0)
    {
        unlink(tmp_path);
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  fio_crc32 — IEEE 802.3, lookup-free bitwise implementation         */
/* ------------------------------------------------------------------ */

unsigned int fio_crc32(const void *buf, int bytes)
{
    const unsigned char *p;
    unsigned int         crc;
    int                  i;
    int                  k;

    p   = (const unsigned char *)buf;
    crc = 0xFFFFFFFF;

    for (i = 0; i < bytes; i++)
    {
        crc ^= p[i];
        for (k = 0; k < 8; k++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFF;
}

/* ------------------------------------------------------------------ */
/*  TEST_FILEIO                                                        */
/* ------------------------------------------------------------------ */

#ifdef TEST_FILEIO

static int g_pass = 0;
static int g_fail = 0;

static void check(const char *name, int cond)
{
    if (cond) {
        printf("[PASS] %s\n", name);
        g_pass++;
    } else {
        printf("[FAIL] %s\n", name);
        g_fail++;
    }
}

int main(void)
{
    char         buf[64];
    int          n;
    unsigned int crc;

    /* --- write + read round-trip --- */
    check("write_atomic",
          fio_write_atomic("/tmp/tetris_test.dat", "hello", 5) == 0);

    n = fio_read_all("/tmp/tetris_test.dat", buf, 64);
    check("read_all: length == 5", n == 5);
    check("read_all: content match", my_strncmp(buf, "hello", 5) == 0);

    /* --- CRC32 known test vector --- */
    crc = fio_crc32("123456789", 9);
    check("crc32(\"123456789\") == 0xCBF43926", crc == 0xCBF43926);
    printf("       crc32 = 0x%08X\n", crc);

    /* --- fio_exists --- */
    check("exists written file",  fio_exists("/tmp/tetris_test.dat") == 1);
    check("!exists missing file", fio_exists("/tmp/no_such_file_xyz") == 0);

    /* --- read_all on missing file returns 0 --- */
    n = fio_read_all("/tmp/no_such_file_xyz", buf, 64);
    check("read_all missing -> 0", n == 0);

    /* --- ensure_parent_dir --- */
    check("ensure_parent_dir",
          fio_ensure_parent_dir("/tmp/tetris_test.dat") == 0);

    /* --- clean up --- */
    unlink("/tmp/tetris_test.dat");

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
#endif
