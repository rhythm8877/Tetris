/*
 * libs/fileio.h
 * Author : Sai Kiran
 * Date   : 2026-04-26
 *
 * File I/O library for persistent leaderboard storage.
 * This is the only place fopen/fread/fwrite/fclose are permitted
 * (File Systems rubric).
 */

#ifndef MY_FILEIO_H
#define MY_FILEIO_H

int          fio_read_all(const char *path, void *buf, int max_bytes);
int          fio_write_atomic(const char *path, const void *buf, int bytes);
unsigned int fio_crc32(const void *buf, int bytes);
int          fio_exists(const char *path);
int          fio_ensure_parent_dir(const char *path);

#endif
