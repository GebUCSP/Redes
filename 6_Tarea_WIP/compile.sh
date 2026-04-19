#!/bin/bash
g++ main.cpp \
imgui/imgui.cpp \
imgui/imgui_draw.cpp \
imgui/imgui_widgets.cpp \
imgui/imgui_tables.cpp \
imgui/imgui_impl_glfw.cpp \
imgui/imgui_impl_opengl3.cpp \
-lglfw -lGL -ldl -o app.exe