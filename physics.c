#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <gtk/gtk.h>
#include <chipmunk/chipmunk.h>
#include "physics.h"


#define ITERATIONS 10
#define TIMESTEP 1.0/60.0
#define GRAVITY cpv (0.0f, -9.8f)
#define SLEEP 0.5f
#define SLOP 0.1f
#define DENSITY (1.0f/10000.0f)

static void draw_intersect (cpShape *shape, cpFloat t, cpVect n, cpQueryInfo *info);
static void ClipPoly(cpSpace *space, cpShape *shape, cpVect n, cpFloat dist);
static void SliceShapePostStep(cpSpace *space, cpShape *shape, cpQueryInfo *info);
static void remove_poly (cpShape *shape, cpSpace *space);
static void post_step_remove (cpSpace *space, void *obj, void *data);
static void shape_size(cpShape* shape, double *size);


void check_shapes (cpSpace *space) {
	cpSpaceEachShape(space, (cpSpaceShapeIteratorFunc) remove_poly, space);
}

static void remove_poly (cpShape *shape, cpSpace *space) {
	if (shape->body->p.y < -3) {
		cpSpaceAddPostStepCallback(space, post_step_remove, shape->body, shape);
	}
}

static void post_step_remove (cpSpace *space, void *obj, void *data){
	cpBody *cur_body = (cpBody*) obj;
	cpShape *shape = (cpShape*) data;

	cpSpaceRemoveBody(space, cur_body);
	cpSpaceRemoveShape(space, shape);
	cpBodyFree(cur_body);
	cpShapeFree(shape);
}


/* Slicing Functionality Adapted from Chipmunk Demo slicing.c
Author: Scott Lembcke
Date: 2007
Code Version: 1.0
Location: GitHub Chipmunk Physics
*/

static void ClipPoly(cpSpace *space, cpShape *shape, cpVect n, cpFloat dist) {
	cpBody *body = cpShapeGetBody(shape);

	int count = cpPolyShapeGetNumVerts(shape);
	int clippedCount = 0;

	cpVect *clipped = (cpVect *)alloca((count + 1)*sizeof(cpVect));

	for(int i=0, j=count-1; i<count; j=i, i++){
		cpVect a = cpBodyLocal2World(body, cpPolyShapeGetVert(shape, j));
		cpFloat a_dist = cpvdot(a, n) - dist;

		if(a_dist < 0.0){
			clipped[clippedCount] = a;
			clippedCount++;
		}

		cpVect b = cpBodyLocal2World(body, cpPolyShapeGetVert(shape, i));
		cpFloat b_dist = cpvdot(b, n) - dist;

		if(a_dist*b_dist < 0.0f){
			cpFloat t = cpfabs(a_dist)/(cpfabs(a_dist) + cpfabs(b_dist));
			clipped[clippedCount] = cpvlerp(a, b, t);
			clippedCount++;
		}
	}

	cpVect centroid = cpCentroidForPoly(clippedCount, clipped);
	cpFloat mass = cpAreaForPoly(clippedCount, clipped)*DENSITY;
	cpFloat moment = cpMomentForPoly(mass, clippedCount, clipped, cpvneg(centroid));

	cpBody *new_body = cpSpaceAddBody(space, cpBodyNew(mass, moment));
	cpBodySetPos(new_body, centroid);
	cpBodySetVel(new_body, cpBodyGetVelAtWorldPoint(body, centroid));
	cpBodySetAngVel(new_body, cpBodyGetAngVel(body));

	int false_index = -1;
	cpBodySetUserData(new_body, physics_create_body_info(space, new_body, false_index, centroid.x, centroid.y));

	cpShape *new_shape = cpSpaceAddShape(space, cpPolyShapeNew(new_body, clippedCount, clipped, cpvneg(centroid)));
	// Copy whatever properties you have set on the original shape that are important
	cpShapeSetFriction(new_shape, cpShapeGetFriction(shape));
}

static void SliceShapePostStep(cpSpace *space, cpShape *shape, cpQueryInfo *info) {
	cpVect a = info->a;
	cpVect b = info->b;

	if (cpBodyGetMass(cpShapeGetBody (shape)) == INFINITY) {
		return;
	}

	info->sliced = 1;
	// Clipping plane normal and distance.
	cpVect n = cpvnormalize(cpvperp(cpvsub(b, a)));
	cpFloat dist = cpvdot(a, n);

	ClipPoly(space, shape, n, dist);
	ClipPoly(space, shape, cpvneg(n), -dist);

	cpBody *body = cpShapeGetBody(shape);
	cpSpaceRemoveShape(space, shape);
	cpSpaceRemoveBody(space, body);
	cpShapeFree(shape);

	space_info* s_info = (space_info *)cpSpaceGetUserData(space);

	if(s_info->space_index <= 1)
		cpBodyFree(body);
}

int game_state_slice (cpSpace *space, double begin_x, double begin_y, double end_x, double end_y) {
	cpVect sliceBegin = {begin_x, begin_y};
	cpVect sliceEnd = {end_x, end_y};

	cpQueryInfo info = {sliceBegin, sliceEnd, space, 0};

	//Iterate through intersected rects.
	cpSpaceSegmentQuery(space, cpv (begin_x, begin_y), cpv (end_x, end_y), CP_ALL_LAYERS, CP_NO_GROUP, (cpSpaceSegmentQueryFunc) draw_intersect, &info);
	//For each, call function that adds

	 if (info.sliced == 1) {
	 	return 1; 
	 }

	 else {
	 	return 0;
	 }
}

static void draw_intersect (cpShape *shape, cpFloat t, cpVect n, cpQueryInfo *info) {
	cpVect enter = info->a;
	cpVect end = info->b;

	if (!cpShapePointQuery(shape, enter) && !cpShapePointQuery(shape, end)) {
		cpSpaceAddPostStepCallback(info->space, (cpPostStepFunc)SliceShapePostStep, shape, info);
	}

	//cpVect enterPoint = cpSegmentQueryHitPoint ()
}

BodyData *body_state_new (double mass, double mom_inertia, double init_veloc) {
	BodyData *bs = (BodyData *) malloc(sizeof(BodyData));

	bs->mass = mass;
	bs->mom_inertia = mom_inertia;

	return bs;
}

body_info *physics_create_body_info(cpSpace* space, cpBody* body, int index, double init_x, double init_y) {
	body_info *b_info = (body_info *)calloc(1, sizeof(body_info));
	space_info *s_info = ((space_info *)cpSpaceGetUserData(space));

	int num_bodies = s_info->num_bodies;

	if(num_bodies == s_info->array_length - 1) {
		s_info->array_length = (s_info->array_length) * 2;
		s_info->body_array = realloc(s_info->body_array, sizeof(cpBody*) * s_info->array_length);
	}

	int body_index;
	if (index == -1)
		body_index = num_bodies;
	else
		body_index = index;

	b_info->index = body_index;
	

	num_bodies++;

	s_info->num_bodies = num_bodies;

	b_info->loaded = 0;
	b_info->removed = 0;
	b_info->updated = 0;

	b_info->prev_x = init_x;
	b_info->prev_y = init_y;
	b_info->prev_angle = 0.0f;

	return b_info;
}

s_body_info *physics_create_s_body_info() {
	s_body_info *s_b_info = (s_body_info *)calloc(1, sizeof(s_body_info));
	s_b_info->loaded = 0;
	return s_b_info;
}

space_info *physics_create_space_info(int index) {
	space_info *info = (space_info *)calloc(1, sizeof(space_info));
	info->space_index = index;
	info->num_bodies = 0;
	info->array_length = 40;
	info->body_array = (cpBody **)calloc(info->array_length, sizeof(cpBody*));
	return info;
}

cpSpace *physics_create_space(int index) {

	cpSpace *space = cpSpaceNew();
	cpSpaceSetUserData (space, physics_create_space_info(index));
	cpSpaceSetIterations (space, ITERATIONS);
	cpSpaceSetGravity (space, GRAVITY);
	cpSpaceSetSleepTimeThreshold (space, SLEEP);
	cpSpaceSetCollisionSlop (space, SLOP);

	return space;
}

GameState *game_state_new () {
	
	GameState *gs = (GameState *) malloc(sizeof(GameState));

	gs->space1 = NULL;
	gs->space2 = NULL;
	gs->in_progress = 0;
	gs->loading = 0;
	gs->game_over = 0;
	gs->dt = TIMESTEP;
	gs->slices = 3;
	gs->percentage1 = 0;
	gs->percentage2 = 0;
	gs->timer = NULL;
	gs->orig_area = 0;

	gs->level = 1;


	return gs;
}

double calculate_area(GameState *gs) {
	double curr_area = 0;
	cpSpaceEachShape(gs->space1, (cpSpaceShapeIteratorFunc) shape_size, &curr_area);
	gs->percentage1 = (1 - curr_area / gs->orig_area) * 100;
	curr_area = 0;
	cpSpaceEachShape(gs->space2, (cpSpaceShapeIteratorFunc) shape_size, &curr_area);
	gs->percentage2 = (1 - curr_area / gs->orig_area) * 100;
	return curr_area;
}

static void shape_size(cpShape* shape, double *size) {
	if (cpBodyGetMass(cpShapeGetBody(shape)) != INFINITY) {
		int num_verts = cpPolyShapeGetNumVerts(shape);
		cpVect *vects = (cpVect *) malloc (sizeof(cpVect) * num_verts);
		for(int i = 0; i<num_verts; i++) {
			vects[i] = cpPolyShapeGetVert(shape, i);
		}
		*size = *size + cpAreaForPoly(cpPolyShapeGetNumVerts(shape), vects)*1000;
		free(vects);
	}
}

void game_state_free (GameState *gs) {
	if (gs->space1 != NULL) {
		cpSpaceFree (gs->space1);
	}

	if (gs->space2 != NULL) {
		cpSpaceFree (gs->space2);
	}

	free (gs);
}

// #ifdef DEBUG

// int main (int argc, char *argv[]) {


// 	GameState *gs = game_state_new ();
	
// 	// Add floor
// 	cpShape *shape = cpSpaceAddShape(gs->space, cpSegmentShapeNew(cpSpaceGetStaticBody(gs->space), cpv(-4, 0), cpv(4, 0), 0.0f));
// 	cpShapeSetElasticity(shape, 1.0f);
// 	cpShapeSetFriction(shape, 1.0f);
// 	//cpShapeSetLayers(shape, NOT_GRABABLE_MASK);

//         game_state_add_rectangle (gs, 0, 200, 4.0f, 5.0f);

// 	for (cpFloat time = 0; time < 10; time += TIMESTEP) {

	
// 	}
// 	game_state_free (gs);

// }

// #endif
