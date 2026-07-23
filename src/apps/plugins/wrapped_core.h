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

/* ============================================================
 *  Spun: parse the core playback log (or a .scrobbler.log when one
 *  holds data) and present Spotify-style stat cards on the device
 *
 *  Shared between wrapped.c (the standalone plugin) and stats.c
 *  (the Apple-style menu). Include after plugin.h; entry point is
 *  wrapped_show().
 * ============================================================ */

#ifndef WRAPPED_CORE_H
#define WRAPPED_CORE_H

#ifndef SCREEN_W
#define SCREEN_W  LCD_WIDTH
#define SCREEN_H  LCD_HEIGHT
#endif

#define LOG_PATH       "/.scrobbler.log"
#define PLAYBACK_LOG_PATH "/.rockbox/playback.log"
#define PLAYBACK_LOG_STEM "/.rockbox/playback"   /* rotated: playback_0001.log ... */
#define NAME_MAX_LEN   40

/* ---- cheat detection ----
 * The core logger signs everything it writes to the playback family
 * (playback_sig.h); the parser below re-hashes the raw bytes as they
 * stream through lrd_buf and compares. History edited on a PC comes up
 * "rigged", and once rigged, always rigged: the verdict is remembered in
 * wrapped_cheat.dat. (Wiping logs + sig + the wrapped .dat files together
 * is a clean factory reset, not a cheat.) */
#include "playback_sig.h"

static unsigned cheat_flags;           /* persistent bits, mirror of the file */
static bool     cheat_flags_loaded;
static int      cheat_live = -1;       /* this crunch: -1 no verdict, 0 clean,
                                          1 rigged */

#define COL_CHEAT     LCD_RGBPACK( 90,225,105)
#define COL_CHEAT_HI  LCD_RGBPACK(200,255,210)

/* reject bogus timestamps from a never-set RTC (those land in the year 2000) */
#define MIN_VALID_TS   1104537600UL    /* 2005-01-01 00:00 UTC */

/* 0 = all-time; otherwise restrict Wrapped to this single calendar year */
static int filter_year = 0;

/* year-progress card: listened seconds per calendar (Mon-Sun) week of the
 * displayed year. Weeks anchor to the Monday on or before Jan 1, so the
 * first/last segments can be partial; a year touches at most 54 of them. */
#define N_WEEKS          54          /* array bound; n_weeks = real count */
#define WEEK_ACTIVE_SECS (30*60)     /* a week turns green at 30+ minutes */
#define SUPER_WEEK_SECS  (1440*60)   /* 24h+ in one week = gold "superweek" */
#define ULTRA_WEEK_SECS  (2880*60)   /* 48h+ = diamond "ultraweek" (touch grass) */

/* week-bar geometry, shared by the year card and the week browser */
#define WB_PITCH 5
#define WB_SEGW  4
#define WB_H     12
#define WB_Y     160
#define WB_X     ((SCREEN_W - (n_weeks * WB_PITCH - (WB_PITCH - WB_SEGW))) / 2)
#define COL_GOLD      LCD_RGBPACK(255,185, 50)
#define COL_GOLD_HI   LCD_RGBPACK(255,240,170)
#define COL_ICE       LCD_RGBPACK(120,220,255)
#define COL_ICE_HI    LCD_RGBPACK(235,252,255)
static unsigned week_secs[N_WEEKS];
static int  n_weeks = 52;            /* Mon-aligned weeks touching the year */
static int  display_year;            /* year the progress card describes */
static long display_day0;            /* unix-day of the Monday on/before its
                                        Jan 1 (bogus sentinel if no year) */

/* forward decls (defined lower, but the log parsers below need them) */
static void civil_from_days(long z, int *y, int *m, int *d);
static long days_from_civil(int y, int m, int d);
static int  year_of_ts(unsigned long ts);

/* How many list items one wheel event should move. The clickwheel driver
 * posts at most ONE scroll event at a time (only when the button queue is
 * empty) and encodes the wheel velocity in the event data; stock lists turn
 * that into a step count with button_apply_acceleration(), which is not in
 * the plugin API, so this replicates it for the ipod6g tuning
 * (WHEEL_ACCEL_START=270, WHEEL_ACCELERATION=3). */
static inline int wheel_steps(void)
{
#ifdef HAVE_BUTTON_DATA
    unsigned data = (unsigned)rb->button_get_data();
    int delta = (data >> 24) & 0x7f;
    if (data & (1u << 31)) {
        unsigned v = data & 0xffffffu;
        v = ((65536u / 270u) * v) >> (16 - 4);   /* 28.4 fixed point */
        v = (v*v*v*v + (1u << 15)) >> 16;
        if (v > 1)
            delta *= v;
    }
    return delta > 0 ? delta : 1;
#else
    return 1;
#endif
}

/* Optional per-entry tap for the achievements plugin: achievements.c
 * defines ACH_HOOK before including this header and receives every entry
 * the parsers actually count (browse-taps excluded), AFTER the aggregate
 * tables were updated for it. No-op otherwise. */
#ifndef ACH_HOOK
#define ACH_HOOK(ts, valid_ts, listened, secs, artist, title) do { } while (0)
#endif

struct agg {
    int            count;    /* listened plays (rating L) */
    unsigned       seconds;  /* summed track length for L plays */
    int            skips;    /* rating S occurrences */
    int            night;    /* plays between midnight and 5am */
    unsigned long  last_ts;  /* most recent listened play (0 = unknown) */
    char           name[NAME_MAX_LEN];
};

/* ranking modes for top_n() */
enum top_mode {
    TOP_PLAYS,      /* most listened plays */
    TOP_SKIPS,      /* most skipped */
    TOP_LOYAL,      /* most plays among never-skipped */
    TOP_REDIS,      /* most plays among not heard since redis_cutoff */
    TOP_NIGHT,      /* most plays between midnight and 5am */
};
static unsigned long redis_cutoff;   /* set after parsing, from ts_max */

struct daycount {
    long     day;    /* unix-day index = ts / 86400 */
    int      count;
    unsigned secs;   /* listened seconds that day (heatmap fuel) */
};

struct htable {
    struct agg *items;
    int        *slots;   /* slot -> (item index + 1); 0 = empty */
    int         cap;
    int         mask;    /* slots size - 1 (power of two) */
    int         n;
};

/* aggregate results */
static struct htable t_artist, t_track, t_album;
static struct daycount *days;
static int    day_cap, day_n;
static int    hour_hist[24];

static long   total_plays;     /* L */
static long   total_skips;     /* S */
static long   total_seconds;   /* sum of L lengths */
static long   ts_min, ts_max;
static int    unique_tracks;   /* distinct titles with >=1 listened play */

/* representative folder per album item, used for cover-art lookup
 * (only populated in playback.log mode, where we have real file paths) */
#define ART_PATH_MAX 104
static char (*album_dir)[ART_PATH_MAX];
static int   album_dir_cap;

/* bump allocator over the plugin buffer */
static char  *abuf;
static size_t abuf_sz, abuf_used;

static void *aalloc(size_t n)
{
    n = (n + 3u) & ~3u;
    if (abuf_used + n > abuf_sz)
        return NULL;
    void *p = abuf + abuf_used;
    abuf_used += n;
    rb->memset(p, 0, n);
    return p;
}

static int next_pow2(int v)
{
    int p = 1;
    while (p < v) p <<= 1;
    return p;
}

static unsigned hash_str(const char *s)
{
    unsigned h = 2166136261u;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
    return h;
}

static bool htable_init(struct htable *t, int cap)
{
    int slots = next_pow2(cap * 2);
    t->items = aalloc((size_t)cap * sizeof(struct agg));
    t->slots = aalloc((size_t)slots * sizeof(int));
    if (!t->items || !t->slots)
        return false;
    t->cap  = cap;
    t->mask = slots - 1;
    t->n    = 0;
    return true;
}

/* find existing or create; returns NULL only if table is full */
static struct agg *htable_get(struct htable *t, const char *name)
{
    /* hash and compare the STORED (truncated) form of the name: a 40+ char
     * name hashed/compared in full never matches its truncated entry and
     * would spawn a duplicate aggregate on every play */
    char key[NAME_MAX_LEN];
    rb->strlcpy(key, name, NAME_MAX_LEN);
    unsigned h = hash_str(key);
    int i = (int)(h & t->mask);
    while (t->slots[i]) {
        struct agg *a = &t->items[t->slots[i] - 1];
        if (rb->strcmp(a->name, key) == 0)
            return a;
        i = (i + 1) & t->mask;
    }
    if (t->n >= t->cap)
        return NULL;
    struct agg *a = &t->items[t->n];
    rb->strlcpy(a->name, key, NAME_MAX_LEN);
    t->slots[i] = t->n + 1;
    t->n++;
    return a;
}

static void day_add(long day, unsigned secs)
{
    /* logs are roughly chronological, so check the tail first */
    if (day_n && days[day_n - 1].day == day) {
        days[day_n - 1].count++;
        days[day_n - 1].secs += secs;
        return;
    }
    for (int i = 0; i < day_n; i++) {
        if (days[i].day == day) {
            days[i].count++;
            days[i].secs += secs;
            return;
        }
    }
    if (day_n < day_cap) {
        days[day_n].day = day;
        days[day_n].count = 1;
        days[day_n].secs = secs;
        day_n++;
    }
}

static unsigned long parse_ul(const char *s)
{
    unsigned long v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v;
}

/* split a tab-separated line in place; fills up to maxf field pointers */
static int split_tabs(char *line, char **f, int maxf)
{
    int n = 0;
    f[n++] = line;
    while (*line && n < maxf) {
        if (*line == '\t') {
            *line = '\0';
            f[n++] = line + 1;
        }
        line++;
    }
    return n;
}

/* ---- buffered line reader ----
 * rb->read_line() fetches 32 bytes at a time and lseek()s backwards after
 * every line to "put back" what it over-read - thousands of read+seek calls
 * per pass, which is what made opening Wrapped crawl once the log grew.
 * This streams the file through one 4 KB buffer instead: sequential reads,
 * no seeking. One log is read at a time, so a single shared instance is fine. */
static struct {
    int  fd;
    int  pos, len;
    int  next;   /* playback-family chain: >0 = numbered log to try next,
                    0 = live log is next, -1 = single file / chain done */
} lrd = { .fd = -1, .next = -1 };
static char lrd_buf[4096];

/* raw-byte hash of a full family pass, accumulated at the refill point
 * below; only playback_parse turns it on (probes stop early) */
static bool     lrd_hash_on;
static uint64_t lrd_hash;
static uint64_t lrd_hash_n;

static bool lrd_open(const char *path)
{
    lrd.fd   = rb->open(path, O_RDONLY);
    lrd.pos  = lrd.len = 0;
    lrd.next = -1;
    return lrd.fd >= 0;
}

static void lrd_close(void)
{
    if (lrd.fd >= 0) rb->close(lrd.fd);
    lrd.fd = -1;
    lrd.next = -1;
}

/* open the next file of the rotated playback-log family */
static bool lrd_advance(void)
{
    if (lrd.fd >= 0) rb->close(lrd.fd);
    lrd.fd  = -1;
    lrd.pos = lrd.len = 0;
    while (lrd.next > 0) {
        char p[48];
        rb->snprintf(p, sizeof p, "%s_%04d.log", PLAYBACK_LOG_STEM, lrd.next);
        lrd.next++;
        lrd.fd = rb->open(p, O_RDONLY);
        if (lrd.fd >= 0)
            return true;
        lrd.next = 0;          /* first missing number ends the numbered run */
    }
    if (lrd.next == 0) {
        lrd.next = -1;         /* the live log is the final link */
        lrd.fd = rb->open(PLAYBACK_LOG_PATH, O_RDONLY);
    }
    return lrd.fd >= 0;
}

/* The core rotates an oversized playback.log (511 KB cap) to the next free
 * playback_NNNN.log at boot, so history accumulates across a whole family
 * of files. Read them all: numbered logs oldest-first, live log last. */
static bool lrd_open_playback(void)
{
    lrd.pos = lrd.len = 0;
    lrd.next = 1;
    return lrd_advance();
}

/* next line into line[] (CR/LF stripped, NUL-terminated; an over-long line
 * is truncated but consumed whole, so its tail can't masquerade as the next
 * line). Steps across the playback family transparently, treating each
 * file's end as a line break. Returns false only when everything is read. */
static bool lrd_line(char *line, int sz)
{
    int n = 0;
    for (;;) {
        if (lrd.pos >= lrd.len) {
            lrd.len = (lrd.fd >= 0) ? rb->read(lrd.fd, lrd_buf, sizeof(lrd_buf)) : 0;
            if (lrd_hash_on && lrd.len > 0) {
                lrd_hash = pbl_hash_step(lrd_hash, lrd_buf, lrd.len);
                lrd_hash_n += lrd.len;
            }
            lrd.pos = 0;
            if (lrd.len <= 0) {
                lrd.len = 0;
                if (n > 0)
                    break;                    /* flush an unterminated tail */
                if (lrd.next >= 0 && lrd_advance())
                    continue;                 /* next file in the family */
                break;
            }
        }
        char c = lrd_buf[lrd.pos++];
        if (c == '\n') { line[n] = '\0'; return true; }
        if (c != '\r' && n < sz - 1) line[n++] = c;
    }
    line[n] = '\0';
    return n > 0;   /* final line without a trailing newline still counts */
}

/* which log feeds the stats. Decided ONCE per run (see wrapped_show) so the
 * year pre-scan, the main parse and the week drill-down can never disagree:
 * a freshly-created .scrobbler.log holding only '#' header lines used to
 * shadow a playback.log full of history and splash "Log is empty". */
static bool src_scrob;

/* 0 = no data lines, 1 = has data (reader must already be open) */
static int lrd_probe_data(void)
{
    char line[640];
    int r = 0;
    while (lrd_line(line, sizeof(line))) {
        if (line[0] != '#' && line[0] != '\0') { r = 1; break; }
    }
    lrd_close();
    return r;
}

/* -1 = file missing, 0 = exists but no data lines, 1 = has data */
static int log_probe(const char *path)
{
    if (!lrd_open(path))
        return -1;
    return lrd_probe_data();
}

static int log_probe_playback(void)
{
    if (!lrd_open_playback())
        return -1;
    return lrd_probe_data();
}

/* ---- cheat verdict machinery ---- */

static bool sig_read(struct pbl_sig *s)
{
    int fd = rb->open(PBL_SIG_PATH, O_RDONLY);
    if (fd < 0)
        return false;
    bool ok = (rb->read(fd, s, sizeof(*s)) == sizeof(*s)
               && s->magic == PBL_SIG_MAGIC);
    rb->close(fd);
    return ok;
}

static void cheat_flags_save(void)
{
    int fd = rb->open(CHEAT_DAT_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return;
    unsigned long magic = CHEAT_DAT_MAGIC;
    rb->write(fd, &magic, 4);
    rb->write(fd, &cheat_flags, 4);
    rb->close(fd);
}

static void cheat_flags_load(void)
{
    if (cheat_flags_loaded)
        return;
    cheat_flags_loaded = true;
    cheat_flags = 0;
    int fd = rb->open(CHEAT_DAT_PATH, O_RDONLY);
    if (fd < 0)
        return;
    unsigned long magic = 0;
    unsigned f = 0;
    if (rb->read(fd, &magic, 4) == 4 && magic == CHEAT_DAT_MAGIC
        && rb->read(fd, &f, 4) == 4)
        cheat_flags = f;
    rb->close(fd);
}

static void cheat_mark(unsigned f)
{
    if ((cheat_flags & f) == f)
        return;
    cheat_flags |= f;
    cheat_flags_save();
}

/* the one question everything else asks */
static bool cheat_rigged(void)
{
    return (cheat_flags & CHEAT_F_CHEATED) || cheat_live == 1;
}

/* verdict for a playback-family crunch: sig_pre was read before the parse,
 * lrd_hash/lrd_hash_n cover every raw byte the parse consumed */
static void cheat_judge(const struct pbl_sig *sig_pre, bool sig_pre_ok)
{
    cheat_flags_load();
    if (!sig_pre_ok) {
        /* no sig at all: an old core build never signed (fair), unless one
         * was seen before and has since vanished (not fair) */
        cheat_live = (cheat_flags & CHEAT_F_SIG_SEEN) ? 1 : -1;
        if (cheat_live == 1)
            cheat_mark(CHEAT_F_CHEATED);
        return;
    }
    cheat_mark(CHEAT_F_SIG_SEEN);
    if (sig_pre->flags & PBL_SIG_TAINTED) {
        cheat_live = 1;
        cheat_mark(CHEAT_F_CHEATED);
        return;
    }
    if (lrd_hash_n == sig_pre->nbytes && lrd_hash == sig_pre->hash) {
        cheat_live = 0;
        return;
    }
    /* mismatch - but if the core flushed a track mid-parse it moved both
     * the log and the sig under us; that's not a cheat, just no verdict */
    struct pbl_sig now;
    if (sig_read(&now)
        && (now.nbytes != sig_pre->nbytes || now.hash != sig_pre->hash)) {
        cheat_live = -1;
        return;
    }
    cheat_live = 1;
    cheat_mark(CHEAT_F_CHEATED);
}

/* verdict for a scrobbler-sourced crunch: a device that signs its playback
 * log sourcing stats from an unsigned .scrobbler.log is the obvious side
 * door, so it counts. A device that never signed stays unverifiable. */
static void cheat_judge_scrob(void)
{
    cheat_flags_load();
    struct pbl_sig s;
    if (sig_read(&s) || (cheat_flags & CHEAT_F_SIG_SEEN)) {
        cheat_live = 1;
        cheat_mark(CHEAT_F_CHEATED);
    } else {
        cheat_live = -1;
    }
}

/* shared per-play timestamp bookkeeping: date span, clock, days, weeks */
static void tally_time(unsigned long ts, unsigned secs)
{
    if (!ts_min || (long)ts < ts_min) ts_min = ts;
    if ((long)ts > ts_max) ts_max = ts;
    hour_hist[(ts % 86400UL) / 3600UL]++;
    day_add(ts / 86400UL, secs);

    long rel = (long)(ts / 86400UL) - display_day0;
    if (rel >= 0 && rel < 7L * N_WEEKS) {
        int wi = (int)(rel / 7);
        if (wi >= n_weeks) wi = n_weeks - 1;
        week_secs[wi] += secs;
    }
}

/* read whole log, build aggregates. returns number of data lines, -1 on error */
static long wrapped_parse(void)
{
    if (!lrd_open(LOG_PATH))
        return -1;

    /* 640 like every other reader: a line truncated here but counted by the
     * year pre-scan would let the picker offer a year the parse then drops */
    char line[640];
    char *f[8];
    long lines = 0;

    while (lrd_line(line, sizeof(line))) {
        if (line[0] == '#' || line[0] == '\0')
            continue;
        int nf = split_tabs(line, f, 8);
        if (nf < 7)
            continue;

        const char *artist = f[0];
        const char *album  = f[1];
        const char *title  = f[2];
        unsigned    length = (unsigned)parse_ul(f[4]);
        char        rating = f[5][0];
        unsigned long ts   = parse_ul(f[6]);

        bool valid_ts = (ts >= MIN_VALID_TS);
        if (filter_year && (!valid_ts || year_of_ts(ts) != filter_year))
            continue;

        if (artist[0] == '\0') artist = "(unknown)";
        if (title[0]  == '\0') title  = "(unknown)";

        bool listened = (rating == 'L' || rating == 'l');
        lines++;

        if (listened) {
            total_plays++;
            total_seconds += length;
            bool night = valid_ts && (ts % 86400UL) / 3600UL < 5;

            struct agg *a;
            a = htable_get(&t_artist, artist);
            if (a) { a->count++; a->seconds += length; if (night) a->night++; }
            a = htable_get(&t_track, title);
            if (a) {
                a->count++; a->seconds += length;
                if (night) a->night++;
                if (valid_ts && ts > a->last_ts) a->last_ts = ts;
            }
            if (album[0]) {
                a = htable_get(&t_album, album);
                if (a) { a->count++; a->seconds += length; }
            }

            if (valid_ts)
                tally_time(ts, length);
            ACH_HOOK(ts, valid_ts, true, length, artist, title);
        } else {
            total_skips++;
            struct agg *a = htable_get(&t_track, title);
            if (a) a->skips++;
            ACH_HOOK(ts, valid_ts, false, 0, artist, title);
        }
    }
    lrd_close();
    return lines;
}

/* ---- fallback: read the core's playback.log directly ----
 * format per line: timestamp(sec):elapsed(ms):length(ms):path
 * metadata is derived from the path since there are no ID3 tags here.  */

static char *next_colon(char *s)
{
    while (*s && *s != ':') s++;
    return *s ? s + 1 : NULL;
}

/* locate a " - " (space-hyphen-space) separator; returns ptr to the space */
static const char *find_dash_sep(const char *s)
{
    for (; s[0]; s++)
        if (s[0] == ' ' && s[1] == '-' && s[2] == ' ')
            return s;
    return NULL;
}

/* skip a leading "NN", "NN.", "NN -", "NN_" track-number prefix; returns ptr */
static char *strip_tracknum(char *s)
{
    char *d = s;
    while (*d >= '0' && *d <= '9') d++;
    if (d != s) {
        while (*d == '.' || *d == ' ' || *d == '-' || *d == '_') d++;
        if (*d) return d;
    }
    return s;
}

/* skip a leading "NN." track-number on an artist field ONLY when a dot follows,
 * so playlist names like "01. Artist" collapse but "21 Savage"/"311" are kept */
static char *strip_tracknum_dot(char *s)
{
    char *d = s;
    while (*d >= '0' && *d <= '9') d++;
    if (d != s && *d == '.') {
        d++;
        while (*d == ' ' || *d == '_') d++;
        if (*d) return d;
    }
    return s;
}

/* Derive artist + title from a file path, preferring the filename's own
 * "Artist - Album - NN Title" convention over the (often format-named) folder:
 *   "/x/Velvet Antenna FLAC/Velvet Antenna - Peel - 01 Peel.flac" -> "Velvet Antenna","Peel"
 *   "/x/Artist - 01 Title.mp3"                              -> "Artist","Title"
 *   "/x/Velvet Antenna/02. Test Pattern.mp4" (no " - ")     -> "Velvet Antenna","Test Pattern"
 */
static void path_to_meta(const char *path, char *artist, int asz, char *title, int tsz)
{
    const char *last  = rb->strrchr(path, '/');
    const char *fname = last ? last + 1 : path;

    char stem[160];
    rb->strlcpy(stem, fname, sizeof(stem));
    char *dot = rb->strrchr(stem, '.');
    if (dot && dot != stem) *dot = '\0';           /* drop extension */

    const char *sep1 = find_dash_sep(stem);
    if (sep1) {
        /* artist = text before the first " - " (minus any "NN." playlist index) */
        int n = (int)(sep1 - stem);
        if (n > asz - 1) n = asz - 1;
        rb->memcpy(artist, stem, n);
        artist[n] = '\0';
        char *as = strip_tracknum_dot(artist);
        if (as != artist) rb->strlcpy(artist, as, asz);

        /* title = field after the album (2nd " - "), else after the 1st */
        char *after1 = (char *)(sep1 + 3);
        const char *sep2 = find_dash_sep(after1);
        char *tsrc = sep2 ? (char *)(sep2 + 3) : after1;
        rb->strlcpy(title, strip_tracknum(tsrc), tsz);
    } else {
        /* no "Artist - " in the name: fall back to the parent folder */
        if (last && last != path) {
            const char *ps = last - 1;
            while (ps > path && *ps != '/') ps--;
            if (*ps == '/') ps++;
            int i = 0;
            while (ps < last && i < asz - 1) artist[i++] = *ps++;
            artist[i] = '\0';
        } else {
            artist[0] = '\0';
        }
        rb->strlcpy(title, strip_tracknum(stem), tsz);
    }

    if (artist[0] == '\0') rb->strlcpy(artist, "(unknown)", asz);
    if (title[0]  == '\0') rb->strlcpy(title, fname, tsz);
}

/* ============================================================
 *  Database-backed metadata. playback.log lines carry only file
 *  paths; the guesswork above can't know albums at all (hence the
 *  legendary five-album library). When the tagcache is ready we
 *  resolve every file it knows ONCE - {path-hash -> artist, album,
 *  title}, strings pooled - and cache that to disk keyed to the
 *  DB's entry count, so later opens skip the (seeky) retrieves.
 *  Files the DB doesn't know fall back to path parsing.
 * ============================================================ */

#define DBMAP_PATH  "/.rockbox/wrapped_dbmap.dat"
#define DBMAP_MAGIC 0x57446231UL   /* "WDb1" */

struct dbmap_ent {
    unsigned       hash;     /* hash_str of the full file path   */
    unsigned short artist;   /* pool offsets; 0 = unknown/empty  */
    unsigned short album;
    unsigned short title;
    unsigned short pad;
};

static struct dbmap_ent *dbmap;
static int      *dbmap_slots;        /* -> entry index + 1; 0 = empty */
static int       dbmap_n, dbmap_cap, dbmap_mask;
static char     *dbmap_pool;
static unsigned  dbmap_pool_used, dbmap_pool_cap;
static size_t    abuf_floor;         /* dbmap lives below this watermark,
                                        wrapped_reload resets down to it */

/* intern a string in the pool (offset 0 = the empty string) */
static unsigned short dbmap_intern(const char *s)
{
    if (!s || !s[0])
        return 0;
    unsigned p = 1;
    while (p < dbmap_pool_used) {           /* linear dedup: pool is small */
        if (!rb->strcmp(dbmap_pool + p, s))
            return (unsigned short)p;
        p += rb->strlen(dbmap_pool + p) + 1;
    }
    unsigned len = rb->strlen(s) + 1;
    if (dbmap_pool_used + len > dbmap_pool_cap || dbmap_pool_used + len > 0xFFFF)
        return 0;                           /* pool full: entry goes unknown */
    rb->memcpy(dbmap_pool + dbmap_pool_used, s, len);
    p = dbmap_pool_used;
    dbmap_pool_used += len;
    return (unsigned short)p;
}

static void dbmap_index(void)               /* (re)build the slot table */
{
    rb->memset(dbmap_slots, 0, (size_t)(dbmap_mask + 1) * sizeof(int));
    for (int i = 0; i < dbmap_n; i++) {
        int s = (int)(dbmap[i].hash & dbmap_mask);
        while (dbmap_slots[s])
            s = (s + 1) & dbmap_mask;
        dbmap_slots[s] = i + 1;
    }
}

static const struct dbmap_ent *dbmap_get(const char *path)
{
    if (!dbmap_n)
        return NULL;
    unsigned h = hash_str(path);
    int s = (int)(h & dbmap_mask);
    while (dbmap_slots[s]) {
        const struct dbmap_ent *e = &dbmap[dbmap_slots[s] - 1];
        if (e->hash == h)
            return e;                       /* hash collision risk accepted */
        s = (s + 1) & dbmap_mask;
    }
    return NULL;
}

static void dbmap_save(int db_entries)
{
    int fd = rb->open(DBMAP_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return;
    unsigned long hdr[4] = { DBMAP_MAGIC, (unsigned long)db_entries,
                             (unsigned long)dbmap_n, dbmap_pool_used };
    rb->write(fd, hdr, sizeof(hdr));
    rb->write(fd, dbmap, (size_t)dbmap_n * sizeof(struct dbmap_ent));
    rb->write(fd, dbmap_pool, dbmap_pool_used);
    rb->close(fd);
}

/* set up the map: cached file if it matches the DB's current entry count,
 * else one full sweep of the database. Call once per plugin run, BEFORE
 * the aggregate tables are allocated (it eats the bottom of the buffer). */
static void dbmap_init(void)
{
    dbmap = NULL;
    dbmap_slots = NULL;
    dbmap_n = 0;

    struct tagcache_stat *stat = rb->tagcache_get_stat();
    if (!stat || !stat->ready || stat->total_entries <= 0)
        return;                             /* no DB: path parsing it is */
    int total = stat->total_entries;

    /* memory: entries + slots + pool, at most ~a third of the buffer */
    dbmap_cap = total;
    int slots = next_pow2(dbmap_cap * 2);
    dbmap_pool_cap = (unsigned)dbmap_cap * 24;
    if (dbmap_pool_cap > 0xFFFF) dbmap_pool_cap = 0xFFFF;
    if (dbmap_pool_cap < 4096)   dbmap_pool_cap = 4096;
    size_t need = (size_t)dbmap_cap * sizeof(struct dbmap_ent)
                + (size_t)slots * sizeof(int) + dbmap_pool_cap;
    if (need > abuf_sz / 3)
        return;                             /* library too big: fall back */

    dbmap       = aalloc((size_t)dbmap_cap * sizeof(struct dbmap_ent));
    dbmap_slots = aalloc((size_t)slots * sizeof(int));
    dbmap_pool  = aalloc(dbmap_pool_cap);
    if (!dbmap || !dbmap_slots || !dbmap_pool) {
        dbmap = NULL;
        return;
    }
    dbmap_mask = slots - 1;
    dbmap_pool[0] = '\0';                   /* offset 0 = "" */
    dbmap_pool_used = 1;

    /* try the cache first */
    int fd = rb->open(DBMAP_PATH, O_RDONLY);
    if (fd >= 0) {
        unsigned long hdr[4];
        if (rb->read(fd, hdr, sizeof(hdr)) == sizeof(hdr)
            && hdr[0] == DBMAP_MAGIC && (int)hdr[1] == total
            && (int)hdr[2] > 0 && (int)hdr[2] <= dbmap_cap
            && hdr[3] > 0 && hdr[3] <= dbmap_pool_cap) {
            int n = (int)hdr[2];
            if (rb->read(fd, dbmap, n * sizeof(struct dbmap_ent))
                    == (ssize_t)(n * sizeof(struct dbmap_ent))
                && rb->read(fd, dbmap_pool, hdr[3]) == (ssize_t)hdr[3]) {
                dbmap_n = n;
                dbmap_pool_used = hdr[3];
                dbmap_index();
                rb->close(fd);
                return;
            }
        }
        rb->close(fd);
    }

    /* full sweep: every file the DB knows, with artist/album/title.
     * Seeky on spinning rust, so it only ever happens when the DB
     * changed; progress so it doesn't look hung. */
    struct tagcache_search tcs;
    if (!rb->tagcache_search(&tcs, tag_filename)) {
        dbmap = NULL;
        return;
    }
    char fname[MAX_PATH], meta[160];
    while (dbmap_n < dbmap_cap
           && rb->tagcache_get_next(&tcs, fname, sizeof(fname))) {
        struct dbmap_ent *e = &dbmap[dbmap_n];
        e->hash = hash_str(fname);
        e->pad  = 0;
        e->artist = rb->tagcache_retrieve(&tcs, tcs.idx_id, tag_artist,
                                          meta, sizeof(meta))
                        ? dbmap_intern(meta) : 0;
        e->album  = rb->tagcache_retrieve(&tcs, tcs.idx_id, tag_album,
                                          meta, sizeof(meta))
                        ? dbmap_intern(meta) : 0;
        e->title  = rb->tagcache_retrieve(&tcs, tcs.idx_id, tag_title,
                                          meta, sizeof(meta))
                        ? dbmap_intern(meta) : 0;
        dbmap_n++;
        if ((dbmap_n & 63) == 0)
            rb->splashf(0, "Reading database... %d/%d", dbmap_n, total);
    }
    rb->tagcache_search_finish(&tcs);
    if (dbmap_n > 0) {
        dbmap_index();
        dbmap_save(total);
    } else {
        dbmap = NULL;
    }
}

/* untagged markers the DB uses for missing fields */
static bool dbmap_real(const char *s)
{
    return s[0] && rb->strcmp(s, "<Untagged>") != 0;
}

/* artist/title (and album when known) for a logged path: database when it
 * knows the file, filename guesswork when it doesn't. Returns true when
 * `album` holds a real album name. */
static bool resolve_meta(const char *path, char *artist, char *title,
                         char *album)
{
    const struct dbmap_ent *e = dbmap_get(path);
    album[0] = '\0';
    if (e) {
        const char *ar = dbmap_pool + e->artist;
        const char *ti = dbmap_pool + e->title;
        const char *al = dbmap_pool + e->album;
        if (dbmap_real(ar) && dbmap_real(ti)) {
            rb->strlcpy(artist, ar, NAME_MAX_LEN);
            rb->strlcpy(title, ti, NAME_MAX_LEN);
            if (dbmap_real(al))
                rb->strlcpy(album, al, NAME_MAX_LEN);
            return album[0] != '\0';
        }
    }
    path_to_meta(path, artist, NAME_MAX_LEN, title, NAME_MAX_LEN);
    return false;
}

static long playback_parse(void)
{
    if (!lrd_open_playback())
        return -1;

    char line[640];
    long lines = 0;

    while (lrd_line(line, sizeof(line))) {
        if (line[0] == '#' || line[0] == '\0')
            continue;

        char *p = line;
        unsigned long ts = parse_ul(p);
        if (!(p = next_colon(p))) continue;
        unsigned long el_ms = parse_ul(p);
        if (!(p = next_colon(p))) continue;
        unsigned long len_ms = parse_ul(p);
        if (!(p = next_colon(p))) continue;
        char *path = p;
        if (path[0] == '\0') continue;

        bool valid_ts = (ts >= MIN_VALID_TS);
        if (filter_year && (!valid_ts || year_of_ts(ts) != filter_year))
            continue;

        char artist[NAME_MAX_LEN], title[NAME_MAX_LEN], album[NAME_MAX_LEN];
        bool have_album = resolve_meta(path, artist, title, album);

        unsigned elapsed = (unsigned)(el_ms / 1000);
        /* Last.fm rule: counts as a play if >=50% heard or >=4 minutes */
        bool listened = (len_ms == 0) || (el_ms * 2 >= len_ms) || (el_ms >= 240000UL);
        lines++;

        if (listened) {
            total_plays++;
            total_seconds += elapsed;
            bool night = valid_ts && (ts % 86400UL) / 3600UL < 5;

            struct agg *a;
            a = htable_get(&t_artist, artist);
            if (a) { a->count++; a->seconds += elapsed; if (night) a->night++; }
            a = htable_get(&t_track, title);
            if (a) {
                a->count++; a->seconds += elapsed;
                if (night) a->night++;
                if (valid_ts && ts > a->last_ts) a->last_ts = ts;
            }
            /* real albums when the database knows them; the artist bucket
             * keeps old logs of unknown files roughly useful */
            a = htable_get(&t_album, have_album ? album : artist);
            if (a) {
                a->count++; a->seconds += elapsed;
                /* remember this album's folder the first time we see it */
                int ai = (int)(a - t_album.items);
                if (album_dir && ai >= 0 && ai < album_dir_cap && album_dir[ai][0] == '\0') {
                    const char *slash = rb->strrchr(path, '/');
                    if (slash && slash != path) {
                        int L = (int)(slash - path) + 1;   /* keep trailing '/' */
                        if (L < ART_PATH_MAX) {
                            rb->memcpy(album_dir[ai], path, L);
                            album_dir[ai][L] = '\0';
                        }
                    }
                }
            }

            if (valid_ts)
                tally_time(ts, elapsed);
            ACH_HOOK(ts, valid_ts, true, elapsed, artist, title);
        } else if (el_ms >= 5000) {
            /* under 5s heard = a browsing tap, not a judgement on the song */
            total_skips++;
            struct agg *a = htable_get(&t_track, title);
            if (a) a->skips++;
            ACH_HOOK(ts, valid_ts, false, 0, artist, title);
        }
    }
    lrd_close();
    return lines;
}

/* the value an entry ranks by under a given top_mode (<=0 = excluded) */
static int agg_metric(const struct agg *a, int mode)
{
    switch (mode) {
    case TOP_SKIPS: return a->skips;
    case TOP_LOYAL: return (a->skips == 0) ? a->count : 0;
    case TOP_REDIS: return (a->last_ts && a->last_ts <= redis_cutoff)
                           ? a->count : 0;
    case TOP_NIGHT: return a->night;
    default:        return a->count;
    }
}

/* selection-sort the top `topn` entries of a table by metric, into out[] */
static int top_n(struct htable *t, struct agg **out, int topn, int mode)
{
    int got = 0;
    for (int k = 0; k < topn; k++) {
        int best = -1, best_val = 0;
        for (int i = 0; i < t->n; i++) {
            struct agg *a = &t->items[i];
            int v = agg_metric(a, mode);
            if (v <= 0) continue;
            bool taken = false;
            for (int j = 0; j < got; j++)
                if (out[j] == a) { taken = true; break; }
            if (taken) continue;
            if (best < 0 || v > best_val) { best = i; best_val = v; }
        }
        if (best < 0) break;
        out[got++] = &t->items[best];
    }
    return got;
}

/* ---- civil date from unix-day index (Howard Hinnant's algorithm) ---- */
static void civil_from_days(long z, int *y, int *m, int *d)
{
    z += 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    long year = (long)yoe + era * 400;
    unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);
    unsigned mp = (5*doy + 2)/153;
    unsigned day = doy - (153*mp+2)/5 + 1;
    unsigned month = mp < 10 ? mp + 3 : mp - 9;
    *y = (int)(year + (month <= 2));
    *m = (int)month;
    *d = (int)day;
}

/* unix-day index from a civil date (inverse of the above, same source) */
static long days_from_civil(int y, int m, int d)
{
    y -= m <= 2;
    long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153u * (unsigned)(m + (m > 2 ? -3 : 9)) + 2u) / 5u + d - 1;
    unsigned doe = yoe * 365 + yoe/4 - yoe/100 + doy;
    return era * 146097 + (long)doe - 719468;
}

static int year_of_ts(unsigned long ts)
{
    int y, m, d;
    civil_from_days((long)(ts / 86400UL), &y, &m, &d);
    return y;
}

/* scan the active log once for the distinct calendar years it covers, newest
 * first; returns the count and fills years[] (up to maxy) */
static int collect_years(int *years, int maxy)
{
    bool scrobbler = src_scrob;
    if (scrobbler ? !lrd_open(LOG_PATH) : !lrd_open_playback())
        return 0;

    char line[640];
    char *f[8];
    int cnt = 0;
    while (lrd_line(line, sizeof(line))) {
        if (line[0] == '#' || line[0] == '\0')
            continue;
        unsigned long ts;
        if (scrobbler) {
            if (split_tabs(line, f, 8) < 7) continue;
            ts = parse_ul(f[6]);
        } else {
            ts = parse_ul(line);            /* first field of playback.log */
        }
        if (ts < MIN_VALID_TS) continue;
        int y = year_of_ts(ts);
        bool seen = false;
        for (int i = 0; i < cnt; i++) if (years[i] == y) { seen = true; break; }
        if (!seen && cnt < maxy) years[cnt++] = y;
    }
    lrd_close();
    for (int i = 0; i < cnt; i++)           /* sort descending (newest first) */
        for (int j = i + 1; j < cnt; j++)
            if (years[j] > years[i]) { int t = years[i]; years[i] = years[j]; years[j] = t; }
    return cnt;
}

static const char *month_abbr[] = {
    "", "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};

/* "1234" -> "1,234" */
static void commafmt(long v, char *out, size_t outsz)
{
    char tmp[24];
    rb->snprintf(tmp, sizeof(tmp), "%ld", v < 0 ? -v : v);
    int len = rb->strlen(tmp);
    int o = 0;
    if (v < 0 && o < (int)outsz - 1) out[o++] = '-';
    for (int i = 0; i < len; i++) {
        if (i && (len - i) % 3 == 0 && o < (int)outsz - 1) out[o++] = ',';
        if (o < (int)outsz - 1) out[o++] = tmp[i];
    }
    out[o] = '\0';
}

/* ============================================================
 *  Rendering toolkit: gradients, AA shapes, vector numerals
 *  (16-bit RGB565; no framebuffer read, so we blend against the
 *   gradient analytically since we know every pixel's bg colour)
 * ============================================================ */

static int body_font = FONT_SYSFIXED;

#define COL_TEXT_LIGHT  LCD_RGBPACK(255, 255, 255)
#define COL_TEXT_DIM    LCD_RGBPACK(205, 210, 220)

struct theme {
    unsigned bg0, bg1;     /* vertical background gradient: top -> bottom   */
    unsigned num0, num1;   /* hero numeral gradient:        top -> bottom   */
    unsigned accent;       /* kickers, badges, highlights                   */
};

/* card order; themes[] below must match */
enum card_id {
    CARD_INTRO = 0,
    CARD_MINUTES,
    CARD_PLAYS,
    CARD_ARTISTS,
    CARD_SONGS,
    CARD_ALBUMS,
    CARD_CLOCK,      /* listening clock (restored by popular demand) */
    CARD_NIGHT,      /* the 3am club */
    CARD_SKIPS,
    CARD_LOYAL,      /* never skipped */
    CARD_REDIS,      /* rediscovery */
    CARD_YEAR,
    CARD_HEAT,       /* every-day heatmap */
    CARD_TYPE,       /* listening personality */
#ifdef WRAPPED_WITH_ACH
    CARD_ACH,        /* achievements summary; SELECT browses the badges */
#endif
    CARD_OUTRO,
    N_CARDS
};

#ifdef WRAPPED_WITH_ACH
/* implemented in achievements_core.h, included after this header */
static int ach_wrapped_card(int idx, int dir);
static int ach_browse(void);
#endif

/* vivid, Spotify-Wrapped-style palettes, one per card */
static const struct theme themes[N_CARDS] = {
    /* intro   */ { LCD_RGBPACK( 45, 22, 80), LCD_RGBPACK(120, 30,110),
                    LCD_RGBPACK(235,215,255), LCD_RGBPACK(255,180,235), LCD_RGBPACK(220,185,255) },
    /* minutes */ { LCD_RGBPACK(  8, 95, 92), LCD_RGBPACK( 22,158, 96),
                    LCD_RGBPACK(225,255,235), LCD_RGBPACK(130,245,175), LCD_RGBPACK(180,255,210) },
    /* plays   */ { LCD_RGBPACK( 78, 42,158), LCD_RGBPACK( 42, 78,188),
                    LCD_RGBPACK(240,235,255), LCD_RGBPACK(175,205,255), LCD_RGBPACK(210,200,255) },
    /* artists */ { LCD_RGBPACK(190, 42, 96), LCD_RGBPACK(150, 28, 62),
                    LCD_RGBPACK(255,235,242), LCD_RGBPACK(255,170,195), LCD_RGBPACK(255,200,215) },
    /* tracks  */ { LCD_RGBPACK( 28, 82,178), LCD_RGBPACK( 20,150,190),
                    LCD_RGBPACK(235,246,255), LCD_RGBPACK(160,215,255), LCD_RGBPACK(190,225,255) },
    /* albums  */ { LCD_RGBPACK(190, 96, 18), LCD_RGBPACK(176,138, 20),
                    LCD_RGBPACK(255,244,225), LCD_RGBPACK(255,205,140), LCD_RGBPACK(255,225,170) },
    /* clock   */ { LCD_RGBPACK( 18, 84,104), LCD_RGBPACK( 20, 40, 84),
                    LCD_RGBPACK(225,250,252), LCD_RGBPACK(130,225,232), LCD_RGBPACK(150,230,235) },
    /* night   */ { LCD_RGBPACK( 20, 18, 70), LCD_RGBPACK(  6,  6, 26),
                    LCD_RGBPACK(255,250,225), LCD_RGBPACK(215,205,255), LCD_RGBPACK(255,225,140) },
    /* skips   */ { LCD_RGBPACK(180, 52, 52), LCD_RGBPACK(112, 28, 40),
                    LCD_RGBPACK(255,238,235), LCD_RGBPACK(255,175,165), LCD_RGBPACK(255,190,180) },
    /* loyal   */ { LCD_RGBPACK(140, 20, 90), LCD_RGBPACK( 60, 12, 50),
                    LCD_RGBPACK(255,235,245), LCD_RGBPACK(255,160,205), LCD_RGBPACK(255,140,200) },
    /* redis   */ { LCD_RGBPACK( 98, 76,140), LCD_RGBPACK( 44, 36, 78),
                    LCD_RGBPACK(245,240,255), LCD_RGBPACK(200,180,250), LCD_RGBPACK(215,190,255) },
    /* year    */ { LCD_RGBPACK( 16, 64, 96), LCD_RGBPACK( 12, 34, 44),
                    LCD_RGBPACK(230,255,240), LCD_RGBPACK(120,245,165), LCD_RGBPACK( 95,235,140) },
    /* heat    */ { LCD_RGBPACK( 14, 58, 34), LCD_RGBPACK(  6, 22, 14),
                    LCD_RGBPACK(230,255,238), LCD_RGBPACK(120,240,160), LCD_RGBPACK(110,235,150) },
    /* type    */ { LCD_RGBPACK(158, 20,104), LCD_RGBPACK( 54, 12, 92),
                    LCD_RGBPACK(255,235,248), LCD_RGBPACK(255,160,215), LCD_RGBPACK(255,150,210) },
#ifdef WRAPPED_WITH_ACH
    /* ach     */ { LCD_RGBPACK( 52, 38, 12), LCD_RGBPACK( 18, 11,  6),
                    LCD_RGBPACK(255,244,200), LCD_RGBPACK(255,196, 70), LCD_RGBPACK(255,185, 50) },
#endif
    /* outro   */ { LCD_RGBPACK(120, 30,110), LCD_RGBPACK( 45, 22, 80),
                    LCD_RGBPACK(235,215,255), LCD_RGBPACK(255,180,235), LCD_RGBPACK(220,185,255) },
};

static inline int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static int isqrtl(long v)
{
    if (v <= 0) return 0;
    long x = v, y = (x + 1) >> 1;
    while (y < x) { x = y; y = (x + v / x) >> 1; }
    return (int)x;
}

/* lerp two RGB565 colours, t in 0..256 (0 -> a, 256 -> b) */
static unsigned blend565(unsigned a, unsigned b, int t)
{
    int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
    int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
    int it = 256 - t;
    int r = (ar * it + br * t) >> 8;
    int g = (ag * it + bg * t) >> 8;
    int bl = (ab * it + bb * t) >> 8;
    return (unsigned)((r << 11) | (g << 5) | bl);
}

static unsigned grad_at(const struct theme *th, int y)
{
    int t = (clampi(y, 0, SCREEN_H - 1) * 256) / (SCREEN_H - 1);
    return blend565(th->bg0, th->bg1, t);
}

static void fill_gradient(const struct theme *th)
{
    rb->lcd_set_drawmode(DRMODE_SOLID);
    for (int y = 0; y < SCREEN_H; y++) {
        rb->lcd_set_foreground(grad_at(th, y));
        rb->lcd_hline(0, SCREEN_W - 1, y);
    }
}

/* repaint a horizontal band back to the gradient (to redraw over it) */
static void repaint_band(const struct theme *th, int y0, int h)
{
    rb->lcd_set_drawmode(DRMODE_SOLID);
    for (int y = y0; y < y0 + h; y++) {
        if (y < 0 || y >= SCREEN_H) continue;
        rb->lcd_set_foreground(grad_at(th, y));
        rb->lcd_hline(0, SCREEN_W - 1, y);
    }
}

/* squared distance from point (px,py) to segment (x1,y1)-(x2,y2), all in
 * glyph-local sub-pixel units; 64-bit intermediates guard against overflow */
static long seg_dist2(long px, long py, long x1, long y1, long x2, long y2)
{
    long vx = x2 - x1, vy = y2 - y1;
    long wx = px - x1, wy = py - y1;
    long c1 = vx * wx + vy * wy;
    if (c1 <= 0) { long dx = px - x1, dy = py - y1; return dx*dx + dy*dy; }
    long c2 = vx * vx + vy * vy;
    if (c2 <= c1) { long dx = px - x2, dy = py - y2; return dx*dx + dy*dy; }
    long projx = x1 + (long)(((long long)vx * c1) / c2);
    long projy = y1 + (long)(((long long)vy * c1) / c2);
    long dx = px - projx, dy = py - projy;
    return dx*dx + dy*dy;
}

#define SUBPX 16

/* AA filled disc; alpha 0..256 scales overall opacity */
static void draw_disc(const struct theme *th, int cx, int cy, int r,
                      unsigned col, int alpha)
{
    rb->lcd_set_drawmode(DRMODE_SOLID);
    long rU = (long)r * SUBPX;
    for (int yy = -r - 1; yy <= r + 1; yy++) {
        int ay = cy + yy;
        if (ay < 0 || ay >= SCREEN_H) continue;
        unsigned bg = grad_at(th, ay);
        long dyU = (long)yy * SUBPX;
        for (int xx = -r - 1; xx <= r + 1; xx++) {
            long dxU = (long)xx * SUBPX;
            int d = isqrtl(dxU*dxU + dyU*dyU);
            int diff = (int)(rU - d);
            int cov;
            if (diff >= SUBPX/2) cov = 256;
            else if (diff <= -SUBPX/2) cov = 0;
            else cov = ((diff + SUBPX/2) * 256) / SUBPX;
            cov = (cov * alpha) >> 8;
            if (cov <= 0) continue;
            if (cov > 256) cov = 256;
            rb->lcd_set_foreground(blend565(bg, col, cov));
            rb->lcd_drawpixel(cx + xx, ay);
        }
    }
}

/* AA rounded capsule (thick line with round caps), half-width hw px */
static void draw_capsule(const struct theme *th, int x1, int y1, int x2, int y2,
                         int hw, unsigned col)
{
    int minx = (x1 < x2 ? x1 : x2) - hw - 1;
    int maxx = (x1 > x2 ? x1 : x2) + hw + 1;
    int miny = (y1 < y2 ? y1 : y2) - hw - 1;
    int maxy = (y1 > y2 ? y1 : y2) + hw + 1;
    long X1 = (long)x1*SUBPX, Y1 = (long)y1*SUBPX;
    long X2 = (long)x2*SUBPX, Y2 = (long)y2*SUBPX;
    long hwU = (long)hw * SUBPX;
    rb->lcd_set_drawmode(DRMODE_SOLID);
    for (int ay = miny; ay <= maxy; ay++) {
        if (ay < 0 || ay >= SCREEN_H) continue;
        unsigned bg = grad_at(th, ay);
        long pyU = (long)ay*SUBPX + SUBPX/2;
        for (int ax = minx; ax <= maxx; ax++) {
            long pxU = (long)ax*SUBPX + SUBPX/2;
            int d = isqrtl(seg_dist2(pxU, pyU, X1, Y1, X2, Y2));
            int diff = (int)(hwU - d);
            int cov;
            if (diff >= SUBPX/2) cov = 256;
            else if (diff <= -SUBPX/2) cov = 0;
            else cov = ((diff + SUBPX/2) * 256) / SUBPX;
            if (cov <= 0) continue;
            if (cov > 256) cov = 256;
            rb->lcd_set_foreground(blend565(bg, col, cov));
            rb->lcd_drawpixel(ax, ay);
        }
    }
}

/* ---- vector numerals: rounded-segment glyphs, AA, vertical gradient ---- */

/* segment bits: a=1 b=2 c=4 d=8 e=16 f=32 g=64 */
static const unsigned char seg_digit[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

static void draw_glyph(const struct theme *th, int gx, int gy, char c,
                       int cw, int ch, int sw)
{
    int hw = sw / 2; if (hw < 1) hw = 1;
    long X1[8], Y1[8], X2[8], Y2[8]; int ns = 0;
    long hwU = (long)hw * SUBPX;

    if (c >= '0' && c <= '9') {
        unsigned char seg = seg_digit[c - '0'];
        int xL = hw, xR = cw - hw, yT = hw, yM = ch/2, yB = ch - hw, gp = hw;
        #define SEG(a,b,cc,d) do{ X1[ns]=(long)(a)*SUBPX; Y1[ns]=(long)(b)*SUBPX; \
            X2[ns]=(long)(cc)*SUBPX; Y2[ns]=(long)(d)*SUBPX; ns++; }while(0)
        if (seg & 0x01) SEG(xL+gp, yT,    xR-gp, yT);
        if (seg & 0x02) SEG(xR,    yT+gp, xR,    yM-gp);
        if (seg & 0x04) SEG(xR,    yM+gp, xR,    yB-gp);
        if (seg & 0x08) SEG(xL+gp, yB,    xR-gp, yB);
        if (seg & 0x10) SEG(xL,    yM+gp, xL,    yB-gp);
        if (seg & 0x20) SEG(xL,    yT+gp, xL,    yM-gp);
        if (seg & 0x40) SEG(xL+gp, yM,    xR-gp, yM);
        #undef SEG
    } else if (c == ',') {
        int x = hw * 2;
        X1[0] = (long)x*SUBPX;      Y1[0] = (long)(ch-hw)*SUBPX;
        X2[0] = (long)(x-hw)*SUBPX; Y2[0] = (long)(ch-hw+sw)*SUBPX; ns = 1;
    } else if (c == ':') {
        int x = hw;
        X1[0]=X2[0]=(long)x*SUBPX; Y1[0]=Y2[0]=(long)(ch/3)*SUBPX;
        X1[1]=X2[1]=(long)x*SUBPX; Y1[1]=Y2[1]=(long)(2*ch/3)*SUBPX; ns = 2;
    } else {
        return;
    }

    int wlim = cw + hw;
    for (int py = 0; py < ch + sw; py++) {
        int ay = gy + py;
        if (ay < 0 || ay >= SCREEN_H) continue;
        unsigned bg = grad_at(th, ay);
        int gt = (ch > 1) ? clampi(py * 256 / (ch - 1), 0, 256) : 0;
        unsigned fg = blend565(th->num0, th->num1, gt);
        long pyU = (long)py*SUBPX + SUBPX/2;
        for (int px = 0; px < wlim; px++) {
            long pxU = (long)px*SUBPX + SUBPX/2;
            long best = 0x7fffffffL;
            for (int k = 0; k < ns; k++) {
                long d = seg_dist2(pxU, pyU, X1[k], Y1[k], X2[k], Y2[k]);
                if (d < best) best = d;
            }
            int d = isqrtl(best);
            int diff = (int)(hwU - d);
            int cov;
            if (diff >= SUBPX/2) cov = 256;
            else if (diff <= -SUBPX/2) cov = 0;
            else cov = ((diff + SUBPX/2) * 256) / SUBPX;
            if (cov <= 0) continue;
            if (cov > 256) cov = 256;
            rb->lcd_set_foreground(blend565(bg, fg, cov));
            rb->lcd_drawpixel(gx + px, ay);
        }
    }
}

static int char_adv(char c, int cw, int sw)
{
    if (c == ',' || c == ':') return sw * 3;
    return cw + sw;
}

static int number_width(const char *s, int cw, int sw)
{
    int w = 0;
    for (const char *p = s; *p; p++) w += char_adv(*p, cw, sw);
    if (w) w -= sw;
    return w;
}

/* draw a numeric string centred horizontally on cx, top at oy */
static void draw_number(const struct theme *th, int cx, int oy, const char *s,
                        int cw, int ch, int sw)
{
    int x = cx - number_width(s, cw, sw) / 2;
    for (const char *p = s; *p; p++) {
        draw_glyph(th, x, oy, *p, cw, ch, sw);
        x += char_adv(*p, cw, sw);
    }
}

/* percent sign built from two discs + a diagonal capsule */
static void draw_percent(const struct theme *th, int cx, int cy, int sz)
{
    unsigned col = th->num1;
    int r = sz / 6; if (r < 2) r = 2;
    int hw = sz / 14; if (hw < 2) hw = 2;
    int x0 = cx - sz/2, y0 = cy - sz/2, x1 = cx + sz/2, y1 = cy + sz/2;
    draw_capsule(th, x0 + r, y1 - r, x1 - r, y0 + r, hw, col);
    draw_disc(th, x0 + r, y0 + r, r, col, 256);
    draw_disc(th, x1 - r, y1 - r, r, col, 256);
}

/* ---- text helpers (drawn transparently over the gradient) ---- */

static void center_text(int y, const char *s, unsigned color)
{
    int w, h;
    rb->lcd_setfont(body_font);
    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_getstringsize(s, &w, &h);
    rb->lcd_set_foreground(color);
    rb->lcd_putsxy((SCREEN_W - w) / 2, y, s);
}

/* two texts on one centred line, each with its own colour */
static void center_text2(int y, const char *a, unsigned ca,
                                const char *b, unsigned cb)
{
    int wa, wb, h;
    rb->lcd_setfont(body_font);
    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_getstringsize(a, &wa, &h);
    rb->lcd_getstringsize(b, &wb, &h);
    int x = (SCREEN_W - wa - wb) / 2;
    rb->lcd_set_foreground(ca);
    rb->lcd_putsxy(x, y, a);
    rb->lcd_set_foreground(cb);
    rb->lcd_putsxy(x + wa, y, b);
}

/* centred, lightly letter-spaced label for that premium "kicker" tracking */
static void kicker(int y, const char *s, unsigned color)
{
    const int sp = 2;
    int total = 0, cw, ch;
    char b[2] = {0, 0};
    rb->lcd_setfont(body_font);
    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_set_foreground(color);
    for (const char *p = s; *p; p++) {
        b[0] = *p; rb->lcd_getstringsize(b, &cw, &ch); total += cw + sp;
    }
    total -= sp;
    int x = (SCREEN_W - total) / 2;
    for (const char *p = s; *p; p++) {
        b[0] = *p; rb->lcd_getstringsize(b, &cw, &ch);
        rb->lcd_putsxy(x, y, b); x += cw + sp;
    }
}

static void accent_underline(const struct theme *th, int y)
{
    draw_capsule(th, SCREEN_W/2 - 16, y, SCREEN_W/2 + 16, y, 2, th->accent);
}

/* copy s into out, truncating with ".." if wider than maxw px */
static void fit_text(const char *s, int maxw, char *out, int outsz)
{
    int w, h;
    rb->lcd_setfont(body_font);
    rb->strlcpy(out, s, outsz);
    rb->lcd_getstringsize(out, &w, &h);
    if (w <= maxw) return;
    int len = rb->strlen(out);
    while (len > 2) {
        len--; out[len] = '\0';
        rb->lcd_getstringsize(out, &w, &h);
        if (w <= maxw - 10) break;
    }
    if (len >= 2) { out[len-1] = '.'; out[len] = '.'; out[len+1] = '\0'; }
}

/* pill-style page indicator at the bottom */
static void page_dots(int idx, const struct theme *th)
{
    int y = SCREEN_H - 11, dot = 5, cur = 15, gap = 5;
    int totw = -gap;
    for (int i = 0; i < N_CARDS; i++) totw += ((i == idx) ? cur : dot) + gap;
    int x = (SCREEN_W - totw) / 2;
    for (int i = 0; i < N_CARDS; i++) {
        int w = (i == idx) ? cur : dot;
        unsigned c = (i == idx) ? th->accent
                                 : blend565(grad_at(th, y), LCD_RGBPACK(255,255,255), 70);
        draw_capsule(th, x + 2, y, x + w - 2, y, 2, c);
        x += w + gap;
    }
}

/* ease-out-quad value at frame fr of frames toward target */
static long ease_val(long target, int fr, int frames)
{
    long inv = frames - fr;
    return target - (target * inv * inv) / ((long)frames * frames);
}

/* abort-worthy buttons during an animation -> hand them back to the loop.
 * BUTTON_REPEAT is masked off: continuous wheel rotation posts repeat-
 * flagged scroll events, which must count as navigation too. */
static bool nav_button(int b)
{
    if (b == SYS_USB_CONNECTED)
        return true;
    b &= ~BUTTON_REPEAT;
    return b == BUTTON_SCROLL_FWD || b == BUTTON_SCROLL_BACK ||
           b == BUTTON_LEFT || b == BUTTON_RIGHT ||
           b == BUTTON_MENU || b == BUTTON_SELECT;
}

/* PLAY saves whatever card is on screen as a BMP in the device root
 * (rb->screen_dump is our own addition to the plugin API) */
static void save_card(void)
{
#ifdef HAVE_SCREENDUMP
    rb->screen_dump();
    rb->splash(HZ * 2/3, "Saved as BMP (device root)");
#else
    rb->splash(HZ, "No screendump in this build");
#endif
}

/* deck export renders every card once: animations jump straight to their
 * final frame so the screendumps show finished cards */
static bool export_mode;

#ifdef HAVE_SCREENDUMP
/* screen_dump() names its file "dump <datetime>.bmp" in the root and offers
 * no say in the matter; claim the newest one and give it a sane name */
static bool dump_claim(const char *dst)
{
    DIR *d = rb->opendir("/");
    if (!d)
        return false;
    char newest[MAX_PATH];
    unsigned long best = 0;
    struct dirent *e;
    newest[0] = '\0';
    while ((e = rb->readdir(d)) != NULL) {
        int len = rb->strlen(e->d_name);
        if (len < 9 || rb->strncmp(e->d_name, "dump", 4)
            || rb->strcasecmp(e->d_name + len - 4, ".bmp"))
            continue;
        struct dirinfo info = rb->dir_get_info(d, e);
        if ((unsigned long)info.mtime >= best) {
            best = (unsigned long)info.mtime;
            rb->snprintf(newest, sizeof(newest), "/%s", e->d_name);
        }
    }
    rb->closedir(d);
    if (!newest[0])
        return false;
    rb->remove(dst);                     /* re-exports overwrite */
    return rb->rename(newest, dst) == 0;
}
#endif

/* render every card in its finished state and screendump each one;
 * lives behind SELECT on the outro card */
static int draw_card(int idx, int dir);

static void export_deck(void)
{
#ifdef HAVE_SCREENDUMP
    rb->mkdir("/wrapped_cards");
    export_mode = true;
    int saved = 0;
    for (int i = 0; i < N_CARDS; i++) {
        draw_card(i, 0);
        rb->screen_dump();
        char dst[40];
        rb->snprintf(dst, sizeof(dst), "/wrapped_cards/card_%02d.bmp", i + 1);
        if (dump_claim(dst))
            saved++;
        rb->button_clear_queue();        /* nav during export goes nowhere */
    }
    export_mode = false;
    rb->splashf(HZ * 2, "%d cards saved in /wrapped_cards", saved);
#else
    rb->splash(HZ, "No screendump in this build");
#endif
}

/* Push the fully-rendered back-buffer to the LCD. dir 0 = plain flush;
 * dir +1/-1 = reveal it as a vertical wipe in that direction (a cheap
 * transition: no re-render, just stream out columns of the new frame). */
static void present_card(int dir)
{
    if (dir == 0) { rb->lcd_update(); return; }
    const int step = 24;
    int nstrips = (SCREEN_W + step - 1) / step;
    for (int s = 0; s < nstrips; s++) {
        int i = (dir > 0) ? s : (nstrips - 1 - s);
        int x = i * step;
        int w = (x + step > SCREEN_W) ? (SCREEN_W - x) : step;
        rb->lcd_update_rect(x, 0, w, SCREEN_H);
        rb->sleep(1);
    }
}

/* ============================================================
 *  Card rendering
 * ============================================================ */

static int hero_band_y, hero_band_h;

/* animated count-up of a hero number in the hero band; returns nav btn or 0 */
static int animate_count(const struct theme *th, int cx, int oy, int cw, int ch,
                         int sw, long target)
{
    if (export_mode) {
        char f[24];
        commafmt(target, f, sizeof f);
        repaint_band(th, hero_band_y, hero_band_h);
        draw_number(th, cx, oy, f, cw, ch, sw);
        rb->lcd_update_rect(0, hero_band_y, SCREEN_W, hero_band_h);
        return 0;
    }
    int frames = 16;
    for (int fr = 0; fr <= frames; fr++) {
        long val = ease_val(target, fr, frames);
        char b[24]; commafmt(val, b, sizeof b);
        repaint_band(th, hero_band_y, hero_band_h);
        draw_number(th, cx, oy, b, cw, ch, sw);
        rb->lcd_update_rect(0, hero_band_y, SCREEN_W, hero_band_h);
        int btn = rb->button_get_w_tmo(HZ / 50);
        if (nav_button(btn)) {
            char f[24]; commafmt(target, f, sizeof f);
            repaint_band(th, hero_band_y, hero_band_h);
            draw_number(th, cx, oy, f, cw, ch, sw);
            rb->lcd_update_rect(0, hero_band_y, SCREEN_W, hero_band_h);
            return btn;
        }
    }
    return 0;
}

/* ---- album cover art (folder images, scaled to thumbnails) ---- */
#define ART_PX 30
static fb_data      art_data[5][ART_PX * ART_PX];
static struct bitmap art_bm[5];

static const char *art_names[] = {
    "cover.bmp", "folder.bmp", "cover.jpg", "folder.jpg", "Folder.jpg",
    "cover.jpeg", "albumart.jpg", "front.jpg",
};

/* probe `dir` for a known cover filename; decode scaled into slot `slot`.
 * returns true and fills art_bm[slot] on success. */
static bool load_cover(const char *dir, int slot)
{
    if (!dir || !dir[0]) return false;
    char path[ART_PATH_MAX + 24];
    int fmt = FORMAT_NATIVE | FORMAT_RESIZE | FORMAT_KEEP_ASPECT;
    for (unsigned k = 0; k < sizeof(art_names)/sizeof(art_names[0]); k++) {
        rb->snprintf(path, sizeof path, "%s%s", dir, art_names[k]);
        struct bitmap *bm = &art_bm[slot];
        bm->width = ART_PX; bm->height = ART_PX;
        bm->format = FORMAT_NATIVE;
        bm->data = (unsigned char *)art_data[slot];
        const char *ext = rb->strrchr(art_names[k], '.');
        bool jpg = ext && (rb->strcasecmp(ext, ".jpg") == 0 ||
                           rb->strcasecmp(ext, ".jpeg") == 0);
        int ret;
        if (jpg)
            ret = rb->read_jpeg_file(path, bm, sizeof art_data[slot], fmt, NULL);
        else
            ret = rb->read_bmp_file(path, bm, sizeof art_data[slot], fmt, NULL);
        if (ret >= 0 && bm->width > 0 && bm->height > 0)
            return true;
    }
    return false;
}

/* one row of a top-N list; if `cover` is set it replaces the rank badge */
static void list_row(const struct theme *th, int y, int rank, const char *name,
                     int value, int maxval, int barfrac, const struct bitmap *cover)
{
    if (cover) {
        int cwd = cover->width, chd = cover->height;
        int ix = 10 + (ART_PX - cwd) / 2, iy = y + 1 + (ART_PX - chd) / 2;
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_bitmap((const fb_data *)cover->data, ix, iy, cwd, chd);
    } else {
        int bx = 26, by = y + 12, br = 12;
        draw_disc(th, bx, by, br, th->accent, 256);
        char num[8]; rb->snprintf(num, sizeof num, "%d", rank);
        int w, h;
        rb->lcd_setfont(body_font);
        rb->lcd_set_drawmode(DRMODE_FG);
        rb->lcd_getstringsize(num, &w, &h);
        rb->lcd_set_foreground(th->bg1);
        rb->lcd_putsxy(bx - w/2, by - h/2, num);
    }

    int w, h;
    rb->lcd_setfont(body_font);
    rb->lcd_set_drawmode(DRMODE_FG);

    char vb[16]; commafmt(value, vb, sizeof vb);
    rb->lcd_getstringsize(vb, &w, &h);
    int valx = SCREEN_W - 14 - w;
    rb->lcd_set_foreground(th->accent);
    rb->lcd_putsxy(valx, y + 1, vb);

    char nb[48];
    fit_text(name, valx - 48 - 8, nb, sizeof nb);
    rb->lcd_set_foreground(COL_TEXT_LIGHT);
    rb->lcd_putsxy(48, y + 1, nb);

    int barx = 48, bary = y + 21, barw_full = valx - 8 - barx;
    int bw = maxval ? (int)((long)barw_full * value / maxval) : 0;
    bw = bw * barfrac / 256;
    if (bw >= 4)
        draw_capsule(th, barx + 2, bary, barx + bw - 2, bary, 2,
                     blend565(grad_at(th, bary), th->accent, 210));
}

static int draw_top_list(const struct theme *th, struct htable *t,
                         const char *kick, int mode, int idx, int dir,
                         bool with_art, const char *empty)
{
    fill_gradient(th);
    kicker(22, kick, th->accent);
    accent_underline(th, 43);

    struct agg *list[5];
    int n = top_n(t, list, 5, mode);
    if (n == 0) {
        center_text(112, empty, COL_TEXT_LIGHT);
        page_dots(idx, th);
        present_card(dir);
        return 0;
    }
    int maxv = agg_metric(list[0], mode);
    if (maxv < 1) maxv = 1;

    int y0 = 58, rh = 35;
    page_dots(idx, th);
    present_card(dir);                 /* reveal the full card, then grow rows */

    /* look up a cover thumbnail for each entry (album card only) */
    const struct bitmap *covers[5] = { 0 };
    if (with_art && album_dir) {
        for (int i = 0; i < n; i++) {
            int ai = (int)(list[i] - t->items);
            if (ai >= 0 && ai < album_dir_cap && load_cover(album_dir[ai], i))
                covers[i] = &art_bm[i];
        }
    }

    int base = 8, F = (n - 1) * 2 + base;
    for (int fr = export_mode ? F : 0; fr <= F; fr++) {
        repaint_band(th, y0 - 2, n * rh + 4);
        for (int i = 0; i < n; i++) {
            int lp = fr - i * 2;
            if (lp <= 0) continue;
            int frac = lp >= base ? 256 : lp * 256 / base;
            int v = agg_metric(list[i], mode);
            list_row(th, y0 + i * rh, i + 1, list[i]->name, v, maxv, frac, covers[i]);
        }
        rb->lcd_update_rect(0, y0 - 2, SCREEN_W, n * rh + 4);
        int btn = rb->button_get_w_tmo(HZ / 60);
        if (nav_button(btn)) {
            repaint_band(th, y0 - 2, n * rh + 4);
            for (int i = 0; i < n; i++) {
                int v = agg_metric(list[i], mode);
                list_row(th, y0 + i * rh, i + 1, list[i]->name, v, maxv, 256, covers[i]);
            }
            rb->lcd_update_rect(0, y0 - 2, SCREEN_W, n * rh + 4);
            return btn;
        }
    }
    return 0;
}

/* longest run of consecutive listening days (shown on the year card) */
static int longest_streak(void)
{
    int best = day_n ? 1 : 0;
    for (int i = 0; i < day_n; i++) {
        int len = 1; long cur = days[i].day;
        for (;;) {
            bool found = false;
            for (int j = 0; j < day_n; j++)
                if (days[j].day == cur + 1) { found = true; break; }
            if (!found) break;
            cur++; len++;
        }
        if (len > best) best = len;
    }
    return best;
}

/* elapsed days of display_year (0 if it hasn't started, full length if over).
 * Rockbox's tm_yday comes from a month*30 approximation that runs up to two
 * days fast, so derive the day-of-year exactly from the civil date instead. */
static int year_elapsed_days(void)
{
    struct tm *now = rb->get_time();
    int nowy = now ? now->tm_year + 1900 : 0;
    long ylen = days_from_civil(display_year + 1, 1, 1)
              - days_from_civil(display_year, 1, 1);
    if (nowy > display_year) return (int)ylen;
    if (nowy < display_year) return 0;
    return (int)(days_from_civil(nowy, now->tm_mon + 1, now->tm_mday)
                 - days_from_civil(nowy, 1, 1)) + 1;
}

static int year_cur_week(void)
{
    if (!display_year)          /* no year: sentinel day0 would overflow */
        return -1;
    int doy = year_elapsed_days();
    if (doy <= 0) return -1;
    /* offset of Jan 1 within its Mon-aligned week (0 if the year starts
     * on a Monday, up to 6) shifts every day-of-year to a bar index */
    int off = (int)(days_from_civil(display_year, 1, 1) - display_day0);
    int w = (off + doy - 1) / 7;
    if (w < 0) return -1;
    return w >= n_weeks ? n_weeks - 1 : w;
}

/* the week bar: gold pill = superweek, green = active, dim = quiet/future;
 * cursor >= 0 draws a white selection frame around that week */
static void draw_week_bar(const struct theme *th, int cursor)
{
    int cur_week = year_cur_week();
    repaint_band(th, WB_Y - 6, WB_H + 12);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    for (int i = 0; i < n_weeks; i++) {
        int x = WB_X + i * WB_PITCH;
        if (week_secs[i] >= ULTRA_WEEK_SECS) {
            /* diamond pill with a sparkle: the device never left the hands */
            int cx = x + WB_SEGW/2;
            draw_capsule(th, cx, WB_Y - 3, cx, WB_Y + WB_H + 2, 2, COL_ICE);
            draw_capsule(th, cx, WB_Y, cx, WB_Y + WB_H - 1, 1, COL_ICE_HI);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));
            rb->lcd_drawpixel(cx + 2, WB_Y - 5);
            rb->lcd_drawpixel(cx + 1, WB_Y - 6);
            rb->lcd_drawpixel(cx + 3, WB_Y - 6);
            rb->lcd_drawpixel(cx + 2, WB_Y - 7);
        } else if (week_secs[i] >= SUPER_WEEK_SECS) {
            int cx = x + WB_SEGW/2;
            draw_capsule(th, cx, WB_Y - 2, cx, WB_Y + WB_H + 1, 2, COL_GOLD);
            draw_capsule(th, cx, WB_Y + 1, cx, WB_Y + WB_H - 2, 1, COL_GOLD_HI);
            rb->lcd_set_drawmode(DRMODE_SOLID);
        } else {
            unsigned c;
            if (week_secs[i] >= WEEK_ACTIVE_SECS)
                c = th->accent;
            else if (cur_week >= 0 && i <= cur_week)
                c = blend565(grad_at(th, WB_Y), LCD_RGBPACK(255,255,255), 60);
            else
                c = blend565(grad_at(th, WB_Y), LCD_RGBPACK(255,255,255), 22);
            rb->lcd_set_foreground(c);
            rb->lcd_fillrect(x, WB_Y, WB_SEGW, WB_H);
        }
        if (i == cursor) {
            rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));
            rb->lcd_drawrect(x - 2, WB_Y - 5, WB_SEGW + 4, WB_H + 10);
        }
    }
}

/* "Jun 22 - Jun 28" for a week of display_year (partial first/last weeks
 * clamp to Jan 1 / Dec 31 - the data holds only this year's days anyway) */
static void week_range(int wi, char *out, size_t sz)
{
    int y1, m1, d1, y2, m2, d2;
    long s = display_day0 + (long)wi * 7;
    long e = s + 6;
    long jan1 = days_from_civil(display_year, 1, 1);
    long last = days_from_civil(display_year + 1, 1, 1) - 1;
    if (s < jan1) s = jan1;
    if (e > last) e = last;
    civil_from_days(s, &y1, &m1, &d1);
    civil_from_days(e, &y2, &m2, &d2);
    rb->snprintf(out, sz, "%s %d - %s %d", month_abbr[m1], d1, month_abbr[m2], d2);
}

/* ---- per-week drill-down: re-scan the log for one week's recap ---- */
struct week_info {
    long plays, skips, mins;
    char top_artist[NAME_MAX_LEN], top_track[NAME_MAX_LEN];
    int  top_artist_n, top_track_n;
};

#define WK_SLOTS 48
struct wk_named { char name[NAME_MAX_LEN]; int c; };
static struct wk_named wk_art[WK_SLOTS], wk_trk[WK_SLOTS];
static int wk_art_n, wk_trk_n;

static void wk_add(struct wk_named *arr, int *n, const char *name)
{
    for (int i = 0; i < *n; i++)
        if (rb->strcmp(arr[i].name, name) == 0) { arr[i].c++; return; }
    if (*n < WK_SLOTS) {
        rb->strlcpy(arr[*n].name, name, NAME_MAX_LEN);
        arr[*n].c = 1;
        (*n)++;
    }
}

static void week_scan(int wi, struct week_info *w)
{
    rb->memset(w, 0, sizeof(*w));
    wk_art_n = wk_trk_n = 0;
    long d_lo = display_day0 + (long)wi * 7, d_hi = d_lo + 7;
    unsigned long secs = 0;

    bool scrob = src_scrob;
    if (scrob ? !lrd_open(LOG_PATH) : !lrd_open_playback())
        return;

    char line[640];
    char *f[8];
    while (lrd_line(line, sizeof(line))) {
        if (line[0] == '#' || line[0] == '\0')
            continue;
        unsigned long ts;
        unsigned psecs;
        bool listened;
        char artist[NAME_MAX_LEN], title[NAME_MAX_LEN];

        if (scrob) {
            if (split_tabs(line, f, 8) < 7) continue;
            ts = parse_ul(f[6]);
            listened = (f[5][0] == 'L' || f[5][0] == 'l');
            psecs = (unsigned)parse_ul(f[4]);
            rb->strlcpy(artist, f[0][0] ? f[0] : "(unknown)", NAME_MAX_LEN);
            rb->strlcpy(title,  f[2][0] ? f[2] : "(unknown)", NAME_MAX_LEN);
        } else {
            char *p = line;
            ts = parse_ul(p);
            if (!(p = next_colon(p))) continue;
            unsigned long el_ms = parse_ul(p);
            if (!(p = next_colon(p))) continue;
            unsigned long len_ms = parse_ul(p);
            if (!(p = next_colon(p))) continue;
            if (p[0] == '\0') continue;
            char walbum[NAME_MAX_LEN];
            resolve_meta(p, artist, title, walbum);
            listened = (len_ms == 0) || (el_ms * 2 >= len_ms) || (el_ms >= 240000UL);
            psecs = (unsigned)(el_ms / 1000);
            if (!listened && el_ms < 5000)
                continue;   /* browsing tap, same rule as playback_parse */
        }

        if (ts < MIN_VALID_TS) continue;
        /* same year scope as the main parse, or the partial first/last week
         * of the bar would count plays the week bar itself excluded */
        if (filter_year && year_of_ts(ts) != filter_year) continue;
        long d = (long)(ts / 86400UL);
        if (d < d_lo || d >= d_hi) continue;

        if (listened) {
            w->plays++;
            secs += psecs;
            wk_add(wk_art, &wk_art_n, artist);
            wk_add(wk_trk, &wk_trk_n, title);
        } else {
            w->skips++;
        }
    }
    lrd_close();
    w->mins = secs / 60;

    int bi = -1;
    for (int i = 0; i < wk_art_n; i++)
        if (bi < 0 || wk_art[i].c > wk_art[bi].c) bi = i;
    if (bi >= 0) {
        rb->strlcpy(w->top_artist, wk_art[bi].name, NAME_MAX_LEN);
        w->top_artist_n = wk_art[bi].c;
    }
    bi = -1;
    for (int i = 0; i < wk_trk_n; i++)
        if (bi < 0 || wk_trk[i].c > wk_trk[bi].c) bi = i;
    if (bi >= 0) {
        rb->strlcpy(w->top_track, wk_trk[bi].name, NAME_MAX_LEN);
        w->top_track_n = wk_trk[bi].c;
    }
}

/* full-screen recap for one week (Spotify listening-stats style) */
static int draw_week_card(int wi, int dir)
{
    const struct theme *th = &themes[CARD_YEAR];
    struct week_info w;
    week_scan(wi, &w);
    bool super = week_secs[wi] >= SUPER_WEEK_SECS;
    bool ultra = week_secs[wi] >= ULTRA_WEEK_SECS;
    char buf2[80], rng[40];

    fill_gradient(th);
    rb->snprintf(buf2, sizeof buf2, "WEEK %d", wi + 1);
    kicker(22, buf2, ultra ? COL_ICE : super ? COL_GOLD : th->accent);
    accent_underline(th, 43);
    week_range(wi, rng, sizeof rng);
    center_text(50, rng, COL_TEXT_DIM);

    if (w.plays == 0) {
        center_text(112, "nothing logged this week", COL_TEXT_LIGHT);
        center_text(214, "spin for other weeks - MENU: back", COL_TEXT_DIM);
        present_card(dir);
        return 0;
    }

    int cw = 30, ch = 46, sw = 8;
    hero_band_y = 70; hero_band_h = ch + sw + 6;
    center_text(132, "minutes this week", th->accent);

    rb->snprintf(buf2, sizeof buf2, "top artist: %s", w.top_artist);
    { char fb[64]; fit_text(buf2, SCREEN_W - 24, fb, sizeof fb);
      center_text(154, fb, COL_TEXT_LIGHT); }
    rb->snprintf(buf2, sizeof buf2, "top song: %s", w.top_track);
    { char fb[64]; fit_text(buf2, SCREEN_W - 24, fb, sizeof fb);
      center_text(174, fb, COL_TEXT_LIGHT); }
    rb->snprintf(buf2, sizeof buf2, "%ld plays  -  %ld skips", w.plays, w.skips);
    center_text(194, buf2, COL_TEXT_DIM);

    if (ultra)
        center_text(216, "ULTRAWEEK - it never left your hands", COL_ICE);
    else if (super)
        center_text(216, "SUPERWEEK - a full day of music", COL_GOLD);
    else
        center_text(216, "spin for other weeks - MENU: back", COL_TEXT_DIM);

    present_card(dir);
    return animate_count(th, SCREEN_W/2, hero_band_y, cw, ch, sw, w.mins);
}

static int draw_card(int idx, int dir)
{
    const struct theme *th = &themes[idx];
    char buf[40], buf2[64];
    int ret = 0;

    /* default hero geometry */
    int cw = 34, ch = 56, sw = 10;
    hero_band_y = 74; hero_band_h = ch + sw + 6;

    switch (idx) {
    case CARD_INTRO: {
        fill_gradient(th);
        draw_disc(th, 40, 50, 46, LCD_RGBPACK(255,255,255), 26);
        draw_disc(th, 290, 210, 60, LCD_RGBPACK(255,255,255), 22);
        kicker(34, "MY YEAR IN MUSIC", th->accent);
        if (ts_max) {
            int y, m, d;
            civil_from_days(ts_max / 86400, &y, &m, &d);
            rb->snprintf(buf, sizeof buf, "%d", y);
            draw_number(th, SCREEN_W/2, 78, buf, 38, 64, 11);
        }
        if (cheat_rigged())
            center_text2(154, "Your Year, ", COL_TEXT_LIGHT,
                              "Rigged", COL_CHEAT);
        else
            center_text(154, "Your Year, Spun", COL_TEXT_LIGHT);
        if (ts_min) {
            int y1,m1,d1,y2,m2,d2;
            civil_from_days(ts_min / 86400, &y1, &m1, &d1);
            civil_from_days(ts_max / 86400, &y2, &m2, &d2);
            rb->snprintf(buf2, sizeof buf2, "%s %d  -  %s %d %d",
                         month_abbr[m1], d1, month_abbr[m2], d2, y2);
            center_text(178, buf2, COL_TEXT_DIM);
        }
        center_text(208, "spin to begin  >", th->accent);
        break;
    }
    case CARD_MINUTES: {
        fill_gradient(th);
        kicker(22, "YOU LISTENED FOR", th->accent);
        accent_underline(th, 43);
        long mins = total_seconds / 60;
        center_text(150, "minutes", th->accent);
        long hrs = mins / 60;
        rb->snprintf(buf2, sizeof buf2, "about %ld hours with your music", hrs);
        center_text(180, buf2, COL_TEXT_DIM);
        page_dots(idx, th);
        present_card(dir);
        return animate_count(th, SCREEN_W/2, hero_band_y, cw, ch, sw, mins);
    }
    case CARD_PLAYS: { /* plays / unique */
        fill_gradient(th);
        kicker(22, "TRACKS PLAYED", th->accent);
        accent_underline(th, 43);
        rb->snprintf(buf2, sizeof buf2, "%d different songs", unique_tracks);
        center_text(150, buf2, th->accent);
        center_text(180, "every one of them, a choice", COL_TEXT_DIM);
        page_dots(idx, th);
        present_card(dir);
        return animate_count(th, SCREEN_W/2, hero_band_y, cw, ch, sw, total_plays);
    }
    case CARD_ARTISTS:
        return draw_top_list(th, &t_artist, "TOP ARTISTS", TOP_PLAYS, idx, dir,
                             false, "No plays logged yet");
    case CARD_SONGS:
        return draw_top_list(th, &t_track, "TOP SONGS", TOP_PLAYS, idx, dir,
                             false, "No plays logged yet");
    case CARD_ALBUMS:
        return draw_top_list(th, &t_album, "TOP ALBUMS", TOP_PLAYS, idx, dir,
                             true, "No plays logged yet");
    case CARD_LOYAL:
        return draw_top_list(th, &t_track, "NEVER SKIPPED", TOP_LOYAL, idx, dir,
                             false, "You skip everything?!");
    case CARD_REDIS:
        return draw_top_list(th, &t_track, "REMEMBER THESE?", TOP_REDIS, idx, dir,
                             false, "No forgotten favourites yet");
    case CARD_NIGHT: { /* the 3am club */
        fill_gradient(th);
        kicker(22, "THE 3AM CLUB", th->accent);
        accent_underline(th, 43);
        long night_plays = 0;
        for (int i = 0; i < 5; i++) night_plays += hour_hist[i];
        if (night_plays == 0) {
            center_text(112, "Your nights are quiet. Respect.", COL_TEXT_LIGHT);
            break;
        }
        center_text(150, "plays between midnight and 5am", th->accent);
        struct agg *na[1];
        if (top_n(&t_artist, na, 1, TOP_NIGHT) == 1) {
            char fb[64];
            rb->snprintf(buf2, sizeof buf2, "usually %s", na[0]->name);
            fit_text(buf2, SCREEN_W - 24, fb, sizeof fb);
            center_text(178, fb, COL_TEXT_LIGHT);
        }
        if (top_n(&t_track, na, 1, TOP_NIGHT) == 1) {
            char fb[64];
            rb->snprintf(buf2, sizeof buf2, "on repeat: %s", na[0]->name);
            fit_text(buf2, SCREEN_W - 24, fb, sizeof fb);
            center_text(198, fb, COL_TEXT_DIM);
        }
        page_dots(idx, th);
        present_card(dir);
        return animate_count(th, SCREEN_W/2, hero_band_y, cw, ch, sw, night_plays);
    }
    case CARD_CLOCK: { /* listening clock */
        fill_gradient(th);
        kicker(22, "YOUR LISTENING CLOCK", th->accent);
        accent_underline(th, 43);
        int peak = 0;
        for (int i = 1; i < 24; i++) if (hour_hist[i] > hour_hist[peak]) peak = i;
        int maxv = 1;
        for (int i = 0; i < 24; i++) if (hour_hist[i] > maxv) maxv = hour_hist[i];
        int bx = 16, bw = (SCREEN_W - 32) / 24, base = 178, maxh = 100;
        rb->snprintf(buf2, sizeof buf2, "you peak around %02d:00", peak);
        center_text(208, buf2, th->accent);
        page_dots(idx, th);
        present_card(dir);
        int F = 14;
        for (int fr = export_mode ? F : 0; fr <= F; fr++) {
            int frac = fr * 256 / F;
            repaint_band(th, base - maxh - 2, maxh + 22);
            for (int i = 0; i < 24; i++) {
                int hh = (hour_hist[i] * maxh) / maxv;
                hh = hh * frac / 256;
                unsigned c = (i == peak) ? th->num1
                                         : blend565(grad_at(th, base), LCD_RGBPACK(255,255,255), 90);
                if (hh > 0) {
                    rb->lcd_set_drawmode(DRMODE_SOLID);
                    rb->lcd_set_foreground(c);
                    rb->lcd_fillrect(bx + i*bw, base - hh, bw - 1, hh);
                }
            }
            rb->lcd_set_drawmode(DRMODE_FG);
            rb->lcd_set_foreground(COL_TEXT_DIM);
            rb->lcd_putsxy(bx, base + 4, "0");
            rb->lcd_putsxy(bx + 12*bw - 4, base + 4, "12");
            rb->lcd_putsxy(bx + 23*bw - 8, base + 4, "23");
            rb->lcd_update_rect(0, base - maxh - 2, SCREEN_W, maxh + 22);
            int btn = rb->button_get_w_tmo(HZ / 60);
            if (nav_button(btn)) { ret = btn; break; }
        }
        return ret;
    }
    case CARD_SKIPS: {
        fill_gradient(th);
        kicker(22, "THE SKIP REPORT", th->accent);
        accent_underline(th, 43);
        long total = total_plays + total_skips;
        int pct = total ? (int)((total_skips * 100) / total) : 0;
        cw = 32; ch = 54; sw = 9;
        int pctsz = 34;
        rb->snprintf(buf, sizeof buf, "%d", pct);
        int numw = number_width(buf, cw, sw);
        int numcx = SCREEN_W/2 - (pctsz + 8)/2;
        int oy = 70;
        center_text(146, "of plays were skips", th->accent);
        struct agg *slist[1];
        if (top_n(&t_track, slist, 1, TOP_SKIPS) == 1) {
            char nb[48];
            center_text(176, "most skipped", COL_TEXT_DIM);
            fit_text(slist[0]->name, SCREEN_W - 24, nb, sizeof nb);
            center_text(196, nb, COL_TEXT_LIGHT);
        }
        page_dots(idx, th);
        present_card(dir);
        int frames = 14;
        for (int fr = export_mode ? frames : 0; fr <= frames; fr++) {
            int val = (int)ease_val(pct, fr, frames);
            rb->snprintf(buf, sizeof buf, "%d", val);
            repaint_band(th, oy - 2, ch + sw + 6);
            draw_number(th, numcx, oy, buf, cw, ch, sw);
            draw_percent(th, numcx + numw/2 + 8 + pctsz/2, oy + ch - pctsz/2, pctsz);
            rb->lcd_update_rect(0, oy - 2, SCREEN_W, ch + sw + 6);
            int btn = rb->button_get_w_tmo(HZ / 50);
            if (nav_button(btn)) {
                rb->snprintf(buf, sizeof buf, "%d", pct);
                repaint_band(th, oy - 2, ch + sw + 6);
                draw_number(th, numcx, oy, buf, cw, ch, sw);
                draw_percent(th, numcx + numw/2 + 8 + pctsz/2, oy + ch - pctsz/2, pctsz);
                rb->lcd_update_rect(0, oy - 2, SCREEN_W, ch + sw + 6);
                ret = btn; break;
            }
        }
        return ret;
    }
    case CARD_YEAR: { /* year progress: Mon-aligned week bar + streak */
        fill_gradient(th);
        if (cheat_rigged())
            kicker(22, "YOUR YEAR, ALLEGEDLY", COL_CHEAT);
        else
            kicker(22, "YOUR YEAR SO FAR", th->accent);
        accent_underline(th, 43);

        if (!display_year) {   /* all-time view with an unset clock */
            center_text(104, "no clock, no calendar", COL_TEXT_LIGHT);
            center_text(126, "set the time & date to unlock this card",
                        COL_TEXT_DIM);
            break;
        }

        /* how far through display_year we are (100% for a past year) */
        long ylen = days_from_civil(display_year + 1, 1, 1)
                  - days_from_civil(display_year, 1, 1);
        int doy = year_elapsed_days();
        int pct = (int)((long)doy * 100 / ylen);

        int active = 0;
        for (int i = 0; i < n_weeks; i++)
            if (week_secs[i] >= WEEK_ACTIVE_SECS) active++;

        rb->snprintf(buf2, sizeof buf2, "of %d behind you", display_year);
        center_text(140, buf2, th->accent);

        draw_week_bar(th, -1);

        rb->snprintf(buf2, sizeof buf2, "%d active weeks - press SELECT", active);
        center_text(184, buf2, COL_TEXT_LIGHT);
        rb->snprintf(buf2, sizeof buf2, "longest daily streak: %d days",
                     longest_streak());
        center_text(202, buf2, COL_TEXT_DIM);

        /* animated hero percentage, same treatment as the skip card */
        cw = 32; ch = 54; sw = 9;
        int pctsz = 34;
        rb->snprintf(buf, sizeof buf, "%d", pct);
        int numw = number_width(buf, cw, sw);
        int numcx = SCREEN_W/2 - (pctsz + 8)/2;
        int oy = 64;
        page_dots(idx, th);
        present_card(dir);
        int frames = 14;
        for (int fr = export_mode ? frames : 0; fr <= frames; fr++) {
            int val = (int)ease_val(pct, fr, frames);
            rb->snprintf(buf, sizeof buf, "%d", val);
            repaint_band(th, oy - 2, ch + sw + 6);
            draw_number(th, numcx, oy, buf, cw, ch, sw);
            draw_percent(th, numcx + numw/2 + 8 + pctsz/2, oy + ch - pctsz/2, pctsz);
            rb->lcd_update_rect(0, oy - 2, SCREEN_W, ch + sw + 6);
            int btn = rb->button_get_w_tmo(HZ / 50);
            if (nav_button(btn)) {
                rb->snprintf(buf, sizeof buf, "%d", pct);
                repaint_band(th, oy - 2, ch + sw + 6);
                draw_number(th, numcx, oy, buf, cw, ch, sw);
                draw_percent(th, numcx + numw/2 + 8 + pctsz/2, oy + ch - pctsz/2, pctsz);
                rb->lcd_update_rect(0, oy - 2, SCREEN_W, ch + sw + 6);
                ret = btn; break;
            }
        }
        return ret;
    }
    case CARD_HEAT: { /* every day of the year, one cell each */
        fill_gradient(th);
        kicker(22, "EVERY DAY OF IT", th->accent);
        accent_underline(th, 43);

        if (!display_year) {
            center_text(104, "no clock, no calendar", COL_TEXT_LIGHT);
            center_text(126, "set the time & date to unlock this card",
                        COL_TEXT_DIM);
            break;
        }

        /* minutes per calendar day, laid out GitHub-style:
         * columns = Mon-aligned weeks, rows = Mon..Sun */
        static unsigned short grid[N_WEEKS][7];
        rb->memset(grid, 0, sizeof(grid));
        int active = 0;
        unsigned best = 0;
        long best_day = 0;
        for (int i = 0; i < day_n; i++) {
            long rel = days[i].day - display_day0;
            if (rel < 0 || rel >= 7L * n_weeks)
                continue;
            unsigned m = days[i].secs / 60;
            if (m > 0xFFFF) m = 0xFFFF;
            grid[rel / 7][rel % 7] = (unsigned short)m;
            active++;
            if (m > best) { best = m; best_day = days[i].day; }
        }

        const int pitch = 5, cell = 4;
        int gx0 = (SCREEN_W - n_weeks * pitch + (pitch - cell)) / 2;
        int gy0 = 78;
        rb->lcd_set_drawmode(DRMODE_SOLID);
        for (int w = 0; w < n_weeks; w++) {
            for (int d = 0; d < 7; d++) {
                int y = gy0 + d * pitch;
                unsigned m = grid[w][d];
                /* GitHub ramp: empty, then four intensities of accent */
                int t = (m == 0) ? 0
                      : (m <  15) ? 70
                      : (m <  60) ? 130
                      : (m < 180) ? 190 : 256;
                unsigned c = (t == 0)
                    ? blend565(grad_at(th, y), LCD_RGBPACK(255,255,255), 22)
                    : blend565(grad_at(th, y), th->accent, t);
                rb->lcd_set_foreground(c);
                rb->lcd_fillrect(gx0 + w * pitch, y, cell, cell);
            }
        }

        rb->snprintf(buf2, sizeof buf2, "%d of %d days had music",
                     active, year_elapsed_days());
        center_text(130, buf2, th->accent);
        if (best) {
            int by, bm, bd;
            civil_from_days(best_day, &by, &bm, &bd);
            rb->snprintf(buf2, sizeof buf2,
                         "brightest square: %uh %02um on %s %d",
                         best / 60, best % 60, month_abbr[bm], bd);
            center_text(158, buf2, COL_TEXT_LIGHT);
        }
        center_text(186, "one square, one day - go fill the grid",
                    COL_TEXT_DIM);
        break;
    }
    case CARD_TYPE: { /* listening personality: 4 axes -> 1 of 16 types */
        fill_gradient(th);
        kicker(22, "YOUR LISTENING TYPE", th->accent);
        accent_underline(th, 43);

        /* axis values, 0..100 */
        struct agg *a1[1];
        long total = total_plays + total_skips;
        long night = 0;
        for (int i = 0; i < 5; i++) night += hour_hist[i];
        int val[4];
        val[0] = (total_plays && top_n(&t_artist, a1, 1, TOP_PLAYS) == 1)
                 ? (int)((long)a1[0]->count * 100 / total_plays) : 0;
        val[1] = total_plays
                 ? (int)((total_plays - unique_tracks) * 100 / total_plays) : 0;
        val[2] = total ? (int)(total_plays * 100 / total) : 100;
        val[3] = total_plays ? (int)(night * 100 / total_plays) : 0;

        static const struct {
            const char *lo, *hi;   /* pole labels, low / high side       */
            char        lc, hc;    /* pole letters for the 4-letter code */
            int         mid;       /* value >= mid tips to the high pole */
        } axis[4] = {
            { "roamer",   "devotee",   'R', 'D', 45 },
            { "explorer", "repeater",  'E', 'F', 55 },
            { "sampler",  "committed", 'S', 'C', 78 },
            { "daylight", "night owl", 'L', 'N',  8 },
        };
        /* indexed by (devotee<<3)|(repeater<<2)|(committed<<1)|nightowl */
        static const char *type_name[16] = {
            "The Free Spirit",       "The Night Scout",
            "The Adventurer",        "The Midnight Wanderer",
            "The Channel Hopper",    "The Midnight Zapper",
            "The Comfort Curator",   "The Night Ritualist",
            "The Catalog Sifter",    "The Night Miner",
            "The Completionist",     "The Deep Diver",
            "The Picky Superfan",    "The Restless Fan",
            "The True Believer",     "The Midnight Devotee",
        };

        int tt = 0, pos[4];
        char code[8];
        for (int i = 0; i < 4; i++) {
            bool hi = val[i] >= axis[i].mid;
            if (hi) tt |= 8 >> i;
            code[i*2]   = hi ? axis[i].hc : axis[i].lc;
            code[i*2+1] = (i < 3) ? ' ' : '\0';
            /* piecewise map so each axis' tipping point sits mid-track */
            pos[i] = (val[i] <= axis[i].mid)
                   ? val[i] * 50 / axis[i].mid
                   : 50 + (val[i] - axis[i].mid) * 50 / (100 - axis[i].mid);
        }

        const int TX0 = 104, TX1 = 216, ROWY = 62, ROWH = 30;
        int F = 14;
        for (int fr = export_mode ? F : 0; fr <= F; fr++) {
            repaint_band(th, ROWY - 10, 4 * ROWH);
            for (int i = 0; i < 4; i++) {
                int yr = ROWY + i * ROWH;
                bool hi = val[i] >= axis[i].mid;
                unsigned bg = grad_at(th, yr);
                draw_capsule(th, TX0, yr, TX1, yr, 3,
                             blend565(bg, LCD_RGBPACK(255,255,255), 55));
                draw_capsule(th, (TX0+TX1)/2, yr - 5, (TX0+TX1)/2, yr + 5, 1,
                             blend565(bg, LCD_RGBPACK(255,255,255), 95));
                int cur = 50 + (int)ease_val(pos[i] - 50, fr, F);
                int mx = TX0 + (TX1 - TX0) * cur / 100;
                draw_disc(th, mx, yr, 5, th->num1, 256);
                rb->lcd_setfont(body_font);
                rb->lcd_set_drawmode(DRMODE_FG);
                int lw, lh;
                rb->lcd_set_foreground(hi ? COL_TEXT_DIM : COL_TEXT_LIGHT);
                rb->lcd_putsxy(14, yr - 6, axis[i].lo);
                rb->lcd_set_foreground(hi ? COL_TEXT_LIGHT : COL_TEXT_DIM);
                rb->lcd_getstringsize(axis[i].hi, &lw, &lh);
                rb->lcd_putsxy(SCREEN_W - 14 - lw, yr - 6, axis[i].hi);
            }
            if (fr == 0 || (export_mode && fr == F)) {
                /* markers start centred; in export the one drawn frame is
                 * already final and just needs the full-card push */
                page_dots(idx, th);
                present_card(dir);
                if (!export_mode)
                    continue;
            }
            rb->lcd_update_rect(0, ROWY - 10, SCREEN_W, 4 * ROWH);
            int btn = rb->button_get_w_tmo(HZ / 50);
            if (nav_button(btn)) { ret = btn; break; }
        }

        /* the reveal (a rigged log gets typed with perfect accuracy) */
        kicker(184, code, th->accent);
        if (cheat_rigged())
            center_text(202, "The Fraud", COL_CHEAT);
        else
            center_text(202, type_name[tt], COL_TEXT_LIGHT);
        rb->lcd_update_rect(0, 180, SCREEN_W, 44);
        return ret;
    }
#ifdef WRAPPED_WITH_ACH
    case CARD_ACH:
        return ach_wrapped_card(idx, dir);
#endif
    case CARD_OUTRO: {
        fill_gradient(th);
        draw_disc(th, 285, 40, 50, LCD_RGBPACK(255,255,255), 24);
        draw_disc(th, 35, 205, 56, LCD_RGBPACK(255,255,255), 20);
        kicker(40, "THAT'S A WRAP", th->accent);
        accent_underline(th, 60);
        if (!export_mode)
            center_text(64, "SELECT: save the whole deck", COL_TEXT_DIM);
        long mins = total_seconds / 60;
        struct agg *a[1];
        int y = 88;
        rb->snprintf(buf2, sizeof buf2, "%ld minutes listened", mins);
        center_text(y, buf2, COL_TEXT_LIGHT); y += 24;
        rb->snprintf(buf2, sizeof buf2, "%ld plays  -  %d songs", total_plays, unique_tracks);
        center_text(y, buf2, COL_TEXT_LIGHT); y += 24;
        if (top_n(&t_artist, a, 1, TOP_PLAYS) == 1) {
            char fb[64];
            rb->snprintf(buf2, sizeof buf2, "#1 artist: %s", a[0]->name);
            fit_text(buf2, SCREEN_W - 24, fb, sizeof fb);
            center_text(y, fb, th->accent);
        }

        /* milestone tracker: progress toward the next minutes badge */
        static const long stones[] =
            { 1000, 2500, 5000, 10000, 25000, 50000, 100000 };
        long next = 0;
        for (unsigned i = 0; i < sizeof(stones)/sizeof(stones[0]); i++)
            if (mins < stones[i]) { next = stones[i]; break; }
        if (next) {
            char nb[16];
            commafmt(next, nb, sizeof nb);
            rb->snprintf(buf2, sizeof buf2, "next badge: %s minutes", nb);
            center_text(162, buf2, COL_TEXT_DIM);
            int bx0 = 70, bx1 = SCREEN_W - 70, bary = 186;
            draw_capsule(th, bx0, bary, bx1, bary, 3,
                         blend565(grad_at(th, bary), LCD_RGBPACK(255,255,255), 55));
            int fill = (int)((long)(bx1 - bx0) * mins / next);
            if (fill > 3)
                draw_capsule(th, bx0, bary, bx0 + fill, bary, 3, th->accent);
        } else {
            center_text(174, "100,000+ minutes. you broke the scale.", COL_GOLD);
        }
        if (cheat_rigged())
            center_text(206, "play it straight next year", COL_CHEAT);
        else
            center_text(206, "see you next time", COL_TEXT_DIM);
        break;
    }
    default:
        fill_gradient(th);
        break;
    }

    page_dots(idx, th);
    present_card(dir);
    return ret;
}

/* interactive week browser on the year card: wheel moves the cursor over
 * the week bar, SELECT opens that week's recap (wheel there flips to
 * adjacent weeks), MENU backs out. Returns SYS_USB_CONNECTED or 0. */
static int week_browse(void)
{
    const struct theme *th = &themes[CARD_YEAR];
    int cursor = year_cur_week();
    if (cursor < 0) cursor = 0;
    int pending = 0;

    for (;;) {
        /* bar with cursor + live footer for the hovered week */
        draw_week_bar(th, cursor);
        repaint_band(th, 178, 46);
        char rng[40], buf2[64];
        week_range(cursor, rng, sizeof rng);
        rb->snprintf(buf2, sizeof buf2, "week %d: %s", cursor + 1, rng);
        center_text(184, buf2, COL_TEXT_LIGHT);
        long m = week_secs[cursor] / 60;
        if (week_secs[cursor] >= ULTRA_WEEK_SECS) {
            rb->snprintf(buf2, sizeof buf2, "%ld min - ULTRAWEEK ?!", m);
            center_text(202, buf2, COL_ICE);
        } else if (week_secs[cursor] >= SUPER_WEEK_SECS) {
            rb->snprintf(buf2, sizeof buf2, "%ld min - SUPERWEEK", m);
            center_text(202, buf2, COL_GOLD);
        } else {
            rb->snprintf(buf2, sizeof buf2, "%ld min", m);
            center_text(202, buf2, COL_TEXT_DIM);
        }
        rb->lcd_update_rect(0, WB_Y - 6, SCREEN_W, WB_H + 12);
        rb->lcd_update_rect(0, 178, SCREEN_W, 46);

        int btn = pending ? pending : rb->button_get(true);
        pending = 0;
        if (btn == SYS_USB_CONNECTED)
            return btn;
        int base = btn & ~BUTTON_REPEAT;
        if (base == BUTTON_SCROLL_FWD || base == BUTTON_RIGHT) {
            if (cursor < n_weeks - 1) cursor++;
        } else if (base == BUTTON_SCROLL_BACK || base == BUTTON_LEFT) {
            if (cursor > 0) cursor--;
        } else if (base == BUTTON_SELECT) {
            int dir = 0;
            for (;;) {
                int b = draw_week_card(cursor, dir);
                if (!b) b = rb->button_get(true);
                if (b == SYS_USB_CONNECTED)
                    return b;
                int bb = b & ~BUTTON_REPEAT;
                if ((bb == BUTTON_SCROLL_FWD || bb == BUTTON_RIGHT)
                        && cursor < n_weeks - 1) { cursor++; dir = +1; }
                else if ((bb == BUTTON_SCROLL_BACK || bb == BUTTON_LEFT)
                        && cursor > 0)           { cursor--; dir = -1; }
                else if (bb == BUTTON_PLAY)      { save_card(); dir = 0; }
                else if (bb == BUTTON_MENU || bb == BUTTON_SELECT)
                    break;
            }
            /* repaint the year card under the browser, keep browsing */
            pending = draw_card(CARD_YEAR, 0);
        } else if (base == BUTTON_PLAY) {
            save_card();
            pending = draw_card(CARD_YEAR, 0);
        } else if (base == BUTTON_MENU) {
            return 0;
        }
    }
}

/* nicer body font for the cards (once; sysfixed stays on failure) */
static void wrapped_font_init(void)
{
    static bool font_tried = false;
    if (!font_tried) {
        int loaded = rb->font_load("/.rockbox/fonts/15-Adobe-Helvetica.fnt");
        if (loaded >= 0)
            body_font = loaded;
        font_tried = true;
    }
}

/* capacities picked by wrapped_load_data, reused by wrapped_reload */
static int g_cap_trk, g_cap_art, g_cap_alb;

static bool wrapped_reload(void);

/* Load and aggregate the log into the tables/counters above. ask_year:
 * offer the year picker when the log spans several (Wrapped); pass false
 * for an unfiltered all-time crunch (achievements). Returns false when
 * there's nothing to show - the reason has already been splashed. */
static bool wrapped_load_data(bool ask_year)
{
    /* grab the plugin buffer for aggregation */
    abuf = rb->plugin_get_buffer(&abuf_sz);
    abuf_used = 0;

    /* the database map claims the bottom of the buffer and survives
     * every reload; the aggregate tables live above the floor */
    dbmap_init();
    abuf_floor = abuf_used;

    /* scale capacities to the memory that's left */
    int cap_trk = 4000, cap_art = 1500, cap_alb = 1500;
    day_cap = 1500;
    size_t need;
    for (;;) {
        need  = (size_t)cap_trk * sizeof(struct agg) + (size_t)next_pow2(cap_trk*2) * 4;
        need += (size_t)cap_art * sizeof(struct agg) + (size_t)next_pow2(cap_art*2) * 4;
        need += (size_t)cap_alb * sizeof(struct agg) + (size_t)next_pow2(cap_alb*2) * 4;
        need += (size_t)day_cap * sizeof(struct daycount);
        need += (size_t)cap_alb * ART_PATH_MAX;
        if (need <= abuf_sz - abuf_floor || cap_trk <= 200)
            break;
        cap_trk /= 2; cap_art /= 2; cap_alb /= 2; day_cap /= 2;
    }
    g_cap_trk = cap_trk;
    g_cap_art = cap_art;
    g_cap_alb = cap_alb;

    rb->splash(0, "Crunching your year...");

    /* pick the data source once, up front: a scrobbler log that actually
     * holds data wins; otherwise the core's playback.log */
    {
        int have_scrob = log_probe(LOG_PATH);
        int have_pbl   = log_probe_playback();
        src_scrob = (have_scrob == 1) || (have_scrob >= 0 && have_pbl < 0);
    }

    /* If the log spans more than one year, let the user scope Wrapped to one
     * (so 2026 stays viewable in 2027). One year -> all-time, no prompt.
     * (splash is already up: this pre-scan reads the whole log too) */
    filter_year = 0;
    if (ask_year) {
        int years[16];
        int ny = collect_years(years, 16);
        if (ny > 1) {
            static char ylabel[17][12];
            struct opt_items opts[17];
            rb->strlcpy(ylabel[0], "All-time", sizeof(ylabel[0]));
            opts[0].string = (unsigned const char *)ylabel[0];
            opts[0].voice_id = -1;
            for (int i = 0; i < ny; i++) {
                rb->snprintf(ylabel[i + 1], sizeof(ylabel[i + 1]), "%d", years[i]);
                opts[i + 1].string = (unsigned const char *)ylabel[i + 1];
                opts[i + 1].voice_id = -1;
            }
            int choice = 1;                 /* default to the newest year */
            rb->set_option("Your year in music", &choice, RB_INT, opts, ny + 1, NULL);
            filter_year = (choice <= 0) ? 0 : years[choice - 1];
        }
    }

    /* the year-progress card describes the filtered year, else the RTC's
     * current year; with neither (all-time + unset clock) it stays empty */
    {
        struct tm *now = rb->get_time();
        display_year = !ask_year ? 0        /* achievements: no year card */
                     : filter_year ? filter_year
                     : (now && now->tm_year + 1900 >= 2005) ? now->tm_year + 1900
                     : 0;
        if (display_year) {
            /* weeks are real Mon-Sun calendar weeks: start the bar on the
             * Monday on or before Jan 1 (unix day 4 = Mon 1970-01-05) */
            long jan1 = days_from_civil(display_year, 1, 1);
            display_day0 = jan1 - (((jan1 - 4) % 7) + 7) % 7;
            n_weeks = (int)((days_from_civil(display_year + 1, 1, 1)
                             - display_day0 + 6) / 7);
        } else {
            display_day0 = 0x7fffffffL;   /* matches no timestamp */
            n_weeks = 52;
        }
    }

    return wrapped_reload();
}

/* (Re-)crunch the log with the CURRENT filter_year: reset every aggregate,
 * rebuild the tables from the bump buffer and parse. Split out so the
 * achievements browser can swap to an all-time crunch and back without
 * re-running the year picker. */
static bool wrapped_reload(void)
{
    abuf_used = abuf_floor;      /* keep the dbmap, rebuild everything above */

    rb->memset(&t_artist, 0, sizeof(t_artist));
    rb->memset(&t_track,  0, sizeof(t_track));
    rb->memset(&t_album,  0, sizeof(t_album));
    rb->memset(hour_hist, 0, sizeof(hour_hist));
    rb->memset(week_secs, 0, sizeof(week_secs));
    total_plays = total_skips = total_seconds = 0;
    ts_min = ts_max = 0;
    day_n = 0;

    if (!htable_init(&t_track, g_cap_trk) ||
        !htable_init(&t_artist, g_cap_art) ||
        !htable_init(&t_album, g_cap_alb)) {
        rb->splash(HZ*2, "Spun: out of memory");
        return false;
    }
    days = aalloc((size_t)day_cap * sizeof(struct daycount));
    if (!days) { rb->splash(HZ*2, "Spun: out of memory"); return false; }

    /* per-album folder cache for cover-art lookup (optional; art off if it fails) */
    album_dir_cap = g_cap_alb;
    album_dir = aalloc((size_t)g_cap_alb * ART_PATH_MAX);
    if (!album_dir) album_dir_cap = 0;

    long n;
    if (src_scrob) {
        n = wrapped_parse();                  /* rich ID3-tagged scrobbles */
        cheat_judge_scrob();
    } else {
        /* get the core's buffered lines (and their sig update) on disk so
         * the bytes we hash and the sig we compare against line up */
        rb->add_playbacklog(NULL);
        struct pbl_sig sig_pre;
        bool sig_pre_ok = sig_read(&sig_pre);
        lrd_hash = PBL_HASH_BASIS;
        lrd_hash_n = 0;
        lrd_hash_on = true;
        n = playback_parse();                 /* the core's playback.log   */
        lrd_hash_on = false;
        cheat_judge(&sig_pre, sig_pre_ok);
    }

    if (n < 0) {
        rb->splash(HZ*3,
            "No log yet. Settings > Playback > Logging = On, reboot, play tracks.");
        return false;
    }
    if (n == 0) {
        rb->splash(HZ*3, "Log is empty - go listen to something!");
        return false;
    }

    unique_tracks = 0;
    for (int i = 0; i < t_track.n; i++)
        if (t_track.items[i].count > 0) unique_tracks++;

    /* rediscovery window: "not heard in a while", scaled to the log's span
     * (a third of it, clamped to 2..6 weeks) so the card works for a
     * month-old log as well as a full year */
    {
        unsigned long span = (ts_min && ts_max > ts_min)
                                 ? (unsigned long)(ts_max - ts_min) : 0;
        unsigned long gap = span / 3;
        if (gap < 14UL * 86400) gap = 14UL * 86400;
        if (gap > 45UL * 86400) gap = 45UL * 86400;
        redis_cutoff = ((unsigned long)ts_max > gap) ? ts_max - gap : 0;
    }
    return true;
}

/* returns PLUGIN_OK normally, PLUGIN_USB_CONNECTED if unplugged mid-view */
static int wrapped_show(void)
{
    wrapped_font_init();
    if (!wrapped_load_data(true))
        return PLUGIN_OK;

    /* card loop */
    int card = 0;
    rb->button_clear_queue();
    int btn = draw_card(card, 0);
    for (;;) {
        if (btn == 0)
            btn = rb->button_get(true);

        if (btn == SYS_USB_CONNECTED) {
            rb->lcd_setfont(FONT_SYSFIXED);
            return PLUGIN_USB_CONNECTED;
        }
        /* continuous wheel rotation posts repeat-flagged events */
        int base = btn & ~BUTTON_REPEAT;
        if (base == BUTTON_SCROLL_FWD || base == BUTTON_RIGHT) {
            if (card < N_CARDS - 1) { card++; btn = draw_card(card, +1); continue; }
#ifdef WRAPPED_WITH_ACH
        } else if (base == BUTTON_SELECT && card == CARD_ACH) {
            /* the achievements card is interactive: SELECT browses badges */
            if (ach_browse() == SYS_USB_CONNECTED) {
                rb->lcd_setfont(FONT_SYSFIXED);
                return PLUGIN_USB_CONNECTED;
            }
            btn = draw_card(card, 0);
            continue;
#endif
        } else if (base == BUTTON_SELECT && card == CARD_YEAR && display_year) {
            /* the year card is interactive: SELECT browses the week bar
             * (unless there's no year to browse - all-time with unset RTC -
             * where week math on the sentinel day0 would render garbage) */
            if (week_browse() == SYS_USB_CONNECTED) {
                rb->lcd_setfont(FONT_SYSFIXED);
                return PLUGIN_USB_CONNECTED;
            }
            btn = draw_card(card, 0);
            continue;
        } else if (base == BUTTON_SCROLL_BACK || base == BUTTON_LEFT) {
            if (card > 0) { card--; btn = draw_card(card, -1); continue; }
        } else if (base == BUTTON_PLAY) {
            save_card();
            btn = draw_card(card, 0);
            continue;
        } else if (base == BUTTON_SELECT && card == CARD_OUTRO) {
            /* the outro's parting trick: dump the finished deck */
            export_deck();
            btn = draw_card(card, 0);
            continue;
        } else if (base == BUTTON_MENU || base == BUTTON_SELECT) {
            break;
        }
        btn = 0;
    }
    rb->lcd_setfont(FONT_SYSFIXED);
    return PLUGIN_OK;
}

#endif /* WRAPPED_CORE_H */
