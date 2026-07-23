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
