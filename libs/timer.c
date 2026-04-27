/*
 * libs/timer.c
 * Author : Sai Kiran
 * Date   : 2026-04-26
 *
 * POSIX interval timer for Tetris gravity.  A 50 ms ITIMER_REAL fires
 * SIGALRM; the handler only bumps a counter.  The game loop calls
 * tm_consume_tick() to drain accumulated ticks at its own pace.
 *
 * All arithmetic routes through libs/math.c (project rule).
 */

#include "timer.h"
#include "math.h"

#include <signal.h>
#include <sys/time.h>

/* ------------------------------------------------------------------ */
/*  Tick counter — only touched by the handler and tm_consume_tick     */
/* ------------------------------------------------------------------ */

static volatile sig_atomic_t g_ticks = 0;

/* ------------------------------------------------------------------ */
/*  Epoch recorded by tm_init for tm_now_ms                            */
/* ------------------------------------------------------------------ */

static struct timeval start_tv;

/* ------------------------------------------------------------------ */
/*  SIGALRM handler — increment and nothing else                       */
/* ------------------------------------------------------------------ */

static void alarm_handler(int sig)
{
    (void)sig;
    g_ticks++;
}

/* ------------------------------------------------------------------ */
/*  Arm / disarm helpers                                               */
/* ------------------------------------------------------------------ */

static void arm_timer(int ms)
{
    struct itimerval it;

    it.it_value.tv_sec     = 0;
    it.it_value.tv_usec    = my_mul(ms, 1000);
    it.it_interval.tv_sec  = 0;
    it.it_interval.tv_usec = my_mul(ms, 1000);

    setitimer(ITIMER_REAL, &it, 0);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

void tm_init(void)
{
    struct sigaction sa;

    sa.sa_handler = alarm_handler;
    sa.sa_flags   = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, 0);

    gettimeofday(&start_tv, 0);

    arm_timer(50);
}

void tm_stop(void)
{
    arm_timer(0);
}

void tm_resume(void)
{
    arm_timer(50);
}

int tm_consume_tick(void)
{
    sigset_t block, prev;
    int      had;

    sigemptyset(&block);
    sigaddset(&block, SIGALRM);
    sigprocmask(SIG_BLOCK, &block, &prev);

    had = (g_ticks > 0);
    if (had)
        g_ticks--;

    sigprocmask(SIG_SETMASK, &prev, 0);
    return had;
}

void tm_reset_ticks(void)
{
    sigset_t block, prev;

    /* Block SIGALRM around the write so the handler cannot race with
     * us and resurrect a non-zero counter. */
    sigemptyset(&block);
    sigaddset(&block, SIGALRM);
    sigprocmask(SIG_BLOCK, &block, &prev);

    g_ticks = 0;

    sigprocmask(SIG_SETMASK, &prev, 0);
}

unsigned long tm_now_ms(void)
{
    struct timeval now;
    long           sec;
    long           usec;

    gettimeofday(&now, 0);

    sec  = now.tv_sec  - start_tv.tv_sec;
    usec = now.tv_usec - start_tv.tv_usec;

    if (usec < 0)
    {
        sec--;
        usec += 1000000;
    }

    return (unsigned long)(my_mul((int)sec, 1000)
         + my_div((int)usec, 1000));
}

int tm_fall_period_ms(int level)
{
    static const int table[] = {
        800, 720, 630, 550, 470,
        380, 300, 220, 130,  80,
         50
    };

    return table[my_clamp(level, 0, 10)];
}
