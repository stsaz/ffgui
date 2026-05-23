# ffgui make config

# Input vars:
# OS

# Output vars:
# CFLAGS_GUI
# LINKFLAGS_GUI
# FFGUI_OBJ

ifeq "$(OS)" "windows"
	CFLAGS_GUI := -Wno-missing-field-initializers
	LINKFLAGS_GUI := -lshell32 -luxtheme -lcomctl32 -lcomdlg32 -lgdi32 -lole32 -luuid
	FFGUI_OBJ := \
		ffgui-winapi.o \
		ffgui-winapi-loader.o

else
	CFLAGS_GUI := -Wno-free-nonheap-object -Wno-deprecated-declarations \
		$(shell pkg-config --cflags gtk+-3.0)
	LINKFLAGS_GUI := $(shell pkg-config --libs gtk+-3.0) \
		-lm
	FFGUI_OBJ := \
		ffgui-gtk.o \
		ffgui-gtk-loader.o
endif
