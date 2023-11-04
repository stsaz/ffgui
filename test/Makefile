# FFGUI tester

ROOT := ..
FFGUI := $(ROOT)/ffgui
FFSYS := $(ROOT)/ffsys
FFBASE := $(ROOT)/ffbase

include $(FFBASE)/conf.mk

CFLAGS += -MMD -MP
CFLAGS += -I$(FFGUI) -I$(FFSYS) -I$(FFBASE)

OBJ := test.o

ifeq "$(OS)" "windows"
	OBJ += \
		ffgui-winapi.o \
		ffgui-winapi-loader.o
	LINKFLAGS += -lshell32 -luxtheme -lcomctl32 -lcomdlg32 -lgdi32 -lole32 -luuid

else
	OBJ += \
		ffgui-gtk.o \
		ffgui-gtk-loader.o
	CFLAGS += -Wno-free-nonheap-object -Wno-deprecated-declarations \
		`pkg-config --cflags gtk+-3.0`
	LINKFLAGS += `pkg-config --libs gtk+-3.0` \
		$(LINK_PTHREAD) -lm
endif
CXXFLAGS := $(CFLAGS)

default: ffgui

-include $(wildcard *.d)

%.o: $(FFGUI)/test/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
%.o: $(FFGUI)/ffgui/gtk/%.c
	$(C) $(CFLAGS) $< -o $@
%.o: $(FFGUI)/ffgui/winapi/%.c
	$(C) $(CFLAGS) $< -o $@

ffgui: $(OBJ)
	$(LINK) $+ $(LINKFLAGS) -o $@
