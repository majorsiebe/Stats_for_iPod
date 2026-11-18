# Spun

*Spotify Wrapped, but it lives on your iPod.*

Written by Siebe Majoor ([@majorsiebe](https://github.com/majorsiebe), AKA rabarber man)  last updated 18-11-2026

**Contains spoilers!** Parts of this plugin are meant to be discovered. If you'd rather not see them immediately, don't expand the spoiler boxes. If you don't care, be my guest!

![the deck in action](screenshots/counter.gif)

> **DISCLAIMER:** I am not a developer, I do hardware, and as a hobby. That means Claude did the code. If you're opposed to that, I recommend skipping this one. You are free to do with the source code as you please, but if you break something, you keep both pieces, and I will not be held responsible.
>
> This plugin was built and tested for/on the iPod Classic 6th and 7th gen. As of 25-07-2026 it also runs on iPod Video (5th/5.5th gen), which has one gotcha you can find in [iPod Video port notes](#ipod-video-port-notes). I do not plan on porting it to any other devices that run Rockbox as I don't have any, so please don't ask. You are welcome to attempt it yourself, and I will be happy to try and merge it, but do not automatically expect a response.

With that out of the way, let me show you some of the features!

## What this is and why it exists

You are looking at a Rockbox plugin that tracks what you listen to and presents it in a nice way on the device itself, instead of needing a computer to check and parse your log. It is inspired by Spotify Wrapped, but has some features I have never seen done like this before.

I got the idea shortly after switching fully to iPods for music and wondering just how much I actually listened. A couple of afternoons of thinking later, a basic version of this came out. It has since been expanded significantly, but instead of wasting your time explaining it in unnecessary detail, let me show you the features.

Put simply, the plugin reads Rockbox's already-existing `playback.log` file and presents it in such a way that normal people can read it. If you already have logging enabled, the plugin will show your stats immediately, with some limitations (more on that in [Known Limitations and Bugs](#known-limitations-and-bugs)). If you don't, the plugin will do that for you automatically. (New!) 

## Features

As of writing this, the deck includes:

- A nice start screen showing stats for the current year
- Time listened, in minutes and hours
- Tracks played
- Top artists
- Top songs
- Top albums
- A listening clock graph, which shows you when you listen most
- The 3am Club:  a card showing how much you listen at night (between midnight and 5 AM)
- A skip report (what it does is self-explanatory)
- Top 5 songs that have never been skipped
- Top 5 songs you used to listen to, but not anymore
- Your Year So Far (YYSF): a year overview sorted by week, showing how far along the current year is, with a bar of every week at a glance. Includes SUPERWEEKs and ULTRAWEEKs (explained below). Can be expanded to get a short summary of a single week!
- A year overview as a heatmap, showing how much you listen on a given day with a coloured square, similar to the GitHub contributions heatmap
- A Listening Type card: there are 16 different ones to collect!
- The **Achievements** tab. This really is its own thing, see below.
- A final card with your year at a glance, which is nice for sharing, contains less information than the year overviews

<details>
<summary><b>Click to see all 16 cards</b> (spoilers, obviously)</summary>

| | |
|---|---|
| ![card 1](screenshots/card_01.png) | ![card 2](screenshots/card_02.png) |
| ![card 3](screenshots/card_03.png) | ![card 4](screenshots/card_04.png) |
| ![card 5](screenshots/card_05.png) | ![card 6](screenshots/card_06.png) |
| ![card 7](screenshots/card_07.png) | ![card 8](screenshots/card_08.png) |
| ![card 9](screenshots/card_09.png) | ![card 10](screenshots/card_10.png) |
| ![card 11](screenshots/card_11.png) | ![card 12](screenshots/card_12.png) |
| ![card 13](screenshots/card_13.png) | ![card 14](screenshots/card_14.png) |
| ![card 15](screenshots/card_15.png) | ![card 16](screenshots/card_16.png) |

</details>

## The non-standard cards, explained

### Your Year So Far (YYSF)

![year overview card](screenshots/card_12.png)

The year overview shows the current year, broken up into calendar weeks. Pressing SELECT highlights the year bar and allows you to scroll between weeks.

- Weeks without activity show up as light grey; weeks yet to come are a transparent grey.
- Weeks with activity are coloured, corresponding to a normal week, a **SUPERWEEK**, an **ULTRAWEEK**, a **HYPERWEEK**, a **GIGAWEEK**, and finally an **OMEGAWEEK**.

| Week type | Listening time | Colour |
|---|---|---|
| Quiet week | under 30 minutes | light grey |
| Normal week | 30 minutes – 23h59 | green |
| SUPERWEEK | 24+ hours (1440+ min) | gold |
| ULTRAWEEK | 48+ hours (2880+ min) | ice blue |
| HYPERWEEK | 72+ hours (4320+ min) | violet |
| GIGAWEEK | 96+ hours (5760+ min) | orange |
| OMEGAWEEK | 120+ hours (7200+ min) | white |



When a week is highlighted, hitting SELECT again opens a card showing that week in more detail: a small time-played counter (similar to the Minutes Listened card), top artist, top song, and total plays and skips. If the week is a **SUPERWEEK**,an **ULTRAWEEK**, a **HYPERWEEK**, a **GIGAWEEK**, or an **OMEGAWEEK**., the card is highlighted. Pressing MENU goes back to the year overview; if the bar is still selected, pressing MENU again returns to scrolling between cards.

### Listening Type

![listening type card](screenshots/card_14.png)

Similar to the Spotify Listening Personality: a 4-letter code based on 4 stats, giving one of 16 listening types.

- **Stat 1: Roamer / Devotee:** how much of your listening goes to your top artist. Lower = Roamer, higher = Devotee.
- **Stat 2:  Explorer / Repeater:** how many of your tracks played are repeats versus new tracks. Lower = Explorer, higher = Repeater.
- **Stat 3: Sampler / Committed:** how many of the tracks you start actually get played to the end instead of skipped. Lower = Sampler, higher = Committed.
- **Stat 4: Daylight / Night Owl:** self-explanatory. How many plays get logged at night (between midnight and 5 AM). Lower = Daylight, higher = Night Owl.

If you'd rather discover the 16 types yourself, do **not** expand this:

<details>
<summary><b>SPOILERS: the full Listening Type list</b></summary>

| Code | Type | |
|---|---|---|
| R E S L | The Free Spirit | no favorite artist, always new tracks, skips freely, daytime |
| R E S N | The Night Scout | same, but after midnight |
| R E C L | The Adventurer | always new music, but plays it through |
| R E C N | The Midnight Wanderer | new music, played through, at 3 AM |
| R F S L | The Channel Hopper | replays familiar stuff but skips constantly |
| R F S N | The Midnight Zapper | late-night skip-happy replayer |
| R F C L | The Comfort Curator | familiar tracks, played fully, no single favorite artist |
| R F C N | The Night Ritualist | that, nocturnally |
| D E S L | The Catalog Sifter | one dominant artist, but digging through new tracks, skipping a lot |
| D E S N | The Night Miner | sifting the catalog after midnight |
| D E C L | The Completionist | one artist, new tracks, everything played through |
| D E C N | The Deep Diver | completionist by night |
| D F S L | The Picky Superfan | one artist on repeat, but choosy about which songs survive |
| D F S N | The Restless Fan | picky superfan, nocturnal edition |
| D F C L | The True Believer | one artist, on repeat, played in full |
| D F C N | The Midnight Devotee | all four maxed |

Letter key: **R**oamer, **D**evotee, **E**xplorer, **F** = repeater, **S**ampler, **C**ommitted, **L** = daylight, **N**ight owl.

</details>

### Achievements

![achievements card](screenshots/card_15.png)

This is one I believe Spotify does not have! My inspiration comes from the popular browser game Cookie Clicker, known for its impossible achievements. As of writing this (July 22nd, 2026) there are 345 to collect, ranging from very simple (1 minute listened) to downright impossible. I won't spoil them here, but the [complete list](ACHIEVEMENTS.md) is available if you are curious.

> **NOTE:** it will know if you tamper with it, and it will call you out. you have been warned. (this sometimes gives false positives. with 1.3 there's a couple improvements to this, and your log gets checked for false positives automatically on first start.) 

To access the Achievements menu, simply **SELECT** its card and the whole list will appear. NEW achievements are highlighted in gold. Secret achievements are usually harder to complete or correspond to a certain action. They stay hidden until unlocked. If you go the tampering route, cheated achievements will show up as green. To get back to the cards, press MENU.

The Achievements card is part of its own plugin set and can technically be disabled if you wish. It includes toasts, similar to what unlocking an achievement on Steam shows. If that doesn't ring any bells, just try it. The rest of the screen freezes for the duration of the toast (2 seconds), and if an achievement triggers while on HOLD, it only shows up once the iPod is taken OFF HOLD. Toast functionality can be tested in settings; the achievement watcher can also be turned OFF there, if you so please.

## Controls & good-to-knows

- To select, press **SELECT** (middle button). To exit or go back, press **MENU**.
- **This plugin has a screenshot feature!** Holding **PLAY/PAUSE** saves a bitmap of the selected card to your device root, named `dump [DATE]-[TIME].bmp`. A feature for saving *all* cards (useful for sharing) exists in the new settings page. Note that this takes a while and cycles through all cards automatically, and on the iPod Video, "a while" means *minutes* (see the port notes).
- Because of the way Rockbox handles logging, forcing a reset (holding MENU + SELECT for 6 seconds) WILL result in log data not saving. Always shut the device down properly (hold PLAY/PAUSE until "Shutting Down" appears) so you don't miss any log entries.
- Enabling the Rockbox Database gives more accurate artist/song names!
- The plugin works best when you don't mess with playback constantly. In my experience, lots of skips, constant pausing, or shutting down randomly can result in an inaccurate log.

## iPod Video port notes

As of 25-07-2026, Spun also runs on the iPod Video (5th/5.5g). These have a slower processor and that changes how a couple things work.

- **Saving cards is SLOW.** The screenshot feature works, but the Video's slower CPU means every card takes a good few seconds to save. A full deck export takes minutes, not seconds. There's a progress counter, so start it, put the iPod down, and have some patience. This is just what the hardware can do, and it will not be "fixed".

Everything else behaves identically to the Classic version.

## Known Limitations and Bugs

- This plugin does not update every second, or even every minute. Give it a while to catch up.
- Achievement earn dates are calculated from your log. in case this cannot be done, unlock date gets replaced by "earned before records began" (it shows this under the relevant achievements)
- Sometimes, the year text does not appear on the start card. I do not know why this happens.
- ~~The "loading..." text overlaps with other announcement texts.~~ <- Should now be fixed! not tested yet. 
- The Top Albums card can render album art in place of the rank numbers. It only reads loose cover files (`cover.jpg`, `folder.jpg`, etc.) inside each album's folder, not embedded tags. Consider this feature untested/unfinished.
- The 7-segment counter animation takes a while and stutters slightly on the real device. Appears to be a hardware limitation.
- Scrolling is a bit sensitive. In my experience you get used to it.
- Song/Album names are not always accurate depending on how they're stored on the device. Simplest fix is labelling correctly. ¯\\\_(ツ)\_/¯
- This plugin works best with flash-modded iPods. During my testing it kept waking up the hard drive which will result in shorter battery life, and drops become more risky. It'll work, it just won't be ideal. 

## Installation

This ships as a full (patched) Rockbox build, not a standalone `.rock` file:

1. Download the zip for **your device** from the [latest release](https://github.com/majorsiebe/Stats_for_iPod/releases/latest): `ipod6g` for the Classic 6th/7th gen, `ipodvideo` for the Video 5th/5.5th gen.
2. Back up your `.rockbox` folder first if you care about it. This zip replaces the main Rockbox binary, so if you run another custom build, it will be overwritten.
3. Extract the zip to the root of your iPod, and if it asks what to do with existing files, replace them. 
4. Eject and reboot the iPod. (or don't eject, if you're the dangerous type.)
5. If you did it correctly, you'll find Spun in the main menu.
6. Logging will be enabled automatically.

WARNING: updating Rockbox the official way (Rockbox Utility or an official build zip) will break it. After any official update, simply reinstall from the release zip here.

## Clock

Spun needs accurate dates to work well. If you don't use the clock on your device, you should set it. If you do, you should check if it's accurate. The plugin ignores data from before 2005. If your plays are not showing up, this is the most likely issue. If your iPod dies fully or you unplug the battery, the RTC will lose its data and you will need to set it again.

Always shut down properly (hold PLAY/PAUSE), as a forced reset can (read: will) cause the last few plays to be lost. 

## Building from source

If you hate yourself and want to build on this, the source can be found in [`src/`](src/), laid out in Rockbox-tree paths. Note that this is Claude's doing and from a couple months ago. I haven't verified it in a while. 

- `src/apps/plugins/`  the plugin code: `wrapped_core.h` (parser + cards), `achievements_core.h` (badge engine), `ach_table.h` (the badge table), and the thin plugin wrappers (`wrapped.c`, `achievements.c`, `stats.c` is in the patch, `achwatch.c`)
- `src/apps/playback_sig.h`  the shared log-signature format
- `src/spun_core.patch`  every change to Rockbox core: playback logging + signing, plugin API additions, the root menu entry, and an LCD hook the achievement toasts use

To build:

1. Clone [Rockbox](https://github.com/Rockbox/rockbox) and check out commit `4c60fe95fc`
2. From the Rockbox tree root: `git apply spun_core.patch`
3. Copy the contents of `src/apps/` over the tree's `apps/` folder.
4. Build the normal Rockbox way for the iPod Classic (`ipod6g`) or iPod Video (`ipodvideo`): standard cross-compilation toolchain, `tools/configure`, `make`, `make zip`, extract the resulting `rockbox.zip` onto the device. The UI simulator target works too and is much less painful for poking around.

