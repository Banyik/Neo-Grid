#include "hexagon.h"
int Hexagon::globalID = 0;
Hexagon::Hexagon(float h_vertices[], float h_colors[]) {
	instanceID = globalID++;
	for (size_t i = 0; i < 9; i++)
	{
		vertices[i] = h_vertices[i];
		vertexColors[i] = h_colors[i];
	}
}

Hexagon::Hexagon(float h_vertices[], int h_indices[]) {
	instanceID = globalID++;
	for (size_t i = 0; i < 18; i++)
	{
		vertices[i] = h_vertices[i];
		vertexColors[i] = 0;
	}
	for (size_t i = 0; i < 12; i++)
	{
		indices[i] = h_indices[i];
	}
}

int Hexagon::getID() {
	return instanceID;
}

float* Hexagon::getVertices() {
	return vertices;
}

int* Hexagon::getIndices() {
	return indices;
}

float* Hexagon::getColors()
{
	return vertexColors;
}