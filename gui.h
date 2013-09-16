#ifndef GUI_H
#define GUI_H

/*
 * All gui construction and call backs will be processed in gui.c.
 * Author: Richard Palomino
 */

#include <gtk/gtk.h>
#include <pthread.h>

#include "graphics.h"
#include "client.h"

typedef struct gui {
  GtkWidget* main_window;
  GtkWidget* box;
  GtkWidget* drawing_grid;
  GtkWidget* aspect_frame;
  
  GameState* game_state;
  GraphState* graph_state;
  //PointData *p_data;
  ClientData* client_data;
  pthread_mutex_t draw_lock;
  pthread_mutex_t drag_lock;
} Gui;

//Create the gui struct
Gui* gui_new(pthread_mutex_t lock1, pthread_mutex_t lock2);

//Attach the mouse callbacks for the GtkDrawingArea
void gui_attach_callbacks(Gui* crayon_gui);

//Free gui memory
void gui_destroy(Gui* crayon_gui);


#endif
