/*
 * libs/timer.h
 * Author : Sai Kiran
 * Date   : 2026-04-26
 *
 * POSIX interval-timer API for Tetris gravity clock.
 * Built on setitimer + SIGALRM (Process Management rubric).
 */

#ifndef MY_TIMER_H
#define MY_TIMER_H

void          tm_init(void);
void          tm_stop(void);
void          tm_resume(void);
int           tm_consume_tick(void);
unsigned long tm_now_ms(void);
int           tm_fall_period_ms(int level);

#endif
