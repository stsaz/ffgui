# FFGUI

Provides basic cross-platform graphical user interface (GUI) for C and C++ based on:

* GTK+
* Win32 API

The main feature is the UI loader - mechanism that builds UI at runtime from specially formatted text file containing definitions of UI elements, their initial properies and runtime action codes.
The back-end parser (`ffbase/conf.h`) can utilize SSE4.2 for ultra fast processing.

Other features:

* Auto hotkeys
* Multi-language ready
* GTK+: asynchronous UI messages
* GTK+: listview on-demand drawing emulation
* Win32 API: basic vertical-horizontal auto positioning of UI elements and resizing
* Win32 API: UTF-8
* Very thin C++ layer that's completely inlined by the compiler

Based on [ffsys](https://github.com/stsaz/ffsys) & [ffbase](https://github.com/stsaz/ffbase).
