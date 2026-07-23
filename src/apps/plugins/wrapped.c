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
 * Wrapped - Spotify-Wrapped-style listening stats from the playback log,
 * with the achievements deck merged in (CARD_ACH: gold summary card,
 * SELECT opens the full badge browser).
 *
 * Standalone plugin build; the stats implementation is shared with the
 * Apple-style menu (stats.c) via wrapped_core.h, the badge engine with
 * achievements.rock/achwatch.rock via achievements_core.h.
 ***************************************************************************/

#include "plugin.h"

/* feed the achievement tallies from the log parsers */
static void ach_tally(unsigned long ts, bool valid_ts, bool listened,
                      unsigned secs, const char *artist, const char *title);
#define ACH_HOOK(ts, valid_ts, listened, secs, artist, title) \
    ach_tally(ts, valid_ts, listened, secs, artist, title)

#define WRAPPED_WITH_ACH        /* adds CARD_ACH to the deck */

#include "wrapped_core.h"
#include "achievements_core.h"

enum plugin_status plugin_start(const void *parameter)
{
    (void)parameter;
    (void)ach_show;             /* standalone-list entry, unused here */
    (void)ach_toast;            /* the banner belongs to the watcher  */

    int st = wrapped_show();
    if (st == PLUGIN_USB_CONNECTED)
        return PLUGIN_USB_CONNECTED;
    return PLUGIN_OK;
}
