#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>
#include <X11/extensions/Xrandr.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drw.h"
#include "util.h"
#include "effects.h"

/* Local copies of shared X11 globals */
static Display *dpy;
static Window root;

/* State maintained for color temperature */
static int current_temp = 6500;
static double saved_brightness = 1.0;

void
effects_init(Display *d, Window r)
{
    dpy = d;
    root = r;
}

double
get_neutral_brightness(void)
{
    XRRScreenResources *res = XRRGetScreenResources(dpy, root);
    if (!res || res->ncrtc == 0) {
        if (res) XRRFreeScreenResources(res);
        return 1.0;
    }

    RRCrtc crtc = res->crtcs[0];
    XRRCrtcGamma *current_gamma = XRRGetCrtcGamma(dpy, crtc);
    double brightness = 1.0;

    if (current_gamma && current_gamma->size > 0) {
        int mid_point = current_gamma->size / 2;
        double expected_mid = (double)mid_point / (current_gamma->size - 1) * 65535.0;
        brightness = current_gamma->green[mid_point] / expected_mid;

        if (brightness < 0.1) brightness = 0.1;
        if (brightness > 1.0) brightness = 1.0;

        XRRFreeGamma(current_gamma);
    }

    XRRFreeScreenResources(res);
    return brightness;
}

static double
get_current_brightness_compensated(void)
{
    XRRScreenResources *res = XRRGetScreenResources(dpy, root);
    if (!res || res->ncrtc == 0) {
        if (res) XRRFreeScreenResources(res);
        return 1.0;
    }

    RRCrtc crtc = res->crtcs[0];
    XRRCrtcGamma *current_gamma = XRRGetCrtcGamma(dpy, crtc);
    double brightness = 1.0;

    if (current_gamma && current_gamma->size > 0) {
        int mid_point = current_gamma->size / 2;
        double tmpKelvin = current_temp / 100.0;
        double expected_green_mult;

        if (tmpKelvin <= 66) {
            double tmp = tmpKelvin;
            double green = 99.4708025861 * log(tmp) - 161.1195681661;
            green = fmax(0, fmin(255, green));
            expected_green_mult = green / 255.0;
        } else {
            double tmp = tmpKelvin - 60;
            double green = 288.1221695283 * pow(tmp, -0.0755148492);
            green = fmax(0, fmin(255, green));
            expected_green_mult = green / 255.0;
        }

        double expected_mid = (double)mid_point / (current_gamma->size - 1) * 65535.0;
        double actual_value = current_gamma->green[mid_point];
        brightness = (actual_value / expected_mid) / expected_green_mult;

        if (brightness < 0.1) brightness = 0.1;
        if (brightness > 1.0) brightness = 1.0;

        XRRFreeGamma(current_gamma);
    }

    XRRFreeScreenResources(res);
    return brightness;
}

void
setcolortemp(int temp)
{
    XRRScreenResources *res = XRRGetScreenResources(dpy, root);
    if (!res) return;

    double tmpKelvin = temp / 100.0;
    double red, green, blue;

    if (tmpKelvin <= 66) {
        red = 255;
    } else {
        double tmp = tmpKelvin - 60;
        red = 329.698727446 * pow(tmp, -0.1332047592);
        red = fmax(0, fmin(255, red));
    }

    if (tmpKelvin <= 66) {
        double tmp = tmpKelvin;
        green = 99.4708025861 * log(tmp) - 161.1195681661;
        green = fmax(0, fmin(255, green));
    } else {
        double tmp = tmpKelvin - 60;
        green = 288.1221695283 * pow(tmp, -0.0755148492);
        green = fmax(0, fmin(255, green));
    }

    if (tmpKelvin >= 66) {
        blue = 255;
    } else if (tmpKelvin <= 19) {
        blue = 0;
    } else {
        double tmp = tmpKelvin - 10;
        blue = 138.5177312231 * log(tmp) - 305.0447927307;
        blue = fmax(0, fmin(255, blue));
    }

    double current_brightness = saved_brightness;
    double r_mult = (red / 255.0) * current_brightness;
    double g_mult = (green / 255.0) * current_brightness;
    double b_mult = (blue / 255.0) * current_brightness;

    for (int c = 0; c < res->ncrtc; c++) {
        RRCrtc crtc = res->crtcs[c];
        int size = XRRGetCrtcGammaSize(dpy, crtc);
        if (size == 0) continue;

        XRRCrtcGamma *gamma = XRRAllocGamma(size);
        if (!gamma) continue;

        for (int i = 0; i < size; i++) {
            double ratio = (double)i / (size - 1.0);
            gamma->red[i] = (unsigned short)(65535.0 * r_mult * ratio + 0.5);
            gamma->green[i] = (unsigned short)(65535.0 * g_mult * ratio + 0.5);
            gamma->blue[i] = (unsigned short)(65535.0 * b_mult * ratio + 0.5);
        }

        XRRSetCrtcGamma(dpy, crtc, gamma);
        XRRFreeGamma(gamma);
    }
    XRRFreeScreenResources(res);
}

void
togglewarm(const Arg *arg)
{
    if (!dpy) return; /* safety if called before effects_init */
    if (current_temp == 6500) {
        saved_brightness = get_neutral_brightness();
    } else {
        saved_brightness = get_current_brightness_compensated();
    }

    current_temp = (current_temp == 6500) ? 3500 : 6500;
    setcolortemp(current_temp);
}
