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
