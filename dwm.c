/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <time.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <xcb/res.h>

#include "drw.h"
#include "util.h"
#include "tomlparser.h"

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         ((mask) & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->mx+(m)->mw) - MAX((x),(m)->mx)) \
                               * MAX(0, MIN((y)+(h),(m)->my+(m)->mh) - MAX((y),(m)->my)))
#define INTERSECTC(x,y,w,h,z)   (MAX(0, MIN((x)+(w),(z)->x+(z)->w) - MAX((x),(z)->x)) \
                               * MAX(0, MIN((y)+(h),(z)->y+(z)->h) - MAX((y),(z)->y)))
#define ISVISIBLE(C)            ((C)->tags & (C)->mon->tagset[(C)->mon->seltags])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1u << LENGTH(tags)) - 1)
#define NTAGS                   ((int)LENGTH(tags))

/* enums */
/* the four resize cursors must stay first: resizemouse() indexes them by corner */
enum { CurResizeBR, CurResizeBL, CurResizeTR, CurResizeTL, CurNormal, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetWMWindowTypeDock, NetClientList, NetDesktopNames, NetDesktopViewport,
       NetNumberOfDesktops, NetCurrentDesktop, NetWMDesktop, NetDwmMonitorDesktops, NetDwmSelectedMonitor,
       NetWMStrut, NetWMStrutPartial, NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
/* clicks dwm itself can see: the bar belongs to Quickshell, so it never reports clicks to dwm */
enum { ClkClientWin, ClkRootWin, ClkLast }; /* clicks */

#include "types.h"
#include "effects.h"

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	float cfact;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen, isterminal, noswallow;
	int issteam;
	int fakefullscreen;
	pid_t pid;
	Client *next;
	Client *snext;
	Client *swallowing;
	Monitor *mon;
	Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	Arg arg;
} Key;

typedef struct {
	void (*arrange)(Monitor *); /* NULL means floating */
} Layout;

typedef struct Pertag Pertag;
struct Monitor {
	float mfact;
	int nmaster;
	int num;
	int by, bh;           /* bar geometry */
	int tx, tw;           /* bar tray geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	int showbar;
	int topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
	Window traywin;
	const Layout *lt[2];
	Pertag *pertag;
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	int isfloating;
	int isterminal;
	int noswallow;
	int monitor;
} Rule;

typedef void (*Func)(const Arg *);

/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachbottom(Client *c);
static void attachstack(Client *c);
static Arg build_arg(const char *func_name, const TomlDoc *doc,
          const char *section, int tidx);
static Arg build_spawn_arg(const TomlDoc *doc, const char *section, int tidx);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void enternotify(XEvent *e);
static void expand_var_to(const char *src, const TomlDoc *doc, char *dst, size_t dstsz);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static void fullscreen(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static int getdatadir(char *buf, size_t size);
static void getfacts(Monitor *m, int msize, int ssize, float *mf, float *sf, int *mr, int *sr);
static int getmonlogicalindex(Monitor *target);
static unsigned int getmontagmask(int monnum);
static pid_t getparentprocess(pid_t p);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static Atom getwinatomprop(Window win, Atom prop);
static int getwinstrut(Window win, int *top, int *bottom);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void initmonitortags(void);
static int inotify_dispatch(void);
static int isaltbar(Window win);
static int isclass(Window win, const char *class);
static int isdescprocess(pid_t p, pid_t c);
#ifdef XINERAMA
static int isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info);
#endif
static int isvisiblefullscreen(Client *c);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void load_hotkeys_toml(const char *user_path, const char *default_path);
static void load_rules_toml(const char *user_path, const char *default_path);
static void load_themes_toml(const char *user_path, const char *default_path);
static unsigned int lookup_click(const char *name);
static Func lookup_func(const char *name);
static void manage(Window w, XWindowAttributes *wa);
static void managealtbar(Window win, XWindowAttributes *wa);
static void managetray(Window win, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static int monitorhasfullscreen(Monitor *m);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static void moveorplace(const Arg *arg);
static void movestack(const Arg *arg);
static Client *nexttiled(Client *c);
static int noborder(Client *c);
static void notify_bad_config(const char *filename, const char *reason);
static int parse_hexcolor(const TomlValue *v, char *dest /* 8 bytes */);
static unsigned int parse_mod_mask(const TomlValue *v);
static unsigned int parse_single_mod(const char *s);
static void placemouse(const Arg *arg);
static void pop(Client *c);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Client *recttoclient(int x, int y, int w, int h);
static Monitor *recttomon(int x, int y, int w, int h);
static void refreshbar(void);
static void reload_config(void);
static void remaptags(void);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void restack(Monitor *m);
static void run(void);
static void runautostart(void);
static void scan(void);
static void scanaltbars(void);
static void scantray(void);
static int sendevent(Client *c, Atom proto);
static void sendmon(Client *c, Monitor *m);
static void setcfact(const Arg *arg);
static void setclientdesktop(Client *c);
static void setclientstate(Client *c, long state);
static void setdesktopnames(void);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setnumdesktops(void);
static void setup(void);
static void setup_inotify(void);
static void seturgent(Client *c, int urg);
static void setviewport(void);
static void showhide(Client *c);
static void sigchld(int unused);
static void sigusr1_handler(int sig);
static void spawn(const Arg *arg);
static int swallow(Client *p, Client *c);
static Client *swallowingclient(Window w);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static Client *termforwin(const Client *w);
static void tile(Monitor *m);
static void togglebar(const Arg *arg);
static void togglefakefullscreen(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void *toml_alloc(size_t sz);
static const TomlDoc *toml_load(const char *what, const char *user_path, const char *default_path);
static void unfocus(Client *c, int revert);
static void unmanage(Client *c, int destroyed);
static void unmanagealtbar(Window w);
static void unmanagetray(Window w);
static void unmapnotify(XEvent *e);
static void unswallow(Client *c);
static int updatealtbar(Monitor *m, Window win, XWindowAttributes *wa);
static void updatebarpos(Monitor *m);
static void updateclientlist(void);
static void updatecurrentdesktop(void);
static void updatefullscreenmonitors(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updateselectedmonitor(void);
static void updatesizehints(Client *c);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static void warppointer(Client *c);
static pid_t winpid(Window w);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int wmclasscontains(Window win, const char *class, const char *name);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);


/* variables */
static const char autostartsh[] = "scripts/autostart.sh";
static const char broken[] = "broken";
static const char dwmdir[] = "dwm";
static const char localshare[] = ".local/share";
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh;               /* bar height (follows the external bar once it is seen) */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast];
static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root;
static Window wmcheckwin;
static xcb_connection_t *xcon;

static int selectedmonitorcache;
static int selectedmonitorcachevalid;
static Atom dwmfullscreenmonitorsatom, dwmtagupdateatom;
static unsigned long tagupdatesequence;

/* Quickshell / External dock integration (always enabled) */
static const char *altbarclass = "quickshell";
static const char *alttrayname = "tray";

/* configuration, allows nested code to access above variables */
#include "config.h"

/* ── Hot-reload runtime state ─────────────────────────────── */
/* Fallback colours (Nord): also used for any colour themes.toml omits or gets wrong.
 * Order per scheme: fg, bg, border. */
static const char *defclrs[2][3] = {
	{ "#D8DEE9", "#434C5E", "#3B4252" },  /* SchemeNorm */
	{ "#ECEFF4", "#434C5E", "#81A1C1" },  /* SchemeSel  */
};
/* Runtime keybindings loaded from hotkeys.toml */
static Key          *rt_keys  = NULL;
static int           rt_nkeys = 0;
/* Runtime window rules loaded from window-rules.toml */
#define TOML_RULES_MAX 64
static Rule          rt_rules_buf[TOML_RULES_MAX];
static char          rt_rules_strbuf[TOML_RULES_MAX * 3][TOML_MAX_STR];
static Rule         *rt_rules  = NULL;
static int           rt_nrules = 0;
/* Runtime mouse buttons loaded from hotkeys.toml [[buttons]] */
#define TOML_BUTTONS_MAX 32
static Button        rt_buttons_buf[TOML_BUTTONS_MAX];
static Button       *rt_buttons  = NULL;
static int           rt_nbuttons = 0;
/* Runtime border width (overridden by themes.toml) */
static unsigned int  dyn_borderpx;
/* Config file locations: user config first, shipped defaults as fallback */
static char          toml_hotkeys_path[PATH_MAX];
static char          toml_themes_path[PATH_MAX];
static char          toml_rules_path[PATH_MAX];
static char          toml_hotkeys_default_path[PATH_MAX];
static char          toml_themes_default_path[PATH_MAX];
static char          toml_rules_default_path[PATH_MAX];
/* inotify watches the config directories; SIGUSR1 reaches run() through a
 * self-pipe so that a signal arriving just before select() is never lost */
static int           inotify_fd = -1;
static int           sigpipe[2] = { -1, -1 };
/* Arena for TOML-loaded spawn argv data. Two halves: a reload fills the idle
 * half and only switches to it once complete, so a failed reload leaves the
 * running bindings untouched. */
#define TOML_ARENA_CAP 65536u
static char   toml_arena_buf[2][TOML_ARENA_CAP] __attribute__((aligned(16)));
static int    toml_arena_idx = 0;   /* half currently in use / being filled */
static size_t toml_arena_pos = 0;
/* ─────────────────────────────────────────────────────────── */

struct Pertag {
	unsigned int curtag, prevtag; /* current and previous tag */
	int nmasters[LENGTH(tags) + 1]; /* number of windows in master area */
	float mfacts[LENGTH(tags) + 1]; /* mfacts per tag */
	unsigned int sellts[LENGTH(tags) + 1]; /* selected layouts */
	const Layout *ltidxs[LENGTH(tags) + 1][2]; /* matrix of tags and layouts indexes  */
	int showbars[LENGTH(tags) + 1]; /* display bar for the current tag */
};

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

/* function implementations */
void
applyrules(Client *c)
{
	const char *class, *instance;
	unsigned int i, montags;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	c->isfloating = 0;
	c->tags = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	if (strstr(class, "Steam") || strstr(class, "steam_app_"))
		c->issteam = 1;

	for (i = 0; i < (unsigned int)rt_nrules; i++) {
		r = &rt_rules[i];
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance)))
		{
			c->isterminal = r->isterminal;
			c->noswallow  = r->noswallow;
			c->isfloating = r->isfloating;
			c->tags |= r->tags;
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);

	/* no tag from the rules: show the client on the monitor's current view */
	c->tags &= TAGMASK;
	if (!c->tags)
		c->tags = c->mon->tagset[c->mon->seltags];

	/* keep only tags this monitor owns; if none are left use its first tag */
	montags = getmontagmask(c->mon->num);
	if (c->tags & montags)
		c->tags &= montags;
	else
		c->tags = montags & -montags;
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m)
{
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

void
attach(Client *c)
{
	c->next = c->mon->clients;
	c->mon->clients = c;
}

/* 1 if the window's WM_CLASS class contains the given string */
static int
isclass(Window win, const char *class)
{
	XClassHint ch = { NULL, NULL };
	int match = 0;

	if (XGetClassHint(dpy, win, &ch))
		match = ch.res_class && strstr(ch.res_class, class);
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	return match;
}

/* Attach c after the last client that is not a "chatterino" window, so
 * chatterino windows always stay at the end of the client list. */
void
attachbottom(Client *c)
{
	Client *i, *last = NULL;

	for (i = c->mon->clients; i; i = i->next)
		if (!isclass(i->win, "chatterino"))
			last = i;
	if (last) {
		c->next = last->next;
		last->next = c;
	} else
		attach(c);
}

void
attachstack(Client *c)
{
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
buttonpress(XEvent *e)
{
	unsigned int i, click = ClkRootWin;
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	/* focus monitor if necessary */
	if ((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	if ((c = wintoclient(ev->window))) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
	for (i = 0; i < (unsigned int)rt_nbuttons; i++)
		if (click == rt_buttons[i].click && rt_buttons[i].func
		&& rt_buttons[i].button == ev->button
		&& CLEANMASK(rt_buttons[i].mask) == CLEANMASK(ev->state))
			rt_buttons[i].func(&rt_buttons[i].arg);
}

void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void)
{
	Monitor *m;
	Layout foo = { NULL };
	size_t i;

	running = 0; /* keep unmanage() from warping the pointer while we tear down */
	selmon->lt[selmon->sellt] = &foo;
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < 2; i++)
		drw_scm_free(drw, scheme[i], 3);
	free(scheme);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	if (inotify_fd >= 0)
		close(inotify_fd);
	if (sigpipe[0] >= 0) {
		close(sigpipe[0]);
		close(sigpipe[1]);
	}
}

void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	/* Quickshell manages its own windows; nothing to destroy here */
	free(mon);
}

void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);
	Monitor *m;
	unsigned long desktop;

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if ((Atom)cme->data.l[1] == netatom[NetWMFullscreen]
		|| (Atom)cme->data.l[2] == netatom[NetWMFullscreen]) {
			if (c->fakefullscreen == 2 && c->isfullscreen)
				c->fakefullscreen = 3;
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
		}
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	} else if (cme->message_type == netatom[NetWMDesktop]) {
		/* external request to move the window to another desktop (= tag) */
		desktop = (unsigned long)cme->data.l[0] & 0xFFFFFFFFul;
		if (desktop < LENGTH(tags)) {
			/* the monitor that owns this tag takes the window */
			for (m = mons; m && !((1u << desktop) & getmontagmask(m->num)); m = m->next);
			if (m && c->mon != m)
				sendmon(c, m);
			c->tags = 1u << desktop;
		} else if (desktop == 0xFFFFFFFFul) {
			/* sticky: shown on every tag of the window's monitor */
			c->tags = getmontagmask(c->mon->num);
		} else
			return;
		setclientdesktop(c);
		updateclientlist();
		arrange(NULL);
		focus(NULL);
	}
}

void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;

	if (noborder(c) && enable_noborder) {
		ce.width += c->bw * 2;
		ce.height += c->bw * 2;
		ce.border_width = 0;
	}

	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			scanaltbars();
			for (m = mons; m; m = m->next) {
				for (c = m->clients; c; c = c->next)
					if (c->isfullscreen && c->fakefullscreen != 1)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
				if (m->barwin)
					XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, m->bh);
			}
			arrange(NULL);
			focus(NULL);
		}
	} else if (!wintoclient(ev->window)) {
		XWindowAttributes wa;
		if (XGetWindowAttributes(dpy, ev->window, &wa) && isaltbar(ev->window)) {
			Monitor *oldm;
			for (oldm = mons; oldm && oldm->barwin != ev->window; oldm = oldm->next);
			m = recttomon(wa.x, wa.y, wa.width, wa.height);
			if (m && INTERSECT(wa.x, wa.y, wa.width, wa.height, m) <= 0)
				m = oldm;
			if (oldm && m && oldm != m) {
				oldm->barwin = 0;
				oldm->bh = 0;
				updatebarpos(oldm);
				arrange(oldm);
			}
			if (m && updatealtbar(m, ev->window, &wa)) {
				arrange(m);
				XRaiseWindow(dpy, ev->window);
				updateclientlist();
			}
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if (!c->issteam) {
				if (ev->value_mask & CWX) {
					c->oldx = c->x;
					c->x = m->mx + ev->x;
				}
				if (ev->value_mask & CWY) {
					c->oldy = c->y;
					c->y = m->my + ev->y;
				}
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2);  /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

Monitor *
createmon(void)
{
	Monitor *m, *t;
	unsigned int i;

	m = ecalloc(1, sizeof(Monitor));

	/* the monitor number is its position in the list */
	for (t = mons; t; t = t->next)
		m->num++;

	m->tagset[0] = m->tagset[1] = 1; /* corrected by initmonitortags() */
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	m->topbar = topbar;
	m->bh = bh;
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	m->pertag = ecalloc(1, sizeof(Pertag));
	m->pertag->curtag = m->pertag->prevtag = 1;

	for (i = 0; i <= LENGTH(tags); i++) {
		m->pertag->nmasters[i] = m->nmaster;
		m->pertag->mfacts[i] = m->mfact;

		m->pertag->ltidxs[i][0] = m->lt[0];
		m->pertag->ltidxs[i][1] = m->lt[1];
		m->pertag->sellts[i] = m->sellt;

		m->pertag->showbars[i] = m->showbar;
	}

	return m;
}

void
destroynotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
	else if ((c = swallowingclient(ev->window)))
		unmanage(c->swallowing, 1);
	else if ((m = wintomon(ev->window)) && m->barwin == ev->window)
		unmanagealtbar(ev->window);
	else if (m && m->traywin == ev->window)
		unmanagetray(ev->window);
}

void
detach(Client *c)
{
	Client **tc;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
	c->next = NULL;
}

void
detachstack(Client *c)
{
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
	c->snext = NULL;
}

Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons)
		for (m = mons; m->next; m = m->next);
	else
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	focus(c);
}

void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	/* the external bar was (re)drawn: its tray may have appeared meanwhile */
	if (ev->count == 0 && (m = wintomon(ev->window)) && m->barwin == ev->window
	&& !m->traywin)
		scantray();
}

/* Warp the pointer to the centre of c unless it is already inside c. With
 * c == NULL, centre it on the selected monitor unless it is already there. */
void
warppointer(Client *c)
{
	int x, y;

	if (!cursorwarp || !getrootptr(&x, &y))
		return;
	if (c) {
		if (x >= c->x && x < c->x + WIDTH(c) && y >= c->y && y < c->y + HEIGHT(c))
			return;
		XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w / 2, c->h / 2);
	} else {
		if (INTERSECT(x, y, 1, 1, selmon) > 0)
			return;
		XWarpPointer(dpy, None, root, 0, 0, 0, 0,
			selmon->mx + selmon->mw / 2, selmon->my + selmon->mh / 2);
	}
}

void
focus(Client *c)
{
	static Monitor *lastmon;

	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	/* the pointer or keyboard crossed to another monitor: tell the bar */
	if (selmon != lastmon) {
		lastmon = selmon;
		updatecurrentdesktop();
	}
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void
focusmon(const Arg *arg)
{
	Monitor *m;

	if (!mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	focus(NULL);
	warppointer(selmon->sel);
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!selmon->sel || (selmon->sel->isfullscreen && !selmon->sel->fakefullscreen))
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
		restack(selmon);
		warppointer(c);
	}
}

Atom
getwinatomprop(Window win, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (XGetWindowProperty(dpy, win, prop, 0L, sizeof atom, False, XA_ATOM,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		XFree(p);
	}
	return atom;
}

Atom
getatomprop(Client *c, Atom prop)
{
	return c ? getwinatomprop(c->win, prop) : None;
}

pid_t
getparentprocess(pid_t p)
{
	char buf[256], *s;
	unsigned int v = 0;
	FILE *f;
	size_t n;

	snprintf(buf, sizeof buf, "/proc/%u/stat", (unsigned)p);
	if (!(f = fopen(buf, "r")))
		return 0;
	n = fread(buf, 1, sizeof buf - 1, f);
	fclose(f);
	buf[n] = '\0';
	/* the command name sits in parentheses and may itself contain spaces or
	 * parentheses, so parse what follows the last ')': "state ppid ..." */
	if ((s = strrchr(buf, ')')) && sscanf(s + 1, " %*c %u", &v) != 1)
		v = 0;
	return (pid_t)v;
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING) {
		strncpy(text, (char *)name.value, size - 1);
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
		strncpy(text, *list, size - 1);
		XFreeStringList(list);
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		if (rt_buttons)
		for (i = 0; i < (unsigned int)rt_nbuttons; i++)
			if (rt_buttons[i].click == ClkClientWin)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, rt_buttons[i].button,
						rt_buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

void
grabkeys(void)
{
	unsigned int i, j;
	int k, start, end, skip;
	KeySym *syms;

	updatenumlockmask();
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };

	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	if (!rt_keys || !rt_nkeys)
		return;
	XDisplayKeycodes(dpy, &start, &end);
	syms = XGetKeyboardMapping(dpy, start, end - start + 1, &skip);
	if (!syms)
		return;
	for (k = start; k <= end; k++)
		for (i = 0; i < (unsigned int)rt_nkeys; i++)
			/* skip modifier codes, we do that ourselves */
			if (rt_keys[i].keysym == syms[(k - start) * skip])
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(dpy, k,
						 rt_keys[i].mod | modifiers[j],
						 root, True,
						 GrabModeAsync, GrabModeAsync);
	XFree(syms);
}

void
incnmaster(const Arg *arg)
{
	selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] = MAX(selmon->nmaster + arg->i, 0);
	arrange(selmon);
}

int
isdescprocess(pid_t p, pid_t c)
{
	while (p != c && c != 0)
		c = getparentprocess(c);

	return (int)c;
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

void
keypress(XEvent *e)
{
	unsigned int i;
	int keysyms_return;
	KeySym* keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XGetKeyboardMapping(dpy, (KeyCode)ev->keycode, 1, &keysyms_return);
	const Key  *akeys        = rt_keys;
	unsigned int nkeys_active = (unsigned int)rt_nkeys;
	if (!akeys || !nkeys_active) { XFree(keysym); return; }
	for (i = 0; i < nkeys_active; i++)
		if (*keysym == akeys[i].keysym
				&& CLEANMASK(akeys[i].mod) == CLEANMASK(ev->state)
				&& akeys[i].func)
			akeys[i].func(&(akeys[i].arg));
	XFree(keysym);
}

void
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;

	if (!sendevent(selmon->sel, wmatom[WMDelete])) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL, *term = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	c->pid = winpid(w);
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;
	c->cfact = 1.0;

	updatetitle(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
	} else {
		c->mon = selmon;
		applyrules(c);
		term = termforwin(c);
	}

	c->bw = dyn_borderpx;
	/* new windows start centred on their monitor */
	c->x = c->mon->mx + (c->mon->mw - WIDTH(c)) / 2;
	c->y = c->mon->my + (c->mon->mh - HEIGHT(c)) / 2;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	attachbottom(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	setclientdesktop(c);
	if (term && swallow(term, c)) {
		focus(NULL);
		return;
	}
	if (c->mon == selmon)
		unfocus(selmon->sel, 0);
	c->mon->sel = c;
	arrange(c->mon);
	XMapWindow(dpy, c->win);
	focus(NULL);
}

int
getwinstrut(Window win, int *top, int *bottom)
{
	Atom da;
	int di;
	unsigned long n, dl;
	unsigned char *p = NULL;

	if (XGetWindowProperty(dpy, win, netatom[NetWMStrutPartial], 0L, 12L, False, XA_CARDINAL,
		&da, &di, &n, &dl, &p) == Success && p) {
		if (n >= 4) {
			long *s = (long *)p;
			if (top) *top = s[2];
			if (bottom) *bottom = s[3];
			XFree(p);
			return 1;
		}
		XFree(p);
	}
	if (XGetWindowProperty(dpy, win, netatom[NetWMStrut], 0L, 4L, False, XA_CARDINAL,
		&da, &di, &n, &dl, &p) == Success && p) {
		if (n >= 4) {
			long *s = (long *)p;
			if (top) *top = s[2];
			if (bottom) *bottom = s[3];
			XFree(p);
			return 1;
		}
		XFree(p);
	}
	return 0;
}

int
updatealtbar(Monitor *m, Window win, XWindowAttributes *wa)
{
	int changed, newbh, newtopbar;
	int top_strut = 0, bottom_strut = 0;

	if (!m || !wa)
		return 0;
	if (wa->override_redirect)
		return 0;

	if (getwinstrut(win, &top_strut, &bottom_strut)) {
		if (top_strut > 0) {
			newtopbar = 1;
			newbh = top_strut;
		} else if (bottom_strut > 0) {
			newtopbar = 0;
			newbh = bottom_strut;
		} else {
			newtopbar = wa->y < m->my + m->mh / 2;
			newbh = wa->height > 0 ? wa->height : (m->bh > 0 ? m->bh : 30);
		}
	} else {
		newtopbar = wa->y < m->my + m->mh / 2;
		newbh = wa->height > 0 ? wa->height : (m->bh > 0 ? m->bh : 30);
	}

	changed = (m->barwin != win || m->topbar != newtopbar || m->bh != newbh);
	m->barwin = win;
	m->topbar = newtopbar;
	m->bh = newbh;
	bh = m->bh;
	updatebarpos(m);
	return changed;
}

void
managealtbar(Window win, XWindowAttributes *wa)
{
	int changed;
	Monitor *m;
	if (!(m = recttomon(wa->x, wa->y, wa->width, wa->height)))
		return;

	changed = updatealtbar(m, win, wa);
	if (changed)
		arrange(m);
	XSelectInput(dpy, win, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask|ExposureMask);
	XMoveResizeWindow(dpy, win, wa->x, wa->y, wa->width, wa->height);
	XRaiseWindow(dpy, win);
	XMapWindow(dpy, win);
	updateclientlist();
	if (!m->traywin)
		scantray();
}

void
scanaltbars(void)
{
	unsigned int i, j, num, nmons;
	Monitor *m, *oldm;
	Window d1, d2, *wins = NULL;
	Window *knownbars;
	XWindowAttributes wa;

	for (nmons = 0, m = mons; m; m = m->next)
		nmons++;
	knownbars = ecalloc(nmons ? nmons : 1, sizeof(Window));
	for (i = 0, m = mons; m; m = m->next, i++)
		knownbars[i] = m->barwin;

	if (!XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		free(knownbars);
		return;
	}

	for (i = 0; i < num; i++) {
		if (!XGetWindowAttributes(dpy, wins[i], &wa)
		|| wa.override_redirect || wa.map_state != IsViewable
		|| !isaltbar(wins[i]))
			continue;

		/* the monitor that already holds this bar, if any */
		for (j = 0, oldm = mons; oldm && j < nmons; oldm = oldm->next, j++)
			if (knownbars[j] == wins[i])
				break;
		m = recttomon(wa.x, wa.y, wa.width, wa.height);
		if (!m || INTERSECT(wa.x, wa.y, wa.width, wa.height, m) <= 0)
			m = oldm;
		if (!m)
			continue;
		if (updatealtbar(m, wins[i], &wa)) {
			arrange(m);
			XRaiseWindow(dpy, wins[i]);
			updateclientlist();
		}
	}
	free(knownbars);
	XFree(wins);
}

void
managetray(Window win, XWindowAttributes *wa)
{
	Monitor *m;
	if (!(m = recttomon(wa->x, wa->y, wa->width, wa->height)))
		return;

	m->traywin = win;
	m->tx = wa->x;
	m->tw = wa->width;
	updatebarpos(m);
	arrange(m);
	XSelectInput(dpy, win, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	XMoveResizeWindow(dpy, win, wa->x, wa->y, wa->width, wa->height);
	XMapWindow(dpy, win);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
			(unsigned char *) &win, 1);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

int
isaltbar(Window win)
{
	int top = 0, bottom = 0;

	if (wmclasscontains(win, altbarclass, ""))
		return 1;
	if (getwinatomprop(win, netatom[NetWMWindowType]) == netatom[NetWMWindowTypeDock])
		return 1;
	return getwinstrut(win, &top, &bottom) && (top > 0 || bottom > 0);
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
		return;
	if (isaltbar(ev->window))
		managealtbar(ev->window, &wa);
	else if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
monocle(Monitor *m)
{
	int w, h, x, y;
	Client *c;
	
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next)) {
		x = m->wx;
		y = m->wy;
		w = m->ww - 2 * c->bw;
		h = m->wh - 2 * c->bw;
		applysizehints(c, &x, &y, &w, &h, 0);
		resizeclient(c, x, y, w, h);
	}
}

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

void
moveorplace(const Arg *arg)
{
	if ((!selmon->lt[selmon->sellt]->arrange || (selmon->sel && selmon->sel->isfloating)))
		movemouse(arg);
	else
		placemouse(arg);
}

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen && c->fakefullscreen != 1) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	nx = ocx = c->x;
	ny = ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y)) {
		XUngrabPointer(dpy, CurrentTime);
		return;
	}
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / refresh_rate))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		updateclientlist();
		selmon = m;
		focus(NULL);
	}
}

void
movestack(const Arg *arg)
{
	Client *c = NULL, *p = NULL, *pc = NULL, *i;

	if (!selmon->sel)
		return;

	if(arg->i > 0) {
		/* find the client after selmon->sel */
		for(c = selmon->sel->next; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);
		if(!c)
			for(c = selmon->clients; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);

	}
	else {
		/* find the client before selmon->sel */
		for(i = selmon->clients; i != selmon->sel; i = i->next)
			if(ISVISIBLE(i) && !i->isfloating)
				c = i;
		if(!c)
			for(; i; i = i->next)
				if(ISVISIBLE(i) && !i->isfloating)
					c = i;
	}
	/* find the client before selmon->sel and c */
	for(i = selmon->clients; i && (!p || !pc); i = i->next) {
		if(i->next == selmon->sel)
			p = i;
		if(i->next == c)
			pc = i;
	}

	/* swap c and selmon->sel selmon->clients in the selmon->clients list */
	if(c && c != selmon->sel) {
		Client *temp = selmon->sel->next==c?selmon->sel:selmon->sel->next;
		selmon->sel->next = c->next==selmon->sel?c:c->next;
		c->next = temp;

		if(p && p != c)
			p->next = c;
		if(pc && pc != selmon->sel)
			pc->next = selmon->sel;

		if(selmon->sel == selmon->clients)
			selmon->clients = c;
		else if(c == selmon->clients)
			selmon->clients = selmon->sel;

		arrange(selmon);
	}
}

Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

int
noborder(Client *c)
{
	int monocle_layout = 0;

	if (&monocle == c->mon->lt[c->mon->sellt]->arrange)
		monocle_layout = 1;

	if (!monocle_layout && (nexttiled(c->mon->clients) != c || nexttiled(c->next)))
		return 0;

	if (c->isfloating)
		return 0;

	if (!c->mon->lt[c->mon->sellt]->arrange)
		return 0;

	if (c->fakefullscreen != 1 && c->isfullscreen)
		return 0;

	return 1;
}

void
placemouse(const Arg *arg)
{
	int x, y, px, py, ocx, ocy, nx = -9999, ny = -9999, freemove = 0;
	Client *c, *r = NULL, *at, *prevr;
	Monitor *m;
	XEvent ev;
	XWindowAttributes wa;
	Time lasttime = 0;
	int attachmode, prevattachmode;
	attachmode = prevattachmode = -1;

	if (!(c = selmon->sel) || !c->mon->lt[c->mon->sellt]->arrange) /* no support for placemouse when floating layout is used */
		return;
	if (c->isfullscreen) /* no support placing fullscreen windows by mouse */
		return;
	restack(selmon);
	prevr = c;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;

	if (!getrootptr(&x, &y)) {
		XUngrabPointer(dpy, CurrentTime);
		return;
	}
	c->isfloating = 0;

	XGetWindowAttributes(dpy, c->win, &wa);
	ocx = wa.x;
	ocy = wa.y;

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / refresh_rate))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);

			if (!freemove && (abs(nx - ocx) > snap || abs(ny - ocy) > snap))
				freemove = 1;

			if (freemove)
				XMoveWindow(dpy, c->win, nx, ny);

			if ((m = recttomon(ev.xmotion.x, ev.xmotion.y, 1, 1)) && m != selmon)
				selmon = m;

			if (arg->i == 1) { // tiled position is relative to the client window center point
				px = nx + wa.width / 2;
				py = ny + wa.height / 2;
			} else { // tiled position is relative to the mouse cursor
				px = ev.xmotion.x;
				py = ev.xmotion.y;
			}

			r = recttoclient(px, py, 1, 1);

			if (!r || r == c)
				break;

			if ((((float)(r->y + r->h - py) / r->h) > ((float)(r->x + r->w - px) / r->w)
			    	&& (abs(r->y - py) < r->h / 2)) || (abs(r->x - px) < r->w / 2))
				attachmode = 1; // above
			else
				attachmode = 0; // below

			if ((r && r != prevr) || (attachmode != prevattachmode)) {
				detachstack(c);
				detach(c);
				if (c->mon != r->mon) {
					arrangemon(c->mon);
					c->tags = r->mon->tagset[r->mon->seltags];
				}

				c->mon = r->mon;
				r->mon->sel = r;

				if (attachmode) {
					if (r == r->mon->clients)
						attach(c);
					else {
						for (at = r->mon->clients; at->next != r; at = at->next);
						c->next = at->next;
						at->next = c;
					}
				} else {
					c->next = r->next;
					r->next = c;
				}

				attachstack(c);
				arrangemon(r->mon);
				prevr = r;
				prevattachmode = attachmode;
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);

	if ((m = recttomon(ev.xmotion.x, ev.xmotion.y, 1, 1)) && m != c->mon) {
		detach(c);
		detachstack(c);
		arrangemon(c->mon);
		c->mon = m;
		c->tags = m->tagset[m->seltags];
		attach(c);
		attachstack(c);
		selmon = m;
	}

	focus(c);

	if (nx != -9999)
		resize(c, nx, ny, c->w, c->h, 0);
	arrangemon(c->mon);
}

void
pop(Client *c)
{
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;
	int wasfloating;

	if (ev->state == PropertyDelete)
		return; /* ignore */

	if (!(c = wintoclient(ev->window))) {
		/* not a client: maybe the external bar changing its strut or type */
		if (ev->atom == netatom[NetWMWindowType] ||
			ev->atom == netatom[NetWMStrut] ||
			ev->atom == netatom[NetWMStrutPartial]) {
			XWindowAttributes wa;
			if (XGetWindowAttributes(dpy, ev->window, &wa) && isaltbar(ev->window)) {
				Monitor *m = recttomon(wa.x, wa.y, wa.width, wa.height);
				if (m && updatealtbar(m, ev->window, &wa)) {
					arrange(m);
					XRaiseWindow(dpy, ev->window);
					updateclientlist();
				}
			}
		}
		return;
	}

	switch (ev->atom) {
	default: break;
	case XA_WM_TRANSIENT_FOR:
		if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
			(c->isfloating = (wintoclient(trans)) != NULL))
			arrange(c->mon);
		break;
	case XA_WM_NORMAL_HINTS:
		updatesizehints(c);
		break;
	case XA_WM_HINTS:
		updatewmhints(c);
		break;
	}
	if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName])
		updatetitle(c);
	else if (ev->atom == netatom[NetWMWindowType]) {
		wasfloating = c->isfloating;
		updatewindowtype(c);
		if (c->isfloating != wasfloating)
			arrange(c->mon);
	}
}

void
quit(const Arg *arg)
{
	running = 0;
}

Client *
recttoclient(int x, int y, int w, int h)
{
	Client *c, *r = NULL;
	int a, area = 0;

	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next)) {
		if ((a = INTERSECTC(x, y, w, h, c)) > area) {
			area = a;
			r = c;
		}
	}
	return r;
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	if (noborder(c) && enable_noborder) {
		wc.width += c->bw * 2;
		wc.height += c->bw * 2;
		wc.border_width = 0;
	}
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh, nx, ny;
	int opx, opy, och, ocw;
	int horizcorner, vertcorner;
	unsigned int dui;
	Window dummy;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen && c->fakefullscreen != 1) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);
	nx = ocx = c->x;
	ny = ocy = c->y;
	nh = c->h;
	nw = c->w;
	och = c->h;
	ocw = c->w;
	if (!XQueryPointer(dpy, c->win, &dummy, &dummy, &opx, &opy, &nx, &ny, &dui))
		return;
	horizcorner = nx < c->w / 2;
	vertcorner  = ny < c->h / 2;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[horizcorner | (vertcorner << 1)]->cursor, CurrentTime) != GrabSuccess)
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / refresh_rate))
				continue;
			lasttime = ev.xmotion.time;

			nx = horizcorner ? (ocx + ev.xmotion.x - opx) : c->x;
			ny = vertcorner ? (ocy + ev.xmotion.y - opy) : c->y;
			nw = MAX(horizcorner ? (ocx + ocw - nx) : (ocw + (ev.xmotion.x - opx)), 1);
			nh = MAX(vertcorner ? (ocy + och - ny) : (och + (ev.xmotion.y - opy)), 1);
			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap)) {
					togglefloating(NULL);
				}
			}
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating) {
				resize(c, nx, ny, nw, nh, 1);
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;
	unsigned int mask;

	if (!m->sel)
		return;
	if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (m->lt[m->sellt]->arrange) {
		/* stack the tiled clients just below the bar, in focus order; without
		 * a bar window start from the bottom of the stack instead */
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c)) {
				mask = wc.sibling ? CWSibling|CWStackMode : CWStackMode;
				XConfigureWindow(dpy, c->win, mask, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

/* React to changes in the watched config directories */
static int
inotify_dispatch(void)
{
	char ibuf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
	char *ptr;
	ssize_t nr;
	int need_reload = 0;

	if ((nr = read(inotify_fd, ibuf, sizeof ibuf)) <= 0)
		return 0;
	for (ptr = ibuf; ptr < ibuf + nr; ptr += sizeof(struct inotify_event) + ((struct inotify_event *)ptr)->len) {
		struct inotify_event *ie = (struct inotify_event *)ptr;
		if (ie->len > 0 &&
			(strcmp(ie->name, "hotkeys.toml")      == 0 ||
			 strcmp(ie->name, "themes.toml")       == 0 ||
			 strcmp(ie->name, "window-rules.toml") == 0))
			need_reload = 1;
	}
	return need_reload;
}

void
run(void)
{
	XEvent ev;
	int x11_fd = ConnectionNumber(dpy);
	int maxfd;
	char drain[64];
	fd_set rfds;

	XSync(dpy, False);
	while (running) {
		/* drain all pending X events */
		while (running && XPending(dpy)) {
			XNextEvent(dpy, &ev);
			if (handler[ev.type])
				handler[ev.type](&ev);
		}
		if (!running)
			break;

		FD_ZERO(&rfds);
		FD_SET(x11_fd, &rfds);
		maxfd = x11_fd;
		if (inotify_fd >= 0) {
			FD_SET(inotify_fd, &rfds);
			maxfd = MAX(maxfd, inotify_fd);
		}
		if (sigpipe[0] >= 0) {
			FD_SET(sigpipe[0], &rfds);
			maxfd = MAX(maxfd, sigpipe[0]);
		}

		if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue; /* interrupted by a signal, e.g. SIGCHLD */
			break;
		}

		int need_reload = 0;
		if (sigpipe[0] >= 0 && FD_ISSET(sigpipe[0], &rfds)) { /* SIGUSR1 */
			while (read(sigpipe[0], drain, sizeof drain) > 0);
			need_reload = 1;
		}
		if (inotify_fd >= 0 && FD_ISSET(inotify_fd, &rfds)) {
			if (inotify_dispatch())
				need_reload = 1;
		}
		if (need_reload)
			reload_config();
	}
}

/* Directory for user data: $XDG_DATA_HOME/dwm or ~/.local/share/dwm.
 * Returns 0 if it cannot be determined. */
static int
getdatadir(char *buf, size_t size)
{
	const char *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");

	if (xdg && *xdg)
		return snprintf(buf, size, "%s/%s", xdg, dwmdir) < (int)size;
	if (home)
		return snprintf(buf, size, "%s/%s/%s", home, localshare, dwmdir) < (int)size;
	return 0;
}

void
runautostart(void)
{
	char cand[3][PATH_MAX];
	const char *home = getenv("HOME");
	char datadir[PATH_MAX];
	int i, n = 0;

	/* candidate scripts, in order of precedence */
	if (getdatadir(datadir, sizeof datadir))
		snprintf(cand[n++], PATH_MAX, "%s/%s", datadir, autostartsh);
	if (home)
		snprintf(cand[n++], PATH_MAX, "%s/.%s/%s", home, dwmdir, autostartsh);
	snprintf(cand[n++], PATH_MAX, "%s/%s", DWM_PATH, autostartsh);

	for (i = 0; i < n; i++) {
		if (access(cand[i], X_OK) != 0)
			continue;
		/* the parent returns immediately: dwm is not blocked by autostart */
		if (fork() == 0) {
			setsid();
			execl(cand[i], cand[i], (char *)NULL);
			_exit(127);
		}
		return;
	}
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (isaltbar(wins[i]))
				managealtbar(wins[i], &wa);
			else if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		XFree(wins);
	}
	scanaltbars();
}

void
scantray(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (!XQueryTree(dpy, root, &d1, &d2, &wins, &num))
		return;
	for (i = 0; i < num; i++)
		if (wmclasscontains(wins[i], altbarclass, alttrayname)
		&& XGetWindowAttributes(dpy, wins[i], &wa))
			managetray(wins[i], &wa);
	if (wins)
		XFree(wins);
}

void
sendmon(Client *c, Monitor *m)
{
	unsigned int montags;

	if (!c || !m || c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;

	/* Tags are per monitor. A sticky client keeps its tags on the new monitor;
	 * any other client appears on the view the target monitor is showing now. */
	montags = getmontagmask(m->num);
	if (c->tags & montags)
		c->tags &= montags;
	else
		c->tags = m->tagset[m->seltags];

	setclientdesktop(c);
	attachbottom(c);
	attachstack(c);
	arrange(NULL);
	focus(NULL);
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

void
setdesktopnames(void)
{
	XTextProperty text;

	if (Xutf8TextListToTextProperty(dpy, (char **)tags, NTAGS, XUTF8StringStyle, &text) >= Success) {
		XSetTextProperty(dpy, root, &text, netatom[NetDesktopNames]);
		XFree(text.value);
	}
}

void
setclientdesktop(Client *c)
{
	long data[1];
	int i;

	if (!c)
		return;
	for (i = 0; i < NTAGS && !(c->tags & (1u << i)); i++);
	data[0] = i < NTAGS ? i : 0xFFFFFFFFl; /* no tag: sticky */
	XChangeProperty(dpy, c->win, netatom[NetWMDesktop], XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)data, 1);
	updatefullscreenmonitors();
	data[0] = (long)++tagupdatesequence;
	XChangeProperty(dpy, root, dwmtagupdateatom, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)data, 1);
}

int
sendevent(Client *c, Atom proto)
{
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	return exists;
}

void
setnumdesktops(void)
{
	long data[] = { NTAGS };

	XChangeProperty(dpy, root, netatom[NetNumberOfDesktops], XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)data, 1);
}

void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
	sendevent(c, wmatom[WMTakeFocus]);
}

void
setfullscreen(Client *c, int fullscreen)
{
	XEvent ev;
	int savestate = 0, restorestate = 0;

	if ((c->fakefullscreen == 0 && fullscreen && !c->isfullscreen) // normal fullscreen
			|| (c->fakefullscreen == 2 && fullscreen)) // fake fullscreen --> actual fullscreen
		savestate = 1; // go actual fullscreen
	else if ((c->fakefullscreen == 0 && !fullscreen && c->isfullscreen) // normal fullscreen exit
			|| (c->fakefullscreen >= 2 && !fullscreen)) // fullscreen exit --> fake fullscreen
		restorestate = 1; // go back into tiled

	/* If leaving fullscreen and the window was previously fake fullscreen (2), then restore
	 * that while staying in fullscreen. The exception to this is if we are in said state, but
	 * the client itself disables fullscreen (3) then we let the client go out of fullscreen
	 * while keeping fake fullscreen enabled (as otherwise there will be a mismatch between the
	 * client and the window manager's perception of the client's fullscreen state). */
	if (c->fakefullscreen == 2 && !fullscreen && c->isfullscreen) {
		c->fakefullscreen = 1;
		c->isfullscreen = 1;
		fullscreen = 1;
	} else if (c->fakefullscreen == 3) // client exiting actual fullscreen
		c->fakefullscreen = 1;

	if (fullscreen != c->isfullscreen) { // only send property change if necessary
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)(fullscreen ? &netatom[NetWMFullscreen] : 0), fullscreen ? 1 : 0);
	}

	c->isfullscreen = fullscreen;

	/* Some clients, e.g. firefox, will send a client message informing the window manager
	 * that it is going into fullscreen after receiving the above signal. This has the side
	 * effect of this function (setfullscreen) sometimes being called twice when toggling
	 * fullscreen on and off via the window manager as opposed to the application itself.
	 * To protect against obscure issues where the client settings are stored or restored
	 * when they are not supposed to we add an additional bit-lock on the old state so that
	 * settings can only be stored and restored in that precise order. */
	if (savestate && !(c->oldstate & (1 << 1))) {
		c->oldbw = c->bw;
		c->oldstate = c->isfloating | (1 << 1);
		c->bw = 0;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	} else if (restorestate && (c->oldstate & (1 << 1))) {
		c->bw = c->oldbw;
		c->isfloating = c->oldstate & 1;
		c->oldstate = 0;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(c->mon);
	} else
		resizeclient(c, c->x, c->y, c->w, c->h);

	/* Exception: if the client was in actual fullscreen and we exit out to fake fullscreen
	 * mode, then the focus would sometimes drift to whichever window is under the mouse cursor
	 * at the time. To avoid this we ask X for all EnterNotify events and just ignore them.
	 */
	if (!c->isfullscreen)
		while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

/* Toggle a distraction-free view: monocle layout with the bar hidden. The
 * layout used before is remembered and restored on the way out. */
void
fullscreen(const Arg *arg)
{
	static const Layout *prevlayout;
	unsigned int i;

	if (selmon->showbar) {
		for (i = 0; i < LENGTH(layouts) && layouts[i].arrange != monocle; i++);
		if (i < LENGTH(layouts)) {
			prevlayout = selmon->lt[selmon->sellt];
			setlayout(&(Arg){ .v = &layouts[i] });
		}
	} else if (prevlayout) {
		setlayout(&(Arg){ .v = prevlayout });
		prevlayout = NULL;
	}
	togglebar(arg);
}

void
setlayout(const Arg *arg)
{
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt]) {
		selmon->pertag->sellts[selmon->pertag->curtag] ^= 1;
		selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
	}
	if (arg && arg->v)
		selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt] = (Layout *)arg->v;
	selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
	arrange(selmon);
}

void
setcfact(const Arg *arg)
{
	float f;
	Client *c;

	c = selmon->sel;

	if (!arg || !c || !selmon->lt[selmon->sellt]->arrange)
		return;
	if (!arg->f)
		f = 1.0;
	else if (arg->f > 4.0) // set fact absolutely
		f = arg->f - 4.0;
	else
		f = arg->f + c->cfact;
	if (f < 0.25)
		f = 0.25;
	else if (f > 4.0)
		f = 4.0;
	c->cfact = f;
	arrange(selmon);
}

/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
	float f;

	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	if (f < 0.05 || f > 0.95)
		return;
	selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = f;
	arrange(selmon);
}

/* ── Hot-reload implementation ────────────────────────────────────────────── */

static void
sigusr1_handler(int sig)
{
	int saved_errno = errno;

	(void)sig;
	if (sigpipe[1] >= 0)
		(void)write(sigpipe[1], "r", 1); /* wakes up select() in run() */
	errno = saved_errno;
}

static void *
toml_alloc(size_t sz)
{
	void *p;

	sz = (sz + 7u) & ~(size_t)7u;
	if (toml_arena_pos + sz > TOML_ARENA_CAP)
		return NULL;
	p = toml_arena_buf[toml_arena_idx] + toml_arena_pos;
	toml_arena_pos += sz;
	return p;
}

static unsigned int
parse_single_mod(const char *s)
{
	if (strcmp(s, "MODKEY")      == 0 || strcmp(s, "SUPER")   == 0) return MODKEY;
	if (strcmp(s, "ShiftMask")   == 0 || strcmp(s, "SHIFT")   == 0) return ShiftMask;
	if (strcmp(s, "ControlMask") == 0 || strcmp(s, "CTRL")    == 0) return ControlMask;
	if (strcmp(s, "Mod1Mask")    == 0 || strcmp(s, "ALT")     == 0) return Mod1Mask;
	if (strcmp(s, "Mod2Mask")    == 0) return Mod2Mask;
	if (strcmp(s, "Mod3Mask")    == 0) return Mod3Mask;
	if (strcmp(s, "Mod4Mask")    == 0) return Mod4Mask;
	if (strcmp(s, "Mod5Mask")    == 0) return Mod5Mask;
	return 0;
}

static unsigned int
parse_mod_mask(const TomlValue *v)
{
	if (!v) return 0;
	/* Compact format: "SUPER SHIFT CTRL" space-separated string */
	if (v->type == TOML_STRING) {
		unsigned int mask = 0;
		const char *p = v->s;
		char tok[32];
		while (*p) {
			while (isspace((unsigned char)*p)) p++;
			if (!*p) break;
			int ti = 0;
			while (*p && !isspace((unsigned char)*p) && ti < 31)
				tok[ti++] = *p++;
			tok[ti] = '\0';
			mask |= parse_single_mod(tok);
		}
		return mask;
	}
	/* Legacy array format: ["MODKEY", "ShiftMask"] */
	if (v->type == TOML_ARRAY) {
		unsigned int mask = 0;
		int i;
		for (i = 0; i < v->a.len; i++)
			mask |= parse_single_mod(v->a.items[i]);
		return mask;
	}
	return 0;
}

static Func
lookup_func(const char *name)
{
	static const struct { const char *name; Func func; } funcs[] = {
		{ "spawn",                spawn },
		{ "killclient",           killclient },
		{ "quit",                 quit },
		{ "focusstack",           focusstack },
		{ "movestack",            movestack },
		{ "incnmaster",           incnmaster },
		{ "setmfact",             setmfact },
		{ "setcfact",             setcfact },
		{ "zoom",                 zoom },
		{ "view",                 view },
		{ "toggleview",           toggleview },
		{ "tag",                  tag },
		{ "toggletag",            toggletag },
		{ "togglebar",            togglebar },
		{ "togglefloating",       togglefloating },
		{ "fullscreen",           fullscreen },
		{ "togglefakefullscreen", togglefakefullscreen },
		{ "setlayout",            setlayout },
		{ "focusmon",             focusmon },
		{ "tagmon",               tagmon },
		{ "moveorplace",          moveorplace },
		{ "resizemouse",          resizemouse },
		{ "togglewarm",           togglewarm },
		{ "adjustbrightness",     adjustbrightness },
	};
	unsigned int i;

	for (i = 0; i < LENGTH(funcs); i++)
		if (strcmp(name, funcs[i].name) == 0)
			return funcs[i].func;
	return NULL;
}

/* Fire a notify-send (async, non-blocking) to warn about bad/missing config */
/* Warn about an unusable config file (asynchronous, never blocks dwm) */
static void
notify_bad_config(const char *filename, const char *reason)
{
	const char *base = strrchr(filename, '/');
	char msg[PATH_MAX + 64];

	base = base ? base + 1 : filename;
	snprintf(msg, sizeof msg, "%s: %s - loaded defaults", base, reason);
	if (fork() == 0) {
		execlp("notify-send", "notify-send", "-u", "critical", "dwm: bad config", msg, (char *)NULL);
		_exit(127);
	}
}

/* Map a TOML click-name string to the dwm Click enum value */
/* Map a TOML click name to the Click enum; ClkLast if dwm cannot see it */
static unsigned int
lookup_click(const char *name)
{
	if (strcmp(name, "ClkClientWin") == 0) return ClkClientWin;
	if (strcmp(name, "ClkRootWin")   == 0) return ClkRootWin;
	if (strcmp(name, "ClkTagBar") == 0 || strcmp(name, "ClkLtSymbol") == 0
	|| strcmp(name, "ClkStatusText") == 0 || strcmp(name, "ClkWinTitle") == 0)
		fprintf(stderr, "dwm: click '%s' belongs to the external bar and is ignored\n", name);
	return ClkLast;
}

/* Expand $varname from [vars] section into dst. */
static void
expand_var_to(const char *src, const TomlDoc *doc, char *dst, size_t dstsz)
{
	if (!strchr(src, '$')) {
		strncpy(dst, src, dstsz - 1);
		dst[dstsz - 1] = '\0';
		return;
	}
	const char *p = src;
	size_t di = 0;
	while (*p && di < dstsz - 1) {
		if (*p == '$') {
			const char *ns = p + 1;
			const char *ne = ns;
			while (isalnum((unsigned char)*ne) || *ne == '_') ne++;
			int nlen = (int)(ne - ns);
			if (nlen > 0 && nlen < TOML_MAX_STR) {
				char vname[TOML_MAX_STR];
				strncpy(vname, ns, (size_t)nlen);
				vname[nlen] = '\0';
				const TomlValue *tv = toml_get(doc, "vars", vname);
				if (tv && tv->type == TOML_STRING) {
					size_t vl = strlen(tv->s);
					if (di + vl < dstsz - 1) {
						memcpy(dst + di, tv->s, vl);
						di += vl;
					}
					p = ne;
					continue;
				}
			}
		}
		dst[di++] = *p++;
	}
	dst[di] = '\0';
}

static Arg
build_spawn_arg(const TomlDoc *doc, const char *section, int tidx)
{
	Arg arg = {0};
	const TomlValue *vexec = toml_table_get(doc, section, tidx, "exec");
	const TomlValue *vcmd  = toml_table_get(doc, section, tidx, "cmd");
	if (vexec && vexec->type == TOML_ARRAY && vexec->a.len > 0) {
		int argc = vexec->a.len;
		const char **argv = toml_alloc((argc + 1) * sizeof(char *));
		if (!argv) return arg;
		for (int j = 0; j < argc; j++) {
			char expanded[TOML_MAX_STR];
			expand_var_to(vexec->a.items[j], doc, expanded, TOML_MAX_STR);
			size_t slen = strlen(expanded) + 1;
			char *s = toml_alloc(slen);
			if (!s) return arg;
			memcpy(s, expanded, slen);
			argv[j] = s;
		}
		argv[argc] = NULL;
		arg.v = argv;
	} else if (vcmd && vcmd->type == TOML_STRING) {
		const char **argv = toml_alloc(4 * sizeof(char *));
		if (!argv) return arg;
		char expanded[TOML_MAX_STR];
		expand_var_to(vcmd->s, doc, expanded, TOML_MAX_STR);
		size_t slen = strlen(expanded) + 1;
		char *cmd = toml_alloc(slen);
		if (!cmd) return arg;
		memcpy(cmd, expanded, slen);
		argv[0] = "/bin/sh";
		argv[1] = "-c";
		argv[2] = cmd;
		argv[3] = NULL;
		arg.v = argv;
	}
	return arg;
}

static Arg
build_arg(const char *func_name, const TomlDoc *doc,
          const char *section, int tidx)
{
	Arg arg = {0};
	const TomlValue *v;
	if (strcmp(func_name, "spawn") == 0)
		return build_spawn_arg(doc, section, tidx);
	if (strcmp(func_name, "setlayout") == 0) {
		v = toml_table_get(doc, section, tidx, "layout_idx");
		int idx = (v && v->type == TOML_INT) ? (int)v->i : 0;
		if (idx == -1) { arg.v = NULL; return arg; }  /* toggle sellt */
		if (idx < 0 || idx >= (int)LENGTH(layouts)) idx = 0;
		arg.v = &layouts[idx];
		return arg;
	}
	v = toml_table_get(doc, section, tidx, "i");
	if (v) {
		if (v->type == TOML_INT)   { arg.i = (int)v->i;   return arg; }
		if (v->type == TOML_FLOAT) { arg.i = (int)v->d;   return arg; }
	}
	v = toml_table_get(doc, section, tidx, "ui");
	if (v && v->type == TOML_INT) { arg.ui = (unsigned int)v->i; return arg; }
	v = toml_table_get(doc, section, tidx, "f");
	if (v) {
		if (v->type == TOML_FLOAT) { arg.f = (float)v->d; return arg; }
		if (v->type == TOML_INT)   { arg.f = (float)v->i; return arg; }
	}
	return arg;
}

static TomlDoc tomldoc; /* scratch document shared by all loaders (large: keep it off the stack) */

/* Parse the user's config; if it is missing or invalid fall back to the
 * shipped default. Returns the document, or NULL if neither is usable. */
static const TomlDoc *
toml_load(const char *what, const char *user_path, const char *default_path)
{
	if (user_path && user_path[0]) {
		if (toml_parse(user_path, &tomldoc) && tomldoc.n > 0)
			return &tomldoc;
		if (access(user_path, F_OK) == 0) /* a missing file just means "use defaults" */
			notify_bad_config(user_path, "invalid config");
	}
	if (default_path && default_path[0] && toml_parse(default_path, &tomldoc))
		return &tomldoc;
	fprintf(stderr, "dwm: cannot load %s (no user or default config)\n", what);
	return NULL;
}

static void
load_hotkeys_toml(const char *user_path, const char *default_path)
{
	static const char *tag_funcs[4] = { "view", "toggleview", "tag", "toggletag" };
	static const unsigned int tag_mod_extra[4] = { 0, ControlMask, ShiftMask, ControlMask|ShiftMask };
	const TomlDoc *doc = toml_load("hotkeys", user_path, default_path);
	int i, j, nregular, ntag, total, nk = 0, nb = 0, nbtn, prev_idx;
	Key *newkeys;

	if (!doc)
		return;
	nregular = toml_table_count(doc, "keys");
	ntag     = toml_table_count(doc, "tag_keys");
	total    = nregular + ntag * 4;
	if (total <= 0)
		return;

	/* fill the idle arena half; the running bindings stay valid until we commit */
	prev_idx = toml_arena_idx;
	toml_arena_idx ^= 1;
	toml_arena_pos = 0;
	if (!(newkeys = toml_alloc((size_t)total * sizeof(Key)))) {
		fprintf(stderr, "dwm: TOML arena overflow\n");
		toml_arena_idx = prev_idx;
		return;
	}

	/* regular [[keys]] entries */
	for (i = 0; i < nregular && nk < total; i++) {
		const TomlValue *vkey  = toml_table_get(doc, "keys", i, "key");
		const TomlValue *vmod  = toml_table_get(doc, "keys", i, "mod");
		const TomlValue *vfunc = toml_table_get(doc, "keys", i, "func");
		KeySym ks;
		Func fn;
		Arg arg;

		if (!vkey || vkey->type != TOML_STRING || !vfunc || vfunc->type != TOML_STRING)
			continue;
		if ((ks = XStringToKeysym(vkey->s)) == NoSymbol) {
			fprintf(stderr, "dwm: unknown keysym '%s'\n", vkey->s);
			continue;
		}
		if (!(fn = lookup_func(vfunc->s))) {
			fprintf(stderr, "dwm: unknown func '%s'\n", vfunc->s);
			continue;
		}
		arg = build_arg(vfunc->s, doc, "keys", i);
		if (fn == spawn && !arg.v) { /* empty command or arena full */
			fprintf(stderr, "dwm: key '%s': spawn without a usable command\n", vkey->s);
			continue;
		}
		newkeys[nk].mod    = parse_mod_mask(vmod);
		newkeys[nk].keysym = ks;
		newkeys[nk].func   = fn;
		newkeys[nk].arg    = arg;
		nk++;
	}

	/* [[tag_keys]] expands to view/toggleview/tag/toggletag bindings */
	for (i = 0; i < ntag; i++) {
		const TomlValue *vkey = toml_table_get(doc, "tag_keys", i, "key");
		const TomlValue *vtag = toml_table_get(doc, "tag_keys", i, "tag");
		KeySym ks;

		if (!vkey || vkey->type != TOML_STRING || !vtag || vtag->type != TOML_INT)
			continue;
		if (vtag->i < 0 || vtag->i >= NTAGS) {
			fprintf(stderr, "dwm: tag_keys: tag %ld out of range (0..%d)\n", vtag->i, NTAGS - 1);
			continue;
		}
		if ((ks = XStringToKeysym(vkey->s)) == NoSymbol)
			continue;
		for (j = 0; j < 4 && nk < total; j++) {
			newkeys[nk].mod    = MODKEY | tag_mod_extra[j];
			newkeys[nk].keysym = ks;
			newkeys[nk].func   = lookup_func(tag_funcs[j]);
			newkeys[nk].arg.ui = 1u << vtag->i;
			nk++;
		}
	}
	rt_keys  = newkeys;
	rt_nkeys = nk;

	/* [[buttons]]: mouse bindings (same arena half) */
	nbtn = MIN(toml_table_count(doc, "buttons"), TOML_BUTTONS_MAX);
	for (i = 0; i < nbtn; i++) {
		const TomlValue *vclick = toml_table_get(doc, "buttons", i, "click");
		const TomlValue *vmod   = toml_table_get(doc, "buttons", i, "mod");
		const TomlValue *vbtn   = toml_table_get(doc, "buttons", i, "button");
		const TomlValue *vfunc  = toml_table_get(doc, "buttons", i, "func");
		unsigned int click;
		Func fn;
		Arg arg;

		if (!vclick || vclick->type != TOML_STRING || !vfunc || vfunc->type != TOML_STRING
		|| !vbtn || vbtn->type != TOML_INT)
			continue;
		if ((click = lookup_click(vclick->s)) == ClkLast)
			continue;
		if (!(fn = lookup_func(vfunc->s))) {
			fprintf(stderr, "dwm: unknown func '%s'\n", vfunc->s);
			continue;
		}
		arg = build_arg(vfunc->s, doc, "buttons", i);
		if (fn == spawn && !arg.v)
			continue;
		rt_buttons_buf[nb].click  = click;
		rt_buttons_buf[nb].mask   = parse_mod_mask(vmod);
		rt_buttons_buf[nb].button = (unsigned int)vbtn->i;
		rt_buttons_buf[nb].func   = fn;
		rt_buttons_buf[nb].arg    = arg;
		nb++;
	}
	rt_buttons  = nb > 0 ? rt_buttons_buf : NULL;
	rt_nbuttons = nb;
	fprintf(stderr, "dwm: loaded %d keybinds and %d button bindings\n", nk, nb);
}

/* Accept "#RRGGBB" (extra digits such as alpha are dropped) and "#RGB".
 * Anything else would make Xft fail, and drw_clr_create() dies on that. */
static int
parse_hexcolor(const TomlValue *v, char *dest /* 8 bytes */)
{
	size_t len, i;

	if (!v || v->type != TOML_STRING)
		return 0;
	len = strlen(v->s);
	if (v->s[0] != '#' || (len != 4 && len < 7))
		return 0;
	for (i = 1; i < (len == 4 ? 4 : 7); i++)
		if (!isxdigit((unsigned char)v->s[i]))
			return 0;
	memcpy(dest, v->s, len == 4 ? 4 : 7);
	dest[len == 4 ? 4 : 7] = '\0';
	return 1;
}

static void
load_themes_toml(const char *user_path, const char *default_path)
{
	static const char *keys[2][3] = { /* fg, bg, border */
		{ "normfgcolor", "normbgcolor", "normbordercolor" },
		{ "selfgcolor",  "selbgcolor",  "selbordercolor"  },
	};
	const TomlDoc *doc = toml_load("themes", user_path, default_path);
	char clr[2][3][8];
	const char *names[2][3];
	char section[TOML_MAX_STR] = "colors"; /* legacy section name */
	const TomlValue *v;
	Monitor *m;
	Client *c;
	Clr *newscm[2];
	int i, j;

	if (!doc)
		return;

	for (i = 0; i < 2; i++)
		for (j = 0; j < 3; j++) {
			strcpy(clr[i][j], defclrs[i][j]);
			names[i][j] = clr[i][j];
		}

	/* [active] theme = "name" selects the section "theme.name" */
	v = toml_get(doc, "active", "theme");
	if (v && v->type == TOML_STRING && v->s[0])
		snprintf(section, sizeof section, "theme.%s", v->s);

	for (i = 0; i < 2; i++)
		for (j = 0; j < 3; j++)
			if ((v = toml_get(doc, section, keys[i][j])) && !parse_hexcolor(v, clr[i][j]))
				fprintf(stderr, "dwm: themes: ignoring invalid colour for %s\n", keys[i][j]);

	/* rebuild the schemes and repaint the borders of existing windows */
	for (i = 0; i < 2; i++)
		newscm[i] = drw_scm_create(drw, names[i], 3);
	for (i = 0; i < 2; i++) {
		drw_scm_free(drw, scheme[i], 3);
		scheme[i] = newscm[i];
	}
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XSetWindowBorder(dpy, c->win, scheme[c == selmon->sel ? SchemeSel : SchemeNorm][ColBorder].pixel);

	v = toml_get(doc, "appearance", "borderpx");
	if (v && v->type == TOML_INT && v->i >= 0 && (unsigned int)v->i != dyn_borderpx) {
		dyn_borderpx = (unsigned int)v->i;
		for (m = mons; m; m = m->next)
			for (c = m->clients; c; c = c->next) {
				if (c->isfullscreen) { /* has no border now; restore to the new width later */
					c->oldbw = dyn_borderpx;
					continue;
				}
				c->bw = dyn_borderpx;
				XSetWindowBorderWidth(dpy, c->win, noborder(c) && enable_noborder ? 0 : c->bw);
			}
	}

	arrange(NULL);
	fprintf(stderr, "dwm: loaded theme from config\n");
}

static void
load_rules_toml(const char *user_path, const char *default_path)
{
	const TomlDoc *doc = toml_load("rules", user_path, default_path);
	int i, k, n, nk = 0;

	if (!doc)
		return;
	n = MIN(toml_table_count(doc, "rules"), TOML_RULES_MAX);
	for (i = 0; i < n; i++) {
		const TomlValue *vtag  = toml_table_get(doc, "rules", i, "tags");
		const TomlValue *vfl   = toml_table_get(doc, "rules", i, "isfloating");
		const TomlValue *vterm = toml_table_get(doc, "rules", i, "isterminal");
		const TomlValue *vno   = toml_table_get(doc, "rules", i, "noswallow");
		const TomlValue *vmon  = toml_table_get(doc, "rules", i, "monitor");
		static const char *fields[3] = { "class", "instance", "title" };
		const char **dst[3];
		Rule *r = &rt_rules_buf[nk];

		dst[0] = &r->class;
		dst[1] = &r->instance;
		dst[2] = &r->title;
		for (k = 0; k < 3; k++) {
			const TomlValue *vs = toml_table_get(doc, "rules", i, fields[k]);
			char *buf = rt_rules_strbuf[nk * 3 + k];

			if (vs && vs->type == TOML_STRING && vs->s[0]) {
				strncpy(buf, vs->s, TOML_MAX_STR - 1);
				buf[TOML_MAX_STR - 1] = '\0';
				*dst[k] = buf;
			} else
				*dst[k] = NULL;
		}
		r->tags       = (vtag && vtag->type == TOML_INT && vtag->i >= 1 && vtag->i <= NTAGS)
						? 1u << (vtag->i - 1) : 0;
		r->isfloating = (vfl   && vfl->type   == TOML_INT) ? (int)vfl->i   : 0;
		r->isterminal = (vterm && vterm->type == TOML_INT) ? (int)vterm->i : 0;
		r->noswallow  = (vno   && vno->type   == TOML_INT) ? (int)vno->i   : 0;
		r->monitor    = (vmon  && vmon->type  == TOML_INT) ? (int)vmon->i  : -1;
		nk++;
	}
	rt_rules  = rt_rules_buf;
	rt_nrules = nk;
	fprintf(stderr, "dwm: loaded %d window rules from config\n", nk);
}

static void
reload_config(void)
{
	char datadir[PATH_MAX], script[PATH_MAX + 32];
	static struct timespec last_reload = { 0, 0 };
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	if (last_reload.tv_sec != 0) {
		long elapsed_ms = (now.tv_sec - last_reload.tv_sec) * 1000 +
		                  (now.tv_nsec - last_reload.tv_nsec) / 1000000;
		if (elapsed_ms >= 0 && elapsed_ms < 200)
			return;
	}
	last_reload = now;

	load_hotkeys_toml(toml_hotkeys_path, toml_hotkeys_default_path);
	load_themes_toml(toml_themes_path,   toml_themes_default_path);
	load_rules_toml(toml_rules_path,     toml_rules_default_path);
	grabkeys();

	/* run the theme-apply script asynchronously (updates terminals/quickshell) */
	script[0] = '\0';
	if (getdatadir(datadir, sizeof datadir))
		snprintf(script, sizeof script, "%s/scripts/theme-apply.sh", datadir);
	if (!script[0] || access(script, X_OK) != 0)
		snprintf(script, sizeof script, "%s/scripts/theme-apply.sh", DWM_PATH);
	if (access(script, X_OK) == 0 && fork() == 0) {
		execl("/bin/sh", "sh", script, (char *)NULL);
		_exit(127);
	}
	/* the child is reaped by the SIGCHLD handler */
}

static void
setup_inotify(void)
{
	char cfgdir[PATH_MAX] = "", defdir[PATH_MAX] = "", datadir[PATH_MAX];
	const char *xdg = getenv("XDG_CONFIG_HOME"), *home = getenv("HOME");
	int wd_user = -1, wd_default = -1;

	/* user-editable config: $XDG_CONFIG_HOME/dwm or ~/.config/dwm */
	if (xdg && xdg[0])
		snprintf(cfgdir, sizeof cfgdir, "%s/dwm", xdg);
	else if (home)
		snprintf(cfgdir, sizeof cfgdir, "%s/.config/dwm", home);
	if (cfgdir[0]) {
		snprintf(toml_hotkeys_path, sizeof toml_hotkeys_path, "%s/hotkeys.toml", cfgdir);
		snprintf(toml_themes_path,  sizeof toml_themes_path,  "%s/themes.toml",  cfgdir);
		snprintf(toml_rules_path,   sizeof toml_rules_path,   "%s/window-rules.toml", cfgdir);
	}

	/* shipped defaults: <data dir>/config, else the source tree's DWM_PATH/config */
	if (getdatadir(datadir, sizeof datadir))
		snprintf(defdir, sizeof defdir, "%s/config", datadir);
	if (!defdir[0] || access(defdir, R_OK) != 0)
		snprintf(defdir, sizeof defdir, "%s/config", DWM_PATH);
	snprintf(toml_hotkeys_default_path, sizeof toml_hotkeys_default_path, "%s/hotkeys.toml", defdir);
	snprintf(toml_themes_default_path,  sizeof toml_themes_default_path,  "%s/themes.toml",  defdir);
	snprintf(toml_rules_default_path,   sizeof toml_rules_default_path,   "%s/window-rules.toml", defdir);

	/* SIGUSR1 -> reload, delivered through a pipe so it cannot be missed */
	if (pipe(sigpipe) == 0) {
		int k;
		for (k = 0; k < 2; k++) {
			fcntl(sigpipe[k], F_SETFD, FD_CLOEXEC);
			fcntl(sigpipe[k], F_SETFL, fcntl(sigpipe[k], F_GETFL) | O_NONBLOCK);
		}
		struct sigaction sa;
		sa.sa_handler = sigusr1_handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		sigaction(SIGUSR1, &sa, NULL);
	} else
		sigpipe[0] = sigpipe[1] = -1;

	if ((inotify_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK)) < 0) {
		perror("dwm: inotify_init1");
		return;
	}
	if (cfgdir[0])
		wd_user = inotify_add_watch(inotify_fd, cfgdir, IN_CLOSE_WRITE | IN_MOVED_TO);
	wd_default = inotify_add_watch(inotify_fd, defdir, IN_CLOSE_WRITE | IN_MOVED_TO);
	if (wd_user < 0 && wd_default < 0) { /* nothing watchable: disable hot-reload */
		close(inotify_fd);
		inotify_fd = -1;
	}
}

void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;

	/* clean up any zombies immediately */
	sigchld(0);
	dyn_borderpx = 1; /* initial value; overridden by themes.toml on load */

	/* the one line of bloat that would have saved a lot of time for a lot of people */
	putenv("_JAVA_AWT_WM_NONREPARENTING=1");

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drw = drw_create(dpy, screen, root, sw, sh);
	bh = 30; /* default panel height until the external bar reports its own */
	updategeom();
	/* per-monitor tag ownership needs the monitor list */
	initmonitortags();
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetWMWindowTypeDock] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	netatom[NetDesktopViewport] = XInternAtom(dpy, "_NET_DESKTOP_VIEWPORT", False);
	netatom[NetNumberOfDesktops] = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
	netatom[NetCurrentDesktop] = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
	netatom[NetDesktopNames] = XInternAtom(dpy, "_NET_DESKTOP_NAMES", False);
	netatom[NetWMDesktop] = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
	netatom[NetDwmMonitorDesktops] = XInternAtom(dpy, "_DWM_MONITOR_DESKTOPS", False);
	netatom[NetDwmSelectedMonitor] = XInternAtom(dpy, "_DWM_SELECTED_MONITOR", False);
	netatom[NetWMStrut] = XInternAtom(dpy, "_NET_WM_STRUT", False);
	netatom[NetWMStrutPartial] = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);
	dwmfullscreenmonitorsatom = XInternAtom(dpy, "_DWM_FULLSCREEN_MONITORS", False);
	dwmtagupdateatom = XInternAtom(dpy, "DWM_TAG_UPDATE", False);
	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResizeBR] = drw_cur_create(drw, XC_bottom_right_corner);
	cursor[CurResizeBL] = drw_cur_create(drw, XC_bottom_left_corner);
	cursor[CurResizeTR] = drw_cur_create(drw, XC_top_right_corner);
	cursor[CurResizeTL] = drw_cur_create(drw, XC_top_left_corner);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
	/* init appearance (fallback colours; themes.toml is applied by reload_config below) */
	scheme = ecalloc(2, sizeof(Clr *));
	for (i = 0; i < 2; i++)
		scheme[i] = drw_scm_create(drw, defclrs[i], 3);
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	setnumdesktops();
	updatecurrentdesktop();
	setdesktopnames();
	setviewport();
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	/* hot-reload plumbing, then load the TOML configs (this also grabs the keys) */
	setup_inotify();
	effects_init(dpy, root);
	reload_config();
	focus(NULL);
	refreshbar();
}

void
setviewport(void)
{
	long data[] = { 0, 0 };

	XChangeProperty(dpy, root, netatom[NetDesktopViewport], XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)data, 2);
}

void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
sigchld(int unused)
{
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("can't install SIGCHLD handler:");
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

int
swallow(Client *p, Client *c)
{
	if (c->noswallow > 0)
		return 0;
	if (!swallowfloating && c->isfloating)
		return 0;

	detach(c);
	detachstack(c);

	setclientstate(c, WithdrawnState);
	XUnmapWindow(dpy, p->win);

	p->swallowing = c;
	c->mon = p->mon;

	Window w = p->win;
	p->win = c->win;
	c->win = w;

	XChangeProperty(dpy, c->win, netatom[NetClientList], XA_WINDOW, 32, PropModeReplace,
		(unsigned char *) &(p->win), 1);

	updatetitle(p);
	XMoveResizeWindow(dpy, p->win, p->x, p->y, p->w, p->h);
	arrange(p->mon);
	configure(p);
	XMapWindow(dpy, p->win);
	updateclientlist();
	return 1;
}

Client *
swallowingclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next) {
		for (c = m->clients; c; c = c->next) {
			if (c->swallowing && c->swallowing->win == w)
				return c;
		}
	}

	return NULL;
}

extern char **environ;

void
spawn(const Arg *arg)
{
	char **argv = (char **)arg->v;

	if (!argv || !argv[0])
		return;
	posix_spawnp(NULL, argv[0], NULL, NULL, argv, environ);
}

/* Ask an already running Quickshell to refresh (it is started from
 * autostart.sh). Asynchronous: a stuck shell must not block dwm's startup. */
static void
refreshbar(void)
{
	static char *argv[] = { "sh", "-c", "quickshell ipc call weather refresh >/dev/null 2>&1", NULL };

	posix_spawn(NULL, "/bin/sh", NULL, NULL, argv, environ);
}

/* Send the selected client to a tag and follow it there */
void
tag(const Arg *arg)
{
	Monitor *m;
	Client *c = selmon->sel;

	if (!c || !(arg->ui & TAGMASK))
		return;
	if (!(arg->ui & getmontagmask(selmon->num))) {
		/* the tag belongs to another monitor: hand the client over and follow */
		for (m = mons; m && !(arg->ui & getmontagmask(m->num)); m = m->next);
		if (!m)
			return;
		sendmon(c, m);
		if (selmon != m) {
			unfocus(selmon->sel, 0);
			selmon = m;
			focus(NULL);
		}
	}
	c->tags = arg->ui & TAGMASK & getmontagmask(c->mon->num);
	setclientdesktop(c);
	updateclientlist();
	view(arg);
	warppointer(selmon->sel);
}

void
tagmon(const Arg *arg)
{
	Client *c = selmon->sel;
	if (!c || !mons->next)
		return;
	if (c->isfullscreen) {
		c->isfullscreen = 0;
		sendmon(c, dirtomon(arg->i));
		c->isfullscreen = 1;
		if (c->fakefullscreen != 1) {
			resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
			XRaiseWindow(dpy, c->win);
		}
	} else
		sendmon(c, dirtomon(arg->i));
}

Client *
termforwin(const Client *w)
{
	Client *c;
	Monitor *m;

	if (!w->pid || w->isterminal)
		return NULL;

	for (m = mons; m; m = m->next) {
		for (c = m->clients; c; c = c->next) {
			if (c->isterminal && !c->swallowing && c->pid && isdescprocess(c->pid, w->pid))
				return c;
		}
	}

	return NULL;
}

static void
getfacts(Monitor *m, int msize, int ssize, float *mf, float *sf, int *mr, int *sr)
{
	unsigned int n;
	float mfacts = 0, sfacts = 0;
	int mtotal = 0, stotal = 0;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++)
		if (n < m->nmaster)
			mfacts += c->cfact;
		else
			sfacts += c->cfact;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++)
		if (n < m->nmaster)
			mtotal += msize * (c->cfact / mfacts);
		else
			stotal += ssize * (c->cfact / sfacts);

	*mf = mfacts; // total factor of master area
	*sf = sfacts; // total factor of stack area
	*mr = msize - mtotal; // the remainder (rest) of pixels after a cfacts master split
	*sr = ssize - stotal; // the remainder (rest) of pixels after a cfacts stack split
}

static void
tile(Monitor *m)
{
	unsigned int i, n;
	int mx, my, mh, mw;   /* master area */
	int stx, sty, sth, stw; /* stack area */
	float mfacts, sfacts;
	int mrest, srest;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);

	if (n == 0)
		return;

	stx = mx = m->wx;
	sty = my = m->wy;
	sth = mh = m->wh;
	stw = mw = m->ww;

	if (m->nmaster && n > (unsigned int)m->nmaster) {
		stw = mw * (1 - m->mfact);
		mw = mw * m->mfact;
		stx = mx + mw;
	}

	getfacts(m, mh, sth, &mfacts, &sfacts, &mrest, &srest);

	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < (unsigned int)m->nmaster) {
			resize(c, mx, my, mw - (2*c->bw), (mh / mfacts) * c->cfact + (i < (unsigned int)mrest ? 1 : 0) - (2*c->bw), 0);
			my += HEIGHT(c);
		} else {
			resize(c, stx, sty, stw - (2*c->bw), (sth / sfacts) * c->cfact + ((i - m->nmaster) < (unsigned int)srest ? 1 : 0) - (2*c->bw), 0);
			sty += HEIGHT(c);
		}
}

void
togglebar(const Arg *arg)
{
	(void)arg;
	/* the tray is not announced by a map request: look for it, but only now
	 * that the bar had time to settle (earlier its size would be wrong) */
	if (!selmon->traywin)
		scantray();

	selmon->showbar = selmon->pertag->showbars[selmon->pertag->curtag] = !selmon->showbar;
	updatebarpos(selmon);
	if (selmon->barwin)
		XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, selmon->bh);
	if (selmon->traywin)
		XMoveResizeWindow(dpy, selmon->traywin, selmon->tx, selmon->by, selmon->tw, selmon->bh);
	arrange(selmon);
}

void
togglefakefullscreen(const Arg *arg)
{
	Client *c = selmon->sel;
	if (!c)
		return;

	if (c->fakefullscreen != 1 && c->isfullscreen) { // exit fullscreen --> fake fullscreen
		c->fakefullscreen = 2;
		setfullscreen(c, 0);
	} else if (c->fakefullscreen == 1) {
		setfullscreen(c, 0);
		c->fakefullscreen = 0;
	} else {
		c->fakefullscreen = 1;
		setfullscreen(c, 1);
	}
}

void
togglefloating(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (selmon->sel->isfullscreen && selmon->sel->fakefullscreen != 1) /* no support for fullscreen windows */
		return;
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if (selmon->sel->isfloating)
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
			selmon->sel->w, selmon->sel->h, 0);
	arrange(selmon);
}

void
toggletag(const Arg *arg)
{
	unsigned int newtags, montags;

	if (!selmon->sel)
		return;
	montags = getmontagmask(selmon->num);

	/* only tags that belong to this monitor may be toggled */
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK & montags);
	if (newtags && (newtags & montags)) {
		selmon->sel->tags = newtags & montags;
		setclientdesktop(selmon->sel);
		arrange(selmon);
		focus(NULL);
	}
	updatecurrentdesktop();
}

void
toggleview(const Arg *arg)
{
	unsigned int newtagset, montags;
	Pertag *pt = selmon->pertag;
	int i;

	montags = getmontagmask(selmon->num);
	/* only tags that belong to this monitor may be toggled */
	newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK & montags);
	if (newtagset) {
		selmon->tagset[selmon->seltags] = newtagset;

		if (newtagset == montags)
			pt->curtag = 0; /* all tags: slot 0 holds the shared settings */
		else if (pt->curtag == 0 || !(newtagset & (1u << (pt->curtag - 1)))) {
			/* the current tag was switched off: use the lowest one still shown */
			for (i = 0; !(newtagset & (1u << i)); i++);
			pt->curtag = i + 1;
		}

		/* apply the settings stored for this view */
		selmon->nmaster = pt->nmasters[pt->curtag];
		selmon->mfact = pt->mfacts[pt->curtag];
		selmon->sellt = pt->sellts[pt->curtag];
		selmon->lt[selmon->sellt] = pt->ltidxs[pt->curtag][selmon->sellt];
		selmon->lt[selmon->sellt^1] = pt->ltidxs[pt->curtag][selmon->sellt^1];

		if (selmon->showbar != pt->showbars[pt->curtag])
			togglebar(NULL);

		arrange(selmon);
		focus(NULL);
	}
	updatecurrentdesktop();
}

void
unfocus(Client *c, int revert)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (revert) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void
unmanage(Client *c, int destroyed)
{
	Monitor *m = c->mon;
	Client *s;
	XWindowChanges wc;

	if (c->swallowing) {
		unswallow(c);
		return;
	}

	if ((s = swallowingclient(c->win))) {
		free(s->swallowing);
		s->swallowing = NULL;
		arrange(m);
		focus(NULL);
		return;
	}

	detach(c);
	detachstack(c);
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xerrordummy);
		XSelectInput(dpy, c->win, NoEventMask);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);
	updateclientlist();
	arrange(m);
	/* Hand focus to the next window in the focus history (the one used before
	 * this one). arrange() -> restack() just discarded the EnterNotify events
	 * the unmapped window produced, so the pointer would be left over a
	 * different window than the one focused: move it along with the focus. */
	focus(NULL);
	if (running && m == selmon)
		warppointer(selmon->sel);
}

void
unmanagealtbar(Window w)
{
	Monitor *m = wintomon(w);

	if (!m)
		return;

	m->barwin = 0;
	m->by = 0;
	m->bh = 0;
	updatebarpos(m);
	arrange(m);
}

void
unmanagetray(Window w)
{
	Monitor *m = wintomon(w);

	if (!m)
		return;

	m->traywin = 0;
	m->tx = 0;
	m->tw = 0;
	updatebarpos(m);
	arrange(m);
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	} else if ((m = wintomon(ev->window)) && m->barwin == ev->window)
		unmanagealtbar(ev->window);
	else if (m && m->traywin == ev->window)
		unmanagetray(ev->window);
}

void
unswallow(Client *c)
{
	c->win = c->swallowing->win;

	free(c->swallowing);
	c->swallowing = NULL;

	XDeleteProperty(dpy, c->win, netatom[NetClientList]);

	/* unfullscreen the client */
	setfullscreen(c, 0);
	updatetitle(c);
	arrange(c->mon);
	XMapWindow(dpy, c->win);
	XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	setclientstate(c, NormalState);
	focus(NULL);
	arrange(c->mon);
	configure(c);
	updateclientlist();
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh -= m->bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + m->bh : m->wy;
	} else
		m->by = -m->bh;
}

void
updateclientlist(void)
{
	Client *c;
	Monitor *m;
	XWindowAttributes wa;
	Window *clients;
	unsigned long count = 0, index = 0;

	for (m = mons; m; m = m->next) {
		if (m->barwin)
			count++;
		if (m->traywin)
			count++;
		for (c = m->clients; c; c = c->next)
			count++;
	}

	clients = ecalloc(count ? count : 1, sizeof(Window));
	for (m = mons; m; m = m->next) {
		if (m->barwin && XGetWindowAttributes(dpy, m->barwin, &wa) &&
		    !wa.override_redirect)
			clients[index++] = m->barwin;
		if (m->traywin)
			clients[index++] = m->traywin;
		for (c = m->clients; c; c = c->next)
			clients[index++] = c->win;
	}

	XChangeProperty(dpy, root, netatom[NetClientList],
		XA_WINDOW, 32, PropModeReplace,
		(unsigned char *)clients, index);
	free(clients);
}

int
isvisiblefullscreen(Client *c)
{
	return c && c->isfullscreen && c->fakefullscreen != 1 && ISVISIBLE(c);
}

int
monitorhasfullscreen(Monitor *m)
{
	Client *c;

	if (!m)
		return 0;
	for (c = m->clients; c; c = c->next)
		if (isvisiblefullscreen(c))
			return 1;
	return 0;
}

void
updatefullscreenmonitors(void)
{
	Monitor *m;
	long *monitors;
	int logicalindex;
	unsigned int count = 0;

	for (m = mons; m; m = m->next)
		count++;
	if (count > 64)
		count = 64;
	monitors = count ? ecalloc(count, sizeof(*monitors)) : NULL;
	count = 0;
	for (m = mons; m; m = m->next) {
		if (!monitorhasfullscreen(m))
			continue;
		logicalindex = getmonlogicalindex(m);
		if (logicalindex >= 0)
			monitors[count++] = logicalindex;
	}
	XChangeProperty(dpy, root, dwmfullscreenmonitorsatom, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)monitors, count);
	free(monitors);
}

void
updateselectedmonitor(void)
{
	long data[] = { 0 };
	int logicalindex;

	if (!selmon)
		return;
	logicalindex = getmonlogicalindex(selmon);
	if (logicalindex < 0
	|| (selectedmonitorcachevalid && logicalindex == selectedmonitorcache))
		return;
	data[0] = logicalindex;
	XChangeProperty(dpy, root, netatom[NetDwmSelectedMonitor], XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)data, 1);
	selectedmonitorcache = logicalindex;
	selectedmonitorcachevalid = 1;
}

void
updatecurrentdesktop(void)
{
	long data[] = { 0 };
	long *monitor_desktops;
	Monitor *m;
	int count, i, logicalindex;
	unsigned int tagset;

	if (!selmon)
		return;

	tagset = selmon->tagset[selmon->seltags];
	for (i = 0; i < NTAGS && !(tagset & 1 << i); i++);
	data[0] = (i < NTAGS) ? i : 0;
	XChangeProperty(dpy, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)data, 1);

	for (count = 0, m = mons; m; m = m->next)
		count++;
	if (count == 0)
		return;

	if (count > 64)
		count = 64;

	monitor_desktops = ecalloc(count * 5, sizeof(long));
	for (m = mons; m; m = m->next) {
		logicalindex = getmonlogicalindex(m);
		if (logicalindex < 0 || logicalindex >= count)
			continue;

		tagset = m->tagset[m->seltags];
		for (i = 0; i < NTAGS && !(tagset & 1 << i); i++);
		monitor_desktops[logicalindex * 5]     = m->mx;
		monitor_desktops[logicalindex * 5 + 1] = m->my;
		monitor_desktops[logicalindex * 5 + 2] = m->mw;
		monitor_desktops[logicalindex * 5 + 3] = m->mh;
		monitor_desktops[logicalindex * 5 + 4] = (i < NTAGS) ? i : 0;
	}
	XChangeProperty(dpy, root, netatom[NetDwmMonitorDesktops], XA_INTEGER, 32,
		PropModeReplace, (unsigned char *)monitor_desktops, count * 5);
	free(monitor_desktops);
	updateselectedmonitor();
	updatefullscreenmonitors();
}

int
updategeom(void)
{
	int dirty = 0, nmonschanged = 0;

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;
		nmonschanged = (nn != n);
		/* new monitors if nn > n */
		for (i = n; i < nn; i++) {
			for (m = mons; m && m->next; m = m->next);
			if (m)
				m->next = createmon();
			else
				mons = createmon();
		}
		for (i = 0, m = mons; i < nn && m; m = m->next, i++)
			if (i >= n
			|| unique[i].x_org != m->mx || unique[i].y_org != m->my
			|| unique[i].width != m->mw || unique[i].height != m->mh)
			{
				dirty = 1;
				m->num = i;
				m->mx = m->wx = unique[i].x_org;
				m->my = m->wy = unique[i].y_org;
				m->mw = m->ww = unique[i].width;
				m->mh = m->wh = unique[i].height;
				updatebarpos(m);
			}
		/* removed monitors if n > nn */
		for (i = nn; i < n; i++) {
			for (m = mons; m && m->next; m = m->next);
			while ((c = m->clients)) {
				dirty = 1;
				m->clients = c->next;
				detachstack(c);
				c->mon = mons;
				attach(c);
				attachstack(c);
			}
			if (m == selmon)
				selmon = mons;
			cleanupmon(m);
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = wintomon(root);
		if (nmonschanged)
			remaptags();
	}
	return dirty;
}

void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

void
view(const Arg *arg)
{
	Pertag *pt;
	Monitor *m, *targetmon = NULL;
	unsigned int montags, newset, tmptag;
	int i;

	montags = getmontagmask(selmon->num);

	/* tags owned by another monitor: switch to that monitor first */
	if ((arg->ui & TAGMASK) && !(arg->ui & montags)) {
		for (m = mons; m && !(arg->ui & getmontagmask(m->num)); m = m->next);
		if (!m || m == selmon)
			return;
		targetmon = m;
		unfocus(selmon->sel, 0);
		selmon = targetmon;
		focus(NULL);
		montags = getmontagmask(selmon->num);
	}

	newset = arg->ui & TAGMASK & montags;
	if ((arg->ui & TAGMASK) && newset == selmon->tagset[selmon->seltags]) {
		/* already showing these tags: at most the monitor changed */
		if (targetmon) {
			arrange(selmon);
			focus(NULL);
			warppointer(selmon->sel);
			updatecurrentdesktop();
		}
		return;
	}

	pt = selmon->pertag;
	selmon->seltags ^= 1; /* toggle sel tagset */
	if (arg->ui & TAGMASK) {
		selmon->tagset[selmon->seltags] = newset;
		pt->prevtag = pt->curtag;
		if (newset == montags)
			pt->curtag = 0; /* all tags: slot 0 holds the shared settings */
		else {
			for (i = 0; !(newset & (1u << i)); i++);
			pt->curtag = i + 1;
		}
	} else { /* back to the previous view */
		tmptag = pt->prevtag;
		pt->prevtag = pt->curtag;
		pt->curtag = tmptag;
	}

	selmon->nmaster = pt->nmasters[pt->curtag];
	selmon->mfact = pt->mfacts[pt->curtag];
	selmon->sellt = pt->sellts[pt->curtag];
	selmon->lt[selmon->sellt] = pt->ltidxs[pt->curtag][selmon->sellt];
	selmon->lt[selmon->sellt^1] = pt->ltidxs[pt->curtag][selmon->sellt^1];

	if (selmon->showbar != pt->showbars[pt->curtag])
		togglebar(NULL);

	arrange(selmon);
	focus(NULL); /* the most recently used window of this view */
	warppointer(selmon->sel);
	updatecurrentdesktop();
}

pid_t
winpid(Window w)
{
	pid_t result = 0;
	xcb_res_client_id_spec_t spec = {0};
	xcb_generic_error_t *err = NULL;
	xcb_res_query_client_ids_cookie_t cookie;
	xcb_res_query_client_ids_reply_t *r;
	xcb_res_client_id_value_iterator_t i;

	spec.client = w;
	spec.mask = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID;
	cookie = xcb_res_query_client_ids(xcon, 1, &spec);
	if (!(r = xcb_res_query_client_ids_reply(xcon, cookie, &err))) {
		free(err);
		return 0;
	}

	for (i = xcb_res_query_client_ids_ids_iterator(r); i.rem; xcb_res_client_id_value_next(&i))
		if (i.data->spec.mask & XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID) {
			result = *xcb_res_client_id_value_value(i.data);
			break;
		}
	free(r);

	return result == (pid_t)-1 ? 0 : result;
}

Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == w)
				return c;
	return NULL;
}

Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin || w == m->traywin)
			return m;
	if ((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

int
wmclasscontains(Window win, const char *class, const char *name)
{
	XClassHint ch = { NULL, NULL };
	int res = 1;

	if (XGetClassHint(dpy, win, &ch)) {
		if (ch.res_name && strstr(ch.res_name, name) == NULL)
			res = 0;
		if (ch.res_class && strstr(ch.res_class, class) == NULL)
			res = 0;
	} else
		res = 0;

	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);

	return res;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("dwm: another window manager is already running");
	return -1;
}

void
zoom(const Arg *arg)
{
	(void)arg;
	Client *c = selmon->sel;

	if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating)
		return;
	if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next)))
		return;
	pop(c);
}

/* Monitor-specific tag management functions */
/* Position of target in the monitor list, or -1 if it is not there */
int
getmonlogicalindex(Monitor *target)
{
	Monitor *m;
	int index = 0;

	if (!target)
		return -1;
	for (m = mons; m; m = m->next, index++)
		if (m == target)
			return index;
	return -1;
}

/* Tags are split between the monitors: with N monitors each owns about
 * LENGTH(tags)/N consecutive tags, the last one also takes the remainder. */
unsigned int
getmontagmask(int monnum)
{
	unsigned int ntags = LENGTH(tags), per, start, end, mask = 0, i;
	int nmons = 0, idx = -1;
	Monitor *m;

	for (m = mons; m; m = m->next, nmons++)
		if (m->num == monnum && idx < 0)
			idx = nmons;
	if (idx < 0 || nmons <= 1)
		return TAGMASK;

	per = ntags / nmons;
	if (!per)
		per = 1;
	start = idx * per;
	end = (idx == nmons - 1) ? ntags : start + per;
	if (start >= ntags)
		start = ntags - 1;
	if (end > ntags)
		end = ntags;
	for (i = start; i < end; i++)
		mask |= 1u << i;
	return mask;
}



/* Initialize monitor-specific tags after all monitors are created */
/* Start every monitor on the first tag it owns */
void
initmonitortags(void)
{
	Monitor *m;
	unsigned int first;
	int i;

	for (m = mons; m; m = m->next) {
		first = getmontagmask(m->num);
		first &= -first; /* lowest set bit */
		m->tagset[0] = m->tagset[1] = first;
		for (i = 0; !(first & (1u << i)); i++);
		m->pertag->curtag = m->pertag->prevtag = i + 1;
	}
}

/* After monitors were added or removed the tag split changed: make sure
 * every monitor and client only uses tags it owns, or nothing is visible. */
static void
remaptags(void)
{
	Monitor *m;
	Client *c;
	unsigned int montags;

	initmonitortags();
	for (m = mons; m; m = m->next) {
		montags = getmontagmask(m->num);
		for (c = m->clients; c; c = c->next) {
			c->tags &= montags;
			if (!c->tags)
				c->tags = m->tagset[m->seltags];
			setclientdesktop(c);
		}
	}
}

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION);
	else if (argc != 1)
		die("usage: dwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display");
	if (!(xcon = XGetXCBConnection(dpy)))
		die("dwm: cannot get xcb connection");
	checkotherwm();
	setup();
	scan();
	runautostart();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
