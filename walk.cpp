//Aurora Hernandez
// 06/07/ 2017
//************ 
//3350
//program: walk.cpp
//author:  Gordon Griesel
//date:    summer 2017
//
//Walk cycle using a sprite sheet.
//images courtesy: http://games.ucla.edu/resource/walk-cycles/
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
//#include "log.h"
#include "ppm.h"
#include "fonts.h"

//defined types
typedef double Flt;
typedef double Vec[3];
typedef Flt	Matrix[4][4];

//macros
#define rnd() (((double)rand())/(double)RAND_MAX)
#define random(a) (rand()%a)
#define MakeVector(x, y, z, v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
#define VecDot(a,b)	((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecSub(a,b,c) (c)[0]=(a)[0]-(b)[0]; \
                      (c)[1]=(a)[1]-(b)[1]; \
                      (c)[2]=(a)[2]-(b)[2]
//constants
const float timeslice = 1.0f;
const float gravity = -0.2f;
#define ALPHA 1

//X Windows variables
Display *dpy;
Window win;

//function prototypes
void initXWindows(void);
void initOpengl(void);
void cleanupXWindows(void);
void checkResize(XEvent *e);
void checkMouse(XEvent *e);
void checkKeys(XEvent *e);
void init();
void physics(void);
void physicsExplosion(void); //Function to do Physics for Explosion
void render(void);
void renderExplosion(void); //Function to render Explosion 
//-----------------------------------------------------------------------------
//Setup timers
class Timers {
public:
	double physicsRate;
	double oobillion;
	struct timespec timeStart, timeEnd, timeCurrent;
	struct timespec walkTime;
        struct timespec explosionTime; // ***********
	Timers() {
		physicsRate = 1.0 / 30.0;
		oobillion = 1.0 / 1e9;
	}
	double timeDiff(struct timespec *start, struct timespec *end) {
		return (double)(end->tv_sec - start->tv_sec ) +
				(double)(end->tv_nsec - start->tv_nsec) * oobillion;
	}
	void timeCopy(struct timespec *dest, struct timespec *source) {
		memcpy(dest, source, sizeof(struct timespec));
	}
	void recordTime(struct timespec *t) {
		clock_gettime(CLOCK_REALTIME, t);
	}
} timers;
//-----------------------------------------------------------------------------

class Global {
public:
        int direction; // for direction/movement
        int keys[65536]; //0x0000ffff
        int keys1[65536];//*
	int done;
	int xres, yres;
        int xres1,yres1;
	int walk;
        int explosion; //*
	int walkFrame;
        int explosionFrame; //*
	double delay;
        double explosionDelay; //*
	Ppmimage *walkImage;
        Ppmimage *explosionImage; //For explosion image
	GLuint walkTexture;
        GLuint explosiveTexture; //*
	Vec box[20];
        
        
	Global() {
                direction = 1;
		done=0;
		xres=800;
		yres=600;
                xres1 =600; //*
                yres1= 400;//*
		walk=0;
                explosion =0; //*
		walkFrame=0;
		walkImage=NULL;
                explosionFrame=0; //*
                explosionImage=NULL; //*
		delay = 0.1;
                
		for (int i=0; i<20; i++) {
			box[i][0] = rnd() * xres;
			box[i][1] = rnd() * (yres-220) + 220.0;
			box[i][2] = 0.0;
		}
                
                //direction = 1;
		done=0;
                xres1 =600; // x coor
                yres1= 400;//y coor
		//walk=0;
                explosion =0; //*
		//walkFrame=0;
		//walkImage=NULL;
                explosionFrame=0; //*
                explosionImage=NULL; //*
                explosionDelay = 0.1;
        }
} gl;

int main(void)
{
	initXWindows();
	initOpengl();
	init();
	while (!gl.done) {
		while (XPending(dpy)) {
			XEvent e;
			XNextEvent(dpy, &e);
			checkResize(&e);
			checkMouse(&e);
			checkKeys(&e);
		}
		physics();
                physicsExplosion(); //*
		render();
                renderExplosion(); // *
		glXSwapBuffers(dpy, win);
	}
	cleanupXWindows();
	cleanup_fonts();
	return 0;
}

void cleanupXWindows(void)
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

void setTitle(void)
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "3350 - Walk Cycle");
}

void setupScreenRes(const int w, const int h)
{
	gl.xres = w;
	gl.yres = h;
}

void initXWindows(void)
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	//GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
	XSetWindowAttributes swa;
	setupScreenRes(gl.xres, gl.yres);
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		printf("\n\tcannot connect to X server\n\n");
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		printf("\n\tno appropriate visual found\n\n");
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
						StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, gl.xres, gl.yres, 0,
							vi->depth, InputOutput, vi->visual,
							CWColormap | CWEventMask, &swa);
	GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
	setTitle();
}

void reshapeWindow(int width, int height)
{
	//window has been resized.
	setupScreenRes(width, height);
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
	setTitle();
}

unsigned char *buildAlphaData(Ppmimage *img)
{
	//add 4th component to RGB stream...
	int i;
	unsigned char *newdata, *ptr;
	unsigned char *data = (unsigned char *)img->data;
	newdata = (unsigned char *)malloc(img->width * img->height * 4);
	ptr = newdata;
	unsigned char a,b,c;
	//use the first pixel in the image as the transparent color.
	unsigned char t0 = *(data+0);
	unsigned char t1 = *(data+1);
	unsigned char t2 = *(data+2);
	for (i=0; i<img->width * img->height * 3; i+=3) {
		a = *(data+0);
		b = *(data+1);
		c = *(data+2);
		*(ptr+0) = a;
		*(ptr+1) = b;
		*(ptr+2) = c;
		*(ptr+3) = 1;
		if (a==t0 && b==t1 && c==t2)
			*(ptr+3) = 0;
		//-----------------------------------------------
		ptr += 4;
		data += 3;
	}
	return newdata;
}

void initOpengl(void)
{
        //OpenGL initialization
        glViewport(0, 0, gl.xres, gl.yres);
        //Initialize matrices
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        //This sets 2D mode (no perspective)
        glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
        //
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_FOG);
        glDisable(GL_CULL_FACE);
        //
        //Clear the screen
        glClearColor(1.0, 1.0, 1.0, 1.0);
        //glClear(GL_COLOR_BUFFER_BIT);
        //Do this to allow fonts
        glEnable(GL_TEXTURE_2D);
        initialize_fonts();
        //
        //load the images file into a ppm structure.
        //
        system("convert ./images/walk.gif ./images/walk.ppm");
        //converted explosion image to explosion.ppm to be able to run
        system("convert ./images/explosion.gif ./images/explosion.ppm");
        gl.walkImage = ppm6GetImage("./images/walk.ppm");
        gl.explosionImage = ppm6GetImage("./images/explosion.ppm"); //*
        int w = gl.walkImage->width;
        int h = gl.walkImage->height;
        //
        //create opengl texture elements
        glGenTextures(1, &gl.walkTexture);
        
        //-------------------------------------------------------------------------
        //silhouette
        //this is similar to a sprite graphic
        //
        glBindTexture(GL_TEXTURE_2D, gl.walkTexture);
        //
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //
        //must build a new set of data...
        unsigned char *walkData = buildAlphaData(gl.walkImage);
        // unsigned buildAlphaData(gl.explosiveImage);//***********
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, walkData);
        free(walkData);

        unlink("./images/walk.ppm");
        /////////////////////////////////
        int wt = gl.explosionImage->width; //*
        int ht = gl.explosionImage->height; //*
        //create opengl texture elements
        glGenTextures(1, &gl.explosiveTexture);
        //-------------------------------------------------------------------------
        //silhouette
        //this is similar to a sprite graphic
        //
        glBindTexture(GL_TEXTURE_2D, gl.explosiveTexture);
        //
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //
        //must build a new set of data...	
        unsigned char *explosionData = buildAlphaData(gl.explosionImage); //*
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wt, ht, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, explosionData); //*

        free(explosionData); //*



        unlink("./images/explosive.ppm"); //*
        //------------------------------------------------------------------
}

void checkResize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != gl.xres || xce.height != gl.yres) {
		//Window size did change.
		reshapeWindow(xce.width, xce.height);
	}
}

void init() {

}

void checkMouse(XEvent *e)
{
	//Did the mouse move?
	//Was a mouse button clicked?
	static int savex = 0;
	static int savey = 0;
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button is down
		}
		if (e->xbutton.button==3) {
			//Right button is down
		}
	}
	if (savex != e->xbutton.x || savey != e->xbutton.y) {
		//Mouse moved
		savex = e->xbutton.x;
		savey = e->xbutton.y;
	}
}

void checkKeys(XEvent *e)
{
	//keyboard input?
	static int shift=0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyRelease) {
            gl.keys[key] = 0; //*
		if (key == XK_Shift_L || key == XK_Shift_R)
			shift=0;// *
		return;
	}
	if (e->type == KeyPress) {
            gl.keys[key] = 1; //*
		if (key == XK_Shift_L || key == XK_Shift_R) {
                     shift=1; // *
			return;
		}
	} else {
		return;
	}
	if (shift) {}
	switch (key) {
		case XK_w:
			timers.recordTime(&timers.walkTime);
			gl.walk ^= 1;
			break;
		case XK_Left:
                        gl.direction = 0;
			break;
		case XK_Right:                    
                        gl.direction = 1;
			break;
		case XK_Up:
			break;
		case XK_Down:
			break;
		case XK_equal:
			gl.delay -= 0.005;
			if (gl.delay < 0.005)
				gl.delay = 0.005;
			break;
		case XK_minus:
			gl.delay += 0.005;
			break;
		case XK_Escape:
			gl.done=1;
			break;
                        //E_key is pressed
                        // Since there is no specification of shift E or non-shift
                case XK_e:
                case XK_E:
                    //explode if 1 or 0 pressed 
                    timers.recordTime(&timers.explosionTime);
                    gl.explosion ^= 1; 
                    break;
                
	}
}

Flt VecNormalize(Vec vec)
{
	Flt len, tlen;
	Flt xlen = vec[0];
	Flt ylen = vec[1];
	Flt zlen = vec[2];
	len = xlen*xlen + ylen*ylen + zlen*zlen;
	if (len == 0.0) {
		MakeVector(0.0,0.0,1.0,vec);
		return 1.0;
	}
	len = sqrt(len);
	tlen = 1.0 / len;
	vec[0] = xlen * tlen;
	vec[1] = ylen * tlen;
	vec[2] = zlen * tlen;
	return(len);
}

void physics(void)
{
	if (gl.walk || gl.keys[XK_Right]) { //Key pressed to move right 
		//man is walking...
		//when time is up, advance the frame.
		timers.recordTime(&timers.timeCurrent);
		double timeSpan = timers.timeDiff(&timers.walkTime, &timers.timeCurrent);
		if (timeSpan > gl.delay) {
			//advance
			++gl.walkFrame;
			if (gl.walkFrame >= 16)
				gl.walkFrame -= 16;
			timers.recordTime(&timers.walkTime);
		}
		for (int i=0; i<20; i++) {
                    if(gl.direction == 1) {
			gl.box[i][0] -= 2.0 * (0.05 / gl.delay);
			if (gl.box[i][0] < -10.0)
				gl.box[i][0] += gl.xres + 10.0;
                    } else {
                        gl.box[i][0] += 2.0 * (0.05 / gl.delay);
                            if (gl.box[i][0] > gl.xres + 10.0)
				gl.box[i][0] -= gl.xres + 20.0;
                    }
		}
	}
}

void render(void)
{
	Rect r;
	//Clear the screen
	glClearColor(0.1, 0.1, 0.1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	float cx = gl.xres/2.0;
	float cy = gl.yres/2.0;
	//
	//show ground
	glBegin(GL_QUADS);
		glColor3f(0.2, 0.2, 0.2);
		glVertex2i(0,       220);
		glVertex2i(gl.xres, 220);
		glColor3f(0.4, 0.4, 0.4);
		glVertex2i(gl.xres,   0);
		glVertex2i(0,         0);
	glEnd();
	//
	//fake shadow
	//glColor3f(0.25, 0.25, 0.25);
	//glBegin(GL_QUADS);
	//	glVertex2i(cx-60, 150);
	//	glVertex2i(cx+50, 150);
	//	glVertex2i(cx+50, 130);
	//	glVertex2i(cx-60, 130);
	//glEnd();
	//
	//show boxes as background
	for (int i=0; i<20; i++) {
		glPushMatrix();
		glTranslated(gl.box[i][0],gl.box[i][1],gl.box[i][2]);
		glColor3f(0.2, 0.2, 0.2);
		glBegin(GL_QUADS);
			glVertex2i( 0,  0);
			glVertex2i( 0, 30);
			glVertex2i(20, 30);
			glVertex2i(20,  0);
		glEnd();
		glPopMatrix();
	}
        float h = 200.0;
	float w = h * 0.5;
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, gl.walkTexture);
       
	//
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glColor4ub(255,255,255,255);
	int ix = gl.walkFrame % 8;
	int iy = 0;
	if (gl.walkFrame >= 8)
		iy = 1;
	float tx = (float)ix / 8.0;
	float ty = (float)iy / 2.0;
	glBegin(GL_QUADS);
        //Direction to move left and right
        if(gl.direction == 1 ) {// 1= right, 0 = left
		glTexCoord2f(tx,      ty+.5); glVertex2i(cx-w, cy-h);
		glTexCoord2f(tx,      ty);    glVertex2i(cx-w, cy+h);
		glTexCoord2f(tx+.125, ty);    glVertex2i(cx+w, cy+h);
		glTexCoord2f(tx+.125, ty+.5); glVertex2i(cx+w, cy-h);
        } else if(gl.direction == 0) { //left
                glTexCoord2f(tx,      ty+.5); glVertex2i(cx+w, cy-h);
		glTexCoord2f(tx,      ty);    glVertex2i(cx+w, cy+h);
		glTexCoord2f(tx+.125, ty);    glVertex2i(cx-w, cy+h);
		glTexCoord2f(tx+.125, ty+.5); glVertex2i(cx-w, cy-h);
        }
        
	glEnd();
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_ALPHA_TEST);
	//
	unsigned int c = 0x00ffff44;
	r.bot = gl.yres - 20;
	r.left = 10;
	r.center = 0;
	ggprint8b(&r, 16, c, "W   Walk cycle");
        ggprint8b(&r, 16, c, "E or e Explosion sequence");
	ggprint8b(&r, 16, c, "+   faster");
	ggprint8b(&r, 16, c, "-   slower");
	ggprint8b(&r, 16, c, "right arrow -> walk right");
	ggprint8b(&r, 16, c, "left arrow  <- walk left");
	ggprint8b(&r, 16, c, "frame: %i", gl.walkFrame);
}

//**************************EXPLOSION*****************************************//
//****************************************************************************//
void setupScreenRes1(const int wgt, const int hgt)
{
	gl.xres1 = wgt;
	gl.yres1 = hgt;
}
void physicsExplosion(void)
{
	if (gl.explosion || gl.keys1[XK_e] || gl.keys1[XK_E]) { //E key press
		
		//when time is up, advance the frame
		timers.recordTime(&timers.timeCurrent);
		double timeSpan = timers.timeDiff(&timers.explosionTime, &timers.timeCurrent);
		if (timeSpan > gl.explosionDelay) {
			//advance explosion frame
			++gl.explosionFrame;
			if (gl.explosionFrame >= 8) {
				gl.explosionFrame = 0;
                                gl.explosion = 0; //turn off explosion when all frames have been displayed
                        }
			timers.recordTime(&timers.explosionTime);
		}
	}
}     
void renderExplosion(void)
{
    if(gl.explosion) {
        float hgt = 960.0 / 6.0 / 2; //image size 960 x 384 //column 1
	float wgt = 384 / 2 / 2; //column 2
        
        float cx1;
        float cy1 = gl.yres1/2.0;
        if(gl.direction == 1) { // move explosion as walk direction moves 
            cx1 = gl.xres1/2.0 + wgt + gl.xres1 / 4.0;
        } else {
            cx1 = gl.xres1/2.0 + wgt - gl.xres1 / 4.0;
        }
	

	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, gl.explosiveTexture);        
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glColor4ub(255,255,255,255);
	int ix1 = gl.explosionFrame % 6; //mod 5
	int iy1 = 0;
	if (gl.explosionFrame >= 6)
		iy1 = 1;
	float tx1 = (float)ix1 / 5.0;
	float ty1 = (float)iy1 / 2.0;
        // changes according to image l and w, pixels 
	glBegin(GL_QUADS);
        glTexCoord2f(tx1,      ty1+.5); glVertex2i(cx1-wgt, cy1-hgt);
        glTexCoord2f(tx1,      ty1);    glVertex2i(cx1-wgt, cy1+hgt);
        glTexCoord2f(tx1+(1.0 / 5.0), ty1);    glVertex2i(cx1+wgt, cy1+hgt);
        glTexCoord2f(tx1+(1.0 / 5.0), ty1+.5); glVertex2i(cx1+wgt, cy1-hgt);
	glEnd();
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_ALPHA_TEST);
    }
}


      
	





