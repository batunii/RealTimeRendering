file := glad.c main.cpp
headers := *.hpp
shaders := vertex.glsl fragment.glsl
imgui := \
	../external/imgui/imgui.cpp \
	../external/imgui/imgui_draw.cpp \
	../external/imgui/imgui_widgets.cpp \
	../external/imgui/imgui_tables.cpp \
	../external/imgui/backends/imgui_impl_glfw.cpp \
	../external/imgui/backends/imgui_impl_opengl3.cpp

main: $(file) $(headers)
	clang++ $(file) -lglfw -lGL -lassimp -g -o main

imgui: $(files) $(headers)
	clang++ $(file) $(imgui) \
		-Iexternal/imgui -Iexternal/imgui/backends \
		-lglfw -lGL -lassimp -g -o main
