#pragma once

class Triangle {
private:
	static int globalID;
	int instanceID;
	float vertices[9];
	float vertexColors[9];
	int indices[3];
public:
	Triangle(float t_vertices[], float t_colors[]);
	int getID();
	float* getVertices();
	int* getIndices();
	float* getColors();
};
