all: sample2D

sample2D: Sample_GL3_2D.cpp glad.c
	g++ -o sample2D Sample_GL3_2D.cpp glad.c -ldl -lGL -lglfw

clean:
	rm sample2D sample3D
