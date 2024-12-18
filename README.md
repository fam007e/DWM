dwm - dynamic window manager
============================
dwm is an extremely fast, small, and dynamic window manager for X.

This is my personal fork with following patches:

+ alwayscenter
+ alwaysfullscreen
+ auto start
+ cfacts
+ chatterino bottom
+ cool autostart
+ fakefullscreen client (with resize fix for chrome-based browsers + noborder fix)
+ multikeycode
+ movestack
+ noborder (floating + border flicker fix)
+ pertag
+ placemouse
+ resizepoint
+ statuscmd
+ swallow
+ switchtag
+ systray
+ true fullscreen
+ hide vacant tags
+ warp v2
+ winicon

Some patches are rewritten or modified to work together.


Requirements
------------
In order to build dwm you need the Xlib header files.

```sh
sudo pacman -S --needed bc jq base-devel extra/git extra/libx11 extra/libxcb extra/libxinerama extra/libxft extra/imlib2
```

If you find yourself missing a library then this can usually be found by searching for the file name using pacman:
```sh
pacman -F Xlib-xcb.h
extra/libx11 1.6.12-1 [installed: 1.7.2-1]
   usr/include/X11/Xlib-xcb.h
```

Installation
------------
Edit config.mk to match your local setup (dwm is installed into
the /usr/local namespace by default).

Afterwards enter the following command to build and install dwm (if
necessary as root):
```sh
make clean install
```

Running dwm
-----------
Add the following line to your .xinitrc to start dwm using startx:
```sh
exec dwm
```

In order to connect dwm to a specific display, make sure that
the DISPLAY environment variable is set correctly, e.g.:
```sh
DISPLAY=foo.bar:1 exec dwm
```

(This will start dwm on display `:1` of the host `foo.bar`.)

In order to display status info in the bar, you can do something
like this in your .xinitrc:
```sh
while xsetroot -name "`date` `uptime | sed 's/.*,//'`"
do
    sleep 1
done &
exec dwm
```

Configuration
-------------
The configuration of dwm is done by creating a custom `config.h`
and (re)compiling the source code.

Credits
-------
Project was inspired by [@ChrisTitusTech](https://github.com/ChrisTitusTech/dwm-titus) `dwm-titus` repo for Arch.
