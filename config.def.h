/* See LICENSE file for copyright and license details. */

/* appearance */
#ifndef REFRESH_RATE
#define REFRESH_RATE                          60     /* fallback */
#endif
static const unsigned int refresh_rate        = REFRESH_RATE;   /* matches dwm's mouse event processing to your monitor's refresh rate for smoother window interactions */
static const unsigned int enable_noborder     = 1;   /* toggles noborder feature (0=disabled, 1=enabled) */
static const int cursorwarp                   = 1;   /* 1 means warp cursor to center of focused window/monitor */
static const unsigned int snap                = 26;  /* snap pixel */
static const int swallowfloating              = 1;   /* 1 means swallow floating windows by default */

static const int showbar                      = 1;   /* 1 means show bar - needed for Quickshell space calculation */
static const int topbar                       = 1;   /* 0 means bottom bar */



/* tagging */
static const char *tags[] = { "", "󸀀 ", "", "", "󰕼", "", "", "", "" };


/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 0;    /* 1 means respect size hints in tiled resizals */

static const Layout layouts[] = {
    /* arrange function */
    { tile },    /* first entry is default */
    { NULL },    /* no layout function means floating behavior */
    { monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
