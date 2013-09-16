/*
 *  Core.c details the fileIO of the Crayon Game, beginning with create_level.
 *
 *  Shapes are to be supplied in the format SHAPE_DESCRIPTION 
 *  Author: Richard Palomino
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <chipmunk/chipmunk_private.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "core.h"
#include "physics.h"


// Old prototypes that might be deprecated
static int core_client_parse_command (char *command);
char *splice_string(char* input_string, int begin, int end);

// Current prototypes for handling commands
static void core_add_body (cpSpace *space, char *command);
static int core_slice_body (cpSpace *space, char *command);
static void core_update_bodies(cpSpace *space, char *command);
static void core_remove_body(cpSpace *space, char *command);
static void h_core_remove_shapes(cpBody* body, cpShape* shape, void *data);
cpBody* get_body(cpSpace *space, int index);
static SendData *send_data_new (char *to_send, int connfd);

// RICK's NEW HEADERS
static void check_adding_queue (cpShape *shape, SendData *sd);
static int compare_doubles(double, double);

void load_test_level (cpSpace *space);

char* core_load_level(GameState *game_state, int level_number) {
  char file_name[30] = "./levels/";
  char number[8] = "";
  snprintf(number, 7, "%d.lvl", level_number);

  strcat(file_name, number);

  FILE* level_file = fopen(file_name, "rb");

  fseek(level_file, 0, SEEK_END);
  unsigned int file_size = (unsigned int) ftell(level_file);
  fseek(level_file, 0, SEEK_SET);

  char *file_contents = (char *) calloc(file_size + 1, sizeof(char));
  
  for(int i = 0; i < file_size; i++) {
    file_contents[i] = fgetc(level_file);
  }

  if (game_state->space1 != NULL) {
    core_iterate_protocol (game_state, file_contents);
  }

  return file_contents;

}

static SendData *send_data_new (char *to_send, int space_index) {
  SendData *sd = (SendData *) malloc (sizeof(SendData));
  sd->to_send = to_send;
  sd->space_index = space_index;

  return sd;
}

static void check_adding_queue (cpShape *shape, SendData *sd) {


  // // Add second base
  // cpBody *right_base = cpBodyNewStatic();
  // //cpSpaceAddBody(s_data->gm_st->space, right_base);
  // cpBodySetPos (right_base, cpv (7, 4));

  // cpShape *right_floor = cpBoxShapeNew(right_base, 1, 1);
  // cpShapeSetElasticity(right_floor, 0.0f);
  // cpShapeSetFriction(right_floor, 0.9f);
  // cpSpaceAddShape(space, right_floor);

  char *to_send = sd->to_send;

  cpBody *body = cpShapeGetBody(shape);

  cpSpace *space = body->/*CP_PRIVATE(*/space; 
  //cpSpace *shape_space = shape->/*CP_PRIVATE*/space;

  //space_info* s_info = (space_info*)cpSpaceGetUserData(shape_space);

  if (space == NULL) {
    if (((s_body_info *)cpBodyGetUserData(body))->loaded > 1) {
      return;
    }

    ((s_body_info *) cpBodyGetUserData(body))->loaded += 1;
    char add_protocol[200] = "";
    snprintf(add_protocol, 199, " %d ADD_BODY S %d %.3f %.3f ", sd->space_index, cpPolyShapeGetNumVerts(shape), cpBodyGetPos(body).x, cpBodyGetPos(body).y);

      for (int i = 0; i < cpPolyShapeGetNumVerts(shape); i++) {
        char vertices[20] = "";
        cpVect this_vect = cpPolyShapeGetVert(shape, i);
        snprintf(vertices, 19, "%.3f %.3f ", this_vect.x, this_vect.y);
        strcat(add_protocol, vertices);
      }

    if(strlen(add_protocol) != 0) {
      strcat(add_protocol, "\n");
    }
    strcat(to_send, add_protocol);
  }

  else {

    body_info *b_info = ((body_info *)cpBodyGetUserData(body));
    space_info *s_info = ((space_info *)cpSpaceGetUserData(space));

    char add_protocol[200] = "";

    if (s_info->body_array[b_info->index] == NULL) {
      snprintf(add_protocol, 199, " %d ADD_BODY %d %d %.3f %.3f ", sd->space_index, b_info->index, cpPolyShapeGetNumVerts(shape), cpBodyGetPos(body).x, cpBodyGetPos(body).y);


      if (b_info->loaded >= 1)
        s_info->body_array[b_info->index] = body;
      else
        b_info->loaded += 1;

      for(int i = 0; i < cpPolyShapeGetNumVerts(shape); i++) {
          char vertices[20] = "";
          cpVect this_vect = cpPolyShapeGetVert(shape, i);
          snprintf(vertices, 19, "%.3f %.3f ", this_vect.x, this_vect.y);
          strcat(add_protocol, vertices);
        }
    }
    
    if(strlen(add_protocol) != 0) {
      strcat(add_protocol, "\n");
    }
    strcat(to_send, add_protocol);
  }
}

int compare_doubles(double first, double second) {
  double epsilon = 0.1f;
  return first - second >= epsilon ;
}

void core_server_generate_updates (char *command, cpSpace *space, int connfd) {

  char *to_send = (char *)calloc(1024, sizeof(char));

  space_info *s_info = ((space_info *)cpSpaceGetUserData(space));

  int space_index = 0;

  if (s_info->space_index != connfd) {
    space_index = 1;
  }

  SendData *sd = send_data_new (to_send, space_index);

  cpSpaceEachShape(space, (cpSpaceShapeIteratorFunc) check_adding_queue, sd);

  int num_bodies = s_info->num_bodies;
  cpBody **body_array = s_info->body_array;
  int updates = 0;


  char update_protocol[700] = "";
  snprintf(update_protocol, 20, " %d UPDATE_BODIES ", space_index);
  char updated_bodies[400] = "";

  for (int i = 0; i < num_bodies; i++) {

    if (body_array[i] == NULL) {
      continue;
    }

    body_info *b_info = ((body_info *)cpBodyGetUserData(body_array[i]));

    if(!cpSpaceContainsBody(space, body_array[i]) && b_info != NULL && b_info->removed < 2) {
      if (b_info->removed == 1)
        body_array[i] = NULL;
      char remove_protocol[20] = "";
      snprintf(remove_protocol, 19, " %d REMOVE %d \n", space_index, i);
      strcat(to_send, remove_protocol);
      b_info->removed += 1;
    }

    else {

      if ( body_array[i] != NULL && ((compare_doubles(b_info->prev_x, cpBodyGetPos(body_array[i]).x)) || 
        ( compare_doubles(b_info->prev_y, cpBodyGetPos(body_array[i]).y)) ||
        ( compare_doubles(b_info->prev_angle, cpBodyGetAngle(body_array[i]))))) { 

        if (b_info->updated == 1) {
          b_info->prev_x = cpBodyGetPos(body_array[i]).x;
          b_info->prev_y = cpBodyGetPos(body_array[i]).y;
          b_info->prev_angle = cpBodyGetAngle(body_array[i]);
          b_info->updated = 0;
        }

        else {
          b_info->updated += 1;
        }


        char body_update[100] = "";
        snprintf(body_update, 99, "%d %.3f %.3f %.3f ", i, b_info->prev_x, b_info->prev_y, b_info->prev_angle);
        strcat(updated_bodies, body_update);
        updates++;
    }
  }
}

  if(updates != 0) {

  char num_updates[4] = "";
  snprintf(num_updates, 3, "%d ", updates);

  strcat(update_protocol, num_updates);
  strcat(update_protocol, updated_bodies);
  update_protocol[strlen(update_protocol) - 1] = '\n';

  strcat(to_send, update_protocol);
}

strcat(command, to_send);

free(to_send);

} 


void core_generate_slice(char *command, double x1, double y1, double x2, double y2) {
  char slice_string[100] = "";
  snprintf(slice_string, sizeof(slice_string), " SLICE %.6f %.6f %.6f %.6f \n", x1, y1, x2, y2);
  strcat(command, slice_string);
}

void core_generate_end_level(char *command) {
  char end_string[5] = "END \n";
  strcat(command, end_string);
}


char *splice_string(char* input_string, int begin, int end) {
  int len = end - begin;
  char *str_splice = (char *) calloc(len + 1, sizeof(char));
  for(int i = 0; i < len + 1; i++) {
    str_splice[i] = input_string[begin + i];
  }

  return str_splice;
}

// Function that returns an int corresponding to the type of command. This function should also remove the 
static int core_client_parse_command (char *command){
  
  char *desc = strtok(command, " ");

  desc = strtok(NULL, " ");

  if (desc == NULL) {
    return -1;
  }

  if ( strcmp ( desc, "ADD_BODY") == 0)
    return ADD_BODY;
  if ( strcmp ( desc, "UPDATE_BODIES") == 0)
    return UPDATE_BODIES;
  if ( strcmp ( desc, "REMOVE") == 0)
    return REMOVE_BODY;
//  if ( strcmp ( desc, "CHAT") == 0)
   // return CHAT;

  return -1;
}


void core_iterate_protocol(GameState *game_state, char *command) {
  int length = strlen(command);
  int index_begin = 1;
  int index_end = 1;

  while (index_end < strlen(command)) {
    while (command[index_end] != '\0' && command[index_end] != '\n') {
      index_end++;
    }

    if (index_begin > length)
      return;

    char *spliced_string = splice_string(command, index_begin, index_end - 1);
    index_begin = index_end + 2;
    index_end = index_begin;

    if (spliced_string[0] == '2') {
      core_client_handle_command(game_state->space1, spliced_string);
      if (game_state->space2 != NULL) {
        core_client_handle_command(game_state->space2, spliced_string);
      }
    }

    if(spliced_string[0] == '0') {
      core_client_handle_command(game_state->space1, spliced_string);
    }

    else if (spliced_string[0] == '1') {
      core_client_handle_command(game_state->space2, spliced_string);
    }
    
    free(spliced_string);
  }
}

// Function that returns the result of handling a command. If positive, it has been successful, returns an index corresponding to 
int core_client_handle_command (cpSpace *space, char *command) {

  char protocol_description[20] = "";
  strncpy(protocol_description, command, 19);

  // Need to have an additional stage that parses the command being sent into individual parts that can be used
  int command_type = core_client_parse_command(protocol_description);

  char *command_copy = (char *) calloc (strlen(command) + 1, sizeof(char));
  strncpy(command_copy, command, strlen(command));
  
  if (command_type < 0) { return command_type; }
  switch (command_type) 
    {
    case ADD_BODY:
      core_add_body (space, command_copy);
      return ADD_BODY;
    case UPDATE_BODIES:
      core_update_bodies (space, command_copy);
      return UPDATE_BODIES;
    case REMOVE_BODY:
      core_remove_body (space, command_copy);
      return REMOVE_BODY;
    case END:
      //core_end_game(space, command_copy);
     
      return END;
    case LOAD: // Client asks the server to load the next level
      //core_create_level (command);
      return LOAD;
    default:
      return -1;
  }
  free(command_copy); 
}

int core_server_handle_slice (cpSpace *space, char *command) {
  return core_slice_body (space, command);
}

static int core_slice_body (cpSpace *space, char *command) {
  
  char *this_token = strtok (command, " ");

  if (this_token == NULL) {
    return 0;
  }

  char *x1 = strtok (NULL, " ");

  if (x1 == NULL) {
    return 0;
  }

  char *y1 = strtok (NULL, " ");

   if (y1 == NULL) {
    return 0;
  }

  char *x2 = strtok (NULL, " ");

  if (x2 == NULL) {
    return 0;
  }

  char *y2 = strtok (NULL, " ");

  if (y2 == NULL) {
    return 0;
  }

  return game_state_slice (space, atof(x1), atof(y1), atof(x2), atof(y2));

}

static void core_add_body (cpSpace *space, char *command) {

  //Space index token
  char *this_token = strtok(command, " ");
  int space_index = atoi(this_token);

  //Disregard ADD_BODY
  strtok(NULL, " ");

  //This is either S for static body or a unique body index
  this_token = strtok(NULL, " ");

  if (this_token == NULL) {
    return;
  }

  cpBool s_body = (this_token[0] == 'S');
  
  int index;

  if (s_body == cpFalse)
    index = atoi(this_token);

  //Number of vertices token
  this_token = strtok(NULL, " ");

  if (this_token == NULL) {
    return;
  }

  int num_vertices = atoi(this_token);


  //X Position token
  this_token = strtok(NULL, " ");

  if (this_token == NULL) {
    return;
  }

  double x_pos = atof(this_token);

  //Y Position token
  this_token = strtok(NULL, " ");

  if (this_token == NULL) {
    return;
  }

  double y_pos = atof(this_token);

  cpVect *poly_vects = (cpVect *)calloc(num_vertices, sizeof(cpVect));

  for(int i = 0; i < num_vertices; i++) {

    double x = atof(strtok(NULL, " "));
    double y = atof(strtok(NULL, " "));
    cpVect this_vect = cpv(x, y);

    poly_vects[i] = this_vect;
  }

  cpVect body_center = cpv(x_pos, y_pos);

  if (!s_body) {
    cpBody *new_body = cpSpaceAddBody(space, cpBodyNew(1.0f, 1.0f));
    cpBodySetPos(new_body, body_center);
    cpBodySetUserData(new_body, physics_create_body_info(space, new_body, index, 0.0f, 0.0f));

    space_info *s_info = ((space_info *)cpSpaceGetUserData(space));
    body_info *b_info = ((body_info *)cpBodyGetUserData(new_body));

    if (space_index != 2) {
      s_info->body_array[b_info->index] = new_body;
    }

    cpShape *new_shape = cpSpaceAddShape(space, cpPolyShapeNew(new_body, num_vertices, poly_vects, cpvzero));
    cpShapeSetElasticity (new_shape, 0.0f);
    cpShapeSetFriction (new_shape, 0.5f);
  }

  else {
    cpBody *new_shelf = cpBodyNewStatic ();
    cpBodySetPos(new_shelf, body_center);
    cpBodySetUserData(new_shelf, physics_create_s_body_info());

    cpShape *shelf_shape = cpBoxShapeNew(new_shelf, ABS(poly_vects[2].x - poly_vects[1].x), ABS(poly_vects[0].y - poly_vects[1].y));
    cpShapeSetElasticity(shelf_shape, 0.0f);
    cpShapeSetFriction(shelf_shape, 0.9f);
    cpSpaceAddShape(space, shelf_shape);
  }

  
  
  // space_add_segments (x_coords, y_coords, index, space);

}

static void core_update_bodies(cpSpace *space, char *command) {
  strtok(command, " ");
  strtok(NULL, " ");

  char *num_updates = strtok(NULL, " ");

  if (num_updates == NULL) {
    return;
  }

  for(int update_i = 0; update_i < atoi(num_updates); update_i++) {

    char *body_index = strtok(NULL, " ");

    if (body_index == NULL) {
      return;
    }

    cpBody* body_to_update = get_body(space, atoi(body_index));

    if (body_to_update == NULL) {
      return;
    }

    char *x1 = strtok(NULL, " ");
    char *y1 = strtok(NULL, " ");
    char *angle = strtok(NULL, " ");

    if (x1 == NULL || y1 == NULL || angle == NULL) {
      return;
    }

    // printf ("New Center: %f, %f; New Angle: %f, %f\n", x1, y1, anglex, angley);

    cpBodySetPos(body_to_update, cpv ( atof(x1), atof(y1)));
    cpBodySetAngle(body_to_update, atof(angle));
  }

}

static void core_remove_body(cpSpace *space, char *command) {

  char *space_index = strtok(command, " ");
  char *cmd =strtok(NULL, " ");
  char *body_index = strtok(NULL, " ");

  if (body_index == NULL) {
    return;
  }

  cpBody* body_to_remove = get_body(space, atoi(body_index));
  //Must clean up shapes attached to body first

  if (body_to_remove == NULL) {
    return;
  }

  cpBodyEachShape(body_to_remove, (cpBodyShapeIteratorFunc) h_core_remove_shapes, NULL);
  
  if (body_to_remove == NULL) {
    return;
  }

  //Clean up body
  cpSpaceRemoveBody(space, body_to_remove);
  //cpBodyFree(body_to_remove);
}


static void h_core_remove_shapes(cpBody* body, cpShape* shape, void *data) {
  if (shape == NULL) {
    return;
  }

  if (body == NULL) {
    return;
  }

  cpSpaceRemoveShape(body->/*CP_PRIVATE*/space, shape);
  //cpShapeFree(shape);
}
 
#ifdef DEBUG
int main(int argc, char** argv) {

  //char input_string[25] = " 123456789! 123451334! ";
  
  //char test_real[415] = " ADD_BODY 0 4 2.000000 7.000000 0.750000 1.500000 1.500000 0.750000 1.500000 -0.75000 0.750000 -1.50000 -0.750000 -1.5000 -1.500000 -0.7500 -1.500000 0.75000 -0.750000 1.50000 ! ADD_BODY 1 4 7.000000 6.000000 0.750000 1.500000 1.500000 0.750000 1.500000 -0.75000 0.750000 -1.50000 -0.750000 -1.5000 -1.500000 -0.7500 -1.500000 0.75000 -0.750000 1.50000 ! UPDATE_BODIES 1 1 X7.000000 Y6.000000 RX1.000000 RY0.000000";

  //core_iterate_protocol (NULL, NULL, test_real);

  //core_load_level(NULL, NULL, 1);

  //Takes a file name at the terminal and tries to create the level
  //core_create_level(NULL, argv[1]);
  return EXIT_SUCCESS;
}
#endif

cpBody* get_body(cpSpace *space, int index) {
  space_info *s_info = ((space_info *)cpSpaceGetUserData(space));
  return (s_info->body_array[index]);
}