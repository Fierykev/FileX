#ifndef _DENSITY_H_
#define _DENSITY_H_

#include <NoiseH.hlsl>

float density(float3 pos)
{
	float density = pos.y - .5f;// -.5f;// +pos.x;
	//density += pos.x / 100.f;
	//density += simplex3D(pos % 32);

	//density += pos.x % 5;

	//density = .2 < pos.x && pos.x < .9 && .2 < pos.y && pos.y < .9? density : 0;

	density += snoise(pos * 4.03) * .25;
	density += snoise(pos * 1.96) * .5;
	density += snoise(pos * 1.01);

	return density;

	//return pos.y - .5;//noise(8);
}

#endif