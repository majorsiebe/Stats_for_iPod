/* ============================================================
 *  Achievements: Cookie-Clicker-scale badges computed from the
 *  same playback log Wrapped reads.
 *
 *  Include from achievements.c AFTER wrapped_core.h (it reuses the
 *  parser, aggregates and rendering toolkit). Everything is
 *  recomputed from the log on every open, all-time, ignoring the
 *  year filter - so there is nothing to corrupt: swap logs around
 *  and the badges simply follow the data. The only persistent
 *  state is a tiny "already seen" bitmask used for NEW markers,
 *  and it self-heals against the current unlock set on load.
 *
 *  Entry point: ach_show(). Merging into Wrapped later = calling
 *  ach_show() from wrapped_show()'s loop, nothing else.
 * ============================================================ */

/* the one personal badge lives in ach_personal.h, which is deliberately
 * never published; the build simply grows it when the file is present */
#if __has_include("ach_personal.h")
#include "ach_personal.h"
#endif

#ifndef ACHIEVEMENTS_CORE_H
#define ACHIEVEMENTS_CORE_H

/* ---- metrics an achievement can test (values computed in ach_eval) ---- */
enum ach_metric {
    /* totals */
    AM_MINUTES,        /* total minutes listened            */
    AM_PLAYS,          /* total listened plays              */
    AM_SKIPS,          /* total skips                       */
    AM_UNIQ_TRACKS,    /* distinct songs played             */
    AM_UNIQ_ARTISTS,   /* distinct artists played           */
    AM_UNIQ_ALBUMS,    /* distinct albums played            */
    AM_NIGHT_PLAYS,    /* plays 00:00-04:59                 */
    AM_HOURS_COVERED,  /* distinct hours-of-day with a play */
    AM_ACTIVE_DAYS,    /* days with at least one play       */
    AM_STREAK,         /* longest consecutive-day run       */
    /* bests */
    AM_TRACK_PLAYS,    /* plays of the most-played song     */
    AM_ARTIST_PLAYS,   /* plays of the most-played artist   */
    AM_ALBUM_PLAYS,    /* plays of the most-played album    */
    AM_LOYAL_PLAYS,    /* best play count among never-skipped songs */
    AM_TRACK_SKIPS,    /* skips of the most-skipped song    */
    AM_DAY_PLAYS,      /* most plays in one day             */
    AM_DAY_MINS,       /* most minutes in one day           */
    AM_TRACK_DAY,      /* most plays of ONE song in ONE day */
    AM_SKIP_RUN,       /* longest consecutive-skip run      */
    AM_PLAY_RUN,       /* longest skip-free run of plays    */
    AM_WEEK_MINS,      /* most minutes in one Mon-Sun week  */
    AM_SUPERWEEKS,     /* weeks with >= 1440 min            */
    AM_ULTRAWEEKS,     /* weeks with >= 2880 min            */
    AM_ACTIVE_WEEKS,   /* weeks with >= 30 min              */
    AM_PERFECT_MONTHS, /* months with every day active      */
    /* one-shot date/time flags (value 0/1) */
    AM_F_NEWYEAR,      /* played during Jan 1, 00:00-00:59  */
    AM_F_XMAS,         /* played on Dec 25                  */
    AM_F_HALLOWEEN,    /* played on Oct 31                  */
    AM_F_FRIDAY13,     /* played on a Friday the 13th       */
    AM_F_MIDNIGHT,     /* play logged at 00:00:xx           */
    AM_F_LEAPDAY,      /* played on Feb 29                  */
    AM_F_TOAST,        /* fired the watcher's test toast (persistent
                          flag file - the one badge not in the log) */
    AM_F_CHEAT,        /* tampered stats detected (persistent verdict in
                          wrapped_cheat.dat - the one badge earned on a PC) */
    /* test-toast bookkeeping (same flag file as AM_F_TOAST, upgraded
     * to a counter - still the badges not in the log) */
    AM_TOAST_BURST,    /* two test toasts inside one minute (0/1)  */
    AM_TOAST_TOTAL,    /* lifetime test toast count                */
    /* listening types: 1 once the running profile matched that type,
     * re-evaluated after every play past a 100-play floor (same axes
     * and tipping points as the personality card - keep them synced) */
    AM_TYPE0,  AM_TYPE1,  AM_TYPE2,  AM_TYPE3,
    AM_TYPE4,  AM_TYPE5,  AM_TYPE6,  AM_TYPE7,
    AM_TYPE8,  AM_TYPE9,  AM_TYPE10, AM_TYPE11,
    AM_TYPE12, AM_TYPE13, AM_TYPE14, AM_TYPE15,
    AM_TYPES_COLLECTED, /* how many distinct types were ever matched */
#ifdef ACH_PERSONAL
    AM_PERSONAL,        /* plays of a certain artist; only counts on the
                           device carrying the opt-in flag file */
#endif
    /* meta */
    AM_META,           /* number of OTHER achievements unlocked */
    AM_COUNT
};

#define ACH_SECRET 1

struct ach_def {
    const char *name;      /* max 26 chars */
    const char *desc;      /* max 40 chars */
    unsigned char metric;
    unsigned char flags;
    long threshold;        /* unlocked when value >= threshold */
};

#include "ach_table.h"
#define N_ACH ((int)(sizeof(ach_table)/sizeof(ach_table[0])))

static long am_val[AM_COUNT];

/* ============================================================
 *  Extra tallies, fed one log entry at a time through ACH_HOOK
 *  (the aggregates Wrapped already keeps cover most metrics;
 *  these cover the running/chronological ones)
 * ============================================================ */

static long     at_cur_day = -1;
static unsigned at_day_secs, at_day_secs_max;
static int      at_skip_run, at_skip_run_max;
static int      at_play_run, at_play_run_max;
static int      at_trackday_max;

/* running profile for chronological listening-type detection */
static long at_plays, at_skips, at_uniq, at_night, at_top_art;
#define AT_TYPE_FLOOR 100   /* plays before a type can count */

/* per-week listened seconds, Mon-aligned unix weeks; linear table with
 * tail-first check like day_add (logs are roughly chronological) */
#define AT_WEEKS 512
static struct { long wk; unsigned secs; } at_weeks[AT_WEEKS];
static int at_week_n;

/* per-track play counts within the current day, by name hash (a hash
 * collision could inflate a joke stat; acceptable) */
#define AT_DAYTRK 96
static struct { unsigned hash; int n; } at_daytrk[AT_DAYTRK];
static int at_daytrk_n;

static void at_reset(void)
{
    rb->memset(am_val, 0, sizeof(am_val));
    at_cur_day = -1;
    at_day_secs = at_day_secs_max = 0;
    at_skip_run = at_skip_run_max = 0;
    at_play_run = at_play_run_max = 0;
    at_trackday_max = 0;
    at_week_n = 0;
    at_daytrk_n = 0;
    at_plays = at_skips = at_uniq = at_night = at_top_art = 0;
}

static void at_week_add(long wk, unsigned secs)
{
    if (at_week_n && at_weeks[at_week_n - 1].wk == wk) {
        at_weeks[at_week_n - 1].secs += secs;
        return;
    }
    for (int i = 0; i < at_week_n; i++)
        if (at_weeks[i].wk == wk) { at_weeks[i].secs += secs; return; }
    if (at_week_n < AT_WEEKS) {
        at_weeks[at_week_n].wk = wk;
        at_weeks[at_week_n].secs = secs;
        at_week_n++;
    }
}

/* receives every counted log entry (via ACH_HOOK in the parsers), AFTER
 * the aggregate tables were updated for it */
static void ach_tally(unsigned long ts, bool valid_ts, bool listened,
                      unsigned secs, const char *artist, const char *title)
{
    if (listened) {
        if (++at_play_run > at_play_run_max) at_play_run_max = at_play_run;
        at_skip_run = 0;

        /* running listening-type profile (chronological, so types you
         * pass THROUGH are collected, not just today's) */
        at_plays++;
        struct agg *aa = htable_get(&t_artist, artist);
        if (aa && aa->count > at_top_art) at_top_art = aa->count;
        struct agg *at = htable_get(&t_track, title);
        if (at && at->count == 1) at_uniq++;
        if (valid_ts && (ts % 86400UL) / 3600UL < 5) at_night++;

        if (at_plays >= AT_TYPE_FLOOR) {
            /* same axes/tipping points as the personality card */
            int v0 = (int)(at_top_art * 100 / at_plays);
            int v1 = (int)((at_plays - at_uniq) * 100 / at_plays);
            long tot = at_plays + at_skips;
            int v2 = (int)(at_plays * 100 / tot);
            int v3 = (int)(at_night * 100 / at_plays);
            int tt = ((v0 >= 45) << 3) | ((v1 >= 55) << 2)
                   | ((v2 >= 78) << 1) |  (v3 >=  8);
            am_val[AM_TYPE0 + tt] = 1;
        }
    } else {
        if (++at_skip_run > at_skip_run_max) at_skip_run_max = at_skip_run;
        at_play_run = 0;
        at_skips++;
    }

    if (!valid_ts || !listened)
        return;

    long day = (long)(ts / 86400UL);
    if (day != at_cur_day) {
        at_cur_day = day;
        at_day_secs = 0;
        at_daytrk_n = 0;
    }
    at_day_secs += secs;
    if (at_day_secs > at_day_secs_max) at_day_secs_max = at_day_secs;

    unsigned h = hash_str(title);
    int i;
    for (i = 0; i < at_daytrk_n; i++)
        if (at_daytrk[i].hash == h) break;
    if (i == at_daytrk_n && i < AT_DAYTRK) {
        at_daytrk[i].hash = h;
        at_daytrk[i].n = 0;
        at_daytrk_n++;
    }
    if (i < at_daytrk_n) {
        if (++at_daytrk[i].n > at_trackday_max)
            at_trackday_max = at_daytrk[i].n;
    }

    at_week_add((day - 4) / 7, secs);   /* unix day 4 = Mon 1970-01-05 */

    /* date/time flags */
    int y, m, d;
    civil_from_days(day, &y, &m, &d);
    unsigned tod = (unsigned)(ts % 86400UL);
    if (m ==  1 && d ==  1 && tod < 3600) am_val[AM_F_NEWYEAR]   = 1;
    if (m == 12 && d == 25)               am_val[AM_F_XMAS]      = 1;
    if (m == 10 && d == 31)               am_val[AM_F_HALLOWEEN] = 1;
    if (m ==  2 && d == 29)               am_val[AM_F_LEAPDAY]   = 1;
    if (d == 13 && (day % 7) == 1)        am_val[AM_F_FRIDAY13]  = 1;  /* day%7: 0=Thu */
    if (tod < 60)                         am_val[AM_F_MIDNIGHT]  = 1;
}

/* ---- turn aggregates + tallies into the final metric values ---- */

#ifdef ACH_PERSONAL
/* the personal badge is a moving target: the moment its play count
 * reaches the posted requirement, the requirement politely steps one
 * further. it only stands still - and can finally be caught - once
 * every other badge on the wall has been earned. */
static bool pers_frozen;
#endif

static long ach_threshold(int i)
{
    long thr = ach_table[i].threshold;
#ifdef ACH_PERSONAL
    if (ach_table[i].metric == AM_PERSONAL && !pers_frozen
        && am_val[AM_PERSONAL] >= thr)
        thr = am_val[AM_PERSONAL] + 1;
#endif
    return thr;
}

static bool ach_unlocked(int i)
{
    return am_val[ach_table[i].metric] >= ach_threshold(i);
}

/* the test-toast badges can't be derived from the log; they live in a
 * tiny counter file the watcher rewrites on every press:
 * "<total presses> <burst flag>". Legacy builds wrote just "1" - that
 * parses as one press, no burst. */
#define ACH_TOAST_FLAG "/.rockbox/wrapped_toast.flag"
static long ach_toast_count;
static bool ach_toast_burst;

#ifdef ACH_PERSONAL
/* the personal badge counts (and shows itself) only on the device
 * carrying the flag file; anywhere else it stays one more anonymous
 * mystery row */
static bool pers_open;
#endif

static void ach_flag_load(void)
{
    char buf[24];
    ach_toast_count = 0;
    ach_toast_burst = false;
    int fd = rb->open(ACH_TOAST_FLAG, O_RDONLY);
    if (fd >= 0) {
        int n = rb->read(fd, buf, sizeof(buf) - 1);
        rb->close(fd);
        if (n < 0) n = 0;
        buf[n] = '\0';
        int i = 0;
        while (buf[i] >= '0' && buf[i] <= '9')
            ach_toast_count = ach_toast_count * 10 + buf[i++] - '0';
        if (buf[i] == ' ')
            ach_toast_burst = (buf[i + 1] == '1');
        /* whatever it says, an existing file means at least one press */
        if (ach_toast_count < 1)
            ach_toast_count = 1;
    }
#ifdef ACH_PERSONAL
    fd = rb->open(ACH_PERSONAL_FLAG, O_RDONLY);
    pers_open = (fd >= 0);
    if (fd >= 0) rb->close(fd);
#endif
}

#ifdef ACH_PERSONAL
/* case-insensitive substring (no strcasestr in the plugin API) */
static bool name_has_ci(const char *hay, const char *needle)
{
    for (; *hay; hay++) {
        const char *h = hay, *n = needle;
        while (*h && *n) {
            char a = *h, b = *n;
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) break;
            h++; n++;
        }
        if (!*n) return true;
    }
    return false;
}
#endif

static void ach_eval(void)
{
    am_val[AM_MINUTES]     = total_seconds / 60;
    am_val[AM_PLAYS]       = total_plays;
    am_val[AM_SKIPS]       = total_skips;
    am_val[AM_UNIQ_TRACKS] = unique_tracks;
    am_val[AM_UNIQ_ARTISTS] = t_artist.n;
    am_val[AM_UNIQ_ALBUMS]  = t_album.n;

    long night = 0, hcov = 0;
    for (int i = 0; i < 24; i++) {
        if (i < 5) night += hour_hist[i];
        if (hour_hist[i] > 0) hcov++;
    }
    am_val[AM_NIGHT_PLAYS]   = night;
    am_val[AM_HOURS_COVERED] = hcov;
    am_val[AM_ACTIVE_DAYS]   = day_n;
    am_val[AM_STREAK]        = longest_streak();

    long btrk = 0, bart = 0, balb = 0, bloyal = 0, bskip = 0;
    for (int i = 0; i < t_track.n; i++) {
        const struct agg *a = &t_track.items[i];
        if (a->count > btrk) btrk = a->count;
        if (a->skips == 0 && a->count > bloyal) bloyal = a->count;
        if (a->skips > bskip) bskip = a->skips;
    }
    for (int i = 0; i < t_artist.n; i++)
        if (t_artist.items[i].count > bart) bart = t_artist.items[i].count;
    for (int i = 0; i < t_album.n; i++)
        if (t_album.items[i].count > balb) balb = t_album.items[i].count;
    am_val[AM_TRACK_PLAYS]  = btrk;
    am_val[AM_ARTIST_PLAYS] = bart;
    am_val[AM_ALBUM_PLAYS]  = balb;
    am_val[AM_LOYAL_PLAYS]  = bloyal;
    am_val[AM_TRACK_SKIPS]  = bskip;

    long dp = 0;
    for (int i = 0; i < day_n; i++)
        if (days[i].count > dp) dp = days[i].count;
    am_val[AM_DAY_PLAYS] = dp;
    am_val[AM_DAY_MINS]  = at_day_secs_max / 60;
    am_val[AM_TRACK_DAY] = at_trackday_max;
    am_val[AM_SKIP_RUN]  = at_skip_run_max;
    am_val[AM_PLAY_RUN]  = at_play_run_max;

    long wmax = 0, sup = 0, ult = 0, act = 0;
    for (int i = 0; i < at_week_n; i++) {
        unsigned s = at_weeks[i].secs;
        if (s / 60 > (unsigned)wmax) wmax = s / 60;
        if (s >= (unsigned)SUPER_WEEK_SECS)  sup++;
        if (s >= (unsigned)ULTRA_WEEK_SECS)  ult++;
        if (s >= (unsigned)WEEK_ACTIVE_SECS) act++;
    }
    am_val[AM_WEEK_MINS]    = wmax;
    am_val[AM_SUPERWEEKS]   = sup;
    am_val[AM_ULTRAWEEKS]   = ult;
    am_val[AM_ACTIVE_WEEKS] = act;

    /* perfect months: every calendar day of a month active */
    {
        struct { short y; signed char m; short n; } mon[60];
        int mn = 0;
        for (int i = 0; i < day_n; i++) {
            int y, m, d;
            civil_from_days(days[i].day, &y, &m, &d);
            int j;
            for (j = 0; j < mn; j++)
                if (mon[j].y == y && mon[j].m == m) break;
            if (j == mn && mn < 60) { mon[mn].y = y; mon[mn].m = m; mon[mn].n = 0; mn++; }
            if (j < mn) mon[j].n++;
        }
        long pm = 0;
        for (int j = 0; j < mn; j++) {
            int ny = mon[j].m == 12 ? mon[j].y + 1 : mon[j].y;
            int nm = mon[j].m == 12 ? 1 : mon[j].m + 1;
            long mlen = days_from_civil(ny, nm, 1)
                      - days_from_civil(mon[j].y, mon[j].m, 1);
            if (mon[j].n >= mlen) pm++;
        }
        am_val[AM_PERFECT_MONTHS] = pm;
    }

    am_val[AM_F_TOAST]     = ach_toast_count > 0 ? 1 : 0;
    am_val[AM_TOAST_BURST] = ach_toast_burst ? 1 : 0;
    am_val[AM_TOAST_TOTAL] = ach_toast_count;
    am_val[AM_F_CHEAT]     = cheat_rigged() ? 1 : 0;

    /* the AM_TYPEx flags were set chronologically during tallying */
    {
        long tc = 0;
        for (int i = 0; i < 16; i++)
            if (am_val[AM_TYPE0 + i]) tc++;
        am_val[AM_TYPES_COLLECTED] = tc;
    }

#ifdef ACH_PERSONAL
    am_val[AM_PERSONAL] = 0;
    if (pers_open) {
        long yp = 0;
        for (int i = 0; i < t_artist.n; i++)
            if (name_has_ci(t_artist.items[i].name, ACH_PERSONAL_ARTIST))
                yp += t_artist.items[i].count;
        am_val[AM_PERSONAL] = yp;
    }
#endif

    /* the flag metrics were set during tallying; meta counts the rest
     * (the moving target is still running here, so it never counts) */
#ifdef ACH_PERSONAL
    pers_frozen = false;
#endif
    long meta = 0;
    for (int i = 0; i < N_ACH; i++)
        if (ach_table[i].metric != AM_META && ach_unlocked(i)) meta++;
    am_val[AM_META] = meta;

#ifdef ACH_PERSONAL
    /* the target stands still only when everything else is done (the
     * cheater badge excepted: catching it legit must not require sinning) */
    pers_frozen = true;
    for (int i = 0; i < N_ACH; i++)
        if (ach_table[i].metric != AM_PERSONAL
            && ach_table[i].metric != AM_F_CHEAT && !ach_unlocked(i)) {
            pers_frozen = false;
            break;
        }
    if (pers_frozen) {          /* it may just have become catchable */
        meta = 0;
        for (int i = 0; i < N_ACH; i++)
            if (ach_table[i].metric != AM_META && ach_unlocked(i)) meta++;
        am_val[AM_META] = meta;
    }
#endif
}

/* ============================================================
 *  Seen-state: one bit per achievement so fresh unlocks can be
 *  flagged NEW exactly once. ANDed with the current unlock set
 *  on load, so stale bits from another log heal themselves.
 *
 *  v2 adds the GOLD mask alongside: badges unlocked while the log
 *  still verified clean. Once tampering is detected the mask
 *  freezes, and every unlock outside it renders green, forever.
 *  Both files are append-only in table order, so an old (shorter)
 *  file maps 1:1 onto a grown table - never insert mid-table
 *  without bumping the magic and remapping on load, the way the
 *  WA4b "It's Over 9000!" insert did.
 * ============================================================ */

#define ACH_SEEN_PATH "/.rockbox/wrapped_ach.dat"
#define ACH_SEEN_MAGIC_V1 0x57413162UL   /* "WA1b": seen only               */
#define ACH_SEEN_MAGIC_V2 0x57413262UL   /* "WA2b": seen + gold mask        */
#define ACH_SEEN_MAGIC_V3 0x57413362UL   /* "WA3b": ... + unlock timestamps */
#define ACH_SEEN_MAGIC    0x57413462UL   /* "WA4b": "It's Over 9000!" was
                                            inserted at slot ACH_INS_9000 */

/* table slot "It's Over 9000!" went into; pre-WA4b saves know nothing
 * of it, so on load everything from here on shifts up one slot */
#define ACH_INS_9000 13

static unsigned char ach_seen[(sizeof(ach_table)/sizeof(ach_table[0]) + 7) / 8];
static unsigned char ach_gold[(sizeof(ach_table)/sizeof(ach_table[0]) + 7) / 8];

/* when each badge was first seen unlocked; 0 = still locked, 1 = it was
 * already unlocked when record-keeping started ("before records began").
 * Fixed-width: the file must mean the same thing on sim and device. */
static uint32_t ach_when[sizeof(ach_table)/sizeof(ach_table[0])];
static bool ach_when_legacy;   /* this load predates v3: backfill with 1 */

static bool seen_get(int i) { return (ach_seen[i >> 3] >> (i & 7)) & 1; }
static void seen_set(int i) { ach_seen[i >> 3] |= 1u << (i & 7); }
static void seen_clr(int i) { ach_seen[i >> 3] &= ~(1u << (i & 7)); }
static bool gold_get(int i) { return (ach_gold[i >> 3] >> (i & 7)) & 1; }
static void gold_set(int i) { ach_gold[i >> 3] |= 1u << (i & 7); }
static void gold_clr(int i) { ach_gold[i >> 3] &= ~(1u << (i & 7)); }

/* pre-WA4b saves predate the "It's Over 9000!" mid-table insert: open
 * one slot at ACH_INS_9000 so every later badge keeps its history */
static void ach_migrate_v4(int count)
{
    if (count > N_ACH - 1) count = N_ACH - 1;    /* room for the shift */
    for (int i = count - 1; i >= ACH_INS_9000; i--) {
        ach_when[i + 1] = ach_when[i];
        if (seen_get(i)) seen_set(i + 1); else seen_clr(i + 1);
        if (gold_get(i)) gold_set(i + 1); else gold_clr(i + 1);
    }
    if (count > ACH_INS_9000) {
        ach_when[ACH_INS_9000] = 0;
        seen_clr(ACH_INS_9000);
        gold_clr(ACH_INS_9000);
    }
}

static void ach_seen_load(void)
{
    rb->memset(ach_seen, 0, sizeof(ach_seen));
    rb->memset(ach_gold, 0, sizeof(ach_gold));
    rb->memset(ach_when, 0, sizeof(ach_when));
    ach_when_legacy = true;                  /* no/old file: backfill */
    int fd = rb->open(ACH_SEEN_PATH, O_RDONLY);
    if (fd < 0) return;
    unsigned long magic = 0;
    int count = 0;
    if (rb->read(fd, &magic, 4) == 4
        && (magic == ACH_SEEN_MAGIC || magic == ACH_SEEN_MAGIC_V3
            || magic == ACH_SEEN_MAGIC_V2 || magic == ACH_SEEN_MAGIC_V1)
        && rb->read(fd, &count, 4) == 4 && count > 0 && count <= N_ACH) {
        int nb = (count + 7) / 8;
        rb->read(fd, ach_seen, nb);
        if (magic == ACH_SEEN_MAGIC_V1) {
            /* v1: what was already seen was already earned */
            rb->memcpy(ach_gold, ach_seen, sizeof(ach_gold));
        } else {
            rb->read(fd, ach_gold, nb);
            if (magic == ACH_SEEN_MAGIC || magic == ACH_SEEN_MAGIC_V3) {
                rb->read(fd, ach_when, (size_t)count * 4);
                ach_when_legacy = false;
            }
        }
        if (magic != ACH_SEEN_MAGIC)
            ach_migrate_v4(count);
    } else {
        rb->memset(ach_seen, 0, sizeof(ach_seen));
    }
    rb->close(fd);
    /* self-heal: only currently-unlocked badges can count as seen
     * (gold and the dates are deliberately NOT pruned: they record
     * what once happened, not what the current log supports) */
    for (int i = 0; i < N_ACH; i++)
        if (seen_get(i) && !ach_unlocked(i))
            ach_seen[i >> 3] &= ~(1u << (i & 7));
}

static void ach_seen_save(void)
{
    int fd = rb->open(ACH_SEEN_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return;
    unsigned long magic = ACH_SEEN_MAGIC;
    int count = N_ACH;
    rb->write(fd, &magic, 4);
    rb->write(fd, &count, 4);
    rb->write(fd, ach_seen, sizeof(ach_seen));
    rb->write(fd, ach_gold, sizeof(ach_gold));
    rb->write(fd, ach_when, sizeof(ach_when));
    rb->close(fd);
}

/* Bookkeeping for every badge observed unlocked: stamp its date the first
 * time (any verdict - cheated unlocks get dated too), and while the crunch
 * verifies clean let it join the gold mask. After a cheat verdict gold is
 * frozen - the wall keeps gold and green apart. Returns true when anything
 * changed (caller should save). */
static bool ach_record_unlocks(void)
{
    bool changed = false;
    unsigned long now = 0;
    for (int i = 0; i < N_ACH; i++) {
        if (!ach_unlocked(i))
            continue;
        if (ach_when[i] == 0) {
            if (ach_when_legacy) {
                ach_when[i] = 1;             /* before records began */
            } else {
                if (!now) {
                    struct tm *t = rb->get_time();
                    now = t ? (unsigned long)rb->mktime(t) : 1;
                    if (now < MIN_VALID_TS)
                        now = 1;             /* unset clock: no fake dates */
                }
                ach_when[i] = now;
            }
            changed = true;
        }
        if (cheat_live == 0 && !gold_get(i)) {
            gold_set(i);
            changed = true;
        }
    }
    ach_when_legacy = false;
    return changed;
}

/* a green badge: unlocked, but not under a clean log */
static bool ach_is_green(int i)
{
    return cheat_rigged() && ach_unlocked(i) && !gold_get(i);
}

/* ============================================================
 *  UI: one scrolling list over all badges
 * ============================================================ */

/* row visibility: unlocked shown in full; the lowest locked rung of each
 * ladder shows as the "next goal" with live progress; everything past it -
 * and every ACH_SECRET - stays a mystery row until earned */
enum { AV_HIDDEN, AV_GOAL, AV_DONE };
static unsigned char ach_vis[sizeof(ach_table)/sizeof(ach_table[0])];
static unsigned char ach_new[(sizeof(ach_table)/sizeof(ach_table[0]) + 7) / 8];

static bool new_get(int i) { return (ach_new[i >> 3] >> (i & 7)) & 1; }

static int ach_classify(void)   /* fills ach_vis/ach_new, returns unlock count */
{
    bool goal_taken[AM_COUNT];
    rb->memset(goal_taken, 0, sizeof(goal_taken));
    rb->memset(ach_new, 0, sizeof(ach_new));
    int done = 0;
    for (int i = 0; i < N_ACH; i++) {
        if (ach_unlocked(i)) {
            ach_vis[i] = AV_DONE;
            done++;
            if (!seen_get(i)) {
                ach_new[i >> 3] |= 1u << (i & 7);
                seen_set(i);
            }
#ifdef ACH_PERSONAL
        } else if (ach_table[i].metric == AM_PERSONAL && pers_open) {
            /* flagged secret so other devices never learn it exists,
             * but on THIS device it shows itself openly */
            ach_vis[i] = AV_GOAL;
#endif
        } else if (!(ach_table[i].flags & ACH_SECRET)
                   && !goal_taken[ach_table[i].metric]) {
            ach_vis[i] = AV_GOAL;
            goal_taken[ach_table[i].metric] = true;
        } else {
            ach_vis[i] = AV_HIDDEN;
        }
    }
    return done;
}

static const struct theme ach_th = {
    LCD_RGBPACK(52, 38, 12), LCD_RGBPACK(18, 11, 6),
    LCD_RGBPACK(255,244,200), LCD_RGBPACK(255,196, 70),
    COL_GOLD
};

/* ---- live toast banner: slender strip that slides in from the top ----
 *
 * Drawing through the lcd_* API from a background thread is a trap: the
 * current viewport is a GLOBAL the UI thread reswitches constantly, so
 * mid-animation draws get translated/clipped through whatever viewport
 * happens to be active (seen on device as banner shards over the status
 * bar). So the LCD API is used exactly once, in one quick batch, to paint
 * the banner; it is then captured as an image and the entire animation -
 * including putting the original pixels back - is raw framebuffer memcpy
 * on stable addresses. No viewports, no ghosts. Blocks ~2.5 s. */

#define TOAST_H 44
static fb_data toast_save[TOAST_H * LCD_WIDTH];   /* what was underneath */
static fb_data toast_img[TOAST_H * LCD_WIDTH];    /* the painted banner  */

/* the cheater's banner: same shape, different metal */
static const struct theme ach_th_green = {
    LCD_RGBPACK(12, 48, 20), LCD_RGBPACK( 4, 16,  8),
    LCD_RGBPACK(225,255,232), LCD_RGBPACK(110,235,135),
    COL_CHEAT
};

static void toast_paint(const struct theme *th, const char *name,
                        unsigned main, unsigned hi, const char *kick)
{
    rb->lcd_set_drawmode(DRMODE_SOLID);
    for (int y = 0; y < TOAST_H; y++) {
        rb->lcd_set_foreground(
            (y == 0 || y == TOAST_H - 1) ? main : grad_at(th, y));
        rb->lcd_hline(0, SCREEN_W - 1, y);
    }

    draw_disc(th, 20, 22, 10, main, 256);
    draw_disc(th, 20, 22, 5, hi, 256);

    kicker(6, kick, main);

    char nb[48];
    fit_text(name, SCREEN_W - 56, nb, sizeof nb);
    int w, hh;
    rb->lcd_setfont(body_font);
    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_getstringsize(nb, &w, &hh);
    rb->lcd_set_foreground(COL_TEXT_LIGHT);
    rb->lcd_putsxy(38 + (SCREEN_W - 38 - 8 - w) / 2, 23, nb);
}

static void ach_toast(const char *name, bool green, const char *kick)
{
    const int FR = 8;

    struct viewport *vp = rb->lcd_set_viewport(NULL);
    if (!vp || !vp->buffer || !vp->buffer->fb_ptr)
        return;                       /* no framebuffer, no toast */
    fb_data *fb = vp->buffer->fb_ptr;
    ptrdiff_t stride = LCD_NATIVE_STRIDE(vp->buffer->stride);
#define TROW(y) (fb + (ptrdiff_t)(y) * stride)

    rb->backlight_on();
    /* freeze display pushes from every other thread: the UI can keep
     * drawing into the framebuffer (we harvest that below), but the panel
     * itself shows only what we push until the toast is gone */
    rb->lcd_freeze_others(true);

    /* snapshot the underlay, paint the banner once (the only lcd_* batch,
     * viewport pinned the instant before), capture it, put the underlay
     * straight back - the animation below never calls a draw function */
    for (int y = 0; y < TOAST_H; y++)
        rb->memcpy(&toast_save[y * LCD_WIDTH], TROW(y),
                   LCD_WIDTH * sizeof(fb_data));
    if (green)
        toast_paint(&ach_th_green, name, COL_CHEAT, COL_CHEAT_HI, kick);
    else
        toast_paint(&ach_th, name, COL_GOLD, COL_GOLD_HI, kick);
    for (int y = 0; y < TOAST_H; y++)
        rb->memcpy(&toast_img[y * LCD_WIDTH], TROW(y),
                   LCD_WIDTH * sizeof(fb_data));
    for (int y = 0; y < TOAST_H; y++)
        rb->memcpy(TROW(y), &toast_save[y * LCD_WIDTH],
                   LCD_WIDTH * sizeof(fb_data));

    /* slide in: screen row r shows banner row (TOAST_H - k + r) */
    for (int f = 1; f <= FR; f++) {
        int k = (TOAST_H * f) / FR;
        for (int r = 0; r < k; r++)
            rb->memcpy(TROW(r), &toast_img[(TOAST_H - k + r) * LCD_WIDTH],
                       LCD_WIDTH * sizeof(fb_data));
        rb->lcd_update_rect(0, 0, SCREEN_W, TOAST_H);
        rb->sleep(HZ / 40);
    }

    /* Hold ~2 s, DEFENDING the banner: the UI thread keeps repainting
     * things like the statusbar clock underneath us (its panel pushes are
     * frozen out, but the framebuffer still changes). Any pixel that no
     * longer matches the banner is fresh underlay - harvest it into the
     * snapshot (so the restore stays current) and reassert the banner. */
    for (int t = 0; t < 40; t++) {
        bool dirty = false;
        for (int y = 0; y < TOAST_H; y++) {
            fb_data *row = TROW(y);
            fb_data *img = &toast_img[y * LCD_WIDTH];
            fb_data *sav = &toast_save[y * LCD_WIDTH];
            bool row_dirty = false;
            for (int x = 0; x < LCD_WIDTH; x++) {
                if (row[x] != img[x]) {
                    sav[x] = row[x];
                    row_dirty = true;
                }
            }
            if (row_dirty) {
                rb->memcpy(row, img, LCD_WIDTH * sizeof(fb_data));
                dirty = true;
            }
        }
        if (dirty)
            rb->lcd_update_rect(0, 0, SCREEN_W, TOAST_H);
        rb->sleep(HZ / 20);
    }

    /* slide out: banner shrinks upward, underlay reappears beneath it */
    for (int f = FR - 1; f >= 0; f--) {
        int k = (TOAST_H * f) / FR;
        for (int r = 0; r < k; r++)
            rb->memcpy(TROW(r), &toast_img[(TOAST_H - k + r) * LCD_WIDTH],
                       LCD_WIDTH * sizeof(fb_data));
        for (int r = k; r < TOAST_H; r++)
            rb->memcpy(TROW(r), &toast_save[r * LCD_WIDTH],
                       LCD_WIDTH * sizeof(fb_data));
        rb->lcd_update_rect(0, 0, SCREEN_W, TOAST_H);
        rb->sleep(HZ / 40);
    }
    rb->lcd_freeze_others(false);
#undef TROW
}

#define ACH_ROWS  6
#define ACH_Y0    56
#define ACH_RH    25

static void ach_draw_row(int y, int i, bool selected)
{
    const struct theme *th = &ach_th;
    int cy = y + ACH_RH / 2 - 1;
#ifdef ACH_PERSONAL
    bool pers = (ach_table[i].metric == AM_PERSONAL)
                && ach_vis[i] != AV_HIDDEN;
#else
    const bool pers = false;   /* no personal badge in this build */
#endif
    bool green = (ach_vis[i] == AV_DONE) && ach_is_green(i);

    if (ach_vis[i] == AV_DONE) {
        draw_disc(th, 17, cy, 8,
                  green ? COL_CHEAT    : pers ? COL_ICE    : COL_GOLD, 256);
        draw_disc(th, 17, cy, 4,
                  green ? COL_CHEAT_HI : pers ? COL_ICE_HI : COL_GOLD_HI, 256);
    } else if (pers) {
        /* ever so slightly special: an icy ring among the gold */
        draw_disc(th, 17, cy, 8, COL_ICE, 140);
        draw_disc(th, 17, cy, 6, grad_at(th, cy), 256);
    } else {
        draw_disc(th, 17, cy, 8, LCD_RGBPACK(255,255,255), 55);
        draw_disc(th, 17, cy, 6, grad_at(th, cy), 256);
    }

    rb->lcd_setfont(body_font);
    rb->lcd_set_drawmode(DRMODE_FG);

    char rtxt[36];
    rtxt[0] = '\0';
    if (ach_vis[i] == AV_DONE) {
        if (new_get(i)) rb->strlcpy(rtxt, "NEW", sizeof(rtxt));
    } else if (ach_vis[i] == AV_GOAL) {
        char a[16], b[16];
        long thr = ach_threshold(i);
        long v = am_val[ach_table[i].metric];
        if (v < 0) v = 0;
        if (v > thr) v = thr;
        commafmt(v, a, sizeof(a));
        commafmt(thr, b, sizeof(b));
        rb->snprintf(rtxt, sizeof(rtxt), "%s/%s", a, b);
    }

    int rw = 0, rh2;
    if (rtxt[0])
        rb->lcd_getstringsize(rtxt, &rw, &rh2);

    char nb[48];
    const char *name = (ach_vis[i] == AV_HIDDEN) ? "? ? ?" : ach_table[i].name;
    fit_text(name, SCREEN_W - 14 - rw - 8 - 32, nb, sizeof(nb));

    unsigned ncol = green                   ? COL_CHEAT_HI
                  : pers                    ? COL_ICE_HI
                  : (ach_vis[i] == AV_DONE) ? COL_GOLD_HI
                  : (ach_vis[i] == AV_GOAL) ? COL_TEXT_LIGHT
                  : blend565(grad_at(th, cy), LCD_RGBPACK(255,255,255), 90);
    rb->lcd_set_foreground(ncol);
    rb->lcd_putsxy(32, y + 4, nb);

    if (rtxt[0]) {
        rb->lcd_set_foreground(new_get(i) ? (green ? COL_CHEAT : COL_GOLD)
                                          : COL_TEXT_DIM);
        rb->lcd_putsxy(SCREEN_W - 12 - rw, y + 4, rtxt);
    }

    if (selected) {
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));
        rb->lcd_drawrect(4, y, SCREEN_W - 8, ACH_RH - 1);
    }
}

static void ach_draw_screen(int cursor, int top, int done, int newn)
{
    const struct theme *th = &ach_th;
    char buf[64];

    fill_gradient(th);
    if (cheat_rigged())
        kicker(10, "YOU KNOW WHAT YOU DID", COL_CHEAT);
    else if (done >= N_ACH - 1)   /* everything except the cheater row */
        kicker(10, "CERTIFIED 100% ORGANIC", COL_ICE);
    else
        kicker(10, "ACHIEVEMENTS", th->accent);
    if (newn > 0)
        rb->snprintf(buf, sizeof buf, "%d / %d unlocked  -  %d NEW", done, N_ACH, newn);
    else
        rb->snprintf(buf, sizeof buf, "%d / %d unlocked", done, N_ACH);
    center_text(28, buf, COL_TEXT_LIGHT);
    accent_underline(th, 48);

    for (int r = 0; r < ACH_ROWS; r++) {
        int i = top + r;
        if (i >= N_ACH) break;
        ach_draw_row(ACH_Y0 + r * ACH_RH, i, i == cursor);
    }

    /* footer: description + live progress bar for the selected row */
    int fy = ACH_Y0 + ACH_ROWS * ACH_RH + 4;
    /* the cheater row is openly visible so everyone knows they are being
     * watched - but "you know what you did" is only true once it's true */
    const char *desc = (ach_vis[cursor] == AV_HIDDEN)
                         ? "keep listening to reveal this one"
                         : (ach_table[cursor].metric == AM_F_CHEAT
                            && !ach_unlocked(cursor))
                         ? "don't even try - the iPod will know"
                         : ach_table[cursor].desc;
    char fb[64];
    fit_text(desc, SCREEN_W - 20, fb, sizeof fb);
    center_text(fy, fb, (ach_vis[cursor] != AV_DONE) ? COL_TEXT_DIM
                        : ach_is_green(cursor) ? COL_CHEAT : COL_GOLD);

    if (ach_vis[cursor] == AV_DONE) {
        /* a finished badge doesn't need a full bar - it gets its date */
        char db[48];
        unsigned long w = ach_when[cursor];
        if (w > MIN_VALID_TS) {
            int dy, dm, dd;
            civil_from_days((long)(w / 86400UL), &dy, &dm, &dd);
            rb->snprintf(db, sizeof db, "earned %s %d %d",
                         month_abbr[dm], dd, dy);
        } else {
            rb->strlcpy(db, "earned before records began", sizeof db);
        }
        center_text(fy + 17, db, COL_TEXT_DIM);
    } else if (ach_vis[cursor] != AV_HIDDEN) {
        int bx0 = 50, bx1 = SCREEN_W - 50, bary = fy + 20;
        draw_capsule(th, bx0, bary, bx1, bary, 3,
                     blend565(grad_at(th, bary), LCD_RGBPACK(255,255,255), 55));
        long v = am_val[ach_table[cursor].metric];
        long t = ach_threshold(cursor);
        if (v > t) v = t;
        if (v < 0) v = 0;
        int fill = (int)((long)(bx1 - bx0) * v / (t ? t : 1));
        if (fill > 3)
            draw_capsule(th, bx0, bary, bx0 + fill, bary, 3, th->accent);
    }

    rb->lcd_update();
}

/* not everything past the end of the list is empty. returns the button
 * that dismissed it so USB unplug still propagates */
static int ach_egg(void)
{
    const struct theme *th = &ach_th;
    int cx = SCREEN_W / 2;
    fill_gradient(th);
    draw_capsule(th, cx, 74, cx, 128, 15, COL_ICE);
    draw_capsule(th, cx, 82, cx, 120, 8, COL_ICE_HI);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));
    rb->lcd_drawpixel(cx + 22, 66);
    rb->lcd_drawpixel(cx + 21, 67);
    rb->lcd_drawpixel(cx + 23, 67);
    rb->lcd_drawpixel(cx + 22, 68);
    rb->lcd_drawpixel(cx - 26, 132);
    rb->lcd_drawpixel(cx - 27, 133);
    rb->lcd_drawpixel(cx - 25, 133);
    rb->lcd_drawpixel(cx - 26, 134);
    kicker(30, "OK, ONE MORE", COL_ICE);
    center_text(154, "no badge for this one -", COL_TEXT_LIGHT);
    center_text(172, "you just wanted to see past the end.", COL_TEXT_DIM);
    center_text(202, "tell Tup the iPod says hi", COL_ICE);
    rb->lcd_update();
    rb->button_clear_queue();
    return rb->button_get(true);
}

/* the badge list itself; call with an ALL-TIME crunch already loaded.
 * returns PLUGIN_OK / PLUGIN_USB_CONNECTED */
static int ach_list_ui(void)
{
    ach_flag_load();
    ach_eval();
    ach_seen_load();
    ach_record_unlocks();            /* stamp dates; clean unlocks go gold */
    int done = ach_classify();       /* also merges fresh unlocks into seen */
    ach_seen_save();

    int newn = 0;
    for (int i = 0; i < N_ACH; i++)
        if (new_get(i)) newn++;

    if (newn > 0)
        rb->splashf(HZ, "%d new achievement%s!", newn, newn == 1 ? "" : "s");

    int cursor = 0, top = 0, egg_taps = 0;
    rb->button_clear_queue();
    ach_draw_screen(cursor, top, done, newn);

    for (;;) {
        int btn = rb->button_get(true);
        if (btn == SYS_USB_CONNECTED) {
            rb->lcd_setfont(FONT_SYSFIXED);
            return PLUGIN_USB_CONNECTED;
        }
        int base = btn & ~BUTTON_REPEAT;
        if (base != BUTTON_RIGHT)
            egg_taps = 0;
        if (base == BUTTON_SCROLL_FWD) {
            cursor += wheel_steps();
            if (cursor > N_ACH - 1) cursor = N_ACH - 1;
        } else if (base == BUTTON_SCROLL_BACK) {
            cursor -= wheel_steps();
            if (cursor < 0) cursor = 0;
        } else if (base == BUTTON_RIGHT) {
            /* hop to the start of the next ladder */
            int prev = cursor;
            int m = ach_table[cursor].metric;
            while (cursor < N_ACH - 1 && ach_table[cursor].metric == m)
                cursor++;
            /* knocking on the end of the list three times... */
            if (cursor == prev && cursor == N_ACH - 1) {
                if (++egg_taps >= 3) {
                    egg_taps = 0;
                    if (ach_egg() == SYS_USB_CONNECTED) {
                        rb->lcd_setfont(FONT_SYSFIXED);
                        return PLUGIN_USB_CONNECTED;
                    }
                }
            } else {
                egg_taps = 0;
            }
        } else if (base == BUTTON_LEFT) {
            /* hop to the start of this ladder; if already there, the
             * start of the previous one */
            int start = cursor;
            while (start > 0
                   && ach_table[start - 1].metric == ach_table[cursor].metric)
                start--;
            if (start == cursor && start > 0) {
                int m = ach_table[start - 1].metric;
                start--;
                while (start > 0 && ach_table[start - 1].metric == m)
                    start--;
            }
            cursor = start;
        } else if (base == BUTTON_SELECT) {
            /* jump to the next NEW unlock, wrapping around */
            if (newn > 0) {
                for (int k = 1; k <= N_ACH; k++) {
                    int i = (cursor + k) % N_ACH;
                    if (new_get(i)) { cursor = i; break; }
                }
            }
        } else if (base == BUTTON_PLAY) {
            save_card();
        } else if (base == BUTTON_MENU) {
            break;
        } else {
            continue;
        }
        if (cursor < top) top = cursor;
        if (cursor >= top + ACH_ROWS) top = cursor - ACH_ROWS + 1;
        ach_draw_screen(cursor, top, done, newn);
    }

    rb->lcd_setfont(FONT_SYSFIXED);
    return PLUGIN_OK;
}

/* standalone plugin entry: crunch all-time, then browse */
static int ach_show(void)
{
    wrapped_font_init();
    at_reset();
    if (!wrapped_load_data(false))   /* all-time, no year picker */
        return PLUGIN_OK;
    return ach_list_ui();
}

#ifdef WRAPPED_WITH_ACH
/* ============================================================
 *  Wrapped integration: the achievements card + in-deck browser.
 *  Achievements are always ALL-TIME; when the deck is scoped to a
 *  single year, we re-crunch without the filter on the way in and
 *  restore the year-scoped crunch on the way out (fast: the log
 *  reader streams the whole family in one pass).
 * ============================================================ */

static bool ach_cache_valid;
static int  ach_cache_done, ach_cache_new;
static long ach_cache_finish;   /* projected completion year; 0 = no data,
                                   -1 = every endless ladder already done */

/* the year the wall is projected to finish: the worst ETA among the
 * endless ladders' top rungs, at the pace the crunch shows */
static long wall_finish_year(void)
{
    long span = (ts_max > ts_min) ? (ts_max - ts_min) / 86400 + 1 : 0;
    if (span < 7)
        return 0;                           /* forecasts need history */
    const struct { long cur, goal; } far[4] = {
        { total_seconds / 60, 1000000 },    /* minutes */
        { total_plays,         250000 },    /* plays   */
        { total_skips,         100000 },    /* skips   */
        { day_n,                 3650 },    /* days    */
    };
    long worst = 0;
    for (int i = 0; i < 4; i++) {
        if (far[i].cur >= far[i].goal)
            continue;
        long rate1000 = far[i].cur * 1000 / span;   /* per-mille per day */
        if (rate1000 <= 0)
            return 0;                       /* a dead ladder: no forecast */
        long dleft = (far[i].goal - far[i].cur) * 1000 / rate1000;
        if (dleft > worst)
            worst = dleft;
    }
    if (!worst)
        return -1;
    int y, m, d;
    civil_from_days(ts_max / 86400, &y, &m, &d);
    return y + (worst + 182) / 365;
}

static void ach_recount(void)
{
    int saved = filter_year;
    if (saved) { filter_year = 0; at_reset(); wrapped_reload(); }

    ach_flag_load();
    ach_eval();
    ach_seen_load();                 /* prunes stale seen bits; no save */
    if (ach_record_unlocks())        /* ...unless dates/gold changed    */
        ach_seen_save();
    ach_cache_done = ach_cache_new = 0;
    for (int i = 0; i < N_ACH; i++) {
        if (!ach_unlocked(i))
            continue;
        ach_cache_done++;
        if (!seen_get(i))
            ach_cache_new++;
    }
    ach_cache_finish = wall_finish_year();   /* while the crunch is all-time */

    if (saved) { filter_year = saved; at_reset(); wrapped_reload(); }
    ach_cache_valid = true;
}

static int ach_wrapped_card(int idx, int dir)
{
    const struct theme *th = &themes[idx];
    char buf2[64];

    if (!ach_cache_valid)
        ach_recount();

    /* the ach palette with the numerals recast in green */
    static const struct theme ach_th_rigged = {
        LCD_RGBPACK(52, 38, 12), LCD_RGBPACK(18, 11, 6),
        LCD_RGBPACK(225,255,232), LCD_RGBPACK(110,235,135),
        COL_CHEAT
    };
    bool rig = cheat_rigged();

    fill_gradient(th);
    if (rig)
        kicker(22, "YOU KNOW WHAT YOU DID", COL_CHEAT);
    else if (ach_cache_done >= N_ACH - 1)
        kicker(22, "CERTIFIED 100% ORGANIC", COL_ICE);
    else
        kicker(22, "ACHIEVEMENTS", th->accent);
    accent_underline(th, 43);
    rb->snprintf(buf2, sizeof buf2, "of %d badges earned", N_ACH);
    center_text(150, buf2, rig ? COL_CHEAT : th->accent);
    if (ach_cache_new > 0) {
        rb->snprintf(buf2, sizeof buf2, "%d NEW - press SELECT", ach_cache_new);
        center_text(180, buf2, COL_GOLD_HI);
    } else {
        center_text(180, "press SELECT to browse", COL_TEXT_DIM);
    }
    /* the Long Game, condensed to its one number */
    if (!rig && ach_cache_finish > 0) {
        rb->snprintf(buf2, sizeof buf2, "on track to finish in the year %ld",
                     ach_cache_finish);
        center_text(205, buf2, COL_TEXT_DIM);
    } else if (!rig && ach_cache_finish < 0) {
        center_text(205, "the wall is complete. go outside.", COL_TEXT_DIM);
    }

    hero_band_y = 74; hero_band_h = 56 + 10 + 6;
    page_dots(idx, th);
    present_card(dir);
    return animate_count(rig ? &ach_th_rigged : th, SCREEN_W/2, hero_band_y,
                         34, 56, 10, ach_cache_done);
}

static int ach_browse(void)
{
    int saved = filter_year;
    if (saved) {
        filter_year = 0;
        at_reset();
        if (!wrapped_reload()) {     /* can't happen normally; recover */
            filter_year = saved;
            at_reset();
            wrapped_reload();
            return 0;
        }
    }

    int r = ach_list_ui();
    ach_cache_valid = false;         /* fresh unlocks were just seen */

    if (saved) {
        filter_year = saved;
        at_reset();
        wrapped_reload();
    }
    return (r == PLUGIN_USB_CONNECTED) ? SYS_USB_CONNECTED : 0;
}
#endif /* WRAPPED_WITH_ACH */

#endif /* ACHIEVEMENTS_CORE_H */
