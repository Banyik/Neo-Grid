#pragma once
class TNonagon {
private:
	static int globalID;
	int instanceID;
	float n_vertexColors[9];
	int z;
	int x;
	int y;
public:
	TNonagon(int tn_z, int tn_x, int tn_y, float h_colors[]);
	int getID();
	float* getColors();
	int getX();
	int getY();
	int getZ();
};