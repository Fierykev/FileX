#ifndef _DENSITY_H_
#define _DENSITY_H_

#include <SimplexH.hlsl>

float density(float3 pos)
{
	float density = -pos.y + .5;// -.5;// +pos.x;
	//density += pos.x / 100.f;
	density += simplex3D(pos);

	return density;

	//return pos.y - .5;//noise(8);
}

#endif