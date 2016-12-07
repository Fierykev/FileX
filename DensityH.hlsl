#ifndef _DENSITY_H_
#define _DENSITY_H_

#include <ProceduralConstantsH.hlsl>
#include "NoiseH.hlsl"

Texture3D<float4> noise0 : register(t3);
Texture3D<float4> noise1 : register(t4);
Texture3D<float4> noise2 : register(t5);

SamplerState repeateSampler : register(s1);

float sampleHQ(float3 uvw, Texture3D tex)
{

}

float density(float3 pos)
{
	
	pos /= chunkSize;
	
	//pos.y *= -1;
	float density = pos.y -.5;
	density += snoise(pos * 4.03) * .25;
	density += snoise(pos * 1.96) * .5;
	density += snoise(pos * 1.01);

	//float density = pos.y - .5;

	//density += noise0.SampleLevel(repeateSampler, pos * 4.03, 0).x * .25;
	//density += noise0.SampleLevel(repeateSampler, pos * 1.96, 0).x * .5;
	//density += noise0.SampleLevel(repeateSampler, pos * 1.01, 0).x;

	//density += pos.z % 2;

	return density;

	//return pos.y - .5;//noise(8);
}

#endif