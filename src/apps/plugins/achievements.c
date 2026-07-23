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
 * Achievements - Cookie-Clicker-scale badges from the playback log.
 *
 * Standalone plugin. Reuses Wrapped's parser and rendering toolkit
 * (wrapped_core.h) and feeds every parsed log entry to the achievement
 * tallies through the ACH_HOOK tap. Badge definitions live in ach_table.h,
 * the engine and UI in achievements_core.h.
 ***************************************************************************/

#include "plugin.h"

/* tap into the log parsers before pulling them in */
static void ach_tally(unsigned long ts, bool valid_ts, bool listened,
                      unsigned secs, const char *artist, const char *title);
#define ACH_HOOK(ts, valid_ts, listened, secs, artist, title) \
    ach_tally(ts, valid_ts, listened, secs, artist, title)

#include "wrapped_core.h"
#include "achievements_core.h"

enum plugin_status plugin_start(const void *parameter)
{
    (void)parameter;
    (void)wrapped_show;   /* the header carries all of Wrapped; only the
                             parser + toolkit are used here */
    (void)ach_toast;      /* the banner belongs to the watcher (for now) */

    int st = ach_show();
    if (st == PLUGIN_USB_CONNECTED)
        return PLUGIN_USB_CONNECTED;
    return PLUGIN_OK;
}
