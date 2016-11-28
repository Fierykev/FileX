#ifndef _DENSITY_H_
#define _DENSITY_H_

#include <SimplexH.hlsl>

float density(float3 pos)
{
	float density = -pos.y;// -.5;// +pos.x;
	//density += pos.x / 100.f;
	density += saturate(simplex3D(pos));

	//density += pos.x % 2;

	//density = .2 < pos.x && pos.x < .9 && .2 < pos.y && pos.y < .9? density : 0;

	return density;

	//return pos.y - .5;//noise(8);
}

#endif