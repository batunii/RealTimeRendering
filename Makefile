file := glad.c main.cpp
headers := *hpp
shaders := vertex.glsl fragment.glsl

main: $(file) $(headers)
	clang++ $(file) -lglfw -lGL -lassimp -g -o main
