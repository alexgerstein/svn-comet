/*physics.h*/

#ifndef PHYSICS_H
#define PHYSICS_H

#include <chipmunk/chipmunk.h>
#include <gtk/gtk.h>

/* Note: This struct will likely get it's own file where we define how each body parameters should be set */
typedef struct body_data {
    double mass;
	double mom_inertia;
} BodyData;

typedef struct body_info {
	int index;
	double prev_x;
	double prev_y;
	double prev_angle;
	int loaded;
	int removed;
	int updated;
	
} body_info;

typedef struct s_body_info {
	int loaded;
} s_body_info;

typedef struct space_info {
	int space_index;
	int num_bodies;
	int array_length;
	cpBody **body_array;

} space_info;

typedef struct shape_data {
	cpShape *shape;
	double x;
	double y;
	double length;
	double width;

} ShapeData;

typedef struct game_state {
	cpSpace *space1;
	cpSpace *space2;
	int in_progress;
	int game_over;
	int loading;
	int level;

	GTimer *timer;

	double dt; /* Fixed time step */
	int slices;
	double orig_area;
	double curr_area;

	int percentage1;
	int percentage2;
	/* Additional fields to be defined later */
} GameState;

typedef struct {
	cpVect a, b;
	cpSpace *space;
	int sliced;
} cpQueryInfo;

void game_state_add_rectangle (GameState *gs, double x, double y, double height, double width);

void game_state_add_segment (GameState *gs, double x_l, double y_l, double x_u, double y_u, double radius);

int game_state_slice (cpSpace *space, double begin_x, double begin_y, double end_x, double end_y);

GameState *game_state_new ();

BodyData *body_state_new (double mass, double mom_inertia, double init_veloc);
body_info *physics_create_body_info(cpSpace* space, cpBody* body, int index, double init_x, double init_y);
cpSpace *physics_create_space(int index);
space_info *physics_create_space_info(int index);
s_body_info *physics_create_s_body_info();

void game_state_free (GameState *gs);

void check_shapes (cpSpace *space);

double calculate_area(GameState *gs);

#endif
