#include "tnonagon.h"
int TNonagon::globalID = 0;
TNonagon::TNonagon(int tn_z, int tn_x, int tn_y, float h_colors[]) {
	instanceID = globalID++;
	x = tn_x;
	y = tn_y;
	z = tn_z;
	for (size_t i = 0; i < 9; i++)
	{
		n_vertexColors[i] = h_colors[i];
	}
}
int TNonagon::getID() {
	return instanceID;
}
float* TNonagon::getColors() {
	return n_vertexColors;
}
int TNonagon::getX() {
	return x;
}
int TNonagon::getY() {
	return y;
}
int TNonagon::getZ() {
	return z;
}