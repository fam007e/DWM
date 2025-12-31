/* appearance */
#include <X11/XF86keysym.h>
static const unsigned int refresh_rate    = 60;
static const unsigned int enable_noborder = 1;
static const unsigned int borderpx        = 1;
static const unsigned int snap            = 26;
static const int swallowfloating          = 1;
static const unsigned int systraypinning  = 0;
static const unsigned int systrayonleft   = 0;
static const unsigned int systrayspacing  = 6;
static const int systraypinningfailfirst  = 1;
static const int showsystray              = 1;
static const int showbar                  = 1;
static const int topbar                   = 1;
#define ICONSIZE                          16
#define ICONSPACING                       6
#define SHOWWINICON                       1
#define SCRIPT(X)                         DWM_PATH "/scripts/" X

/* fonts */
static const char *fonts[] = {
    "MesloLGS Nerd Font Mono:size=13",
    "NotoColorEmoji:pixelsize=13:antialias=true:autohint=true"
};

/* colors */
static const char normbordercolor[]           = "#3b4252";
static const char normbgcolor[]               = "#2e3440";
static const char normfgcolor[]               = "#d8dee9";
static const char selbordercolor[]            = "#88c0d0";
static const char selbgcolor[]                = "#5e81ac";
static const char selfgcolor[]                = "#eceff4";

static const char *colors[][3] = {
    /*               fg           bg           border   */
    [SchemeNorm] = { normfgcolor, normbgcolor, normbordercolor },
    [SchemeSel]  = { selfgcolor,  selbgcolor,  selbordercolor },
};

/* autostart */
static const char *const autostart[] = {
    "xset", "s", "off", NULL,
    "xset", "s", "noblank", NULL,
    "xset", "-dpms", NULL,
    "dbus-update-activation-environment", "--systemd", "--all", NULL,
    "/usr/lib/mate-polkit/polkit-mate-authentication-agent-1", NULL,
    "flameshot", NULL,
    "dunst", NULL,
    "picom", "--vsync", "--animations", "-b", NULL,
    SCRIPT("wallpapersSS"), NULL,
    SCRIPT("status"), NULL,
    "slstatus", NULL,
    "alacritty", NULL,
/*    "protonvpn-app", NULL, */
    NULL /* terminate */
};

/* tagging */
static const char *tags[] = { "", "", "", "", "󰕼", "", "", "", "" };
static const char ptagf[] = "[%s %s]";
static const char etagf[] = "[%s]";
static const int lcaselbl = 0;

static const Rule rules[] = {
    /* class     instance  title           tags mask  isfloating  isterminal  noswallow  monitor */
    { "St",      NULL,     NULL,           0,         0,          1,          0,        -1 },
    { "alacritty",   NULL,     NULL,           0,         0,          1,          0,        -1 },
    { NULL,      NULL,     "Event Tester", 0,         0,          0,          1,        -1 }, /* xev */
};

/* layout(s) */
static const float mfact     = 0.75;
static const int nmaster     = 1;
static const int resizehints = 0;
static const int lockfullscreen = 1;

static const Layout layouts[] = {
    /* symbol     arrange function */
    { "",      tile },    /* first entry is default */
    { "",      NULL },    /* no layout function means floating behavior */
    { "",      monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
    { MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} }

#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }
#define STATUSBAR "dwmblocks"


/* commands */
static const char *launchercmd[]      = { "rofi", "-show", "drun", NULL };
static const char *launcheremojicmd[] = { "rofi", "-show", "emoji", NULL };
static const char *launchercalccmd[]  = { "rofi", "-show", "calc", "-modi", "calc", "-no-show-match", "-no-sort", NULL };
static const char *launchersearchcmd[]= { "sh", "-c", "echo \"\" | rofi -dmenu -p \"Search\" | xargs -I {} xdg-open \"https://duckduckgo.com/?q={}&source=desktop\"", NULL };
static const char *termcmd[]          = { "alacritty", NULL };
static const char *volumeupcmd[]      = { "amixer", "-D", "pulse", "sset", "Master", "5%+", NULL };
static const char *volumedowncmd[]    = { "amixer", "-D", "pulse", "sset", "Master", "5%-", NULL };
static const char *brightnessupcmd[]  = {
    "sh", "-c",
    "xrandr --output $(xrandr --current | grep \" connected\" | cut -f1 -d \" \") --brightness $(echo \"$(xrandr --verbose | grep -m 1 -i brightness | cut -f2 -d ' ') + 0.1\" | bc)",
    NULL
};
static const char *brightnessdowncmd[]= {
    "sh", "-c",
    "xrandr --output $(xrandr --current | grep \" connected\" | cut -f1 -d \" \") --brightness $(echo \"$(xrandr --verbose | grep -m 1 -i brightness | cut -f2 -d ' ') - 0.1\" | bc)",
    NULL
};

static const char *airplanecmd[]      = { "nmcli", "radio", "all", "off", NULL };
static const char *playpausecmd[]     = { "playerctl", "play-pause", NULL };
static const char *nextcmd[]          = { "playerctl", "next", NULL };
static const char *prevcmd[]          = { "playerctl", "previous", NULL };
static const char *stopcmd[]          = { "playerctl", "stop", NULL };
static const char *keybindingscmd[]   = { SCRIPT("keybindings"), NULL };
static const char *bluetoothmenucmd[] = { SCRIPT("bluetoothmenu"), NULL };
static const char *wifimenucmd[]      = { SCRIPT("wifimenu"), NULL };
static const char *powermenucmd[]     = { SCRIPT("powermenu"), NULL };
static const char *togglemutecmd[]    = { "amixer", "-D", "pulse", "sset", "Master", "toggle", NULL };

static Key keys[] = {
    /* modifier                     key            function                argument */
    { MODKEY,                       XK_r,          spawn,                  {.v = launchercmd} },
    { MODKEY|ControlMask,           XK_r,          spawn,                  SHCMD ("protonrestart")},
    { ControlMask,                  XK_e,          spawn,                  {.v = launcheremojicmd} },
    { MODKEY,        	    	    XK_c,          spawn,                  {.v = launchercalccmd} },
    { ControlMask|Mod1Mask,         XK_t,          spawn,                  {.v = termcmd } },
    { MODKEY,                       XK_b,          spawn,                  SHCMD ("brave-browser-nightly")},
    { MODKEY,                       XK_s,          spawn,                  {.v = launchersearchcmd } },
    { MODKEY|ShiftMask,             XK_b,          spawn,                  SHCMD ("tor-browser")},
    { MODKEY,                       XK_p,          spawn,                  {.v = powermenucmd } },
    { MODKEY,                       XK_slash,      spawn,                  {.v = keybindingscmd } },
    { MODKEY|ShiftMask,             XK_p,          spawn,                  SHCMD ("flameshot gui -p ~/Pictures/Screenshots/")},
    { MODKEY|ControlMask,           XK_p,          spawn,                  SHCMD ("flameshot full -p ~/Pictures/Screenshots/")},
    { MODKEY,                       XK_v,          spawn,                  SHCMD ("vlc")},
    { MODKEY,                       XK_l,          spawn,                  SHCMD ("slock")},
    { MODKEY,                       XK_e,          spawn,                  SHCMD ("thunar")},
    { MODKEY,                       XK_z,          spawn,                  SHCMD ("~/.local/bin/zed")},
    { MODKEY,                       XK_a,          spawn,                  SHCMD ("/usr/bin/antigravity")},
    { MODKEY,                       XK_u,          spawn,                  {.v = bluetoothmenucmd}},
    { MODKEY,                       XK_w,          spawn,                  {.v = wifimenucmd } },
    { MODKEY|ControlMask,           XK_w,          spawn,                  SHCMD("feh --randomize --bg-fill ~/Pictures/Wallpapers/*")},
    { MODKEY|ShiftMask,             XK_w,          spawn,                  SHCMD ("looking-glass-client -F")},
    { 0, 			        XF86XK_AudioMute,      spawn,       		   {.v = togglemutecmd } },
    { 0,			      XF86XK_AudioRaiseVolume, spawn, 	               {.v = volumeupcmd } },
    { 0, 			      XF86XK_AudioLowerVolume, spawn,           	   {.v = volumedowncmd } },
    { 0, 			       XF86XK_MonBrightnessUp, spawn,	               {.v = brightnessupcmd } },
    { 0,			     XF86XK_MonBrightnessDown, spawn, 	               {.v = brightnessdowncmd } },
    { 0,                 XF86XK_AudioMicMute,      spawn,                  SHCMD("pactl set-source-mute @DEFAULT_SOURCE@ toggle") },
    { 0,                         XF86XK_WLAN,      spawn,                  {.v = airplanecmd } },
    { 0,                         XF86XK_AudioPlay, spawn,                  {.v = playpausecmd } },
    { 0,                         XF86XK_AudioNext, spawn,                  {.v = nextcmd } },
    { 0,                         XF86XK_AudioPrev, spawn,                  {.v = prevcmd } },
    { 0,                         XF86XK_AudioStop, spawn,                  {.v = stopcmd } },
    { 0,                        XF86XK_Calculator, spawn,                  {.v = launchercalccmd } },
    { MODKEY|ControlMask,           XK_b,          togglebar,              {0} },
    { MODKEY,                       XK_j,          focusstack,             {.i = +1 } },
    { MODKEY,                       XK_k,          focusstack,             {.i = -1 } },
    { MODKEY|ShiftMask,             XK_j,          movestack,              {.i = +1 } },
    { MODKEY|ShiftMask,             XK_k,          movestack,              {.i = -1 } },
    { MODKEY,                       XK_i,          incnmaster,             {.i = +1 } },
    { MODKEY,                       XK_d,          incnmaster,             {.i = -1 } },
    { MODKEY,                       XK_h,          setmfact,               {.f = -0.05} },
    { MODKEY|ControlMask,           XK_l,          setmfact,               {.f = +0.05} },
    { MODKEY|ShiftMask,             XK_h,          setcfact,               {.f = +0.25} },
    { MODKEY|ShiftMask,             XK_l,          setcfact,               {.f = -0.25} },
    { MODKEY|ShiftMask,             XK_o,          setcfact,               {.f =  0.00} },
    { MODKEY,                       XK_Return,     zoom,                   {0} },
    { MODKEY,                       XK_Tab,        view,                   {0} },
    { MODKEY,                       XK_q,          killclient,             {0} },
    { MODKEY,                       XK_t,          setlayout,              {.v = &layouts[0]} },
    { MODKEY,                       XK_f,          setlayout,              {.v = &layouts[1]} },
    { MODKEY,                       XK_m,          setlayout,              {.v = &layouts[2]} },
    { MODKEY,                       XK_space,      setlayout,              {0} },
    { MODKEY|ShiftMask,             XK_space,      togglefloating,         {0} },
    { MODKEY,                       XK_0,          view,                   {.ui = ~0 } },
    { MODKEY|ShiftMask,             XK_0,          tag,                    {.ui = ~0 } },
    { MODKEY,                       XK_comma,      focusmon,               {.i = -1 } },
    { MODKEY,                       XK_period,     focusmon,               {.i = +1 } },
    { MODKEY|ShiftMask,             XK_comma,      tagmon,                 {.i = -1 } },
    { MODKEY|ShiftMask,             XK_period,     tagmon,                 {.i = +1 } },
    { MODKEY|ShiftMask,             XK_n,          togglewarm,             {0} },
    TAGKEYS(                        XK_1,                                  0),
    TAGKEYS(                        XK_2,                                  1),
    TAGKEYS(                        XK_3,                                  2),
    TAGKEYS(                        XK_4,                                  3),
    TAGKEYS(                        XK_5,                                  4),
   	TAGKEYS(                        XK_6,                                  5),
	TAGKEYS(                        XK_7,                                  6),
	TAGKEYS(                        XK_8,                                  7),
	TAGKEYS(                        XK_9,                                  8),
    { MODKEY|ShiftMask,             XK_e,          quit,                   {0} },
};

static Button buttons[] = {
    /* click                event mask      button          function        argument */
    { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
    { ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
    { ClkWinTitle,          0,              Button2,        zoom,           {0} },
    { ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
    { ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
    { ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
    { ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
    { ClkTagBar,            0,              Button1,        view,           {0} },
    { ClkTagBar,            0,              Button3,        toggleview,     {0} },
    { ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
    { ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};
