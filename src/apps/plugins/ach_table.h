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

/* ach_table.h - achievement definitions for achievements_core.h.
 * Include only from achievements_core.h. */
#ifndef ACH_TABLE_H
#define ACH_TABLE_H

static const struct ach_def ach_table[] = {
    /* ---- LISTENING TIME ---- */
    { "First Note",             "listen for 1 minute",                    AM_MINUTES, 0, 1 },
    { "Sound Check",            "listen for 5 minutes",                   AM_MINUTES, 0, 5 },
    { "Warming Up",             "listen for 15 minutes",                  AM_MINUTES, 0, 15 },
    { "Side A",                 "listen for 30 minutes - flip it over",   AM_MINUTES, 0, 30 },
    { "Hour of Power",          "listen for 60 minutes",                  AM_MINUTES, 0, 60 },
    { "Double LP",              "listen for 120 minutes",                 AM_MINUTES, 0, 120 },
    { "Five Hour Groove",       "listen for 300 minutes",                 AM_MINUTES, 0, 300 },
    { "Marathon Session",       "listen for 600 minutes",                 AM_MINUTES, 0, 600 },
    { "Kilominute",             "listen for 1,000 minutes",               AM_MINUTES, 0, 1000 },
    { "Two Kilominutes",        "listen for 2,000 minutes",               AM_MINUTES, 0, 2000 },
    { "Ear Conditioning",       "listen for 3,500 minutes",               AM_MINUTES, 0, 3500 },
    { "Audio Devotee",          "listen for 5,000 minutes",               AM_MINUTES, 0, 5000 },
    { "Wall of Sound",          "listen for 7,500 minutes",               AM_MINUTES, 0, 7500 },
    /* WA4b insert at slot 13 (ACH_INS_9000) - the ONE sanctioned
     * mid-table insert; ach_seen_load() remaps older save files */
    { "It's Over 9000!",        "listen for more than 9,000 minutes",     AM_MINUTES, 0, 9001 },
    { "Five Figure Ears",       "listen for 10,000 minutes",              AM_MINUTES, 0, 10000 },
    { "Ten Days Solid",         "listen for 15,000 minutes",              AM_MINUTES, 0, 15000 },
    { "Audiophile Ascendant",   "listen for 25,000 minutes",              AM_MINUTES, 0, 25000 },
    { "The Long Groove",        "listen for 40,000 minutes",              AM_MINUTES, 0, 40000 },
    { "Vinyl Monk",             "listen for 60,000 minutes",              AM_MINUTES, 0, 60000 },
    { "One Hundred Grand",      "listen for 100,000 minutes",             AM_MINUTES, 0, 100000 },
    { "Beyond the Charts",      "listen for 150,000 minutes",             AM_MINUTES, 0, 150000 },
    { "Halfway to Absurd",      "listen for 250,000 minutes",             AM_MINUTES, 0, 250000 },
    { "Touch Grass",            "listen for 400,000 minutes - go outside", AM_MINUTES, ACH_SECRET, 400000 },
    { "The Gladwell Line",      "listen for 600,000 min - 10,000 hours",  AM_MINUTES, ACH_SECRET, 600000 },
    { "Heat Death of the iPod", "listen for 1,000,000 minutes",           AM_MINUTES, ACH_SECRET, 1000000 },

    /* ---- PLAYS ---- */
    { "Press Play",             "finish 1 play",                          AM_PLAYS, 0, 1 },
    { "Hooked",                 "log 5 plays",                            AM_PLAYS, 0, 5 },
    { "EP's Worth",             "log 10 plays",                           AM_PLAYS, 0, 10 },
    { "Mixtape",                "log 25 plays",                           AM_PLAYS, 0, 25 },
    { "Crate Digger",           "log 50 plays",                           AM_PLAYS, 0, 50 },
    { "Century of Songs",       "log 100 plays",                          AM_PLAYS, 0, 100 },
    { "Double Century",         "log 200 plays",                          AM_PLAYS, 0, 200 },
    { "B-Side Devourer",        "log 350 plays",                          AM_PLAYS, 0, 350 },
    { "Five Hundred Spins",     "log 500 plays",                          AM_PLAYS, 0, 500 },
    { "Setlist Veteran",        "log 750 plays",                          AM_PLAYS, 0, 750 },
    { "The Thousandaire",       "log 1,000 plays",                        AM_PLAYS, 0, 1000 },
    { "Fifteen Hundred Deep",   "log 1,500 plays",                        AM_PLAYS, 0, 1500 },
    { "Jukebox Hero",           "log 2,500 plays",                        AM_PLAYS, 0, 2500 },
    { "Four Thousand Holes",    "log 4,000 plays",                        AM_PLAYS, 0, 4000 },
    { "Six Thousand Strong",    "log 6,000 plays",                        AM_PLAYS, 0, 6000 },
    { "Myriad",                 "log 10,000 plays",                       AM_PLAYS, 0, 10000 },
    { "Playback Machine",       "log 15,000 plays",                       AM_PLAYS, 0, 15000 },
    { "Industrial Playback",    "log 25,000 plays",                       AM_PLAYS, 0, 25000 },
    { "Spin Doctor Emeritus",   "log 50,000 plays",                       AM_PLAYS, 0, 50000 },
    { "Playtergeist",           "log 100,000 plays",                      AM_PLAYS, ACH_SECRET, 100000 },
    { "The Algorithm Fears You", "log 250,000 plays",                     AM_PLAYS, ACH_SECRET, 250000 },

    /* ---- SKIPS ---- */
    { "Next!",                  "skip 1 track",                           AM_SKIPS, 0, 1 },
    { "Not Feeling It",         "skip 5 tracks",                          AM_SKIPS, 0, 5 },
    { "Channel Surfer",         "skip 10 tracks",                         AM_SKIPS, 0, 10 },
    { "Picky",                  "skip 25 tracks",                         AM_SKIPS, 0, 25 },
    { "Quality Control",        "skip 50 tracks",                         AM_SKIPS, 0, 50 },
    { "The Curator",            "skip 100 tracks",                        AM_SKIPS, 0, 100 },
    { "Ruthless",               "skip 250 tracks",                        AM_SKIPS, 0, 250 },
    { "Skipper",                "skip 500 tracks",                        AM_SKIPS, 0, 500 },
    { "Kiloskip",               "skip 1,000 tracks",                      AM_SKIPS, 0, 1000 },
    { "Commitment Issues",      "skip 2,500 tracks",                      AM_SKIPS, ACH_SECRET, 2500 },
    { "Why Even Have a Library", "skip 10,000 tracks",                    AM_SKIPS, ACH_SECRET, 10000 },
    { "The Grand Refusal",      "skip 100,000 tracks - but why",          AM_SKIPS, ACH_SECRET, 100000 },

    /* ---- LIBRARY DEPTH ---- */
    { "Track One",              "play 1 distinct song",                   AM_UNIQ_TRACKS, 0, 1 },
    { "Sampler Platter",        "play 5 distinct songs",                  AM_UNIQ_TRACKS, 0, 5 },
    { "Variety Pack",           "play 10 distinct songs",                 AM_UNIQ_TRACKS, 0, 10 },
    { "Getting Around",         "play 25 distinct songs",                 AM_UNIQ_TRACKS, 0, 25 },
    { "Deep Cuts",              "play 50 distinct songs",                 AM_UNIQ_TRACKS, 0, 50 },
    { "Century Collection",     "play 100 distinct songs",                AM_UNIQ_TRACKS, 0, 100 },
    { "Shuffle Fodder",         "play 150 distinct songs",                AM_UNIQ_TRACKS, 0, 150 },
    { "Crate of Crates",        "play 250 distinct songs",                AM_UNIQ_TRACKS, 0, 250 },
    { "The Back Catalog",       "play 400 distinct songs",                AM_UNIQ_TRACKS, 0, 400 },
    { "Six Hundred Songs",      "play 600 distinct songs",                AM_UNIQ_TRACKS, 0, 600 },
    { "Librarian",              "play 1,000 distinct songs",              AM_UNIQ_TRACKS, 0, 1000 },
    { "Discography Diver",      "play 1,500 distinct songs",              AM_UNIQ_TRACKS, 0, 1500 },
    { "The Archivist",          "play 2,500 distinct songs",              AM_UNIQ_TRACKS, 0, 2500 },
    { "Five Thousand Deep",     "play 5,000 distinct songs",              AM_UNIQ_TRACKS, 0, 5000 },
    { "Infinite Jukebox",       "play 10,000 distinct songs",             AM_UNIQ_TRACKS, 0, 10000 },
    { "Alexandria, but Audio",  "play 25,000 distinct songs",             AM_UNIQ_TRACKS, ACH_SECRET, 25000 },

    /* ---- ARTISTS MET ---- */
    { "One Band Fan",           "play 1 distinct artist",                 AM_UNIQ_ARTISTS, 0, 1 },
    { "Power Trio",             "play 3 distinct artists",                AM_UNIQ_ARTISTS, 0, 3 },
    { "Starting Five",          "play 5 distinct artists",                AM_UNIQ_ARTISTS, 0, 5 },
    { "Ten Piece Band",         "play 10 distinct artists",               AM_UNIQ_ARTISTS, 0, 10 },
    { "Festival Lineup",        "play 20 distinct artists",               AM_UNIQ_ARTISTS, 0, 20 },
    { "Genre Tourist",          "play 35 distinct artists",               AM_UNIQ_ARTISTS, 0, 35 },
    { "Radio Rotation",         "play 50 distinct artists",               AM_UNIQ_ARTISTS, 0, 50 },
    { "Scene Historian",        "play 75 distinct artists",               AM_UNIQ_ARTISTS, 0, 75 },
    { "A Hundred Voices",       "play 100 distinct artists",              AM_UNIQ_ARTISTS, 0, 100 },
    { "Label Executive",        "play 150 distinct artists",              AM_UNIQ_ARTISTS, 0, 150 },
    { "Human Discovery Engine", "play 250 distinct artists",              AM_UNIQ_ARTISTS, 0, 250 },
    { "Five Hundred Names",     "play 500 distinct artists",              AM_UNIQ_ARTISTS, 0, 500 },
    { "Ethnomusicologist",      "play 1,000 distinct artists",            AM_UNIQ_ARTISTS, 0, 1000 },
    { "Every Band Ever",        "play 2,500 distinct artists",            AM_UNIQ_ARTISTS, ACH_SECRET, 2500 },

    /* ---- ALBUM SHELF ---- */
    { "First Pressing",         "play 1 distinct album",                  AM_UNIQ_ALBUMS, 0, 1 },
    { "Short Stack",            "play 3 distinct albums",                 AM_UNIQ_ALBUMS, 0, 3 },
    { "Record Shelf",           "play 5 distinct albums",                 AM_UNIQ_ALBUMS, 0, 5 },
    { "Ten LPs Deep",           "play 10 distinct albums",                AM_UNIQ_ALBUMS, 0, 10 },
    { "Collector",              "play 20 distinct albums",                AM_UNIQ_ALBUMS, 0, 20 },
    { "Shelf Two",              "play 35 distinct albums",                AM_UNIQ_ALBUMS, 0, 35 },
    { "Fifty Sleeves",          "play 50 distinct albums",                AM_UNIQ_ALBUMS, 0, 50 },
    { "Vinyl Hoarder",          "play 75 distinct albums",                AM_UNIQ_ALBUMS, 0, 75 },
    { "Hundred Albums",         "play 100 distinct albums",               AM_UNIQ_ALBUMS, 0, 100 },
    { "Warehouse Wing",         "play 200 distinct albums",               AM_UNIQ_ALBUMS, 0, 200 },
    { "Four Hundred Sleeves",   "play 400 distinct albums",               AM_UNIQ_ALBUMS, 0, 400 },
    { "The Stacks",             "play 1,000 distinct albums",             AM_UNIQ_ALBUMS, ACH_SECRET, 1000 },

    /* ---- NIGHT OWL ---- */
    { "Past Bedtime",           "log 1 play between 00:00 and 04:59",     AM_NIGHT_PLAYS, 0, 1 },
    { "Night Shift",            "log 5 night plays (00:00-04:59)",        AM_NIGHT_PLAYS, 0, 5 },
    { "Insomniac Intro",        "log 10 night plays (00:00-04:59)",       AM_NIGHT_PLAYS, 0, 10 },
    { "3 AM Thoughts",          "log 25 night plays (00:00-04:59)",       AM_NIGHT_PLAYS, 0, 25 },
    { "Nocturne",               "log 50 night plays (00:00-04:59)",       AM_NIGHT_PLAYS, 0, 50 },
    { "Creature of the Night",  "log 100 night plays (00:00-04:59)",      AM_NIGHT_PLAYS, 0, 100 },
    { "Vampire Hours",          "log 250 night plays (00:00-04:59)",      AM_NIGHT_PLAYS, 0, 250 },
    { "Sleep Is a Suggestion",  "log 1,000 night plays (00:00-04:59)",    AM_NIGHT_PLAYS, ACH_SECRET, 1000 },
    { "The Sun Is a Lie",       "log 5,000 night plays (00:00-04:59)",    AM_NIGHT_PLAYS, ACH_SECRET, 5000 },

    /* ---- ROUND THE CLOCK ---- */
    { "Three Hour Tour",        "play in 3 distinct hours of the day",    AM_HOURS_COVERED, 0, 3 },
    { "Quarter Day",            "play in 6 distinct hours of the day",    AM_HOURS_COVERED, 0, 6 },
    { "Nine to Five",           "play in 9 distinct hours of the day",    AM_HOURS_COVERED, 0, 9 },
    { "Half the Clock",         "play in 12 distinct hours of the day",   AM_HOURS_COVERED, 0, 12 },
    { "Fifteen Slots",          "play in 15 distinct hours of the day",   AM_HOURS_COVERED, 0, 15 },
    { "Mostly Awake",           "play in 18 distinct hours of the day",   AM_HOURS_COVERED, 0, 18 },
    { "Almost Always On",       "play in 21 distinct hours of the day",   AM_HOURS_COVERED, 0, 21 },
    { "Rock Around the Clock",  "play in all 24 hours of the day",        AM_HOURS_COVERED, 0, 24 },

    /* ---- ACTIVE DAYS ---- */
    { "Day One",                "listen on 1 day",                        AM_ACTIVE_DAYS, 0, 1 },
    { "Three-Day Weekend",      "listen on 3 different days",             AM_ACTIVE_DAYS, 0, 3 },
    { "One Week Wonder",        "listen on 7 different days",             AM_ACTIVE_DAYS, 0, 7 },
    { "Fortnight",              "listen on 14 different days",            AM_ACTIVE_DAYS, 0, 14 },
    { "Three Weeks In",         "listen on 21 different days",            AM_ACTIVE_DAYS, 0, 21 },
    { "Month of Music",         "listen on 30 different days",            AM_ACTIVE_DAYS, 0, 30 },
    { "Fifty Days In",          "listen on 50 different days",            AM_ACTIVE_DAYS, 0, 50 },
    { "Steady Habit",           "listen on 75 different days",            AM_ACTIVE_DAYS, 0, 75 },
    { "Hundred Day Club",       "listen on 100 different days",           AM_ACTIVE_DAYS, 0, 100 },
    { "Routine, Established",   "listen on 150 different days",           AM_ACTIVE_DAYS, 0, 150 },
    { "Two Hundred Days",       "listen on 200 different days",           AM_ACTIVE_DAYS, 0, 200 },
    { "Nine Month Stretch",     "listen on 270 different days",           AM_ACTIVE_DAYS, 0, 270 },
    { "Year-Rounder",           "listen on 365 different days",           AM_ACTIVE_DAYS, 0, 365 },
    { "Five Hundred Days",      "listen on 500 different days",           AM_ACTIVE_DAYS, 0, 500 },
    { "Two Years of Days",      "listen on 730 different days",           AM_ACTIVE_DAYS, 0, 730 },
    { "A Thousand Days",        "listen on 1,000 different days",         AM_ACTIVE_DAYS, 0, 1000 },
    { "Beyond Habit",           "listen on 1,500 different days",         AM_ACTIVE_DAYS, 0, 1500 },
    { "Decade Pace",            "listen on 2,500 different days",         AM_ACTIVE_DAYS, ACH_SECRET, 2500 },
    { "Ten Years After",        "listen on 3,650 different days",         AM_ACTIVE_DAYS, ACH_SECRET, 3650 },

    /* ---- STREAKS ---- */
    { "Back for More",          "keep a 2-day listening streak",          AM_STREAK, 0, 2 },
    { "Hat Trick",              "keep a 3-day listening streak",          AM_STREAK, 0, 3 },
    { "Five Alive",             "keep a 5-day listening streak",          AM_STREAK, 0, 5 },
    { "Seven Day Groove",       "keep a 7-day listening streak",          AM_STREAK, 0, 7 },
    { "Double Digits",          "keep a 10-day listening streak",         AM_STREAK, 0, 10 },
    { "Two Week Streak",        "keep a 14-day listening streak",         AM_STREAK, 0, 14 },
    { "Habit Formed",           "keep a 21-day listening streak",         AM_STREAK, 0, 21 },
    { "Thirty and Counting",    "keep a 30-day listening streak",         AM_STREAK, 0, 30 },
    { "Forty-Five Unbroken",    "keep a 45-day listening streak",         AM_STREAK, 0, 45 },
    { "Sixty Deep",             "keep a 60-day listening streak",         AM_STREAK, 0, 60 },
    { "Quarterly Report",       "keep a 90-day listening streak",         AM_STREAK, 0, 90 },
    { "Four Months Solid",      "keep a 120-day listening streak",        AM_STREAK, 0, 120 },
    { "Half Year, No Misses",   "keep a 180-day listening streak",        AM_STREAK, 0, 180 },
    { "The Full Orbit",         "keep a 365-day listening streak",        AM_STREAK, 0, 365 },
    { "Two Years Unbroken",     "keep a 730-day listening streak",        AM_STREAK, ACH_SECRET, 730 },
    { "Streak Eternal",         "keep a 1,000-day listening streak",      AM_STREAK, ACH_SECRET, 1000 },

    /* ---- ONE TRACK MIND ---- */
    { "Encore",                 "play one song 2 times",                  AM_TRACK_PLAYS, 0, 2 },
    { "On Repeat",              "play one song 5 times",                  AM_TRACK_PLAYS, 0, 5 },
    { "Certified Earworm",      "play one song 10 times",                 AM_TRACK_PLAYS, 0, 10 },
    { "Fifteen Replays",        "play one song 15 times",                 AM_TRACK_PLAYS, 0, 15 },
    { "Comfort Song",           "play one song 25 times",                 AM_TRACK_PLAYS, 0, 25 },
    { "Ritual",                 "play one song 40 times",                 AM_TRACK_PLAYS, 0, 40 },
    { "Anthem Status",          "play one song 60 times",                 AM_TRACK_PLAYS, 0, 60 },
    { "One Hundred Replays",    "play one song 100 times",                AM_TRACK_PLAYS, 0, 100 },
    { "It's Still Good",        "play one song 150 times",                AM_TRACK_PLAYS, 0, 150 },
    { "The Song Owns You",      "play one song 250 times",                AM_TRACK_PLAYS, 0, 250 },
    { "Five Hundred Loops",     "play one song 500 times",                AM_TRACK_PLAYS, 0, 500 },
    { "Monotrack",              "play one song 1,000 times",              AM_TRACK_PLAYS, 0, 1000 },
    { "Duran Duran Duran Duran", "play one song 2,500 times",             AM_TRACK_PLAYS, ACH_SECRET, 2500 },
    { "The Loop Consumes",      "play one song 10,000 times",             AM_TRACK_PLAYS, ACH_SECRET, 10000 },

    /* ---- ARTIST DEVOTION ---- */
    { "Casual Fan",             "play one artist 5 times",                AM_ARTIST_PLAYS, 0, 5 },
    { "Repeat Customer",        "play one artist 10 times",               AM_ARTIST_PLAYS, 0, 10 },
    { "In the Fan Club",        "play one artist 25 times",               AM_ARTIST_PLAYS, 0, 25 },
    { "Merch Table Regular",    "play one artist 50 times",               AM_ARTIST_PLAYS, 0, 50 },
    { "Front Row",              "play one artist 75 times",               AM_ARTIST_PLAYS, 0, 75 },
    { "Superfan",               "play one artist 100 times",              AM_ARTIST_PLAYS, 0, 100 },
    { "Street Team",            "play one artist 150 times",              AM_ARTIST_PLAYS, 0, 150 },
    { "Deep Discography",       "play one artist 250 times",              AM_ARTIST_PLAYS, 0, 250 },
    { "Certified Stan",         "play one artist 400 times",              AM_ARTIST_PLAYS, 0, 400 },
    { "Beyond Stan",            "play one artist 600 times",              AM_ARTIST_PLAYS, 0, 600 },
    { "Parasocial",             "play one artist 1,000 times",            AM_ARTIST_PLAYS, 0, 1000 },
    { "Their Biggest Fan",      "play one artist 2,500 times",            AM_ARTIST_PLAYS, 0, 2500 },
    { "Groupie Emeritus",       "play one artist 5,000 times",            AM_ARTIST_PLAYS, 0, 5000 },
    { "You ARE the Band Now",   "play one artist 20,000 times",           AM_ARTIST_PLAYS, ACH_SECRET, 20000 },

    /* ---- ALBUM DEVOTION ---- */
    { "Front to Back",          "play one album 5 times",                 AM_ALBUM_PLAYS, 0, 5 },
    { "Repeat Pressing",        "play one album 10 times",                AM_ALBUM_PLAYS, 0, 10 },
    { "Liner Notes Memorized",  "play one album 25 times",                AM_ALBUM_PLAYS, 0, 25 },
    { "Album of the Month",     "play one album 50 times",                AM_ALBUM_PLAYS, 0, 50 },
    { "Album of the Year",      "play one album 100 times",               AM_ALBUM_PLAYS, 0, 100 },
    { "Concept Album Convert",  "play one album 150 times",               AM_ALBUM_PLAYS, 0, 150 },
    { "Deluxe Edition",         "play one album 250 times",               AM_ALBUM_PLAYS, 0, 250 },
    { "Anniversary Reissue",    "play one album 400 times",               AM_ALBUM_PLAYS, 0, 400 },
    { "Box Set Devotee",        "play one album 750 times",               AM_ALBUM_PLAYS, 0, 750 },
    { "The Only Album",         "play one album 1,500 times",             AM_ALBUM_PLAYS, 0, 1500 },
    { "Dark Side of the Loop",  "play one album 5,000 times",             AM_ALBUM_PLAYS, ACH_SECRET, 5000 },

    /* ---- LOYALTY ---- */
    { "No Skips",               "play a never-skipped song 3 times",      AM_LOYAL_PLAYS, 0, 3 },
    { "True to You",            "play a never-skipped song 5 times",      AM_LOYAL_PLAYS, 0, 5 },
    { "Ride or Die",            "play a never-skipped song 10 times",     AM_LOYAL_PLAYS, 0, 10 },
    { "Twenty, No Doubts",      "play a never-skipped song 20 times",     AM_LOYAL_PLAYS, 0, 20 },
    { "Unskippable",            "play a never-skipped song 35 times",     AM_LOYAL_PLAYS, 0, 35 },
    { "Fifty and Faithful",     "play a never-skipped song 50 times",     AM_LOYAL_PLAYS, 0, 50 },
    { "Undying Loyalty",        "play a never-skipped song 100 times",    AM_LOYAL_PLAYS, 0, 100 },
    { "Two Hundred, Zero Skips", "play a never-skipped song 200 times",   AM_LOYAL_PLAYS, 0, 200 },
    { "Til Battery Do Us Part", "play a never-skipped song 500 times",    AM_LOYAL_PLAYS, ACH_SECRET, 500 },

    /* ---- SKIP TARGET ---- */
    { "It's Not You",           "skip the same song 3 times",             AM_TRACK_SKIPS, ACH_SECRET, 3 },
    { "It's Definitely You",    "skip the same song 10 times",            AM_TRACK_SKIPS, ACH_SECRET, 10 },
    { "Why Is It Still Here",   "skip the same song 25 times",            AM_TRACK_SKIPS, ACH_SECRET, 25 },
    { "Delete It Already",      "skip the same song 50 times",            AM_TRACK_SKIPS, ACH_SECRET, 50 },
    { "Nemesis Track",          "skip the same song 100 times",           AM_TRACK_SKIPS, ACH_SECRET, 100 },

    /* ---- BIG DAYS ---- */
    { "Light Rotation",         "log 5 plays in one day",                 AM_DAY_PLAYS, 0, 5 },
    { "Ten a Day",              "log 10 plays in one day",                AM_DAY_PLAYS, 0, 10 },
    { "Busy Ears",              "log 25 plays in one day",                AM_DAY_PLAYS, 0, 25 },
    { "Power Listener",         "log 40 plays in one day",                AM_DAY_PLAYS, 0, 40 },
    { "All-Day Rotation",       "log 60 plays in one day",                AM_DAY_PLAYS, 0, 60 },
    { "Ninety in a Day",        "log 90 plays in one day",                AM_DAY_PLAYS, 0, 90 },
    { "Track Gatling",          "log 120 plays in one day",               AM_DAY_PLAYS, 0, 120 },
    { "Human Radio Station",    "log 160 plays in one day",               AM_DAY_PLAYS, 0, 160 },
    { "Playquake",              "log 220 plays in one day",               AM_DAY_PLAYS, 0, 220 },
    { "Three Hundred Special",  "log 300 plays in one day",               AM_DAY_PLAYS, 0, 300 },
    { "The Day the Music Won",  "log 500 plays in one day",               AM_DAY_PLAYS, ACH_SECRET, 500 },

    /* ---- MARATHON DAYS ---- */
    { "Half Hour Day",          "listen 30 minutes in one day",           AM_DAY_MINS, 0, 30 },
    { "Hour Day",               "listen 60 minutes in one day",           AM_DAY_MINS, 0, 60 },
    { "Two Hours Deep",         "listen 120 minutes in one day",          AM_DAY_MINS, 0, 120 },
    { "Four Hour Block",        "listen 240 minutes in one day",          AM_DAY_MINS, 0, 240 },
    { "Six Hour Session",       "listen 360 minutes in one day",          AM_DAY_MINS, 0, 360 },
    { "The Full Workday",       "listen 480 minutes in one day",          AM_DAY_MINS, 0, 480 },
    { "Ten Hour Odyssey",       "listen 600 minutes in one day",          AM_DAY_MINS, 0, 600 },
    { "Half the Day",           "listen 720 minutes in one day",          AM_DAY_MINS, 0, 720 },
    { "Nine Hundred Minutes",   "listen 900 minutes in one day",          AM_DAY_MINS, 0, 900 },
    { "Eighteen Hour Day",      "listen 1,080 minutes in one day",        AM_DAY_MINS, 0, 1080 },
    { "Twenty Hour Bender",     "listen 1,200 minutes in one day",        AM_DAY_MINS, 0, 1200 },
    { "Midnight to Midnight",   "listen 1,440 min in one day - all of it", AM_DAY_MINS, ACH_SECRET, 1440 },

    /* ---- OBSESSION DAYS ---- */
    { "Triple Take",            "play one song 3 times in one day",       AM_TRACK_DAY, 0, 3 },
    { "One More Time",          "play one song 5 times in one day",       AM_TRACK_DAY, 0, 5 },
    { "Eight Is Enough",        "play one song 8 times in one day",       AM_TRACK_DAY, 0, 8 },
    { "Twelve Rounds",          "play one song 12 times in one day",      AM_TRACK_DAY, ACH_SECRET, 12 },
    { "Song Hostage Situation", "play one song 20 times in one day",      AM_TRACK_DAY, ACH_SECRET, 20 },
    { "The Wall Is Melting",    "play one song 35 times in one day",      AM_TRACK_DAY, ACH_SECRET, 35 },
    { "Sixty Times Today",      "play one song 60 times in one day",      AM_TRACK_DAY, ACH_SECRET, 60 },
    { "One Song, One Day, 100x", "play one song 100 times in one day",    AM_TRACK_DAY, ACH_SECRET, 100 },

    /* ---- SKIP SPREE ---- */
    { "Rapid Fire",             "skip 3 tracks in a row",                 AM_SKIP_RUN, ACH_SECRET, 3 },
    { "Nothing Sounds Right",   "skip 5 tracks in a row",                 AM_SKIP_RUN, ACH_SECRET, 5 },
    { "Decision Paralysis",     "skip 10 tracks in a row",                AM_SKIP_RUN, ACH_SECRET, 10 },
    { "Scorched Playlist",      "skip 20 tracks in a row",                AM_SKIP_RUN, ACH_SECRET, 20 },
    { "The Great Purge",        "skip 40 tracks in a row",                AM_SKIP_RUN, ACH_SECRET, 40 },
    { "Eighty and Nothing",     "skip 80 tracks in a row - just stop",    AM_SKIP_RUN, ACH_SECRET, 80 },

    /* ---- FLOW STATE ---- */
    { "Letting It Ride",        "play 5 tracks without skipping",         AM_PLAY_RUN, 0, 5 },
    { "Trust the Queue",        "play 10 tracks without skipping",        AM_PLAY_RUN, 0, 10 },
    { "Twenty, No Touching",    "play 20 tracks without skipping",        AM_PLAY_RUN, 0, 20 },
    { "Zen Shuffle",            "play 35 tracks without skipping",        AM_PLAY_RUN, 0, 35 },
    { "Fifty Straight",         "play 50 tracks without skipping",        AM_PLAY_RUN, 0, 50 },
    { "Deep Flow",              "play 75 tracks without skipping",        AM_PLAY_RUN, 0, 75 },
    { "Hundred Track Trance",   "play 100 tracks without skipping",       AM_PLAY_RUN, 0, 100 },
    { "The Long Haul",          "play 150 tracks without skipping",       AM_PLAY_RUN, 0, 150 },
    { "Playlist Pacifist",      "play 250 tracks without skipping",       AM_PLAY_RUN, 0, 250 },
    { "Five Hundred Serene",    "play 500 tracks without skipping",       AM_PLAY_RUN, 0, 500 },
    { "The Skip Button Rusts",  "play 1,000 tracks without skipping",     AM_PLAY_RUN, ACH_SECRET, 1000 },

    /* ---- BIG WEEKS ---- */
    { "Weekly Warmup",          "listen 60 minutes in one week",          AM_WEEK_MINS, 0, 60 },
    { "Three Hour Week",        "listen 180 minutes in one week",         AM_WEEK_MINS, 0, 180 },
    { "An Hour a Day",          "listen 420 minutes in one week",         AM_WEEK_MINS, 0, 420 },
    { "Two a Day",              "listen 840 minutes in one week",         AM_WEEK_MINS, 0, 840 },
    { "SUPERWEEK",              "listen 1,440 minutes in one week",       AM_WEEK_MINS, 0, 1440 },
    { "Beyond Super",           "listen 2,016 minutes in one week",       AM_WEEK_MINS, 0, 2016 },
    { "ULTRAWEEK",              "listen 2,880 minutes in one week",       AM_WEEK_MINS, 0, 2880 },
    { "HYPERWEEK",              "listen 4,320 minutes in one week",       AM_WEEK_MINS, 0, 4320 },
    { "GIGAWEEK",               "listen 5,760 minutes in one week",       AM_WEEK_MINS, 0, 5760 },
    { "OMEGAWEEK",              "listen 7,200 minutes in one week",       AM_WEEK_MINS, 0, 7200 },
    { "Sleepless in Stereo",    "listen 8,640 minutes in one week",       AM_WEEK_MINS, ACH_SECRET, 8640 },
    { "THE ETERNAL WEEK",       "listen 10,080 minutes in one week - all", AM_WEEK_MINS, ACH_SECRET, 10080 },

    /* ---- SUPERWEEKS ---- */
    { "First Superweek",        "log 1 superweek (1,440+ min)",           AM_SUPERWEEKS, 0, 1 },
    { "Super Again",            "log 2 superweeks (1,440+ min)",          AM_SUPERWEEKS, 0, 2 },
    { "Super Trilogy",          "log 3 superweeks (1,440+ min)",          AM_SUPERWEEKS, 0, 3 },
    { "Five Supers",            "log 5 superweeks (1,440+ min)",          AM_SUPERWEEKS, 0, 5 },
    { "Super Eight",            "log 8 superweeks (1,440+ min)",          AM_SUPERWEEKS, 0, 8 },
    { "Twelve Weeks of Power",  "log 12 superweeks (1,440+ min)",         AM_SUPERWEEKS, 0, 12 },
    { "Twenty Supers",          "log 20 superweeks (1,440+ min)",         AM_SUPERWEEKS, 0, 20 },
    { "Thirty Supers",          "log 30 superweeks (1,440+ min)",         AM_SUPERWEEKS, 0, 30 },
    { "A Super Year",           "log 52 superweeks (1,440+ min)",         AM_SUPERWEEKS, 0, 52 },
    { "Two Years, All Super",   "log 104 superweeks (1,440+ min)",        AM_SUPERWEEKS, ACH_SECRET, 104 },

    /* ---- ULTRAWEEKS ---- */
    { "First Ultraweek",        "log 1 ultraweek (2,880+ min)",           AM_ULTRAWEEKS, 0, 1 },
    { "Ultra Again",            "log 2 ultraweeks (2,880+ min)",          AM_ULTRAWEEKS, 0, 2 },
    { "Ultra Trilogy",          "log 3 ultraweeks (2,880+ min)",          AM_ULTRAWEEKS, 0, 3 },
    { "Penta Ultra",            "log 5 ultraweeks (2,880+ min)",          AM_ULTRAWEEKS, 0, 5 },
    { "Deca Ultra",             "log 10 ultraweeks (2,880+ min)",         AM_ULTRAWEEKS, 0, 10 },
    { "Twenty Ultras",          "log 20 ultraweeks (2,880+ min)",         AM_ULTRAWEEKS, 0, 20 },
    { "The Ultra Year",         "log 52 ultraweeks (2,880+ min)",         AM_ULTRAWEEKS, ACH_SECRET, 52 },

    /* ---- CONSISTENCY ---- */
    { "One Good Week",          "log 1 active week (30+ min)",            AM_ACTIVE_WEEKS, 0, 1 },
    { "Week Two",               "log 2 active weeks (30+ min)",           AM_ACTIVE_WEEKS, 0, 2 },
    { "One Month Steady",       "log 4 active weeks (30+ min)",           AM_ACTIVE_WEEKS, 0, 4 },
    { "Two Months Running",     "log 8 active weeks (30+ min)",           AM_ACTIVE_WEEKS, 0, 8 },
    { "A Season of Sound",      "log 13 active weeks (30+ min)",          AM_ACTIVE_WEEKS, 0, 13 },
    { "Half Year Regular",      "log 26 active weeks (30+ min)",          AM_ACTIVE_WEEKS, 0, 26 },
    { "Three Quarters In",      "log 39 active weeks (30+ min)",          AM_ACTIVE_WEEKS, 0, 39 },
    { "The Whole Year",         "log 52 active weeks (30+ min)",          AM_ACTIVE_WEEKS, 0, 52 },
    { "Eighteen Months",        "log 78 active weeks (30+ min)",          AM_ACTIVE_WEEKS, 0, 78 },
    { "Two Year Regular",       "log 104 active weeks (30+ min)",         AM_ACTIVE_WEEKS, 0, 104 },
    { "Three Year Fixture",     "log 156 active weeks (30+ min)",         AM_ACTIVE_WEEKS, 0, 156 },
    { "Five Year Plan",         "log 260 active weeks (30+ min)",         AM_ACTIVE_WEEKS, 0, 260 },
    { "Decade Subscriber",      "log 520 active weeks (30+ min)",         AM_ACTIVE_WEEKS, ACH_SECRET, 520 },

    /* ---- PERFECT MONTHS ---- */
    { "Perfect Month",          "1 month with every single day active",   AM_PERFECT_MONTHS, 0, 1 },
    { "Perfect Pair",           "2 months with every single day active",  AM_PERFECT_MONTHS, 0, 2 },
    { "Perfect Quarter",        "3 months with every single day active",  AM_PERFECT_MONTHS, 0, 3 },
    { "Four Flawless",          "4 months with every single day active",  AM_PERFECT_MONTHS, 0, 4 },
    { "Half Year of Halos",     "6 months with every single day active",  AM_PERFECT_MONTHS, 0, 6 },
    { "Nine Perfect Months",    "9 months with every single day active",  AM_PERFECT_MONTHS, 0, 9 },
    { "The Perfect Year",       "12 months with every single day active", AM_PERFECT_MONTHS, 0, 12 },
    { "Eighteen Immaculate",    "18 months with every single day active", AM_PERFECT_MONTHS, 0, 18 },
    { "Two Years Untouched",    "24 months with every single day active", AM_PERFECT_MONTHS, 0, 24 },
    { "Four Years, No Rest",    "48 months with every single day active", AM_PERFECT_MONTHS, ACH_SECRET, 48 },

    /* ---- RED LETTER DAYS ---- */
    { "Auld Lang Sync",         "play music during 00:00-00:59 on jan 1", AM_F_NEWYEAR,   ACH_SECRET, 1 },
    { "Last Christmas",         "play music on december 25",              AM_F_XMAS,      ACH_SECRET, 1 },
    { "Spooky Scary Playback",  "play music on october 31",               AM_F_HALLOWEEN, ACH_SECRET, 1 },
    { "Unlucky Groove",         "play music on a friday the 13th",        AM_F_FRIDAY13,  ACH_SECRET, 1 },
    { "Stroke of Midnight",     "log a play in a day's first minute",     AM_F_MIDNIGHT,  ACH_SECRET, 1 },
    { "Bonus Level",            "play music on february 29",              AM_F_LEAPDAY,   ACH_SECRET, 1 },

    /* ---- ??? ---- */
    { "It Works!",              "you will know it when you see it",       AM_F_TOAST, 0, 1 },

    /* ---- LISTENING TYPES (secret; profile re-typed after every play
     *      once past 100 plays, so types passed through still count) ---- */
    { "The Free Spirit",        "be typed as the free spirit",            AM_TYPE0,  ACH_SECRET, 1 },
    { "The Night Scout",        "be typed as the night scout",            AM_TYPE1,  ACH_SECRET, 1 },
    { "The Adventurer",         "be typed as the adventurer",             AM_TYPE2,  ACH_SECRET, 1 },
    { "The Midnight Wanderer",  "be typed as the midnight wanderer",      AM_TYPE3,  ACH_SECRET, 1 },
    { "The Channel Hopper",     "be typed as the channel hopper",         AM_TYPE4,  ACH_SECRET, 1 },
    { "The Midnight Zapper",    "be typed as the midnight zapper",        AM_TYPE5,  ACH_SECRET, 1 },
    { "The Comfort Curator",    "be typed as the comfort curator",        AM_TYPE6,  ACH_SECRET, 1 },
    { "The Night Ritualist",    "be typed as the night ritualist",        AM_TYPE7,  ACH_SECRET, 1 },
    { "The Catalog Sifter",     "be typed as the catalog sifter",         AM_TYPE8,  ACH_SECRET, 1 },
    { "The Night Miner",        "be typed as the night miner",            AM_TYPE9,  ACH_SECRET, 1 },
    { "The Completionist",      "be typed as the completionist",          AM_TYPE10, ACH_SECRET, 1 },
    { "The Deep Diver",         "be typed as the deep diver",             AM_TYPE11, ACH_SECRET, 1 },
    { "The Picky Superfan",     "be typed as the picky superfan",         AM_TYPE12, ACH_SECRET, 1 },
    { "The Restless Fan",       "be typed as the restless fan",           AM_TYPE13, ACH_SECRET, 1 },
    { "The True Believer",      "be typed as the true believer",          AM_TYPE14, ACH_SECRET, 1 },
    { "The Midnight Devotee",   "be typed as the midnight devotee",       AM_TYPE15, ACH_SECRET, 1 },

    /* ---- TYPE COLLECTOR ---- */
    { "Two-Faced",              "collect 2 listening types",              AM_TYPES_COLLECTED, 0, 2 },
    { "Phase Shifter",          "collect 4 listening types",              AM_TYPES_COLLECTED, 0, 4 },
    { "Mood Ring",              "collect 8 listening types",              AM_TYPES_COLLECTED, 0, 8 },
    { "Identity Crisis",        "collect 12 listening types",             AM_TYPES_COLLECTED, 0, 12 },
    { "All Of Them At Once",    "collect all 16 listening types",         AM_TYPES_COLLECTED, 0, 16 },

#ifdef ACH_PERSONAL
    /* one personal badge, nobody else's business - see ach_personal.h
       (untracked; public builds simply do not have this row) */
    ACH_PERSONAL_ROW
#endif

    /* ---- META ---- */
    { "Achievement Achieved",   "unlock 10 other achievements",           AM_META, 0, 10 },
    { "Plaque Collector",       "unlock 25 other achievements",           AM_META, 0, 25 },
    { "Fifty Trophies",         "unlock 50 other achievements",           AM_META, 0, 50 },
    { "Wall of Fame",           "unlock 100 other achievements",          AM_META, 0, 100 },
    { "Cabinet Overflow",       "unlock 150 other achievements",          AM_META, 0, 150 },
    { "Two Hundred Medals",     "unlock 200 other achievements",          AM_META, 0, 200 },
    { "Completionist Rising",   "unlock 250 other achievements",          AM_META, 0, 250 },
    { "All This For Music",     "unlock 280 other achievements",          AM_META, 0, 280 },

    /* ---- the last row of the wall, deliberately NOT secret: everyone
     * gets to know they are being watched. Persistence keys on TABLE
     * ORDER: only ever append below this line, never insert above.
     * (Sole exception so far: the "It's Over 9000!" slot-13 insert,
     * which shipped WITH the WA4b magic bump + on-load remap in
     * ach_seen_load(). No insert without that same ceremony.) ---- */
    { "Cheated songs sound awful", "you know what you did",               AM_F_CHEAT, 0, 1 },

    /* ---- the test toast button saga (appended below the line, as the
     * law above commands) ---- */
    { "Did you expect something to change?",
                                "fire two test toasts inside one minute", AM_TOAST_BURST, ACH_SECRET, 1 },
    { "Seriously, what do you think is going to happen?",
                                "fire the test toast five times",         AM_TOAST_TOTAL, ACH_SECRET, 5 },
    { "Toast addict",           "ten test toasts. it's still just a test", AM_TOAST_TOTAL, ACH_SECRET, 10 },
};

#endif
