#ifndef _DENSITY_H_
#define _DENSITY_H_

#include "NoiseH.hlsl"

float density(float3 pos)
{
	//pos.y *= -1;
	float density = pos.y -.5;
	
	//density += snoise(pos * 4.03) * .25;
	//density += snoise(pos * 1.96) * .5;
	//density += snoise(pos * 1.01);

	//density += (int)pos.z % 2;

	return density;

	//return pos.y - .5;//noise(8);
}

#endif