COMPILERFLAGS = -Wall -g -std="c99"
CC = gcc

CFLAGS = $(COMPILERFLAGS)
CFLAGS += `pkg-config --cflags gtk+-3.0`

LIBRARIES = -lchipmunk -lm -lpthread
LIBRARIES += `pkg-config --libs gtk+-3.0`

LIBPATH = /usr/local/lib

SOBJS = core.o physics.o server.o
GOBJS = gui.o core.o graphics.o physics.o client.o sleeping.o
BINS = gui game server graphics core client physics
TBINS = test_gui test_server test_graphics test_core test_client test_physics

all: svn_comet server

svn_comet: $(GOBJS)
	$(CC) $(FRAMEWORK) $(CFLAGS) -o game $(GOBJS) -L$(LIBPATH) $(LIBRARIES)

server: $(SOBJS)
	$(CC) $(FRAMEWORK) $(CFLAGS) -o server $(SOBJS) -L$(LIBPATH) $(LIBRARIES)

core: physics.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -o test_core core.c physics.o -L$(LIBPATH) $(LIBRARIES) 

graphics: physics.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -o test_graphics graphics.c physics.o -L$(LIBPATH) $(LIBRARIES) 

physics: physics.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -o test_physics physics.c -L$(LIBPATH) $(LIBRARIES) 

gui: client.o physics.o graphics.o core.o sleeping.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -o test_gui gui.c client.o physics.o graphics.o core.o sleeping.o -L$(LIBPATH) $(LIBRARIES) -pthread

client: core.o physics.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -D_XOPEN_SOURCE -o test_client client.c core.o physics.o -L$(LIBPATH) -pthread

test_core: physics.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -D DEBUG -o test_core core.c physics.o -L$(LIBPATH) $(LIBRARIES) 

test_graphics: physics.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -D DEBUG -o test_graphics graphics.c physics.o -L$(LIBPATH) $(LIBRARIES) 

test_physics: physics.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -D DEBUG -o test_physics physics.c -L$(LIBPATH) $(LIBRARIES) 

test_gui: client.o physics.o graphics.o core.o sleeping.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -o test_gui gui.c client.o physics.o graphics.o core.o sleeping.o -L$(LIBPATH) $(LIBRARIES) -pthread

test_server: physics.o core.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -D DEBUG -o test_server server.c physics.o core.o -L$(LIBPATH) $(LIBRARIES) -pthread

test_client: core.o physics.o
	$(CC) $(FRAMEWORK) $(CFLAGS) -D DEBUG -D_XOPEN_SOURCE -o test_client client.c core.o physics.o -L$(LIBPATH) -pthread

clean:
	rm -f $(BINS) $(TBINS) $(SOBJS) $(GOBJS)
