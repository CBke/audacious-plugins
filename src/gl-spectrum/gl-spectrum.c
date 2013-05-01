/*  OpenGL Spectrum Visualizer for Audacious
 *  Copyright (C) 2013 Christophe Budé
 *  Based on the XMMS plugin:
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, 
 *  Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <math.h>


#include <audacious/debug.h>
#include <audacious/drct.h>
#include <audacious/i18n.h>
#include <audacious/misc.h>
#include <audacious/playlist.h>
#include <audacious/plugin.h>
#include <libaudcore/hook.h>
#include <libaudgui/libaudgui.h>
#include <libaudgui/libaudgui-gtk.h>

#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <gdk/gdkx.h>

#define NUM_BANDS 24

GLXContext context;

static GtkWidget * spect_widget = NULL;
static gfloat xscale[NUM_BANDS + 1];
static gint width, height;
static gint pos = 0;

static GLfloat bar_innerwidth =  0.8f * 3.2f / (gfloat)(NUM_BANDS - 1);
static GLfloat bar_width = 3.2f / (NUM_BANDS - 1);
static GLenum g_mode = GL_FILL;

static GLfloat y_angle = 25.0f, y_speed = 0.058f;
static GLfloat heights[NUM_BANDS][NUM_BANDS];
static GLfloat colors[NUM_BANDS][NUM_BANDS][3];


void draw_rectangle (GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2, GLfloat red, GLfloat green, GLfloat blue)
{

    glColor3f (red, green, blue);

    glBegin (GL_POLYGON);
    glVertex3f (x1, y2, z1);
    glVertex3f (x2, y2, z1);
    glVertex3f (x2, y2, z2);
    glVertex3f (x1, y2, z2);
    glEnd();

    glColor3f (0.65f * red, 0.65f * green, 0.65f * blue);

    glBegin (GL_POLYGON);
    glVertex3f (x1, y1, z1);
    glVertex3f (x1, y2, z1);
    glVertex3f (x1, y2, z2);
    glVertex3f (x1, y1, z2);
    glEnd ();

    glBegin (GL_POLYGON);
    glVertex3f (x2, y2, z1);
    glVertex3f (x2, y1, z1);
    glVertex3f (x2, y1, z2);
    glVertex3f (x2, y2, z2);
    glEnd();
 
    glColor3f (0.8f * red, 0.8f * green, 0.8f * blue);
  
    glBegin (GL_POLYGON);
    glVertex3f (x1, y1, z1);
    glVertex3f (x2, y1, z1);
    glVertex3f (x2, y2, z1);
    glVertex3f (x1, y2, z1);
    glEnd();

}

void draw_bar (GLfloat x_offset, GLfloat z_offset, GLfloat height, GLfloat red, GLfloat green, GLfloat blue)
{

    draw_rectangle (x_offset, 0.0f, z_offset, 
                    x_offset + bar_innerwidth, 
                    height, 
                    z_offset + bar_innerwidth,
	            red   * (0.2f + height),
	            green * (0.2f + height),
	            blue  * (0.2f + height));

}

void draw_bars()
{

    GLfloat x_offset, z_offset;
  
    glPushMatrix();
    glTranslatef (0.0f, -0.5f, -5.0f);
    glRotatef (38.0f, 1.0f, 0.0f, 0.0f);
    glRotatef (y_angle + 180.0f, 0.0f, 1.0f, 0.0f);
    glPolygonMode (GL_FRONT_AND_BACK, g_mode);

    for (int y = 0; y < NUM_BANDS; y++)
    {
        z_offset = - 1.6f + (gfloat)(NUM_BANDS  - y) * bar_width;

        for (int x = 0; x < NUM_BANDS; x++)
        {
            x_offset = 1.6f - (gfloat)x * bar_width; 
            draw_bar (x_offset, 
                      z_offset,
                      heights[(y + pos) % NUM_BANDS][x],
                      colors[y][x][0],
                      colors[y][x][1],
                      colors[y][x][2]);
        }
    } 

    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();

}

static void render_cb (gfloat * freq)
{ 

    for (int i = 0; i < NUM_BANDS; i++)
    {
        gint a = ceill (xscale[i]);
        gint b = floor (xscale[i + 1]);
        gfloat n = 0;

        if (b < a)
            n += freq[b] * (xscale[i + 1] - xscale[i]);
        else
        {
            if (a > 0)
                n += freq[a - 1] * (a - xscale[i]);
            for (; a < b; a ++)
                n += freq[a];
            if (b < 256)
                n += freq[b] * (xscale[i + 1] - b);
        }

	heights[pos][i] = log10 (1 + n * 50);// / 2.0f;
           
    }

    pos = (pos + 1) % NUM_BANDS;
    gtk_widget_queue_draw (spect_widget);

}

static void draw_visualizer (GtkWidget *widget, cairo_t *cr)
{

    Display *xdisplay = GDK_SCREEN_XDISPLAY (gdk_screen_get_default());
    Window xwindow    = GDK_WINDOW_XID (gtk_widget_get_window(widget));
    glXMakeCurrent(xdisplay, xwindow, context);
 
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    glViewport (0, 0, alloc.width, alloc.height);

    glDisable (GL_BLEND);
    glMatrixMode (GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glFrustum (-1.1, 1, -1.5, 1, 2, 10 );
    glMatrixMode (GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glEnable (GL_DEPTH_TEST);
    glDepthFunc( GL_LESS);
    glPolygonMode (GL_FRONT, g_mode);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    y_angle += y_speed;

    if (y_angle >= 45.0f)
        y_speed = -y_speed;

    if (y_angle <= - 45.0f)
        y_speed =- y_speed;

    glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    draw_bars();

    glPopMatrix();
    glMatrixMode (GL_PROJECTION);
    glPopMatrix();
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_BLEND);
    glDepthMask (GL_TRUE);
    glXSwapBuffers (xdisplay, xwindow);

}

static gboolean configure_event (GtkWidget * widget, GdkEventConfigure * event)
{

    width  = event->width;
    height = event->height;
    gtk_widget_queue_draw (widget);

    return TRUE;

}

static gboolean draw_event (GtkWidget * widget, cairo_t * cr, GtkWidget * area)
{

    draw_visualizer (widget, cr);

    return TRUE;

}

static gboolean destroy_event (void)
{

    aud_vis_func_remove ((VisFunc) render_cb);
    spect_widget = NULL;

    return TRUE;

}

static /* GtkWidget * */ gpointer get_widget(void)
{

    GtkWidget *window = gtk_drawing_area_new();
    spect_widget = window;

    g_signal_connect (window, "draw", (GCallback)draw_event, NULL);
    g_signal_connect (window, "configure-event", (GCallback)configure_event, NULL);
    g_signal_connect (window, "destroy", (GCallback)destroy_event, NULL);

    aud_vis_func_add (AUD_VIS_TYPE_FREQ, (VisFunc)render_cb);

    GdkScreen *screen    = gdk_screen_get_default();
    Display   *xdisplay  = GDK_SCREEN_XDISPLAY (screen);
    gint       nscreen   = GDK_SCREEN_XNUMBER (screen);

    /* Create context */
    int attribs[]        = {GLX_RGBA,
                            GLX_RED_SIZE,    1,
                            GLX_GREEN_SIZE,  1,
                            GLX_BLUE_SIZE,   1,
                            GLX_ALPHA_SIZE,  1,
                            GLX_DOUBLEBUFFER,
                            GLX_DEPTH_SIZE,  1,
                            None};
    XVisualInfo *xvinfo  = glXChooseVisual (xdisplay, nscreen, attribs);
    context = glXCreateContext (xdisplay, xvinfo, 0, True);

     /* Fix up visual/colormap */

    GdkVisual *visual = gdk_x11_screen_lookup_visual (screen, xvinfo->visualid);
    gtk_widget_set_visual (window, visual);
	
    /* Disable GTK double buffering */
    gtk_widget_set_double_buffered (window, FALSE);

    GLfloat b_base, r_base;
    for (int y = 0; y < NUM_BANDS; y++)
    {
        b_base = (gfloat)y / ((gfloat)(NUM_BANDS -1) * 2);
        r_base = 0.5f - b_base;

        for (int x = 0; x < NUM_BANDS; x++)
        {
            heights[x][y] = 0.0f;
            colors[x][y][0] = r_base - ( (gfloat)x * (r_base / (gfloat)(NUM_BANDS - 1)) );
            colors[x][y][1] = (gfloat)x / ( (gfloat)(NUM_BANDS - 1) * 2);
            colors[x][y][2] = b_base;
        }
    }

    for (int i = 0; i <= NUM_BANDS; i++)
        xscale[i] = powf(257.0f, ((gfloat)i / (gfloat)NUM_BANDS)) - 1.0f;

    y_speed = 0.058f;
    y_angle = 5.0f;
    pos     = 0;

    return window;

}

AUD_VIS_PLUGIN
(
    .name = N_("GL Spectrum Analyzer"),
    .domain = PACKAGE,
    .get_widget = get_widget
)
