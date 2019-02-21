/* 
 * iuvui - intel-undervolt user interface
 * made with nuklear - version git commit 181cfd8 - public domain
 * using demo "x11_opengl2" as base
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>

#include <GL/glx.h>
#include <GL/glxext.h>

#include "intel_undervolt.h"

#define IUVUIVERSION    "iuvui 0.3.0"

#define SEPARATOR       "---------------------------------------------------------------------"

#define GUILOGSIZE      256
#define ROWHEIGHT       25
#define ROWWIDTH        110
#define MVLABEL         15

/* number of power value elements - make dynamic? */
#define POWERVALEL      4
/* number of temperature elements - make dynamic? */
#define TEMPERATUREEL   9

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_XLIB_GL2_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_xlib_gl2.h"

#define WINDOW_WIDTH 510
#define WINDOW_HEIGHT 570

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

struct XWindow {
        Display *dpy;
        Window win;
        XVisualInfo *vis;
        Colormap cmap;
        XSetWindowAttributes swa;
        XWindowAttributes attr;
        GLXFBConfig fbc;
        Atom wm_delete_window;
        int width, height;
};
static int gl_err = nk_false;
static int gl_error_handler(Display *dpy, XErrorEvent *ev)
{NK_UNUSED(dpy); NK_UNUSED(ev); gl_err = nk_true; return 0;}

static void
die (const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fputs("\n", stderr);
        exit(EXIT_FAILURE);
}

static int
has_extension (const char *string, const char *ext) {
        const char *start, *where, *term;
        where = strchr(ext, ' ');
        if (where || *ext == '\0')
                return nk_false;

        for (start = string;;) {
                where = strstr((const char*)start, ext);
                if (!where) break;
                term = where + strlen(ext);
                if (where == start || *(where - 1) == ' ') {
                        if (*term == ' ' || *term == '\0')
                                return nk_true;
                }
                start = term;
        }
        return nk_false;
}

int main(void) {

        /* check if user is root or has root access */
        if (geteuid() != 0) {
                fprintf(stdout, "%s", "\nplease run as root\nexiting...\n");
                exit(1);
        }

        /* check if intel-undervolt is available and accessible */
        if (access("/bin/intel-undervolt", X_OK) < 0) {
                fprintf(stdout, "%s", "\nerror accessing intel-undervolt. Does it exist in /bin/ ?\n");
                exit(1);
        }

        /* Platform */
        int running = 1;
        struct XWindow win;
        GLXContext glContext;
        struct nk_context *ctx;
        struct nk_colorf bg;

        /* variables for intel-undervolt */
        static char onBoot[MAXLENSENTENCE];
        static char guiLog[GUILOGSIZE];
        int currentValues[COUNTELEMENTS] = {0};
        int newValues[COUNTELEMENTS] = {0};
        time_t startT = 0.0l;
        time_t endT = 0.0l;
        /* power consumption measurement: */
        static char currPowerVals[POWERVALEL][BUFSZSMALL] = {""};
        static char currTempVals[TEMPERATUREEL][BUFSZSMALL] = {""};
        int maxname = 0;
        powercap_list_t *pcapList = getPowercap(&maxname);
        hwmon_list_t *hwmonList = getCoretemp(&maxname);

        memset(&win, 0, sizeof(win));
        win.dpy = XOpenDisplay(NULL);
        if (!win.dpy) die ("Failed to open X display\n");
        {
                /* check glx version */
                int glx_major, glx_minor;
                if (!glXQueryVersion(win.dpy, &glx_major, &glx_minor))
                        die("[X11]: Error: Failed to query OpenGL version\n");
                if ((glx_major == 1 && glx_minor < 3) || (glx_major < 1))
                        die("[X11]: Error: Invalid GLX version!\n");
        }
        {
                /* find and pick matching framebuffer visual */
                int fb_count;
                static GLint attr[] = {
                        GLX_X_RENDERABLE,   True,
                        GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
                        GLX_RENDER_TYPE,        GLX_RGBA_BIT,
                        GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
                        GLX_RED_SIZE,           8,
                        GLX_GREEN_SIZE,         8,
                        GLX_BLUE_SIZE,          8,
                        GLX_ALPHA_SIZE,         8,
                        GLX_DEPTH_SIZE,         24,
                        GLX_STENCIL_SIZE,   8,
                        GLX_DOUBLEBUFFER,   True,
                        None
                };
                GLXFBConfig *fbc;
                fbc = glXChooseFBConfig(win.dpy, DefaultScreen(win.dpy), attr, &fb_count);
                if (!fbc) die("[X11]: Error: failed to retrieve framebuffer configuration\n");
                {
                        /* pick framebuffer with most samples per pixel */
                        int i;
                        int fb_best = -1, best_num_samples = -1;
                        for (i = 0; i < fb_count; ++i) {
                                XVisualInfo *vi = glXGetVisualFromFBConfig(win.dpy, fbc[i]);
                                if (vi) {
                                        int sample_buffer, samples;
                                        glXGetFBConfigAttrib(win.dpy, fbc[i], GLX_SAMPLE_BUFFERS, &sample_buffer);
                                        glXGetFBConfigAttrib(win.dpy, fbc[i], GLX_SAMPLES, &samples);
                                        if ((fb_best < 0) || (sample_buffer && samples > best_num_samples))
                                                fb_best = i, best_num_samples = samples;
                                }
                        }
                        win.fbc = fbc[fb_best];
                        XFree(fbc);
                        win.vis = glXGetVisualFromFBConfig(win.dpy, win.fbc);
                }
        }
        {
                /* create window */
                win.cmap = XCreateColormap(win.dpy, RootWindow(win.dpy, win.vis->screen), win.vis->visual, AllocNone);
                win.swa.colormap =  win.cmap;
                win.swa.background_pixmap = None;
                win.swa.border_pixel = 0;
                win.swa.event_mask =
                        ExposureMask | KeyPressMask | KeyReleaseMask |
                        ButtonPress | ButtonReleaseMask| ButtonMotionMask |
                        Button1MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask|
                        PointerMotionMask| StructureNotifyMask;
                win.win = XCreateWindow(win.dpy, RootWindow(win.dpy, win.vis->screen), 0, 0,
                        WINDOW_WIDTH, WINDOW_HEIGHT, 0, win.vis->depth, InputOutput,
                        win.vis->visual, CWBorderPixel|CWColormap|CWEventMask, &win.swa);
                if (!win.win) die("[X11]: Failed to create window\n");
                XFree(win.vis);
                XStoreName(win.dpy, win.win, IUVUIVERSION);
                XMapWindow(win.dpy, win.win);
                win.wm_delete_window = XInternAtom(win.dpy, "WM_DELETE_WINDOW", False);
                XSetWMProtocols(win.dpy, win.win, &win.wm_delete_window, 1);
        }
        {
                /* create opengl context */
                typedef GLXContext(*glxCreateContext)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
                int(*old_handler)(Display*, XErrorEvent*) = XSetErrorHandler(gl_error_handler);
                const char *extensions_str = glXQueryExtensionsString(win.dpy, DefaultScreen(win.dpy));
                glxCreateContext create_context = (glxCreateContext)
                        glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");

                gl_err = nk_false;
                if (!has_extension(extensions_str, "GLX_ARB_create_context") || !create_context) {
                        fprintf(stdout, "[X11]: glXCreateContextAttribARB() not found...\n");
                        fprintf(stdout, "[X11]: ... using old-style GLX context\n");
                        glContext = glXCreateNewContext(win.dpy, win.fbc, GLX_RGBA_TYPE, 0, True);
                } else {
                        GLint attr[] = {
                                GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
                                GLX_CONTEXT_MINOR_VERSION_ARB, 2,
                                None
                        };
                        glContext = create_context(win.dpy, win.fbc, 0, True, attr);
                        XSync(win.dpy, False);
                        if (gl_err || !glContext) {
                                /* Could not create GL 3.0 context. Fallback to old 2.x context.
                                 * If a version below 3.0 is requested, implementations will
                                 * return the newest context version compatible with OpenGL
                                 * version less than version 3.0.*/
                                attr[1] = 1; attr[3] = 0;
                                gl_err = nk_false;
                                fprintf(stdout, "[X11] Failed to create OpenGL 3.0 context\n");
                                fprintf(stdout, "[X11] ... using old-style GLX context!\n");
                                glContext = create_context(win.dpy, win.fbc, 0, True, attr);
                        }
                }
                XSync(win.dpy, False);
                XSetErrorHandler(old_handler);
                if (gl_err || !glContext)
                        die("[X11]: Failed to create an OpenGL context\n");
                glXMakeCurrent(win.dpy, win.win, glContext);
        }

        ctx = nk_x11_init(win.dpy, win.win);
        /* Load Fonts: if none of these are loaded a default font will be used  */
        /* Load Cursor: if you uncomment cursor loading please hide the cursor */
        {struct nk_font_atlas *atlas;
        nk_x11_font_stash_begin(&atlas);
        /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
        nk_x11_font_stash_end();
        /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
        /*nk_style_set_font(ctx, &droid->handle);*/}

        bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

        if (readVal(currentValues)) {
                strncpy(guiLog, "ERROR reading values from /etc/intel-undervolt.conf", GUILOGSIZE);
        }

        newValues[CPUOFFSET]            = currentValues[CPUOFFSET];
        newValues[GPUOFFSET]            = currentValues[GPUOFFSET];
        newValues[CPUCACHEOFFSET]       = currentValues[CPUCACHEOFFSET];
        newValues[SYSAOFFSET]           = currentValues[SYSAOFFSET];
        newValues[ANALOGIOOFFSET]       = currentValues[ANALOGIOOFFSET];
        newValues[DAEMONINTERVAL]       = currentValues[DAEMONINTERVAL];
        
        /* check if it's enabled on boot via systemd: */
        if (systemdService(2) == 0) {
                strncpy(onBoot, "YES", 4);
        } else {
                strncpy(onBoot, "NO", 3);
        }

        while (running) {
                /* Input */
                XEvent evt;
                nk_input_begin(ctx);
                while (XPending(win.dpy)) {
                        XNextEvent(win.dpy, &evt);
                        if (evt.type == ClientMessage) goto cleanup;
                        if (XFilterEvent(&evt, win.win)) continue;
                        nk_x11_handle_event(&evt);
                }
                nk_input_end(ctx);

                /* GUI */
                if (nk_begin(ctx, "maingui", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT), NK_WINDOW_BORDER)) {
                        static int retVal = 0;

                        static char cpuvstr[MAXDIGIT];
                        static char gpuvstr[MAXDIGIT];
                        static char cachevstr[MAXDIGIT];
                        static char sysagvstr[MAXDIGIT];
                        static char analogvstr[MAXDIGIT];
                        static char dintervalstr[10];

                        /* Value sliders */
                        nk_layout_row_begin(ctx, NK_STATIC, ROWHEIGHT, 4);
                        {
                                nk_layout_row_push(ctx, ROWWIDTH);
                                nk_label(ctx, "CPU:", NK_TEXT_LEFT);
                                nk_layout_row_push(ctx, 300);
                                nk_slider_int(ctx, MINVOLTAGEOFF, &newValues[CPUOFFSET], MAXVOLTAGEOFF, VOLTAGESTEP);
                                snprintf(cpuvstr, MAXDIGIT, "%d", newValues[CPUOFFSET]);
                                nk_layout_row_push(ctx, 45);
                                nk_label(ctx, cpuvstr, NK_TEXT_RIGHT);
                                nk_layout_row_push(ctx, MVLABEL);
                                nk_label(ctx, "mV", NK_TEXT_RIGHT);
                        }
                        nk_layout_row_end(ctx);

                        nk_layout_row_begin(ctx, NK_STATIC, ROWHEIGHT, 4);
                        {
                                nk_layout_row_push(ctx, ROWWIDTH);
                                nk_label(ctx, "GPU:", NK_TEXT_LEFT);
                                nk_layout_row_push(ctx, 300);
                                nk_slider_int(ctx, MINVOLTAGEOFF, &newValues[GPUOFFSET], MAXVOLTAGEOFF, VOLTAGESTEP);
                                snprintf(gpuvstr, MAXDIGIT, "%d", newValues[GPUOFFSET]);
                                nk_layout_row_push(ctx, 45);
                                nk_label(ctx, gpuvstr, NK_TEXT_RIGHT);
                                nk_layout_row_push(ctx, MVLABEL);
                                nk_label(ctx, "mV", NK_TEXT_RIGHT);
                        }
                        nk_layout_row_end(ctx);

                        nk_layout_row_begin(ctx, NK_STATIC, ROWHEIGHT, 4);
                        {
                                nk_layout_row_push(ctx, ROWWIDTH);
                                nk_label(ctx, "CPU Cache:", NK_TEXT_LEFT);
                                nk_layout_row_push(ctx, 300);
                                nk_slider_int(ctx, MINVOLTAGEOFF, &newValues[CPUCACHEOFFSET], MAXVOLTAGEOFF, VOLTAGESTEP);
                                snprintf(cachevstr, MAXDIGIT, "%d", newValues[CPUCACHEOFFSET]);
                                nk_layout_row_push(ctx, 45);
                                nk_label(ctx, cachevstr, NK_TEXT_RIGHT);
                                nk_layout_row_push(ctx, MVLABEL);
                                nk_label(ctx, "mV", NK_TEXT_RIGHT);
                        }
                        nk_layout_row_end(ctx);

                        nk_layout_row_begin(ctx, NK_STATIC, ROWHEIGHT, 4);
                        {
                                nk_layout_row_push(ctx, ROWWIDTH);
                                nk_label(ctx, "System Agent:", NK_TEXT_LEFT);
                                nk_layout_row_push(ctx, 300);
                                nk_slider_int(ctx, MINVOLTAGEOFF, &newValues[SYSAOFFSET], MAXVOLTAGEOFF, VOLTAGESTEP);
                                snprintf(sysagvstr, MAXDIGIT, "%d", newValues[SYSAOFFSET]);
                                nk_layout_row_push(ctx, 45);
                                nk_label(ctx, sysagvstr, NK_TEXT_RIGHT);
                                nk_layout_row_push(ctx, MVLABEL);
                                nk_label(ctx, "mV", NK_TEXT_RIGHT);
                        }
                        nk_layout_row_end(ctx);

                        nk_layout_row_begin(ctx, NK_STATIC, ROWHEIGHT, 4);
                        {
                                nk_layout_row_push(ctx, ROWWIDTH);
                                nk_label(ctx, "Analog IO:", NK_TEXT_LEFT);
                                nk_layout_row_push(ctx, 300);
                                nk_slider_int(ctx, MINVOLTAGEOFF, &newValues[ANALOGIOOFFSET], MAXVOLTAGEOFF, VOLTAGESTEP);
                                snprintf(analogvstr, MAXDIGIT, "%d", newValues[ANALOGIOOFFSET]);
                                nk_layout_row_push(ctx, 45);
                                nk_label(ctx, analogvstr, NK_TEXT_RIGHT);
                                nk_layout_row_push(ctx, MVLABEL);
                                nk_label(ctx, "mV", NK_TEXT_RIGHT);
                        }
                        nk_layout_row_end(ctx);

                        nk_layout_row_begin(ctx, NK_STATIC, ROWHEIGHT, 4);
                        {
                                nk_layout_row_push(ctx, ROWWIDTH);
                                nk_label(ctx, "Daemon Interval:", NK_TEXT_LEFT);
                                nk_layout_row_push(ctx, 300);
                                nk_slider_int(ctx, 1000, &newValues[DAEMONINTERVAL], 100000, 500);
                                snprintf(dintervalstr, 10, "%d", newValues[DAEMONINTERVAL]);
                                nk_layout_row_push(ctx, 45);
                                nk_label(ctx, dintervalstr, NK_TEXT_RIGHT);
                                nk_layout_row_push(ctx, MVLABEL);
                                nk_label(ctx, "ms", NK_TEXT_RIGHT);
                        }
                        nk_layout_row_end(ctx);

                        nk_layout_row_dynamic(ctx, ROWHEIGHT/2, 1);
                        nk_label(ctx, SEPARATOR, NK_TEXT_LEFT);

                        /* Read / Reset / Apply */
                        nk_layout_row_static(ctx, ROWHEIGHT, 95, 3);
                        if (nk_button_label(ctx, "Read values")) {
                                /* open /etc/intel-undervolt.conf and read values */
                                if(readVal(currentValues)) {
                                        strncpy(guiLog, "ERROR reading values from /etc/intel-undervolt.conf", GUILOGSIZE);
                                } else {
                                        newValues[CPUOFFSET]            = currentValues[CPUOFFSET];
                                        newValues[GPUOFFSET]            = currentValues[GPUOFFSET];
                                        newValues[CPUCACHEOFFSET]       = currentValues[CPUCACHEOFFSET];
                                        newValues[SYSAOFFSET]           = currentValues[SYSAOFFSET];
                                        newValues[ANALOGIOOFFSET]       = currentValues[ANALOGIOOFFSET];
                                        newValues[DAEMONINTERVAL]       = currentValues[DAEMONINTERVAL];
                                        strncpy(guiLog, "Values read from /etc/intel-undervolt.conf", GUILOGSIZE);
                                }
                        }
                        if (nk_button_label(ctx, "Reset")) {
                                if (uvResetAll()) {
                                        strncpy(guiLog, "ERROR resetting values!", GUILOGSIZE);
                                } else {
                                        strncpy(guiLog, "All values have been reset to default.", GUILOGSIZE);
                                        currentValues[CPUOFFSET]        = newValues[CPUOFFSET] = 0;
                                        currentValues[GPUOFFSET]        = newValues[GPUOFFSET] = 0;
                                        currentValues[CPUCACHEOFFSET]   = newValues[CPUCACHEOFFSET] = 0;
                                        currentValues[SYSAOFFSET]       = newValues[SYSAOFFSET] = 0;
                                        currentValues[ANALOGIOOFFSET]   = newValues[ANALOGIOOFFSET] = 0;
                                        currentValues[DAEMONINTERVAL]   = newValues[DAEMONINTERVAL] = 5000;
                                }
                        }
                        if (nk_button_label(ctx, "Apply")) {
                                retVal = applyValues(newValues, currentValues);
                                if (retVal == 0) {
                                        strncpy(guiLog, "Values are same as in config. Nothing changed.", GUILOGSIZE);
                                } else if (retVal == -1) {
                                        strncpy(guiLog, "Error while trying to apply values with 'intel-undervolt apply'.", GUILOGSIZE);
                                } else {
                                        strncpy(guiLog, "New values set.", GUILOGSIZE);
                                }
                        }

                        nk_layout_row_dynamic(ctx, ROWHEIGHT/2, 1);
                        nk_label(ctx, SEPARATOR, NK_TEXT_LEFT);

                        /* Enable / disable on Boot */
                        nk_layout_row_begin(ctx, NK_STATIC, ROWHEIGHT, 3);
                        {
                                nk_layout_row_push(ctx, ROWWIDTH);
                                nk_label(ctx, "Enabled on Boot:", NK_TEXT_LEFT);
                                nk_layout_row_push(ctx, 80);
                                nk_label(ctx, onBoot, NK_TEXT_LEFT);
                                nk_layout_row_push(ctx, 120);
                                if (strcmp(onBoot, "YES") && nk_button_label(ctx, "Enable on boot")) {
                                        if (systemdService(1)) {
                                                strncpy(guiLog, "ERROR enabling intel-undervolt-loop.service!", GUILOGSIZE);
                                                strncpy(onBoot, "???", 4);
                                        } else {
                                                strncpy(guiLog, "intel-undervolt-loop.service successfully enabled.", GUILOGSIZE);
                                                strncpy(onBoot, "YES", 4);
                                        }
                                } else if (strcmp(onBoot, "NO") && nk_button_label(ctx, "Disable on boot")) {
                                        if (systemdService(0)) {
                                                strncpy(guiLog, "ERROR disabling intel-undervolt-loop.service!", GUILOGSIZE);
                                                strncpy(onBoot, "???", 4);
                                        } else {
                                                strncpy(guiLog, "intel-undervolt-loop.service successfully disabled.", GUILOGSIZE);
                                                strncpy(onBoot, "NO", 3);
                                        }
                                }
                        }
                        nk_layout_row_end(ctx);

                        nk_layout_row_dynamic(ctx, ROWHEIGHT/2, 1);
                        nk_label(ctx, SEPARATOR, NK_TEXT_LEFT);

                        /* Power consumption */
                        nk_layout_row_static(ctx, ROWHEIGHT, ROWWIDTH*3, 1);
                        nk_label(ctx, "Current power consumption:", NK_TEXT_LEFT);
                        
                        nk_layout_row_dynamic(ctx, ROWHEIGHT/2, 2);
                        nk_label(ctx, currPowerVals[3], NK_TEXT_LEFT);
                        nk_label(ctx, currPowerVals[2], NK_TEXT_LEFT);
                        nk_layout_row_dynamic(ctx, ROWHEIGHT/2, 2);
                        nk_label(ctx, currPowerVals[1], NK_TEXT_LEFT);
                        nk_label(ctx, currPowerVals[0], NK_TEXT_LEFT);

                        if (startT > endT) {
                                startT = 0.0l;
                                endT = 0.0l;
                                measurePowerConsumption(currPowerVals, pcapList, 4, maxname);
                                getCpuTemp(currTempVals, hwmonList, &maxname, TEMPERATUREEL);
                        } else if (startT == 0.0l && endT == 0.0l) {
                                time(&startT);
                                endT = startT + 1;
                        } else {
                                time(&startT);
                        }

                        nk_layout_row_dynamic(ctx, ROWHEIGHT/2, 1);
                        nk_label(ctx, SEPARATOR, NK_TEXT_LEFT);

                        nk_layout_row_static(ctx, ROWHEIGHT, ROWWIDTH*3, 1);
                        nk_label(ctx, "Current temperatures:", NK_TEXT_LEFT);
                        
                        int i = TEMPERATUREEL-1;
                        while (i > -1) {
                                if (strlen(currTempVals[i]) < 2) {
                                        --i;
                                        continue;
                                }
                                nk_layout_row_dynamic(ctx, ROWHEIGHT/2, 2);
                                nk_label(ctx, currTempVals[i], NK_TEXT_LEFT);
                                if (i > 0) {
                                        nk_label(ctx, currTempVals[i-1], NK_TEXT_LEFT);
                                }
                                i -= 2;
                        }

                        nk_layout_row_dynamic(ctx, ROWHEIGHT/2, 1);
                        nk_label(ctx, SEPARATOR, NK_TEXT_LEFT);

                        /* Output Log */
                        nk_layout_row_dynamic(ctx, ROWHEIGHT, 1);
                        nk_label(ctx, "Output log:", NK_TEXT_LEFT);
                        nk_layout_row_dynamic(ctx, ROWHEIGHT, 1);
                        nk_label(ctx, guiLog, NK_TEXT_LEFT);
                }
                nk_end(ctx);

                /* Draw */
                XGetWindowAttributes(win.dpy, win.win, &win.attr);
                glViewport(0, 0, win.width, win.height);
                glClear(GL_COLOR_BUFFER_BIT);
                glClearColor(bg.r, bg.g, bg.b, bg.a);
                /* IMPORTANT: `nk_x11_render` modifies some global OpenGL state
                 * with blending, scissor, face culling, depth test and viewport and
                 * defaults everything back into a default state.
                 * Make sure to either a.) save and restore or b.) reset your own state after
                 * rendering the UI. */
                nk_x11_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
                glXSwapBuffers(win.dpy, win.win);
        }

cleanup:

        while (pcapList) {
		powercap_list_t * next = pcapList->next;
		free(pcapList->name);
		free(pcapList->dir);
		free(pcapList);
		pcapList = next;
	}

        /*while (coretemp_list) {
		hwmon_list_t * next = coretemp_list->next;
		free(coretemp_list->name);
		free(coretemp_list->dir);
		free(coretemp_list);
		coretemp_list = next;
        }*/

        nk_x11_shutdown();
        glXMakeCurrent(win.dpy, 0, 0);
        glXDestroyContext(win.dpy, glContext);
        XUnmapWindow(win.dpy, win.win);
        XFreeColormap(win.dpy, win.cmap);
        XDestroyWindow(win.dpy, win.win);
        XCloseDisplay(win.dpy);
        exit(0);

}
