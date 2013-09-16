#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>
#include <chipmunk/chipmunk.h>
#include "graphics.h"

#define BACKGROUND_WIDTH 500.0
#define BACKGROUND_HEIGHT 500.0

#define LOADING_WIDTH 500.0
#define LOADING_HEIGHT 500.0

#define TITLE_WIDTH 1800
#define TITLE_HEIGHT 423

#define SINGLE_PLAYER_WIDTH 1800
#define SINGLE_PLAYER_HEIGHT 260

#define CHIPMUNK_WIDTH 1800
#define CHIPMUNK_HEIGHT 1012

#define MULTI_WIDTH 1800
#define MULTI_HEIGHT 250

#define CONGRATS_WIDTH 500.0
#define CONGRATS_HEIGHT 500.0

#define NUMBER_WIDTH 500.0
#define NUMBER_HEIGHT 500.0

#define COMET_WIDTH 500.0
#define COMET_HEIGHT 500.0

#define ROCKET_WIDTH 354.0
#define ROCKET_HEIGHT 391.0

static void graphics_menu_single(GraphState *gs);
static void graphics_menu_multi(GraphState *gs);
static void graphics_menu_chipmunk(GraphState *gs);
static void graphics_menu_title(GraphState *gs);

static void graphics_draw_countdown (char *orientation, GraphState *gstate, GTimer *timer, gdouble max_time);

static void post_step_draw(cpSpace* space, void *obj, void* data);

static void graphics_draw_poly (cpShape *shape, GraphState *gstate);
static void graphics_draw_rocket (cpShape *shape, GraphState *gs);
static void graphics_draw_comet (cpShape *shape, GraphState *gstate);
static float get_rect_height (cpShape *shape);
static float get_rect_width (cpShape *shape);
  
GraphState *graph_state_new (GtkWidget *da, double meter_scale) {
  GraphState *gs = (GraphState *) malloc (sizeof(GraphState));
  gs->da = da;
  gs->meter_scale = meter_scale;
  gs->drawing = FALSE;
  gs->button_hover = 0;
  gs->timer = NULL;
  return gs;
}

void graphics_draw_menu(GraphState *gs) {

  graphics_menu_chipmunk(gs);
  graphics_menu_title(gs);

  graphics_menu_single(gs);
  graphics_menu_multi(gs);

}

static void graphics_menu_chipmunk(GraphState *gs) {
  cairo_t *cr = cairo_create(gs->surface);

  cairo_matrix_t saved_transform;
  cairo_get_matrix (cr, &saved_transform);

  double window_width = gtk_widget_get_allocated_width(gs->da);
  double window_height = gtk_widget_get_allocated_height(gs->da);

  double chipmunk_width_scaler = window_width / CHIPMUNK_WIDTH / 2;

  cairo_translate(cr, window_width / 4, window_height / 20);
  cairo_scale(cr, chipmunk_width_scaler, chipmunk_width_scaler);
  
  cairo_set_source_surface(cr, gs->chipmunk, 0, 0);
  cairo_set_matrix(cr, &saved_transform);
  cairo_paint(cr);

  cairo_destroy(cr);
}

static void graphics_menu_title(GraphState *gs) {
  cairo_t *cr = cairo_create(gs->surface);

  cairo_matrix_t saved_transform;
  cairo_get_matrix (cr, &saved_transform);

  double window_width = gtk_widget_get_allocated_width(gs->da);
  double window_height = gtk_widget_get_allocated_height(gs->da);

  double title_width_scaler = window_width / TITLE_WIDTH;

  cairo_translate(cr, 0, window_height / 6);
  cairo_scale(cr, title_width_scaler, title_width_scaler);
  
  cairo_set_source_surface(cr, gs->title, 0, 0);
  cairo_set_matrix(cr, &saved_transform);
  cairo_paint(cr);

  cairo_destroy(cr);
}

static void graphics_menu_single(GraphState *gs) {
  cairo_t *cr = cairo_create(gs->surface);

  cairo_matrix_t saved_transform;
  cairo_get_matrix (cr, &saved_transform);

  double window_width = gtk_widget_get_allocated_width(gs->da);
  double window_height = gtk_widget_get_allocated_height(gs->da);

  double button_width_scale = window_width / SINGLE_PLAYER_WIDTH / 2;
  double button_width_translate = window_width / 4;
  
  if (gs->button_hover == 1) {
    button_width_scale *= 2;
    button_width_translate = 0;
  }

  cairo_translate(cr, button_width_translate, window_height / 2);
  cairo_scale(cr, button_width_scale, button_width_scale);
  
  cairo_set_source_surface(cr, gs->single_player, 0, 0);
  cairo_set_matrix(cr, &saved_transform);
  cairo_paint(cr);

  cairo_destroy(cr);
}

static void graphics_menu_multi(GraphState *gs) {
  cairo_t *cr = cairo_create(gs->surface);

  cairo_matrix_t saved_transform;
  cairo_get_matrix (cr, &saved_transform);

  double window_width = gtk_widget_get_allocated_width(gs->da);
  double window_height = gtk_widget_get_allocated_height(gs->da);

  double button_width_scale = window_width / MULTI_WIDTH / 2;
  double button_width_translate = window_width / 4;
  
  if (gs->button_hover == 2) {
    button_width_scale *= 2;
    button_width_translate = 0;
  }

  cairo_translate(cr, button_width_translate, 3 * window_height / 4);

  cairo_scale(cr, button_width_scale, button_width_scale);
  
  cairo_set_source_surface(cr, gs->multi, 0, 0);
  cairo_set_matrix(cr, &saved_transform);
  cairo_paint(cr);

  cairo_destroy(cr);
}

void graphics_draw_info(GraphState *gstate, GTimer *timer, int slices, int percentage) {
	cairo_t *cr = cairo_create(gstate->surface);
	double wscaler = gstate->wscaler;

  double window_width = gtk_widget_get_allocated_width(gstate->da);

  double box_dimensions = window_width / 50;

	for (int i = 0; i < slices; i++) {
		cairo_rectangle(cr, gstate->meter_scale * wscaler - ((i + 1) * 3 * box_dimensions / 2), 10, window_width / 50, window_width / 50);
		cairo_set_source_rgb(cr, 1, 0, 0);
		cairo_fill(cr);

	}

  char string[11];
  snprintf(string, sizeof(string), "Score: %d", percentage);

	cairo_set_source_rgb(cr, 1, 1, 1);

	cairo_select_font_face(cr, "Purisa", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

	cairo_set_font_size(cr, window_width / 25);

	cairo_move_to(cr, 10, window_width / 25);
	cairo_show_text(cr, string);

  graphics_draw_countdown("TOP", gstate, timer, 30);
}

void graphics_draw_alert (GraphState *gstate, double percentage) {
  cairo_t *cr;

  //Draw congratulations
  cr = cairo_create(gstate->surface);

  cairo_matrix_t saved_transform;
  cairo_get_matrix (cr, &saved_transform);

  double window_width = gtk_widget_get_allocated_width(gstate->da);
  double window_height = gtk_widget_get_allocated_height(gstate->da);

  cairo_scale(cr, window_width / CONGRATS_WIDTH, window_height / CONGRATS_HEIGHT);


  if (percentage == 100) {
    cairo_set_source_surface (cr, gstate->winner, 0, 0);
  } else {
    cairo_set_source_surface (cr, gstate->loser, 0, 0);
  }

  cairo_set_matrix(cr, &saved_transform);
  cairo_paint(cr);

  graphics_draw_countdown ("BOTTOM", gstate, gstate->timer, 6);

}

void graphics_draw_load_screen (GraphState *gstate) {
  cairo_t *cr;

  //Draw congratulations
  cr = cairo_create(gstate->surface);

  cairo_matrix_t saved_transform;
  cairo_get_matrix (cr, &saved_transform);

  double window_width = gtk_widget_get_allocated_width(gstate->da);
  double window_height = gtk_widget_get_allocated_height(gstate->da);

  cairo_scale(cr, window_width / LOADING_WIDTH, window_height / LOADING_HEIGHT);

  cairo_set_source_surface(cr, gstate->loading, 0, 0);

  cairo_set_matrix(cr, &saved_transform);
  cairo_paint(cr);

  graphics_draw_countdown ("BOTTOM", gstate, gstate->timer, 10);
}

static void graphics_draw_countdown (char *orientation, GraphState *gstate, GTimer *timer, gdouble max_time) {
  //Draw number on countdown

  if (timer == NULL)
    return;

  cairo_t *cr;
  cr = cairo_create(gstate->surface);

  double window_width = gtk_widget_get_allocated_width(gstate->da);
  double window_height = gtk_widget_get_allocated_height(gstate->da);

  double number_width_scale = window_width / NUMBER_WIDTH / 6;
  double number_height_scale = window_height / NUMBER_HEIGHT / 6;

  int seconds_remaining = (max_time) - g_timer_elapsed(timer, NULL);

  int num_digits = log10(seconds_remaining);


  double vertical_alignment = 0;
  if (strcmp(orientation, "BOTTOM") == 0) {
    vertical_alignment = 3 * window_height / 4;
  }

  for (int i = 0; i <= num_digits; i++) {

    cairo_matrix_t saved_transform;
    cairo_get_matrix (cr, &saved_transform);

    int digit = seconds_remaining % 10;


    if (i == 0 && num_digits == 1) {
      cairo_translate (cr, window_width / 2, vertical_alignment);
    } else if (i == 1) {
      cairo_translate (cr, window_width / 2 - number_width_scale * NUMBER_WIDTH, vertical_alignment);
    } else {
      cairo_translate (cr, window_width / 2 - number_width_scale * NUMBER_WIDTH / 2, vertical_alignment);
    }

    cairo_scale(cr, number_width_scale, number_height_scale);

    switch (digit) {

      case 0:
        cairo_set_source_surface (cr, gstate->timer_zero, 0, 0);
        break;

      case 1:
        cairo_set_source_surface (cr, gstate->timer_one, 0, 0);
        break;

      case 2:
        cairo_set_source_surface (cr, gstate->timer_two, 0, 0);
        break;
        
      case 3:
        cairo_set_source_surface (cr, gstate->timer_three, 0, 0);
        break;

      case 4:
        cairo_set_source_surface (cr, gstate->timer_four, 0, 0);
        break;

      case 5:
        cairo_set_source_surface (cr, gstate->timer_five, 0, 0);
        break;

      case 6:
        cairo_set_source_surface (cr, gstate->timer_six, 0, 0);
        break;

      case 7:
        cairo_set_source_surface (cr, gstate->timer_seven, 0, 0);
        break;  

      case 8:
        cairo_set_source_surface (cr, gstate->timer_eight, 0, 0);
        break;

      case 9:
        cairo_set_source_surface (cr, gstate->timer_nine, 0, 0);
        break;

      case 10:
        cairo_set_source_surface (cr, gstate->timer_ten, 0, 0);
        break;

      default:
        cairo_destroy(cr);
        return;
    }
    cairo_set_matrix(cr, &saved_transform);

    cairo_paint(cr);

    seconds_remaining /= 10;
  }
  cairo_destroy(cr);
}

//Iterates through shapes in space, calling draw_shape on each one
void graphics_draw_shapes (GraphState *gstate, cpSpace *space) {
	cpSpaceEachShape(space, (cpSpaceShapeIteratorFunc) graphics_draw_poly, gstate);
}

static void graphics_draw_poly (cpShape *shape, GraphState *gstate) {
  cpSpaceAddPostStepCallback(shape->CP_PRIVATE(space), post_step_draw, shape, gstate);
  
}

static void post_step_draw(cpSpace* space, void *obj, void* data) {
  cpShape* current_shape = (cpShape *)obj;
  GraphState* graph_state = (GraphState *)data;


  if (cpBodyGetMass(cpShapeGetBody(current_shape)) == INFINITY) {
    graphics_draw_rocket(current_shape, graph_state);
  } else {
    graphics_draw_comet (current_shape, graph_state);
  }
}

static void graphics_draw_comet (cpShape *shape, GraphState *gstate) {
  cairo_t *cr = cairo_create(gstate->surface);

  cairo_matrix_t saved_transform;
  cairo_get_matrix (cr, &saved_transform);

  cpVect pos = cpBodyGetPos(shape->body);
  cpVect rot = cpBodyGetRot(shape->body);

  double x = pos.x;
  double y = pos.y;

  double wscaler = gstate->wscaler;
  double hscaler = gstate->hscaler;

  int num_vects = cpPolyShapeGetNumVerts(shape);

  cairo_translate(cr, x * wscaler, (gstate->meter_scale - y) * hscaler);

  double angle = atan(ABS(rot.y)  / ABS(rot.x));

  double quadrant = 0;

  if (rot.y >= 0 && rot.x >= 0) {
    angle = -angle;
  }

  else if (rot.y <= 0 && rot.x <= 0) {
    angle = -angle;
    quadrant = M_PI;
  }

  else if (rot.y >= 0 && rot.x <= 0) {
    quadrant = M_PI;
  }

  else if (rot.y <= 0 && rot.x >= 0) {
    quadrant = 0;
  }

  cairo_rotate(cr, quadrant + angle);

  cairo_translate(cr, -x * wscaler, -(gstate->meter_scale - y) * hscaler);

  double left = 0;
  double top = 0;

  for(int i = 0; i < num_vects; i++) {
    cpVect vect = cpPolyShapeGetVert(shape, i);

    if (vect.x < left) {
      left = vect.x;
    }

    if (vect.y < top) {
      top = vect.y;
    }
  }

  cairo_set_source_surface(cr, gstate->asteroid, (x + left) * wscaler - 50.0, (gstate->meter_scale - (y - top)) * hscaler - 50.0);

  for(int i = 0; i < num_vects; i++) {
    cpVect vect = cpPolyShapeGetVert(shape, i);
    cairo_line_to(cr, (x + vect.x) * wscaler, (gstate->meter_scale - (y + vect.y)) * hscaler);
  }

  cairo_set_matrix(cr, &saved_transform);

  cairo_close_path(cr);

  cairo_fill(cr);
  cairo_destroy(cr);

}

static void graphics_draw_rocket (cpShape *shape, GraphState *gs) {
  cairo_t *cr = cairo_create(gs->surface);

  //cairo_matrix_t saved_transform;
  //cairo_get_matrix (cr, &saved_transform);

  double wscaler = gs->wscaler;
  double hscaler = gs->hscaler;

  double window_height = gtk_widget_get_allocated_height(gs->da);

  cpVect pos = cpBodyGetPos(shape->body);

  double x = pos.x * wscaler;
  double y = window_height - pos.y * hscaler;

  double box_width = get_rect_width(shape) * wscaler;
  double box_height = get_rect_height(shape) * hscaler;

  double rocket_width_scale = box_width / ROCKET_WIDTH;
  double rocket_height_scale = box_height / ROCKET_HEIGHT;

  cairo_translate (cr, x - box_width / 2.0, y - box_height / 2.0);
  cairo_scale(cr, rocket_width_scale, rocket_height_scale);

  cairo_set_source_surface(cr, gs->rocket, 0, 0);
  cairo_paint(cr);

  cairo_destroy(cr);
}

static float get_rect_height (cpShape *shape) {
  cpVect topVert = cpPolyShapeGetVert (shape, 0);
  cpVect bottomVert = cpPolyShapeGetVert (shape, 1);

  return ABS(topVert.y - bottomVert.y);
}

static float get_rect_width (cpShape *shape) {
  cpVect leftVert = cpPolyShapeGetVert (shape, 1);
  cpVect rightVert = cpPolyShapeGetVert (shape, 2);

  return ABS(rightVert.x - leftVert.x);
}

#ifdef DEBUG
//Test the graphics.c program
int main(int argc, char *argv[]) {
	GtkWidget *window;
	GtkWidget *frame;
	GtkWidget *da;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Drawing Area");

	g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);

	gtk_container_set_border_width(GTK_CONTAINER(window), 8);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(window), frame);

	da = gtk_drawing_area_new();
	/* set a minimum size */
	gtk_widget_set_size_request(da, 500, 500);

	hscaler = gtk_widget_get_allocated_height(da) / meter_scale;
	wscaler = gtk_widget_get_allocated_width(da) / meter_scale;

	gtk_container_add(GTK_CONTAINER(frame), da);

	GameState *gs = game_state_new();

	// Add floor
	cpShape *shape = cpSpaceAddShape(gs->space, cpSegmentShapeNew(cpSpaceGetStaticBody(gs->space), cpv(-600, 0), cpv(600, 0), 0.0f));
	cpShapeSetElasticity(shape, 1.0f);
	cpShapeSetFriction(shape, 1.0f);


	// Add rectangle
	cpBody *body = cpSpaceAddBody(gs->space, cpBodyNew (gs->body_data->mass, gs->body_data->mom_inertia));
	cpBodySetPos (body, cpv (5, 5));

	cpShape *rect = cpSpaceAddShape(gs->space, cpBoxShapeNew(body, 2, 2));
	cpShapeSetUserData (shape, (int *) 1);
	cpShapeSetElasticity (shape, 0.0f);
	cpShapeSetFriction (shape, 0.7f);

	/* Signals used to handle the backing surface */
	g_signal_connect(da, "draw", G_CALLBACK(draw_cb), gs);
	g_signal_connect(da, "configure-event", G_CALLBACK(configure_event_cb), NULL);

	/* Event signals */
	g_signal_connect(da, "button-press-event", G_CALLBACK(button_press_event_cb), gs);
	g_signal_connect(da, "motion-notify-event", G_CALLBACK(motion_notify_event_cb), gs);
	g_signal_connect(da, "button-release-event", G_CALLBACK(button_release_event_cb), gs);

	/* Ask to receive events the drawing area doesn't normally
	 * subscribe to. In particular, we need to ask for the
	 * button press and motion notify events that want to handle.
	 */
	gtk_widget_set_events(da, gtk_widget_get_events(da) | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
        g_timeout_add(100, (GSourceFunc) time_handler, (gpointer) gs);
	gtk_widget_show_all(window);

	gtk_main();
	return 0;
}

#endif

// Draws rectangle onto surface
void graphics_drag_slice (GraphState *gstate, gdouble x2, gdouble y2) {
  cairo_t *cr;

  /* Paint to the surface, where we store our state */
  cr = cairo_create(gstate->surface);

  cairo_set_source_rgb(cr, 1, 1, 1);

  gdouble ix = gstate->ix;
  gdouble iy = gstate->iy;

  cairo_move_to(cr, ix, iy);
  cairo_line_to(cr, x2, y2);
  cairo_stroke(cr);

  /* Now invalidate the affected region of the drawing area. */
  gtk_widget_queue_draw_area(gstate->da, MIN(ix, x2), MIN(iy, y2), ABS(x2 - ix), ABS(y2 - iy));

  cairo_destroy (cr);
}

void clear_surface(GraphState *gs) {
  cairo_t *cr;

  cr = cairo_create(gs->surface);

  cairo_matrix_t saved_transform;
  cairo_get_matrix (cr, &saved_transform);

  double window_width = gtk_widget_get_allocated_width(gs->da);
  double window_height = gtk_widget_get_allocated_height(gs->da);

  cairo_scale(cr, window_width / BACKGROUND_WIDTH, window_height / BACKGROUND_HEIGHT);

  cairo_set_source_surface(cr, gs->background, 0, 0);

  cairo_set_matrix(cr, &saved_transform);

  cairo_paint(cr);

  cairo_destroy(cr);
}
