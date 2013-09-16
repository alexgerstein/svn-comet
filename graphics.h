#ifndef GRAPHICS_H
#define GRAPHICS_H

// graphics.h
// Author: Alex Gerstein
// Header file for drawing all objects in the game

#include <gtk/gtk.h>
#include <chipmunk/chipmunk.h>
#include "physics.h"

typedef struct graphstate {
	GtkWidget *da;
	cairo_surface_t *surface;
	double ix, iy, fx, fy;
	double hscaler, wscaler;
	double meter_scale;
	gboolean drawing;

	int button_hover;


  	GTimer* timer;

	cairo_surface_t *background;
	cairo_surface_t *chipmunk;
	cairo_surface_t *title;
	cairo_surface_t *single_player;
	cairo_surface_t *multi;
	cairo_surface_t *asteroid;
	cairo_surface_t *rocket;
	cairo_surface_t *winner;
	cairo_surface_t *loser;
	cairo_surface_t *loading;

	cairo_surface_t *timer_zero;
	cairo_surface_t *timer_one;
	cairo_surface_t *timer_two;
	cairo_surface_t *timer_three;
	cairo_surface_t *timer_four;
	cairo_surface_t *timer_five;
	cairo_surface_t *timer_six;
	cairo_surface_t *timer_seven;
	cairo_surface_t *timer_eight;
	cairo_surface_t *timer_nine;
	cairo_surface_t *timer_ten;




} GraphState;

GraphState *graph_state_new (GtkWidget *da, double meter_scale);

void graphics_draw_menu(GraphState *gs);

// Iterates through shapes in space, calling draw_shape on each one
void graphics_draw_shapes (GraphState *gstate, cpSpace *space);
void graphics_drag_slice (GraphState *gstate, gdouble x2, gdouble y2);

void clear_surface(GraphState *gs);

void graphics_draw_info(GraphState *gstate, GTimer *, int slices, int percentage);
void graphics_draw_alert (GraphState *gstate, double percentage);
void graphics_draw_load_screen (GraphState *gs);

#endif
