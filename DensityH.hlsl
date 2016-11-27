#ifndef _DENSITY_H_
#define _DENSITY_H_

float density(float3 pos)
{
	return pos.y - .5;//noise(8);
}

#endif