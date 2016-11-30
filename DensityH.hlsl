#ifndef _DENSITY_H_
#define _DENSITY_H_

#include <ProceduralConstantsH.hlsl>
#include "NoiseH.hlsl"

float density(float3 pos)
{
	pos /= chunkSize;

	//pos.y *= -1;
	float density = pos.y -.5;
	//density += snoise(pos * 4.03) / 100.f;
	density += snoise(pos * 4.03) * .25;
	density += snoise(pos * 1.96) * .5;
	density += snoise(pos * 1.01);
/*
	density += snoise(pos * 34.01) * .32;
	density -= snoise(pos * 112.223) * .32;
	density -= snoise(pos * 13.34) * .32;

	density += snoise(pos * .89) * .9;
	density -= snoise(pos * .111) * 4.2;
	density += snoise(pos * .23) * 8.56;*/

	//density += pos.z % 2;

	return density;

	//return pos.y - .5;//noise(8);
}

#endif