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
 *  playback_sig.h - rolling signature over the playback-log family.
 *
 *  The core logger (playback.c) extends the hash with every byte it
 *  writes to /.rockbox/playback.log and keeps the running state in a
 *  sidecar file. The Wrapped/achievements plugins recompute the hash
 *  over the whole family (playback_0001.log ... playback_NNNN.log,
 *  then the live log - the exact order the log reader streams them,
 *  and rename-rotation never changes the concatenated bytes) and
 *  compare. A mismatch means the history was edited by something
 *  other than this device: Wrapped turns green and says so.
 *
 *  This is a tamper *detector*, not tamper *proofing* - anyone with
 *  this header can forge the sidecar. Same spirit as Cookie Clicker's
 *  cheater achievement: it doesn't stop you, it just knows.
 * ============================================================ */

#ifndef PLAYBACK_SIG_H
#define PLAYBACK_SIG_H

#include <stdint.h>
#include <stddef.h>

#define PBL_SIG_PATH   "/.rockbox/playback.sig"
#define PBL_SIG_MAGIC  0x57536731UL          /* "WSg1" */

/* the writer found a chain break it could not attribute to a lost
 * flush and resynced; the break itself is remembered forever */
#define PBL_SIG_TAINTED 0x1

/* The plugins' persistent verdict file. Shared here because the writer
 * consults it too: a missing sig is first-run amnesty ONLY on a device
 * where no sig was ever seen - otherwise deleting the sig would launder
 * an edited log through the writer's re-bless. */
#define CHEAT_DAT_PATH   "/.rockbox/wrapped_cheat.dat"
#define CHEAT_DAT_MAGIC  0x57436831UL   /* "WCh1" */
#define CHEAT_F_SIG_SEEN 0x1   /* a valid sig existed; its absence = cheat */
#define CHEAT_F_CHEATED  0x2   /* tampering was detected at least once     */

struct pbl_sig {
    uint32_t magic;
    uint32_t flags;
    uint64_t nbytes;   /* bytes signed, family concatenated in read order */
    uint64_t hash;     /* pbl_hash_step chain over exactly those bytes   */
};

/* FNV-1a 64 with a non-standard offset basis, so a stock FNV tool run
 * over the file doesn't reproduce the chain */
#define PBL_HASH_BASIS 0x5349454245133700ULL

static inline uint64_t pbl_hash_step(uint64_t h, const void *data, size_t n)
{
    const unsigned char *p = (const unsigned char *)data;
    while (n--) {
        h ^= *p++;
        h *= 0x100000001B3ULL;
    }
    return h;
}

#endif /* PLAYBACK_SIG_H */
