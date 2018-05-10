/* See LICENSE file for license details. */
#define _XOPEN_SOURCE 500
#if HAVE_SHADOW_H
#include <shadow.h>
#endif

#include <cairo.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "arg.h"
#include "util.h"

#define APP_NAME "xglock"

char *argv0;

struct lock {
	int screen;
	Window root, win;
	Pixmap pmap;
	GLXContext glc;
};

struct xrandr {
	int active;
	int evbase;
	int errbase;
};

#include "config.h"


/* update screen each (SKIP_FRAMES+1)-th frame */
static const int SKIP_FRAMES = 3;

static const float bg_color[3] = { 0.36, 0.36, 0.36 }; // #5C5C5C
static const float logo_color[3] = { 0.09, 0.58, 0.82 }; // #1793D1
static const float text_color[3] = { 1, 1, 1 }; // #FFFFFF
static const float error_color[3] = { 1, 0.5, 0.5 }; // #FF8080

/* logo full rotations per second */
static const float rps = 0.2;

/* logo position and path definition */
static const int logo_y = 100;
static const int logo_width = 600;
static const int logo_height = 600;
static const char *logo_path = "m 299.9436,0 c -26.71054,65.47481 -42.82104,108.30302 -72.55973,171.83152 18.23355,19.3239"
	" 40.61435,41.82756 76.96073,67.24332 C 265.26868,222.99838 238.61414,206.85799 218.69477,190.10906 180.63493"
	",269.51182 121.0061,382.61754 0,600 95.10673,545.10357 168.83178,511.25931 237.53998,498.34524 c -2.95035"
	",-12.68706 -4.62775,-26.41059 -4.51382,-40.72959 l 0.11282,-3.04629 c 1.50913,-60.92088 33.2061,-107.76903"
	" 70.75421,-104.58816 37.54809,3.1809 66.73386,55.17534 65.22474,116.09628 -0.2838,11.46339 -1.57707,22.49103"
	" -3.83673,32.71905 C 433.24338,512.08866 506.1801,545.84625 600,600 581.50053,565.94763 564.9882,535.25187"
	" 549.21948,506.01729 524.38122,486.76962 498.47367,461.71866 445.62723,434.5995 c 36.32361,9.43662 62.33085"
	",20.32395 82.60296,32.49339 C 367.90545,168.65225 354.92256,128.99565 299.9436,0 Z"
	;

/* bottom text position and path definition */
static const int text_y = 880;
static const int text_width = 1570;
static const int text_height = 100;
static const char *text_path = "M 0,0"
	" m 20,0 l 60,0 c 10,0 20,10 20,20 l 0,3 -20,10 0,-13 -60,0 0,20 60,0 c 10,0 20,10 20,20 l 0,20 c 0,10 -10,20 -20,20 l -60,0 c -10,0 -20,-10 -20,-20 l 0,-3 20,-10 0,13 60,0 0,-20 -60,0 c -10,0 -20,-10 -20,-20 l 0,-20 c 0,-10 10,-20 20,-20 Z"
	" m 100,0"
	" l 100,0 0,20 -40,0 0,80 -20,0 0,-80 -40,0 0,-20 Z"
	" m 100,0"
	" m 0,100 l 50,-100 20,0 50,100 -20,0 -15,-30 -50,0 -15,30 -20,0 Z m 45,-50 l 30,0 -15,-30 -15,30 m -45,-50 Z"
	" m 120,0"
	" l 100,0 0,20 -40,0 0,80 -20,0 0,-80 -40,0 0,-20 Z"
	" m 120,0"
	" l 20,0 0,100 -20,0 0,-100 Z"
	" m 60,0"
	" l 60,0 c 10,0 20,10 20,20 l 0,60 c 0,10 -10,20 -20,20 l -60,0 c -10,0 -20,-10 -20,-20 l 0,-60 c 0,-10 10,-20 20,-20 Z m 0,20 l 0,60 60,0 0,-60 -60,0 m 0,-20 Z"
	" m 100,0"
	" l 17,0 74,69 0,-69 20,0 0,100 -17,0 -74,-69 0,69 -20,0 0,-100 Z"
	" m 220,0"
	" l 20,0 0,80 l 70,0 0,20 -90,0 0,-100 Z"
	" m 140,0"
	" l 60,0 c 10,0 20,10 20,20 l 0,60 c 0,10 -10,20 -20,20 l -60,0 c -10,0 -20,-10 -20,-20 l 0,-60 c 0,-10 10,-20 20,-20 Z m 0,20 l 0,60 60,0 0,-60 -60,0 m 0,-20 Z"
	" m 110,0"
	" m 20,0 l 60,0 c 10,0 20,10 20,20 l 0,10 -20,10 0,-20 -60,0 0,60 60,0 0,-20 20,10 0,10 c 0,10 -10,20 -20,20 l -60,0 c -10,0 -20,-10 -20,-20 l 0,-60 c 0,-10 10,-20 20,-20 Z"
	" m 110,0"
	" l 20,0 0,41.25 49.5,-41.25 30.5,0 -60,50 60,50 -30.5,0 -49.5,-41.25 0,41.25 -20,0 0,-100 Z"
	" m 130,0"
	" l 90,0 0,20 -70,0 0,20 45,0 0,20 -45,0 0,20 70,0 0,20 -90,0 0,-100 Z"
	" m 120,0"
	" l 55,0 c 30,0 45,20 45,50 0,30 -20,50 -45,50 l -55,0 0,-100 Z m 20,20 l 0,60 30,0 c 20,0 30,-10 30,-30 0,-20 -10,-30 -30,-30 l -30,0 Z"
	;

static void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(1);
}

#ifdef __linux__
#include <fcntl.h>
#include <linux/oom.h>

static void
dontkillme(void)
{
	FILE *f;
	const char oomfile[] = "/proc/self/oom_score_adj";

	if (!(f = fopen(oomfile, "w"))) {
		if (errno == ENOENT)
			return;
		die(APP_NAME ": fopen %s: %s\n", oomfile, strerror(errno));
	}
	fprintf(f, "%d", OOM_SCORE_ADJ_MIN);
	if (fclose(f)) {
		if (errno == EACCES)
			die(APP_NAME ": unable to disable OOM killer. "
			    "Make sure to suid or sgid " APP_NAME ".\n");
		else
			die(APP_NAME ": fclose %s: %s\n", oomfile, strerror(errno));
	}
}
#endif

static const char *
gethash(void)
{
	const char *hash;
	struct passwd *pw;

	/* Check if the current user has a password entry */
	errno = 0;
	if (!(pw = getpwuid(getuid()))) {
		if (errno)
			die(APP_NAME ": getpwuid: %s\n", strerror(errno));
		else
			die(APP_NAME ": cannot retrieve password entry\n");
	}
	hash = pw->pw_passwd;

#if HAVE_SHADOW_H
	if (!strcmp(hash, "x")) {
		struct spwd *sp;
		if (!(sp = getspnam(pw->pw_name)))
			die(APP_NAME ": getspnam: cannot retrieve shadow entry. "
			    "Make sure to suid or sgid " APP_NAME ".\n");
		hash = sp->sp_pwdp;
	}
#else
	if (!strcmp(hash, "*")) {
#ifdef __OpenBSD__
		if (!(pw = getpwuid_shadow(getuid())))
			die(APP_NAME ": getpwnam_shadow: cannot retrieve shadow entry. "
			    "Make sure to suid or sgid " APP_NAME ".\n");
		hash = pw->pw_passwd;
#else
		die(APP_NAME ": getpwuid: cannot retrieve shadow entry. "
		    "Make sure to suid or sgid " APP_NAME ".\n");
#endif /* __OpenBSD__ */
	}
#endif /* HAVE_SHADOW_H */

	return hash;
}

static void
draw_path(cairo_t *cr, const float scale, const char* path)
{
	char token[256];
	int pos = 0;
	float x = 0, y = 0;
	float x1, y1, x2, y2, x3, y3;

	while (sscanf(path, "%s%n", token, &pos) == 1) {
		path += pos;
		switch (token[0]) {
			case 'M':
			case 'm':
				sscanf(path, "%f,%f%n", &x1, &y1, &pos); path += pos;
				if (token[0] == 'm') {
					x1 += x;
					y1 += y;
				}
				cairo_move_to(cr, x1 * scale, y1 * scale);
				x = x1;
				y = y1;
			break;
			case 'C':
			case 'c':
				while (sscanf(path, "%f,%f%n", &x1, &y1, &pos) == 2) {
					path += pos;
					sscanf(path, "%f,%f%n", &x2, &y2, &pos);
					path += pos;
					sscanf(path, "%f,%f%n", &x3, &y3, &pos);
					path += pos;
					if (token[0] == 'c') {
						x1 += x;
						y1 += y;
						x2 += x;
						y2 += y;
						x3 += x;
						y3 += y;
					}
					cairo_curve_to(cr, x1 * scale, y1 * scale, x2 * scale, y2 * scale, x3 * scale, y3 * scale);
					x = x3;
					y = y3;
				}
			break;
			case 'L':
			case 'l':
				while (sscanf(path, "%f,%f%n", &x1, &y1, &pos) == 2) {
					path += pos;
					if (token[0] == 'l') {
						x1 += x;
						y1 += y;
					}
					cairo_line_to(cr, x1 * scale, y1 * scale);
					x = x1;
					y = y1;
				}
			break;
			case 'Z':
			case 'z':
				cairo_close_path(cr);
			break;
			default:
				fprintf(stderr, "draw_path: unknown command '%s'\n", token);
		}
	}
	cairo_fill(cr);
}

static void
create_texture(int id, float scale, int width, int height, const char *path)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	unsigned char *surface_data;

	width *= scale;
	height *= scale;

	surface_data = calloc(4 * width * height, sizeof(unsigned char));
	surface = cairo_image_surface_create_for_data(surface_data, CAIRO_FORMAT_ARGB32, width, height, 4 * width);
	cr = cairo_create(surface);

	cairo_set_source_rgb(cr, 1, 1, 1);
	draw_path(cr, scale, path);

	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, surface_data);

	free(surface_data);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

static void
screen_updated(int width, int height) {
	float scale = height / 1080.0;

	glViewport(0, 0, width, height);

	create_texture(1, scale, logo_width, logo_height, logo_path);
	create_texture(2, scale, text_width, text_height, text_path);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90, 1.0 * width / height, 0.1, 5);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -1);
	glScalef(scale, scale, 1);
	glScalef(2.0 / height, 2.0 / height, 1.0 / height);
}

static void
draw(float angle, Bool error) {
	float w2, h2;

	glClear(GL_COLOR_BUFFER_BIT);

	w2 = logo_width >> 1;
	h2 = logo_height >> 1;
	glPushMatrix();
		glTranslatef(0, 540 - logo_y - h2, 0);
		glRotatef(angle, 0, 1, 0);

		glBindTexture(GL_TEXTURE_2D, 1);

		glBegin(GL_QUADS);
			glColor3f(logo_color[0], logo_color[1], logo_color[2]);
			glTexCoord2f(0, 1); glVertex3f(-w2, -h2, 0);
			glTexCoord2f(1, 1); glVertex3f(w2, -h2, 0);
			glTexCoord2f(1, 0); glVertex3f(w2, h2, 0);
			glTexCoord2f(0, 0); glVertex3f(-w2, h2, 0);
		glEnd();
	glPopMatrix();

	w2 = text_width >> 1;
	h2 = text_height >> 1;
	glPushMatrix();
		glTranslatef(0, 540 - text_y - h2, 0);

		glBindTexture(GL_TEXTURE_2D, 2);
		glBegin(GL_QUADS);
			if (error) {
				glColor3f(error_color[0], error_color[1], error_color[2]);
			} else {
				glColor3f(text_color[0], text_color[1], text_color[2]);
			}
			glTexCoord2f(0, 1); glVertex3f(-w2, -h2, 0);
			glTexCoord2f(1, 1); glVertex3f(w2, -h2, 0);
			glTexCoord2f(1, 0); glVertex3f(w2, h2, 0);
			glTexCoord2f(0, 0); glVertex3f(-w2, h2, 0);
		glEnd();
	glPopMatrix();
}

static void
readpw(Display *dpy, struct xrandr *rr, struct lock **locks, int nscreens,
       const char *hash)
{
	XRRScreenChangeNotifyEvent *rre;
	char buf[32], passwd[256], *inputhash;
	int num, screen, running;
	unsigned int len;
	KeySym ksym;
	XEvent ev;
	int new_width, new_height;
	struct timeval start, stop, error_time = {0};
	float frame_time;
	float angle;
	Bool error;

	len = 0;
	running = 1;

	angle = 0;

	while (running) {
		gettimeofday(&start, NULL);
		error = error_time.tv_sec > start.tv_sec || (error_time.tv_sec == start.tv_sec && error_time.tv_usec > start.tv_usec);
		static int skip = 0;
		if (skip >= SKIP_FRAMES) {
			skip = 0;
			for(screen = 0; screen != nscreens; ++screen) {
				struct lock *lock = locks[screen];
				glXMakeCurrent(dpy, lock->win, lock->glc);
				draw(angle, error);
				glXSwapBuffers(dpy, lock->win);
			}
		} else {
			usleep(16666);
		}
		++skip;
		gettimeofday(&stop, NULL);
		frame_time = (1000000 * (stop.tv_sec - start.tv_sec) + stop.tv_usec - start.tv_usec) / 1000000.0;
		angle += frame_time * rps * 360;
		if (angle > 360) angle -= 360;

		if (!XPending(dpy))
			continue;

		XNextEvent(dpy, &ev);
		if (ev.type == KeyPress) {
			explicit_bzero(&buf, sizeof(buf));
			num = XLookupString(&ev.xkey, buf, sizeof(buf), &ksym, 0);
			if (IsKeypadKey(ksym)) {
				if (ksym == XK_KP_Enter)
					ksym = XK_Return;
				else if (ksym >= XK_KP_0 && ksym <= XK_KP_9)
					ksym = (ksym - XK_KP_0) + XK_0;
			}
			if (IsFunctionKey(ksym) ||
			    IsKeypadKey(ksym) ||
			    IsMiscFunctionKey(ksym) ||
			    IsPFKey(ksym) ||
			    IsPrivateKeypadKey(ksym))
				continue;
			switch (ksym) {
			case XK_Return:
				passwd[len] = '\0';
				errno = 0;
				if (!(inputhash = crypt(passwd, hash)))
					fprintf(stderr, APP_NAME ": crypt: %s\n", strerror(errno));
				else
					running = !!strcmp(inputhash, hash);
				if (running) {
					XBell(dpy, 100);
					gettimeofday(&error_time, NULL);
					error_time.tv_sec += 1;
				}
				explicit_bzero(&passwd, sizeof(passwd));
				len = 0;
				break;
			case XK_Escape:
				explicit_bzero(&passwd, sizeof(passwd));
				len = 0;
				break;
			case XK_BackSpace:
				if (len)
					passwd[--len] = '\0';
				break;
			default:
				if (num && !iscntrl((int)buf[0]) &&
				    (len + num < sizeof(passwd))) {
					memcpy(passwd + len, buf, num);
					len += num;
				}
				break;
			}
		} else if (rr->active && ev.type == rr->evbase + RRScreenChangeNotify) {
			rre = (XRRScreenChangeNotifyEvent*)&ev;
			for (screen = 0; screen < nscreens; screen++) {
				if (locks[screen]->win == rre->window) {
					if (rre->rotation == RR_Rotate_90 ||
					    rre->rotation == RR_Rotate_270) {
						new_width = rre->height;
						new_height = rre->width;
					} else {
						new_width = rre->width;
						new_height = rre->height;
					}
					XResizeWindow(dpy, locks[screen]->win,
								  new_height, new_width);
					glXMakeCurrent(dpy, locks[screen]->win, locks[screen]->glc);
					screen_updated(new_width, new_height);
					break;
				}
			}
		} else if (ev.type != KeyRelease) {
			for (screen = 0; screen < nscreens; screen++)
				XRaiseWindow(dpy, locks[screen]->win);
		}
	}
}

static struct lock *
lockscreen(Display *dpy, struct xrandr *rr, int screen)
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	char curs[] = {0, 0, 0, 0, 0, 0, 0, 0};
	int i, ptgrab, kbgrab;
	struct lock *lock;
	XColor color;
	XSetWindowAttributes wa;
	XVisualInfo *vi;
	Cursor invisible;
	int width, height, depth;

	if (dpy == NULL || screen < 0 || !(lock = malloc(sizeof(struct lock))))
		return NULL;

	lock->screen = screen;
	lock->root = RootWindow(dpy, lock->screen);

	/* init */

	width = DisplayWidth(dpy, lock->screen);
	height = DisplayHeight(dpy, lock->screen);
	depth = DefaultDepth(dpy, lock->screen);
	att[2] = depth;

	vi = glXChooseVisual(dpy, lock->screen, att);
	if (!vi)
		die(APP_NAME ": cannot choose visual\n");

	wa.override_redirect = 1;
	wa.colormap = XCreateColormap(dpy, lock->root, vi->visual, AllocNone);

	lock->win = XCreateWindow(dpy, lock->root, 0, 0,
	                          width,
	                          height,
	                          0, depth,
	                          InputOutput,
	                          vi->visual,
	                          CWOverrideRedirect | CWColormap, &wa);
	lock->pmap = XCreateBitmapFromData(dpy, lock->win, curs, 8, 8);
	invisible = XCreatePixmapCursor(dpy, lock->pmap, lock->pmap,
	                                &color, &color, 0, 0);
	XDefineCursor(dpy, lock->win, invisible);

	/* Try to grab mouse pointer *and* keyboard for 600ms, else fail the lock */
	for (i = 0, ptgrab = kbgrab = -1; i < 6; i++) {
		if (ptgrab != GrabSuccess) {
			ptgrab = XGrabPointer(dpy, lock->root, False,
			                      ButtonPressMask | ButtonReleaseMask |
			                      PointerMotionMask, GrabModeAsync,
			                      GrabModeAsync, None, invisible, CurrentTime);
		}
		if (kbgrab != GrabSuccess) {
			kbgrab = XGrabKeyboard(dpy, lock->root, True,
			                       GrabModeAsync, GrabModeAsync, CurrentTime);
		}

		/* input is grabbed: we can lock the screen */
		if (ptgrab == GrabSuccess && kbgrab == GrabSuccess)
			break;

		/* retry on AlreadyGrabbed but fail on other errors */
		if ((ptgrab != AlreadyGrabbed && ptgrab != GrabSuccess) ||
		    (kbgrab != AlreadyGrabbed && kbgrab != GrabSuccess))
			break;

		usleep(100000);
	}

	if (ptgrab == GrabSuccess && kbgrab == GrabSuccess) {
		lock->glc = glXCreateContext(dpy, vi, NULL, True);
		glXMakeCurrent(dpy, lock->win, lock->glc);

		screen_updated(width, height);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		glClearColor(bg_color[0], bg_color[1], bg_color[2], 1.0);

		XMapRaised(dpy, lock->win);
		if (rr->active)
			XRRSelectInput(dpy, lock->win, RRScreenChangeNotifyMask);

		XSelectInput(dpy, lock->root, SubstructureNotifyMask);

		return lock;
	}

	/* we couldn't grab all input: fail out */
	if (ptgrab != GrabSuccess)
		fprintf(stderr, APP_NAME ": unable to grab mouse pointer for screen %d\n",
		        screen);
	if (kbgrab != GrabSuccess)
		fprintf(stderr, APP_NAME ": unable to grab keyboard for screen %d\n",
		        screen);
	return NULL;
}

static void
usage(void)
{
	die("usage: " APP_NAME " [-v] [cmd [arg ...]]\n");
}

int
main(int argc, char **argv) {
	struct xrandr rr;
	struct lock **locks;
	struct passwd *pwd;
	struct group *grp;
	uid_t duid;
	gid_t dgid;
	const char *hash;
	Display *dpy;
	int s, nlocks, nscreens;

	ARGBEGIN {
	case 'v':
		fprintf(stderr, APP_NAME "-" VERSION "\n");
		return 0;
	default:
		usage();
	} ARGEND

	/* validate drop-user and -group */
	errno = 0;
	if (!(pwd = getpwnam(user)))
		die(APP_NAME ": getpwnam %s: %s\n", user,
		    errno ? strerror(errno) : "user entry not found");
	duid = pwd->pw_uid;
	errno = 0;
	if (!(grp = getgrnam(group)))
		die(APP_NAME ": getgrnam %s: %s\n", group,
		    errno ? strerror(errno) : "group entry not found");
	dgid = grp->gr_gid;

#ifdef __linux__
	dontkillme();
#endif

	hash = gethash();
	errno = 0;
	if (!crypt("", hash))
		die(APP_NAME ": crypt: %s\n", strerror(errno));

	if (!(dpy = XOpenDisplay(NULL)))
		die(APP_NAME ": cannot open display\n");

	/* drop privileges */
	if (setgroups(0, NULL) < 0)
		die(APP_NAME ": setgroups: %s\n", strerror(errno));
	if (setgid(dgid) < 0)
		die(APP_NAME ": setgid: %s\n", strerror(errno));
	if (setuid(duid) < 0)
		die(APP_NAME ": setuid: %s\n", strerror(errno));

	/* check for Xrandr support */
	rr.active = XRRQueryExtension(dpy, &rr.evbase, &rr.errbase);

	/* get number of screens in display "dpy" and blank them */
	nscreens = ScreenCount(dpy);
	if (!(locks = calloc(nscreens, sizeof(struct lock *))))
		die(APP_NAME ": out of memory\n");
	for (nlocks = 0, s = 0; s < nscreens; s++) {
		if ((locks[s] = lockscreen(dpy, &rr, s)) != NULL)
			nlocks++;
		else
			break;
	}
	XSync(dpy, 0);

	/* did we manage to lock everything? */
	if (nlocks != nscreens)
		return 1;

	/* run post-lock command */
	if (argc > 0) {
		switch (fork()) {
		case -1:
			die(APP_NAME ": fork failed: %s\n", strerror(errno));
		case 0:
			if (close(ConnectionNumber(dpy)) < 0)
				die(APP_NAME ": close: %s\n", strerror(errno));
			execvp(argv[0], argv);
			fprintf(stderr, APP_NAME ": execvp %s: %s\n", argv[0], strerror(errno));
			_exit(1);
		}
	}

	/* everything is now blank. Wait for the correct password */
	readpw(dpy, &rr, locks, nscreens, hash);

	return 0;
}
