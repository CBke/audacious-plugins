PLUGIN =gl-spectrum${PLUGIN_SUFFIX}

SRCS = gl-spectrum.c

include ../../buildsys.mk
include ../../extra.mk

plugindir := ${plugindir}/${VISUALIZATION_PLUGIN_DIR}

CFLAGS += ${PLUGIN_CFLAGS} -Wall
CPPFLAGS += ${PLUGIN_CPPFLAGS} -I../.. ${GTK_CFLAGS} `pkg-config --cflags  gtkglext-1.0`
LIBS += -lm ${GTK_LIBS} -lGL -lglut -lSDL -lGLU `pkg-config --libs gtkglext-1.0`
