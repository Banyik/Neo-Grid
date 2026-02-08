#pragma once
class Hexagon {
private:
	static int globalID;
	int instanceID;
	float vertices[18];
	float vertexColors[9];
	int indices[12];
	int x;
	int y;
public:
	Hexagon(float h_vertices[], float h_colors[]);
	Hexagon(int h_x, int h_y, float h_colors[]);
	Hexagon(float h_vertices[], int h_indices[]);
	int getID();
	float* getVertices();
	int* getIndices();
	float* getColors();
	int getX();
	int getY();
};