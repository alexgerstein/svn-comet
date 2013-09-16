#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <stdbool.h>
#include "gui.h"
#include "core.h"
#include "sleeping.h"

#define METER_SCALE 10
#define MAX_BUFFER_LENGTH 80

#define MAX_LEVEL 3

static bool choose_server (Gui *crayon_gui);
static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, Gui *crayon_gui);
static void draw_game (GtkWidget *widget, Gui *crayon_gui, int space_index);
static gboolean button_press_event_cb(GtkWidget *widget, GdkEventButton *event, Gui *crayon_gui);
static void set_game_status (Gui *crayon_gui);
static void handle_menu_buttons (GtkWidget *widget, Gui *crayon_gui, int mouse_x, int mouse_y);
static void add_opponent_display (Gui *crayon_gui);
static void remove_opponent_display (Gui *, GtkWidget *, GtkWidget *);
static void show_server_error (Gui * crayon_gui);
static gboolean motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, Gui *crayon_gui);
static gboolean button_release_event_cb(GtkWidget *widget, GdkEventButton *event, Gui *crayon_gui);
static gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, Gui *crayon_gui);


// Does this callback get called by the time handler or by something else?
static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, Gui *crayon_gui) {
  	pthread_mutex_lock (&(crayon_gui->draw_lock));
	cairo_set_source_surface (cr, crayon_gui->graph_state->surface, 0, 0);
	cairo_paint(cr);

	clear_surface(crayon_gui->graph_state);

	if (crayon_gui->game_state == NULL) {
		graphics_draw_menu(crayon_gui->graph_state);
	}

	else if (crayon_gui->game_state->loading) {
		graphics_draw_load_screen (crayon_gui->graph_state);
	}

	else {
		draw_game(widget, crayon_gui, 0);


		if(crayon_gui->game_state != NULL) {
			if (crayon_gui->game_state->game_over) {
				graphics_draw_alert (crayon_gui->graph_state, crayon_gui->game_state->percentage1);
			}
		}
	}
  	pthread_mutex_unlock (&(crayon_gui->draw_lock));


	gtk_widget_queue_draw(widget);

	return FALSE;
}

// Does this callback get called by the time handler or by something else?
static gboolean draw_opponent_cb (GtkWidget *widget, cairo_t *cr, Gui *crayon_gui) {
  	pthread_mutex_lock (&(crayon_gui->draw_lock));
	cairo_set_source_surface (cr, crayon_gui->graph_state->surface, 0, 0);
	cairo_paint(cr);

	clear_surface(crayon_gui->graph_state);

	if (crayon_gui->game_state == NULL) {
		graphics_draw_menu(crayon_gui->graph_state);
	}

	else if (crayon_gui->game_state->loading) {
		graphics_draw_load_screen (crayon_gui->graph_state);
	}

	else {
		draw_game(widget, crayon_gui, 1);

		if(crayon_gui->game_state != NULL) {
			if (crayon_gui->game_state->game_over) {
				graphics_draw_alert (crayon_gui->graph_state, crayon_gui->game_state->percentage2);
			}
		}
	}
  	pthread_mutex_unlock (&(crayon_gui->draw_lock));


	gtk_widget_queue_draw(widget);

	return FALSE;
}

static int single_player_game (Gui *crayon_gui) {
	set_game_status(crayon_gui);

	if (crayon_gui->game_state != NULL && crayon_gui->game_state->in_progress) {

		pthread_mutex_lock(&crayon_gui->draw_lock);
		cpSpaceStep (crayon_gui->game_state->space1, crayon_gui->game_state->dt);
		check_shapes(crayon_gui->game_state->space1);
		calculate_area(crayon_gui->game_state);
		pthread_mutex_unlock(&crayon_gui->draw_lock);

	}

	return TRUE;

}

static void draw_game (GtkWidget *widget, Gui *crayon_gui, int space_index) {

	//pthread_mutex_lock(&crayon_gui->draw_lock);
	int percentage;
	cpSpace *drawn_space;
	int slices;
	if (space_index == 0) {
		drawn_space = crayon_gui->game_state->space1;
		percentage = crayon_gui->game_state->percentage1;
		slices = crayon_gui->game_state->slices;
	} else {
		drawn_space = crayon_gui->game_state->space2;
		percentage  = crayon_gui->game_state->percentage2;
		slices = 0;
	}

	graphics_draw_shapes(crayon_gui->graph_state, drawn_space);

	graphics_draw_info(crayon_gui->graph_state, crayon_gui->game_state->timer, slices, percentage);

	if (crayon_gui->graph_state->drawing && crayon_gui->game_state->in_progress && space_index == 0) {
		graphics_drag_slice(crayon_gui->graph_state, crayon_gui->graph_state->fx, crayon_gui->graph_state->fy);
	}

	//pthread_mutex_unlock(&crayon_gui->draw_lock);
}

static void set_game_status (Gui *crayon_gui) {

	if (crayon_gui->game_state == NULL) {
		return;
	}

	// While loading multiplayer
	if (crayon_gui->game_state->loading) {
		if (crayon_gui->graph_state->timer == NULL) {
			crayon_gui->graph_state->timer = g_timer_new();
		}

		if (((space_info *)cpSpaceGetUserData(crayon_gui->game_state->space1))->num_bodies > 0) {
			GtkWidget *opponent_window = gtk_grid_get_child_at (GTK_GRID (crayon_gui->drawing_grid), 1, 0);
			if (opponent_window == NULL)
				add_opponent_display(crayon_gui);
			g_timer_destroy(crayon_gui->graph_state->timer);
			crayon_gui->graph_state->timer = NULL;
			crayon_gui->game_state->loading = 0;
			crayon_gui->game_state->in_progress = 2;
			crayon_gui->game_state->orig_area = calculate_area(crayon_gui->game_state);

		} else if (g_timer_elapsed(crayon_gui->graph_state->timer, NULL) > 10) {
			GtkWidget *opponent_window = gtk_grid_get_child_at (GTK_GRID (crayon_gui->drawing_grid), 1, 0);

			g_timer_destroy(crayon_gui->graph_state->timer);
			crayon_gui->graph_state->timer = NULL;

			if (opponent_window != NULL)
				remove_opponent_display(crayon_gui, crayon_gui->main_window, crayon_gui->drawing_grid);

			crayon_gui->game_state->loading = 0;
			game_state_free (crayon_gui->game_state);
			client_data_destroy (crayon_gui->client_data);
			crayon_gui->game_state = NULL;
		}
	}

  // While player has won
	else if (crayon_gui->game_state->percentage1 == 100 || crayon_gui->game_state->percentage2 == 100 || !crayon_gui->game_state->in_progress || (!crayon_gui->game_state->slices && test_all_sleeping(crayon_gui->game_state->space1)) ) {
		crayon_gui->game_state->game_over = 1;
		crayon_gui->game_state->in_progress = 0;

		if (crayon_gui->graph_state->timer == NULL) {
			crayon_gui->graph_state->timer = g_timer_new();

			g_timer_stop (crayon_gui->game_state->timer);
		}

		if (g_timer_elapsed(crayon_gui->graph_state->timer, NULL) > 5) {
			GtkWidget *opponent_window = gtk_grid_get_child_at (GTK_GRID (crayon_gui->drawing_grid), 1, 0);

			g_timer_destroy(crayon_gui->graph_state->timer);
			g_timer_destroy(crayon_gui->game_state->timer);
			crayon_gui->graph_state->timer = NULL;
			crayon_gui->game_state->timer = NULL;

			if (crayon_gui->game_state->level == MAX_LEVEL && (crayon_gui->game_state->percentage1 == 100 || opponent_window != NULL)) {
				if (opponent_window != NULL) {
					remove_opponent_display (crayon_gui, crayon_gui->main_window, crayon_gui->drawing_grid);
				}
				game_state_free (crayon_gui->game_state);
				crayon_gui->game_state = NULL;
				return;
			}

			else if (opponent_window == NULL) {
				int new_level = crayon_gui->game_state->level;

				if (crayon_gui->game_state->percentage1 == 100) {
					new_level = new_level + 1;
				}

				game_state_free (crayon_gui->game_state);
				crayon_gui->game_state = NULL;

				crayon_gui->game_state = game_state_new();
				crayon_gui->game_state->space1 = physics_create_space(0);
				crayon_gui->game_state->space2 = physics_create_space(1);

				crayon_gui->game_state->level = new_level;
				pthread_mutex_lock(&(crayon_gui->draw_lock));
				core_load_level (crayon_gui->game_state, crayon_gui->game_state->level);
				pthread_mutex_unlock(&(crayon_gui->draw_lock));
				crayon_gui->game_state->in_progress = 1;
				crayon_gui->game_state->orig_area = calculate_area(crayon_gui->game_state);
			}

			else if (opponent_window != NULL) {

				//int old_level = crayon_gui->game_state->level;
				//remove_opponent_display (crayon_gui, crayon_gui->main_window, crayon_gui->drawing_grid);
				crayon_gui->client_data->send_waiting = 1;
				core_generate_end_level(crayon_gui->client_data->command); 

				game_state_free(crayon_gui->game_state);
				crayon_gui->game_state = NULL;

				crayon_gui->game_state = game_state_new();
				crayon_gui->client_data->game_state = crayon_gui->game_state;
				crayon_gui->game_state->space1 = physics_create_space(0);
				crayon_gui->game_state->space2 = physics_create_space(1);
				
				crayon_gui->client_data->send_waiting = 1;
				core_generate_end_level(crayon_gui->client_data->command); 


				//crayon_gui->game_state->level = old_level + 1;
				crayon_gui->game_state->loading = 1;
				//crayon_gui->game_state->orig_area = calculate_area(crayon_gui->game_state);
			}

		}
	}   

  // While playing game
	else if (crayon_gui->game_state->in_progress) {
		if (crayon_gui->game_state->timer == NULL) {
			crayon_gui->game_state->timer = g_timer_new();
		}

		int seconds_remaining = 30 - g_timer_elapsed(crayon_gui->game_state->timer, NULL);

		if (seconds_remaining <= 0) {
			crayon_gui->game_state->slices = 0;
			crayon_gui->game_state->in_progress = 0;
		}
	}
}

// Wrapper function to handle the communication with the server
static void* game_thread (void *ptr) {

	Gui *crayon_gui = (Gui *) ptr;

  // Loops until we tell it that a game is not active. Should probably change this variable name to something else (game_active doesn't imply anyone is playing yet)
	while (1) {

    // Loop allows us to dynamically change between single and multiplayer

		while (crayon_gui->game_state == NULL) {
			sleep(1);
		}

		while (crayon_gui->game_state != NULL && (crayon_gui->game_state->in_progress != 1 || crayon_gui->game_state->loading)) {
			client_handle_communication (crayon_gui->client_data);
			set_game_status(crayon_gui);
			usleep (25000);
		}
		if (crayon_gui->client_data != NULL && crayon_gui->game_state != NULL && !crayon_gui->game_state->loading) {
			client_data_destroy (crayon_gui->client_data);
		}

		while (crayon_gui->game_state != NULL && crayon_gui->game_state->in_progress != 2) {
			single_player_game (crayon_gui);
			usleep (25000);
		}
	
	}
}


static bool choose_server (Gui *crayon_gui) {
	GtkWidget* address_entry = gtk_entry_new();
	GtkWidget* port_entry = gtk_entry_new();
	GtkWidget* server_instructions = gtk_label_new("Enter the address and port of the server.");

	gtk_entry_set_width_chars (GTK_ENTRY(address_entry), 20);
	gtk_entry_set_width_chars (GTK_ENTRY(port_entry), 10);

	gtk_entry_set_placeholder_text (GTK_ENTRY(address_entry), "Server address");
	gtk_entry_set_placeholder_text (GTK_ENTRY(port_entry), "Port");
	gtk_entry_set_text(GTK_ENTRY(address_entry), "127.0.0.1");
	gtk_entry_set_text(GTK_ENTRY(port_entry), "3002");


	GtkWidget* server_dialog = gtk_dialog_new_with_buttons ("Choose Server", GTK_WINDOW(crayon_gui->main_window),
		GTK_DIALOG_MODAL,
		GTK_STOCK_OK,
		GTK_RESPONSE_ACCEPT, NULL);

	GtkWidget* content = gtk_dialog_get_content_area (GTK_DIALOG(server_dialog));

	gtk_box_pack_start(GTK_BOX(content), server_instructions, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content), address_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content), port_entry, FALSE, FALSE, 0);

	gtk_widget_show_all(server_dialog);

	gint choice = gtk_dialog_run (GTK_DIALOG (server_dialog));


  // Create a client data struct
	ClientData* cd = NULL;
	if(choice == GTK_RESPONSE_ACCEPT){
		cd = client_data_new (gtk_entry_get_text(GTK_ENTRY(address_entry)), gtk_entry_get_text(GTK_ENTRY(port_entry)), crayon_gui->draw_lock, crayon_gui->drag_lock);
	}
	crayon_gui->client_data = cd;


	gtk_widget_destroy(server_instructions);
	gtk_widget_destroy(address_entry);
	gtk_widget_destroy(port_entry);
	gtk_widget_destroy(server_dialog);

	if (crayon_gui->client_data == NULL) {
		show_server_error (crayon_gui);
		return FALSE;
	}

	return TRUE;
}

static void show_server_error (Gui * crayon_gui) {
	GtkWidget *server_error;

	server_error = gtk_message_dialog_new (GTK_WINDOW(crayon_gui->main_window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error connecting to server");
	gtk_window_set_title(GTK_WINDOW(server_error), "Error");
	gtk_dialog_run (GTK_DIALOG(server_error));

	gtk_widget_show_all (server_error);

	gtk_widget_destroy (server_error);

}

// // Should be ported over to physics, can probably be taken out
// static void update_coords (PointData *p_data, double new_x, double new_y) {
//   int buffer_length = p_data->max_buffer_length;
//   int index = p_data->index;
//   if (buffer_length == index) {
//     buffer_length *= 2;
//     p_data->max_buffer_length = buffer_length;
//     p_data->x_coords = (double *) realloc (p_data->x_coords, buffer_length * sizeof(double));
//     p_data->y_coords = (double *) realloc (p_data->y_coords, buffer_length * sizeof(double));
//   }
//   (p_data->x_coords)[index] = new_x;
//   (p_data->y_coords)[index] = new_y;
//   p_data->index += 1;
//   return;
// }



// Grab the first coordinate and store it in 
static gboolean button_press_event_cb(GtkWidget *widget, GdkEventButton *event, Gui *crayon_gui) {

	// if (crayon_gui->graph_state->surface == NULL) {
	// 	return FALSE;
	// }

	if (crayon_gui->game_state == NULL) {
		if (event->button == GDK_BUTTON_PRIMARY) {
			handle_menu_buttons (widget, crayon_gui, event->x, event->y);
			return TRUE;
		}
	}

	if (crayon_gui->game_state->in_progress && crayon_gui->game_state->slices > 0) {

		pthread_mutex_lock(&crayon_gui->drag_lock);
		if ((event->button == GDK_BUTTON_PRIMARY)) {
			crayon_gui->graph_state->ix = event->x;
			crayon_gui->graph_state->iy = event->y;
		}

		crayon_gui->graph_state->drawing = FALSE;
	} 

  /* We've handled the event, stop processing */
	return TRUE;
}

static void handle_menu_buttons (GtkWidget *widget, Gui *crayon_gui, int mouse_x, int mouse_y) {
	double window_height = gtk_widget_get_allocated_height(widget);

	if (mouse_y > 3 * window_height / 4) {
		bool valid_server = choose_server(crayon_gui);

		if (valid_server) {
			crayon_gui->game_state = game_state_new();
			crayon_gui->client_data->game_state = crayon_gui->game_state;
			crayon_gui->game_state->loading = 2;
			crayon_gui->game_state->space1 = physics_create_space(0);
			crayon_gui->game_state->space2 = physics_create_space(1);
		}

	} else if (mouse_y > window_height / 2) {
		crayon_gui->game_state = game_state_new();
		crayon_gui->game_state->space1 = physics_create_space (0);
		crayon_gui->game_state->space2 = physics_create_space (0);
		core_load_level (crayon_gui->game_state, crayon_gui->game_state->level);
		crayon_gui->game_state->in_progress = 1;
		crayon_gui->game_state->orig_area = calculate_area(crayon_gui->game_state);
	}
}

static void add_opponent_display (Gui *crayon_gui) {
	GtkWidget *original_da = gtk_grid_get_child_at (GTK_GRID (crayon_gui->drawing_grid), 0, 0);

	GtkWidget *da = gtk_drawing_area_new();
	gtk_widget_set_size_request (GTK_WIDGET (da), gtk_widget_get_allocated_width(original_da), gtk_widget_get_allocated_height(original_da));

	gtk_widget_set_hexpand(da, TRUE);
	gtk_widget_set_vexpand(da, TRUE);

	gtk_aspect_frame_set(GTK_ASPECT_FRAME (crayon_gui->aspect_frame), 0, 0, 2, FALSE);

	gtk_grid_attach (GTK_GRID (crayon_gui->drawing_grid), GTK_WIDGET (da), 1, 0, 1, 1);

	g_signal_connect(GTK_WIDGET (da), "draw", G_CALLBACK(draw_cb), crayon_gui);
	g_signal_connect(GTK_WIDGET (original_da), "draw", G_CALLBACK(draw_opponent_cb), crayon_gui);

	gtk_widget_show_all (GTK_WIDGET (crayon_gui->drawing_grid));
}

static void remove_opponent_display (Gui *crayon_gui, GtkWidget *main_window, GtkWidget *drawing_grid) {
	gtk_widget_destroy (gtk_grid_get_child_at (GTK_GRID (drawing_grid), 1, 0));

	GtkWidget *main_da = gtk_grid_get_child_at (GTK_GRID (drawing_grid), 0, 0);

	gtk_aspect_frame_set(GTK_ASPECT_FRAME (crayon_gui->aspect_frame), 0, 0, 1, FALSE);

	gtk_window_resize (GTK_WINDOW (main_window), gtk_widget_get_allocated_width(main_da), gtk_widget_get_allocated_height(main_da));
	gtk_widget_show_all (GTK_WIDGET (drawing_grid));
}

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a GdkEventMotion struct which contains this information.
 */
 static gboolean motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event, Gui *crayon_gui) {

  /* paranoia check, in case we haven't gotten a configure event */
 	if (crayon_gui->graph_state->surface == NULL) {
 		return FALSE;
 	}

 	double window_height = gtk_widget_get_allocated_height(widget);

 	if (crayon_gui->game_state == NULL) {
 		if (event->y > 3 * window_height / 4) {
 			crayon_gui->graph_state->button_hover = 2;
 		} else if (event->y > window_height / 2) {
 			crayon_gui->graph_state->button_hover = 1;
 		} else {
 			crayon_gui->graph_state->button_hover = 0;
 		}

 		return FALSE;
 	}

  /*NEEDS TO BE INCLUDED*/
  // if (event->state & GDK_BUTTON1_MASK) {
  //   crayon_gui->client_data->vertices[2] = event->x;
  //   crayon_gui->client_data->vertices[3] = event->y;
  //   crayon_gui->graph_state->drawing = TRUE;
  // }

 	if (crayon_gui->game_state->in_progress) {

 		if ((event->state & GDK_BUTTON1_MASK) && crayon_gui->game_state->slices > 0) {
 			crayon_gui->graph_state->fx = event->x;
 			crayon_gui->graph_state->fy = event->y;
 			crayon_gui->graph_state->drawing = TRUE;
 		}
 	}

  /* We've handled it, stop processing */
 	return TRUE;
 }


// static gboolean cb_open_file (GtkMenuShell *ms, Gui *crayon_gui){
//   GtkWidget *file_chooser = gtk_file_chooser_dialog_new ("Open File", GTK_WINDOW(crayon_gui->main_window), 
//                                               GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, 
//                                               GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);   

//   char *file_name;

//   gint choose_option = gtk_dialog_run (GTK_DIALOG (file_chooser));

//   if (choose_option == GTK_RESPONSE_CANCEL) {
//     #ifdef DEBUG
//       g_print ("Cancel accepted.\n");
//     #endif
//     gtk_widget_destroy (file_chooser);
//     return TRUE;
//   }
//   #ifdef DEBUG
//     g_print ("GTK_RESPONSE_ACCEPTED.\n");
//   #endif

//   file_name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
//   gtk_widget_destroy (file_chooser);
//   core_create_level (crayon_gui->game_state, file_name);
//   g_free (file_name);
//   return TRUE;
// }

//Handles button releases
 static gboolean button_release_event_cb(GtkWidget *widget, GdkEventButton *event, Gui *crayon_gui) {  

 	if (crayon_gui->game_state == NULL) {
 		return FALSE;
 	}

 	if (crayon_gui->game_state->in_progress && crayon_gui->graph_state->drawing) {

 		double ix = crayon_gui->graph_state->ix;
 		double iy = crayon_gui->graph_state->iy;

    //update_coords (crayon_gui->p_data, event->x, event->y);
 		double hscaler = crayon_gui->graph_state->hscaler;
 		double wscaler = crayon_gui->graph_state->wscaler;

 		if (event->x - ix != 0.0f && event->y - iy != 0.0f && crayon_gui->game_state->slices > 0) {
 			if (crayon_gui->game_state->in_progress == 1) {
 				int valid_slice = game_state_slice(crayon_gui->game_state->space1, ix / wscaler, (gtk_widget_get_allocated_height(widget) - iy) / hscaler, event->x/wscaler, (gtk_widget_get_allocated_height(widget) - event->y)/hscaler);
 				if (valid_slice)
 					crayon_gui->game_state->slices -= 1;

 			} else if (crayon_gui->game_state->slices > 0) {
 				crayon_gui->client_data->send_waiting = 1;
 				core_generate_slice(crayon_gui->client_data->command, ix / wscaler, (gtk_widget_get_allocated_height(widget) - iy) / hscaler, event->x/wscaler, (gtk_widget_get_allocated_height(widget) - event->y)/hscaler);
 			}
 		}
 		pthread_mutex_unlock(&crayon_gui->drag_lock);

 		crayon_gui->graph_state->drawing = FALSE;
 	}

  /* We've handled the event, stop processing */
 	return TRUE;
 }

/* Create a new surface of the appropriate size to store our scribbles */
 static gboolean configure_event_cb(GtkWidget *widget, GdkEventConfigure *event, Gui *crayon_gui) {

 	crayon_gui->graph_state->surface = gdk_window_create_similar_surface(gtk_widget_get_window(widget),
 		CAIRO_CONTENT_COLOR, gtk_widget_get_allocated_width(widget),
 		gtk_widget_get_allocated_height(widget));

 	if (crayon_gui->graph_state->surface != NULL) {
 		clear_surface (crayon_gui->graph_state);
 	}

 	double meter_scale = crayon_gui->graph_state->meter_scale;
 	crayon_gui->graph_state->hscaler = gtk_widget_get_allocated_height(widget) / meter_scale;
 	crayon_gui->graph_state->wscaler = gtk_widget_get_allocated_width(widget) / meter_scale;

	/* We've handled the configure event, no need for further processing. */
 	return TRUE;
 }


// Declare a new gui struct
 Gui *gui_new (pthread_mutex_t lock1, pthread_mutex_t lock2){
 	Gui *crayon_gui = (Gui *) malloc (sizeof(Gui));
  /* Create the two windows */

 	GtkWidget *main_window;
 	GtkWidget *box;

 	GtkWidget *da;
 	GtkWidget *drawing_grid;
 	GtkWidget *drawing_aspect_frame;

 	main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
 	gtk_window_set_title (GTK_WINDOW (main_window), "SVN Comet");
 	gtk_window_set_default_size (GTK_WINDOW (main_window), 500, 500);

  /* Add the box */
 	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
 	gtk_container_add (GTK_CONTAINER (main_window), box);

 	drawing_aspect_frame = gtk_aspect_frame_new(NULL, 0, 0, 1, FALSE);

 	da = gtk_drawing_area_new ();
 	gtk_widget_set_size_request (da, 500, 500);

 	drawing_grid = gtk_grid_new();
 	gtk_widget_set_hexpand(da, TRUE);
 	gtk_widget_set_vexpand(da, TRUE);
 	gtk_grid_attach (GTK_GRID (drawing_grid), da, 0, 0, 1, 1);

 	gtk_container_add (GTK_CONTAINER (drawing_aspect_frame), drawing_grid);

 	gtk_box_pack_start (GTK_BOX (box), drawing_aspect_frame, TRUE, TRUE, 0);

  /* Assign to the gui and return it */
 	crayon_gui->main_window = main_window;
 	crayon_gui->box = box;
 	crayon_gui->game_state = NULL;
 	crayon_gui->aspect_frame = drawing_aspect_frame;
 	crayon_gui->graph_state = graph_state_new (da, (double) METER_SCALE);
 	crayon_gui->client_data = NULL;
 	crayon_gui->drag_lock = lock1;
 	crayon_gui->draw_lock = lock2;
 	crayon_gui->drawing_grid = drawing_grid;

 	return crayon_gui;
 }

/* Function that associates all of the various widgets with the correct callbacks */
 void gui_attach_callbacks (Gui *crayon_gui) {
 	GtkWidget *da = crayon_gui->graph_state->da;

 	g_signal_connect (GTK_WINDOW (crayon_gui->main_window), "destroy", G_CALLBACK (gtk_main_quit), crayon_gui);
 	g_signal_connect(crayon_gui->graph_state->da, "draw", G_CALLBACK(draw_cb), crayon_gui);
 	g_signal_connect(crayon_gui->graph_state->da, "configure-event", G_CALLBACK(configure_event_cb), crayon_gui);
 	g_signal_connect(crayon_gui->graph_state->da, "button-press-event", G_CALLBACK(button_press_event_cb), crayon_gui);
 	g_signal_connect(crayon_gui->graph_state->da, "motion-notify-event", G_CALLBACK(motion_notify_event_cb), crayon_gui);
 	g_signal_connect(crayon_gui->graph_state->da, "button-release-event", G_CALLBACK(button_release_event_cb), crayon_gui);

 	gtk_widget_set_events(da, gtk_widget_get_events(da) | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
 }

 void gui_destroy (Gui *crayon_gui) {
 	free (crayon_gui->game_state);
 }


 int main (int argc, char **argv) {
 	Gui *crayon_gui;

 	gtk_init (&argc, &argv);

 	pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
 	pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

 	crayon_gui = gui_new (lock1, lock2);

  // Create a thread, associate it with a wrapper function and pass it the necessary callback 
 	pthread_t thread;
 	int iret1 = pthread_create (&thread, NULL, game_thread, crayon_gui);

 	crayon_gui->graph_state->chipmunk = cairo_image_surface_create_from_png ("Images/chipmunk.png");
 	crayon_gui->graph_state->title = cairo_image_surface_create_from_png("Images/title.png");
 	crayon_gui->graph_state->single_player = cairo_image_surface_create_from_png("Images/singleplayer.png");
 	crayon_gui->graph_state->multi = cairo_image_surface_create_from_png("Images/multiplayer.png");

 	crayon_gui->graph_state->background = cairo_image_surface_create_from_png("Images/Background.png");
 	crayon_gui->graph_state->rocket = cairo_image_surface_create_from_png("Images/Rocket.png");
 	crayon_gui->graph_state->asteroid = cairo_image_surface_create_from_png("Images/Asteroid_Surface.png");
 	crayon_gui->graph_state->winner = cairo_image_surface_create_from_png("Images/Win.png");
 	crayon_gui->graph_state->loser = cairo_image_surface_create_from_png("Images/Lose.png");
 	crayon_gui->graph_state->loading = cairo_image_surface_create_from_png("Images/loading.png");

 	crayon_gui->graph_state->timer_zero = cairo_image_surface_create_from_png("Images/0.png");
 	crayon_gui->graph_state->timer_one = cairo_image_surface_create_from_png("Images/1.png");
 	crayon_gui->graph_state->timer_two = cairo_image_surface_create_from_png("Images/2.png");
 	crayon_gui->graph_state->timer_three = cairo_image_surface_create_from_png("Images/3.png");
 	crayon_gui->graph_state->timer_four = cairo_image_surface_create_from_png("Images/4.png");
 	crayon_gui->graph_state->timer_five = cairo_image_surface_create_from_png("Images/5.png");
 	crayon_gui->graph_state->timer_six = cairo_image_surface_create_from_png("Images/6.png");
 	crayon_gui->graph_state->timer_seven = cairo_image_surface_create_from_png("Images/7.png");
 	crayon_gui->graph_state->timer_eight = cairo_image_surface_create_from_png("Images/8.png");
 	crayon_gui->graph_state->timer_nine = cairo_image_surface_create_from_png("Images/9.png");
 	crayon_gui->graph_state->timer_ten = cairo_image_surface_create_from_png("Images/10.png");



 	gui_attach_callbacks(crayon_gui);

 	gtk_widget_show_all(crayon_gui->main_window);

  /* Show the appropriate widgets */

 	gtk_main();
 }



