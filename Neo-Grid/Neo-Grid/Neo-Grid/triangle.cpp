#include "triangle.h"

Triangle::Triangle(float t_vertices[], float t_colors[]) {
	instanceID = globalID++;
	for (size_t i = 0; i < 9; i++)
	{
		vertices[i] = t_vertices[i];
		vertexColors[i] = t_colors[i];
	}
}

int Triangle::getID() {
	return instanceID;
}

float* Triangle::getVertices() {
	return vertices;
}

int* Triangle::getIndices() {
	return indices;
}

float* Triangle::getColors()
{
	return vertexColors;
}