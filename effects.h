#ifndef EFFECTS_H
#define EFFECTS_H

#include <X11/Xlib.h>
#include "types.h"

/* Effect toggles */
void togglewarm(const Arg *arg);

/* Modular initialization to keep dwm.c globals static */
void effects_init(Display *dpy, Window root);

#endif /* EFFECTS_H */
