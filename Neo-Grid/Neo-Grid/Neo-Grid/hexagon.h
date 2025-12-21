#pragma once
class Hexagon {
private:
	static int globalID;
	int instanceID;
	float vertices[18];
	float vertexColors[18];
	int indices[12];
public:
	Hexagon(float h_vertices[], float h_colors[]);
	Hexagon(float h_vertices[], int h_indices[]);
	int getID();
	float* getVertices();
	int* getIndices();
	float* getColors();
};