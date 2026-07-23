/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Siebe Majoor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, without warranty of any
 * kind, either express or implied.
 *
 ****************************************************************************/

/***************************************************************************
 * Achievement watcher - live "achievement unlocked" toasts.
 *
 * TSR plugin: run it once and it stays resident, watching every track you
 * finish; the moment a badge from ach_table.h unlocks it splashes over
 * whatever screen you're on (WPS included). Run it again to turn it off.
 *
 * When another plugin needs the RAM this steps aside with
 * PLUGIN_TSR_SUSPEND and the plugin loader relaunches it automatically
 * as soon as that plugin exits. On USB disconnect it re-crunches the log
 * from scratch in case files changed while mounted.
 *
 * EXPERIMENTAL and deliberately separate from the publishable
 * Wrapped/achievements set.
 ***************************************************************************/

#include "plugin.h"

/* tap into the log parsers before pulling them in */
static void ach_tally(unsigned long ts, bool valid_ts, bool listened,
                      unsigned secs, const char *artist, const char *title);
#define ACH_HOOK(ts, valid_ts, listened, secs, artist, title) \
    ach_tally(ts, valid_ts, listened, secs, artist, title)

#include "wrapped_core.h"
#include "achievements_core.h"

#define EV_EXIT  MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0xFF)
#define EV_TRACK MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x01)
#define EV_TEST  MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x02)

/* while this file exists, the root menu skips the boot auto-start;
 * "Turn off" creates it, a manual start deletes it again */
#define ACHWATCH_OFF_FLAG "/.rockbox/achwatch_off"

/* track info is copied here on the playback thread, processed on ours */
struct pend {
    char path[MAX_PATH];
    unsigned long el_ms, len_ms, ts;
};
static struct pend pend_ring[4];
static int pend_w;

static struct {
    unsigned int id;
    long stack[2048];             /* 8 KB: the rebase path parses the log */
    struct event_queue queue;
} gThread;

static bool armed;

/* which badges were already unlocked the last time we looked */
static unsigned char was_unl[(sizeof(ach_table)/sizeof(ach_table[0]) + 7) / 8];
static bool wu_get(int i) { return (was_unl[i >> 3] >> (i & 7)) & 1; }
static void wu_set(int i) { was_unl[i >> 3] |= 1u << (i & 7); }

/* crunch the whole log family and snapshot the unlocked set */
static bool rebase(void)
{
    at_reset();
    ach_flag_load();
    if (!wrapped_load_data(false)) {
        /* "empty log" still leaves usable (zeroed) tables; only an
         * out-of-memory failure makes live counting impossible */
        if (!t_track.items || !t_artist.items || !t_album.items || !days)
            return false;
    }
    ach_eval();
    ach_seen_load();                /* the gold mask feeds toast colours */
    if (ach_record_unlocks())       /* stamp dates; record what's legit  */
        ach_seen_save();
    rb->memset(was_unl, 0, sizeof(was_unl));
    for (int i = 0; i < N_ACH; i++)
        if (ach_unlocked(i)) wu_set(i);
    return true;
}

/* test-toast button bookkeeping: when the last press happened (for the
 * two-inside-a-minute badge) and the counter file the core reads back */
static long last_test_tick;
static bool have_test_tick;

static void toast_flag_save(void)
{
    int fd = rb->open(ACH_TOAST_FLAG, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return;
    rb->fdprintf(fd, "%ld %d", ach_toast_count, ach_toast_burst ? 1 : 0);
    rb->close(fd);
}

/* toasts wait here while the hold switch is on (nobody's looking) and
 * play back one by one - never on top of each other - once it's off */
static unsigned char pend_toast[(sizeof(ach_table)/sizeof(ach_table[0]) + 7) / 8];
static int pend_toast_n;

static void flush_toasts(void)
{
    if (!pend_toast_n || rb->button_hold())
        return;
    int shown = 0, extra = 0;
    for (int i = 0; i < N_ACH && pend_toast_n; i++) {
        if (!((pend_toast[i >> 3] >> (i & 7)) & 1))
            continue;
        pend_toast[i >> 3] &= ~(1u << (i & 7));
        pend_toast_n--;
        if (shown < 3) {
            ach_toast(ach_table[i].name, ach_is_green(i),
                      "ACHIEVEMENT UNLOCKED");
            shown++;
        } else {
            extra++;
        }
    }
    if (extra) {
        char b[32];
        rb->snprintf(b, sizeof b, "...and %d more!", extra);
        ach_toast(b, false, "ACHIEVEMENT UNLOCKED");
    }
}

/* round-number moments deserve a banner too. No persistence needed:
 * the boot rebase sets the baseline, so a crossing can only fire live.
 * If the hold switch is on the moment simply passes - unlike badges,
 * a milestone re-toasted later would just be a lie. */
static void milestone_check(long prev_plays, long prev_mins)
{
    static const long play_stones[] =
        { 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000, 250000 };
    static const long min_stones[] =
        { 1000, 2500, 5000, 10000, 25000, 50000, 100000, 250000,
          500000, 1000000 };
    long mins = total_seconds / 60;
    char nb[24], b[48];

    if (rb->button_hold())
        return;
    for (unsigned i = 0; i < sizeof(play_stones)/sizeof(play_stones[0]); i++)
        if (prev_plays < play_stones[i] && total_plays >= play_stones[i]) {
            commafmt(play_stones[i], nb, sizeof nb);
            rb->snprintf(b, sizeof b, "Play #%s", nb);
            ach_toast(b, false, "MILESTONE");
            break;
        }
    for (unsigned i = 0; i < sizeof(min_stones)/sizeof(min_stones[0]); i++)
        if (prev_mins < min_stones[i] && mins >= min_stones[i]) {
            commafmt(min_stones[i], nb, sizeof nb);
            rb->snprintf(b, sizeof b, "Minute %s", nb);
            ach_toast(b, false, "MILESTONE");
            break;
        }
}

/* re-evaluate everything, queue whatever newly unlocked, show if we can */
static void toast_new_unlocks(void)
{
    ach_eval();
    if (ach_record_unlocks())       /* live legit unlocks turn gold now,  */
        ach_seen_save();            /* not at the next boot               */
    for (int i = 0; i < N_ACH; i++) {
        if (wu_get(i) || !ach_unlocked(i))
            continue;
        wu_set(i);
        pend_toast[i >> 3] |= 1u << (i & 7);
        pend_toast_n++;
    }
    flush_toasts();
}

/* mirror of playback_parse's per-entry accounting, fed live */
static void process_track(const struct pend *t)
{
    if (t->el_ms <= 500)          /* the core log skips these too */
        return;

    char artist[NAME_MAX_LEN], title[NAME_MAX_LEN], album[NAME_MAX_LEN];
    bool have_album = resolve_meta(t->path, artist, title, album);
    unsigned elapsed = (unsigned)(t->el_ms / 1000);
    bool listened = (t->len_ms == 0) || (t->el_ms * 2 >= t->len_ms)
                                     || (t->el_ms >= 240000UL);
    bool valid_ts = (t->ts >= MIN_VALID_TS);
    long prev_plays = total_plays, prev_mins = total_seconds / 60;

    if (listened) {
        total_plays++;
        total_seconds += elapsed;
        bool night = valid_ts && (t->ts % 86400UL) / 3600UL < 5;

        struct agg *a = htable_get(&t_artist, artist);
        if (a) { a->count++; a->seconds += elapsed; if (night) a->night++; }
        a = htable_get(&t_track, title);
        if (a) {
            if (a->count == 0) unique_tracks++;
            a->count++; a->seconds += elapsed;
            if (night) a->night++;
            if (valid_ts && t->ts > a->last_ts) a->last_ts = t->ts;
        }
        a = htable_get(&t_album, have_album ? album : artist);
        if (a) { a->count++; a->seconds += elapsed; }

        if (valid_ts)
            tally_time(t->ts, elapsed);
        ach_tally(t->ts, valid_ts, true, elapsed, artist, title);
    } else if (t->el_ms >= 5000) {
        total_skips++;
        struct agg *a = htable_get(&t_track, title);
        if (a) a->skips++;
        ach_tally(t->ts, valid_ts, false, 0, artist, title);
    } else {
        return;                   /* browsing tap: no stats, no toast */
    }

    milestone_check(prev_plays, prev_mins);
    toast_new_unlocks();
}

/* runs on the playback thread: copy what we need and get out fast */
static void track_finish_cb(unsigned short id, void *data)
{
    (void)id;
    const struct track_event *te = (const struct track_event *)data;
    if (!armed || !te || !te->id3)
        return;
    struct pend *p = &pend_ring[pend_w];
    rb->strlcpy(p->path, te->id3->path, sizeof(p->path));
    p->el_ms  = te->id3->elapsed;
    p->len_ms = te->id3->length;
    struct tm *now = rb->get_time();
    p->ts = now ? (unsigned long)rb->mktime(now) : 0;
    rb->queue_post(&gThread.queue, EV_TRACK, pend_w);
    pend_w = (pend_w + 1) & 3;
}

static void watch_thread(void)
{
    struct queue_event ev;
    for (;;) {
        if (pend_toast_n)   /* poll for the hold switch releasing */
            rb->queue_wait_w_tmo(&gThread.queue, &ev, HZ / 2);
        else
            rb->queue_wait(&gThread.queue, &ev);
        switch (ev.id) {
        case SYS_TIMEOUT:
            flush_toasts();
            break;
        case EV_TRACK:
            process_track(&pend_ring[ev.data & 3]);
            break;
        case SYS_USB_CONNECTED:
            rb->usb_acknowledge(SYS_USB_CONNECTED_ACK, ev.data);
            break;
        case SYS_USB_DISCONNECTED: {
            /* the logs may have changed on the PC; anything that unlocked
             * while mounted gets its toast - the one badge honestly earned
             * that way arrives green, with company */
            unsigned char before[sizeof(was_unl)];
            rb->memcpy(before, was_unl, sizeof(before));
            if (rebase()) {
                for (int i = 0; i < N_ACH; i++) {
                    if (!wu_get(i) || ((before[i >> 3] >> (i & 7)) & 1))
                        continue;
                    if (!((pend_toast[i >> 3] >> (i & 7)) & 1)) {
                        pend_toast[i >> 3] |= 1u << (i & 7);
                        pend_toast_n++;
                    }
                }
                flush_toasts();
            }
            break;
        }
        case EV_TEST: {
            /* every press counts (ev.data is the tick of the actual
             * button press, so queueing delays can't fake a burst) */
            long press = (long)ev.data;
            ach_toast_count++;
            if (!ach_toast_burst && have_test_tick
                && press - last_test_tick < 60 * HZ)
                ach_toast_burst = true;
            last_test_tick = press;
            have_test_tick = true;
            toast_flag_save();
            rb->sleep(HZ * 5);
            /* let the real machinery toast whatever the press just
             * earned ("It Works!", or one of the button-mashing rows);
             * if it changed nothing, re-queue the first badge as a
             * demo. Either way it goes through the pending queue, so
             * hold defers it like any real unlock. */
            int before = 0, after = 0;
            for (int i = 0; i < N_ACH; i++) if (wu_get(i)) before++;
            toast_new_unlocks();
            for (int i = 0; i < N_ACH; i++) if (wu_get(i)) after++;
            if (after == before) {
                for (int i = 0; i < N_ACH; i++) {
                    if (ach_table[i].metric == AM_F_TOAST) {
                        if (!((pend_toast[i >> 3] >> (i & 7)) & 1)) {
                            pend_toast[i >> 3] |= 1u << (i & 7);
                            pend_toast_n++;
                        }
                        break;
                    }
                }
                flush_toasts();
            }
            break;
        }
        case EV_EXIT:
            rb->thread_exit();
            return;
        }
    }
}

static void watcher_stop(void)
{
    armed = false;
    rb->remove_event(PLAYBACK_EVENT_TRACK_FINISH, track_finish_cb);
    rb->queue_post(&gThread.queue, EV_EXIT, 0);
    rb->thread_wait(gThread.id);
    rb->queue_delete(&gThread.queue);
}

static int exit_tsr(bool reenter)
{
    if (!reenter) {
        /* another plugin needs the RAM; the loader brings us back after */
        watcher_stop();
        return PLUGIN_TSR_SUSPEND;
    }

    /* the user launched us again: small control menu */
    MENUITEM_STRINGLIST(menu, "Achievement watcher", NULL,
                        "Send test toast (5 s)", "Turn off", "Keep running");
    switch (rb->do_menu(&menu, NULL, NULL, false)) {
    case 0:
        rb->queue_post(&gThread.queue, EV_TEST, *rb->current_tick);
        rb->splash(HZ, "Go where you like - toast in 5 s");
        return PLUGIN_TSR_CONTINUE;
    case 1: {
        watcher_stop();
        int fd = rb->open(ACHWATCH_OFF_FLAG, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd >= 0) { rb->write(fd, "1", 1); rb->close(fd); }
        rb->splash(HZ, "Watcher off (start it to re-enable)");
        return PLUGIN_TSR_TERMINATE;
    }
    default:
        return PLUGIN_TSR_CONTINUE;
    }
}

enum plugin_status plugin_start(const void *parameter)
{
    bool resumed = (parameter == rb->plugin_tsr);
    bool autoboot = (parameter != NULL && !resumed);  /* root menu boot hook */
    (void)wrapped_show;           /* header carries all of Wrapped;  */
    (void)ach_show;               /* only engine + parser used here  */

    if (!resumed && !autoboot)
        rb->remove(ACHWATCH_OFF_FLAG);   /* manual start = re-enable boot */

    wrapped_font_init();          /* the toast banner draws with it */
    if (!rebase()) {
        rb->splash(HZ * 2, "Watcher: out of memory");
        return PLUGIN_OK;
    }

    rb->queue_init(&gThread.queue, true);   /* true: receive SYS events */
    gThread.id = rb->create_thread(watch_thread, gThread.stack,
                                   sizeof(gThread.stack), 0, "achwatch"
                                   IF_PRIO(, PRIORITY_BACKGROUND)
                                   IF_COP(, CPU));
    rb->add_event(PLAYBACK_EVENT_TRACK_FINISH, track_finish_cb);
    armed = true;
    rb->plugin_tsr(exit_tsr);

    if (!resumed && !autoboot)    /* silent when auto-started or resumed */
        rb->splash(HZ, "Achievement watcher on");
    return PLUGIN_OK;
}
