
#ifdef WIN32
#include <windows.h>
#include <process.h>
#endif

#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#ifdef UNIX
#include <unistd.h>
#endif

#include "twirlies.h"

/* DEFINES */

#define DISPMAX     100.0
#define MAXTWIRLIES 7      // default 7
#define MINPOINTS   2      // default 2
#define MAXPOINTS   17     // default 17
#define LINEARSTEP  5.0
#define COLORINC    0.01

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

#ifndef M_2PI
#define M_2PI 6.28318530717958647692
#endif

/* TYPEDEFS */

typedef struct _circle_oscillation {
	double mag; /* magnitude */
	double mff; /* magnitude offset */
	double lff; /* linear offset */
	double frq; /* frequency */
} circle_oscillation;

typedef struct _twirlie {
	double             enter[2]; /* x,y entrance */
	double             exit[2] ; /* x,y exit */
	double             incr[2] ; /* 1-linear-unit x,y increment */
	double             len     ; /* track length */
	double             lv      ; /* linear velocity */
	double             rv      ; /* rotation velocity */
	long               pts     ; /* points */
	circle_oscillation outer   ; /* outer circle oscillation */
	circle_oscillation inner   ; /* inner circle oscillation */
	GLdouble           rgb[3]  ; /* color */
	double             pos[2]  ; /* current x,y position */
	struct _twirlie *next;
	double n;
} twirlie;

/* EXTERNAL VARIABLES */

#ifdef WIN32
extern HWND ScreenHWnd = NULL;
#endif

/* GLOBAL VARIABLES */

static int firstreshape = 0;

static double disp_lr, disp_bt; /* left-right , bottom-top screen dimensions (from center) */
static double disp_hp;          /* screen hypotenuse (from center) */

static twirlie *twroot = NULL;

/* FUNCTION PROTOTYPES */

double random_double(double lower, double upper);
unsigned long random_ulong(unsigned long lower, unsigned long upper);

twirlie *new_twirlie();
void remove_twirlie(twirlie *tw);
void process_twirlie(twirlie *tw);
void figure_coordinates(twirlie *tw);
void rotate_color(double n, double rgb[3]);
void draw_twirlie(twirlie *tw);
void pick_a_color(double rgb[3]);

/* FUNCTION IMPLEMENTATIONS */

void tw_init()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_FLAT);
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef WIN32
	srand(time(NULL) + getpid());
#else
	srand48((long)(time(NULL)) + getpid());
#endif
}

void tw_reshape(int w, int h)
{
	double lr, bt;
	
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	lr = bt = DISPMAX;

	if     (w > h) lr = ((double)w) * ((double)DISPMAX) / ((double)h);
	else if(w < h) bt = ((double)h) * ((double)DISPMAX) / ((double)w);

	disp_lr = lr;
	disp_bt = bt;

	disp_hp = sqrt((disp_lr * disp_lr) + (disp_bt * disp_bt));
	gluOrtho2D( -lr , +lr , -bt , +bt );

//	printf("gluOrtho2d( %g , %g , %g , %g)\n", -lr, +lr, -bt, +bt);

	glMatrixMode(GL_MODELVIEW);

	glClear(GL_COLOR_BUFFER_BIT);

	firstreshape = 1;
}

void tw_dummy_display() {}

static int twdinc = 0;

void pause_display()
{
	struct timespec ts;
	ts.tv_nsec = 10000000;
	ts.tv_sec = 0;
	nanosleep(&ts, NULL);
}

void tw_display()
{
	twirlie **twp;

	if(!firstreshape) return;

	new_twirlie();
	for(twp = &twroot ; twp && *twp ; twp = (*twp)?(&((*twp)->next)):NULL)
		process_twirlie(*twp);

	fprintf(stderr, "\r%20d", twdinc++); fflush(stderr);
	
	pause_display();

	glFlush();
}

twirlie *new_twirlie()
{
	static twirlie *tw, **twp;
	static int i;

	for(twp=&twroot, i=0 ; *twp && i<MAXTWIRLIES ; twp=&((*twp)->next), i++);

	if(*twp || i==MAXTWIRLIES) return NULL;

	tw = (twirlie *)malloc(sizeof(twirlie));
        if(!tw)
        {
            fprintf(stderr, "Failed to allocate twirlie: %s\n", strerror(errno)); fflush(stderr);
        }

	tw->outer.mag = random_double((double)(DISPMAX/8), (double)(DISPMAX/4)); /* double -- outer circle magnitude */
//	printf("tw->outer.mag = %g\n", tw->outer.mag);
	tw->outer.mff = random_double(-0.2*tw->outer.mag, 0.2*tw->outer.mag);    /* double -- outer circle magnitude offset */
	tw->outer.lff = random_double(-tw->outer.mag/5.0,tw->outer.mag/5.0);     /* double -- outer circle linear offset */
	tw->outer.frq = random_double(2.0, 10.0);                                /* double -- outer circle frequency */
	tw->inner.mag = 0.0;                                                     /* double -- inner circle magnitude */
	tw->inner.mff = 0.0;                                                     /* double -- inner circle magnitude offset */
	tw->inner.lff = 0.0;                                                     /* double -- inner circle linear offset */
	tw->inner.frq = random_double(1.0, tw->outer.frq);                       /* double -- inner circle frequency */
	figure_coordinates(tw);
	tw->len       = sqrt(((tw->exit[0]-tw->enter[0])*
	                      (tw->exit[0]-tw->enter[0])) +
	                     ((tw->exit[1]-tw->enter[1])*
	                      (tw->exit[1]-tw->enter[1])) );                     /* double -- track length */
	tw->incr[0]   = (tw->exit[0] - tw->enter[0]) / tw->len;                  /* double -- 1-linear-unit x increment */
	tw->incr[1]   = (tw->exit[1] - tw->enter[1]) / tw->len;                  /* double -- 1-linear-unit y increment */
	tw->lv        = random_double(0.5, 0.5);                                 /* double -- linear velocity */
	tw->rv        = random_double(0.0, 0.005);                                /* double -- rotation velocity */
	tw->pts       = random_ulong (MINPOINTS, MAXPOINTS);                     /* long   -- points */
	pick_a_color(tw->rgb);                                                   /* double -- color rgb */
	tw->pos[0]    = tw->enter[0];                                            /* double -- current x position */
	tw->pos[1]    = tw->enter[1];                                            /* double -- current y position */
	tw->next      = NULL;
	tw->n        = 0.0;

	*twp = tw;

	return tw;
}

void pick_a_color(double rgb[3])
{
	static unsigned long clr;
	
	clr = random_ulong(1,6);

	switch(clr)
	{
		case 1:          rgb[0] = 1.0; rgb[1] = 0.0; rgb[2] = 0.0; break; /* RED    */
		case 2:          rgb[0] = 1.0; rgb[1] = 0.5; rgb[2] = 0.0; break; /* ORANGE */
		case 3:          rgb[0] = 1.0; rgb[1] = 1.0; rgb[2] = 0.0; break; /* YELLOW */
		case 4:          rgb[0] = 0.0; rgb[1] = 1.0; rgb[2] = 0.0; break; /* GREEN  */
		case 5:          rgb[0] = 0.0; rgb[1] = 0.0; rgb[2] = 1.0; break; /* BLUE   */
		case 6: default: rgb[0] = 1.0; rgb[1] = 0.0; rgb[2] = 1.0; break; /* VIOLET */
	}
}

void process_twirlie(twirlie *tw)
{

	draw_twirlie(tw);

	if( (fabs(tw->pos[0] - tw->enter[0]) > fabs(tw->exit[0] - tw->enter[0])) &&
	    (fabs(tw->pos[1] - tw->enter[1]) > fabs(tw->exit[1] - tw->enter[1])) )
	{
		remove_twirlie(tw);
	}

	tw->pos[0] = tw->pos[0] + (tw->incr[0] * tw->lv);
	tw->pos[1] = tw->pos[1] + (tw->incr[1] * tw->lv);

	tw->n += LINEARSTEP;

	rotate_color(tw->n, tw->rgb);
}

void remove_twirlie(twirlie *tw)
{
	static twirlie **twp;

	for(twp = &twroot ; *twp && *twp != tw ; twp = &((*twp)->next));
	if(! *twp) return;
	
	*twp = (*twp)->next;
	free(tw);
}

double random_double(double lower, double upper)
{
	static double range;
	static double rv;

	range = upper - lower;

#ifdef WIN32
	rv = lower + (((double)rand()) * range) / (double)RAND_MAX;

//	printf("random_double(%.1f,%.1f) = %.1f\n", lower, upper, rv);
	return rv;
#else
	return lower + (drand48() * range);
#endif
}

unsigned long random_ulong(unsigned long lower, unsigned long upper)
{
#ifdef WIN32
	static ULONGLONG range;
	static ULONGLONG lr;
#else
	static unsigned long long range;
	static unsigned long long lr;
#endif

	range = upper - lower + 1;

#ifdef WIN32
	lr = (rand() * range) / RAND_MAX;
#else
	lr = (lrand48() * range) / 0x80000000;
#endif

	return lower + (long)lr;
}

void figure_coordinates(twirlie *tw)
{
	static unsigned long edges[2];
	static double *xy[2];
	static int i;
	static double v_rad;
	
	v_rad = (tw->outer.mag * 2.0) + tw->outer.lff;

	xy[0] = tw->enter;
	xy[1] = tw->exit;
	
	edges[0] = random_ulong(1,4);
	while((edges[1] = random_ulong(1,4)) == edges[0]);
	
	for(i=0 ; i<2 ; i++)
	{
		switch(edges[i])
		{
			case 1 : xy[i][0] = random_double(-disp_lr, disp_lr);  xy[i][1] =  disp_bt + v_rad;                   break;
			case 2 : xy[i][0] =  disp_lr + v_rad;                  xy[i][1] = random_double(-disp_bt, disp_bt); break;
			case 3 : xy[i][0] = random_double(-disp_lr, disp_lr);  xy[i][1] = -disp_bt - v_rad;                   break;
			case 4 : xy[i][0] = -disp_lr - v_rad;                  xy[i][1] = random_double(-disp_bt, disp_bt); break;
			default: fprintf(stderr, "edge error: %ld\n", edges[i]); exit(1);
		}
	}

}

void rotate_color(double n, double rgb[3])
{
	static const double color_inc1 = COLORINC, color_inc2 = COLORINC / 2.0;

	if     ((rgb[0] == 1.0                ) && (rgb[1] >= 0.0 && rgb[1] < 0.5) && (rgb[2] == 0.0                )) { rgb[1] += color_inc2;                       } /* RED TO ORANGE    */
	else if((rgb[0] == 1.0                ) && (rgb[1] >= 0.5 && rgb[1] < 1.0) && (rgb[2] == 0.0                )) { rgb[1] += color_inc2;                       } /* ORANGE TO YELLOW */
	else if((rgb[0] <= 1.0 && rgb[0] > 0.0) && (rgb[1] == 1.0                ) && (rgb[2] == 0.0                )) { rgb[0] -= color_inc1;                       } /* YELLOW TO GREEN  */
	else if((rgb[0] == 0.0                ) && (rgb[1] <= 1.0 && rgb[1] > 0.0) && (rgb[2] >= 0.0 && rgb[2] < 1.0)) { rgb[1] -= color_inc1; rgb[2] += color_inc1; } /* GREEN  TO BLUE   */
	else if((rgb[0] >= 0.0 && rgb[0] < 1.0) && (rgb[1] == 0.0                ) && (rgb[2] == 1.0                )) { rgb[0] += color_inc1;                       } /* BLUE   TO VIOLET */
	else if((rgb[0] == 1.0                ) && (rgb[1] == 0.0                ) && (rgb[2] <= 1.0 && rgb[2] > 0.0)) { rgb[2] -= color_inc1;                       } /* VIOLET TO RED    */

	if(rgb[0] < 0.0) rgb[0] = 0.0; else if(rgb[0] > 1.0) rgb[0] = 1.0;
	if(rgb[1] < 0.0) rgb[1] = 0.0; else if(rgb[1] > 1.0) rgb[1] = 1.0;
	if(rgb[2] < 0.0) rgb[2] = 0.0; else if(rgb[2] > 1.0) rgb[2] = 1.0;
}

void draw_twirlie(twirlie *tw)
{
	static int i;
	static double ang1,       ang3a, ang3b, ang0, ang0a;
	static double v_rad1, v_rad2, v_rad3;
	static double pos1 [MAXPOINTS][2]; /* outer points */
	static double pos2 [MAXPOINTS][2]; /* center points */
	static double pos3a[MAXPOINTS][2]; /* side A */
	static double pos3b[MAXPOINTS][2]; /* side B */

#if 0
	tw->pos[0]    = 0.0;
	tw->pos[1]    = 0.0;
	tw->outer.mag = 50.0;
//	tw->outer.frq = 0.0;
//	tw->outer.lff = 10.0;
//	tw->inner.frq = 5.0;
	tw->rv        = 0.0;
//	tw->pts       = 5;
//	tw->rgb[0]    = 1.0;
//	tw->rgb[1]    = 1.0;
//	tw->rgb[2]    = 1.0;
#endif
	
	v_rad1 = (tw->outer.mag * (sin(tw->outer.frq * ((double)tw->n)/1000.0) + 1.0)) + tw->outer.lff;
	v_rad2 = 0.2 * v_rad1;
	v_rad3 = (((v_rad1 - v_rad2)/2.0) * (sin(tw->inner.frq * ((double)tw->n)/1000.0) + 1.0)) + v_rad2;

	ang0 = M_2PI / (double)(tw->pts);
	ang0a = (ang0/4.0) * (sin((tw->inner.frq/2.0) * ((double)tw->n)/1000.0) + 1.0);
	
	for(i=0 ; i<tw->pts ; i++)
	{
		ang1 = (double)i * ang0;
		pos1[i][0]  = ((cos(ang1 + (tw->n*tw->rv))) * v_rad1) + tw->pos[0]; /* x-coordinate */
		pos1[i][1]  = ((sin(ang1 + (tw->n*tw->rv))) * v_rad1) + tw->pos[1]; /* y-coordinate */

		pos2[i][0]  = ((cos(ang1 + (tw->n*tw->rv))) * v_rad2) + tw->pos[0]; /* x-coordinate */
		pos2[i][1]  = ((sin(ang1 + (tw->n*tw->rv))) * v_rad2) + tw->pos[1]; /* y-coordinate */

		ang3a = ang1 + ang0a;
		ang3b = ang1 - ang0a;

		pos3a[i][0] = ((cos(ang3a + (tw->n*tw->rv))) * v_rad3) + tw->pos[0]; /* x-coordinate */
		pos3a[i][1] = ((sin(ang3a + (tw->n*tw->rv))) * v_rad3) + tw->pos[1]; /* y-coordinate */

		pos3b[i][0] = ((cos(ang3b + (tw->n*tw->rv))) * v_rad3) + tw->pos[0]; /* x-coordinate */
		pos3b[i][1] = ((sin(ang3b + (tw->n*tw->rv))) * v_rad3) + tw->pos[1]; /* y-coordinate */
	}

	glEnable(GL_BLEND);

	glColor4ub(0,0,0, 0x60);
	for(i=0 ; i<tw->pts ; i++)
	{
		glBegin(GL_TRIANGLE_STRIP);
			glVertex2dv(pos3a[i]);
			glVertex2dv(pos1 [i]);
			glVertex2dv(pos2 [i]);
			glVertex2dv(pos3b[i]);
		glEnd();
	}

	glColor3dv(tw->rgb);
	glBegin(GL_LINES);
	for(i=0 ; i<tw->pts ; i++)
	{
		glVertex2dv(pos1[i]); glVertex2dv(pos3a[i]);
		glVertex2dv(pos1[i]); glVertex2dv(pos3b[i]);
		glVertex2dv(pos2[i]); glVertex2dv(pos3a[i]);
		glVertex2dv(pos2[i]); glVertex2dv(pos3b[i]);
	}
	glEnd();

	glDisable(GL_BLEND);
}

#ifdef UNIX
void tw_keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 0x1b:
		case 0x71: exit(0); break;
		default: break;
	}
}

void tw_visibility(int state)
{
	glClear(GL_COLOR_BUFFER_BIT);
}
#endif
