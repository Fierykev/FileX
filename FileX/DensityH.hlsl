#ifndef _DENSITY_H_
#define _DENSITY_H_

#include <ProceduralConstantsH.hlsl>
#include "NoiseH.hlsl"

Texture3D<float4> noise0 : register(t4);
Texture3D<float4> noise1 : register(t5);
Texture3D<float4> noise2 : register(t6);

SamplerState repeatSampler : register(s1);

static const float TEX_SIZE = 16.f;
static const float2 INV_TEX_SIZE = float2(1.f / TEX_SIZE, 0);

// shelf
static const float SHELF_THICK = 3.f;
static const float SHELF_UP = -1;
static const float SHELF_STRENGTH = 80.f;

float4 sampleType(int type, float3 pos)
{
	if (type == 0)
		return noise0.SampleLevel(repeatSampler, pos, 0);
	if (type == 1)
		return noise1.SampleLevel(repeatSampler, pos, 0);
	if (type == 2)
		return noise2.SampleLevel(repeatSampler, pos, 0);

	return float4(0, 0, 0, 0);
}

float sampleNoise(float3 uvw, int type, float soften = 1)
{
	//return snoise(uvw);
	float3 tmp1 = floor(uvw * TEX_SIZE) * INV_TEX_SIZE.x;
	float3 delta = (uvw - tmp1) * TEX_SIZE;
	delta = lerp(delta, delta * delta * (3.f - 2.f * delta), soften);

	float4 sample1 = sampleType(type, tmp1).zxyw;
	float4 sample2 = sampleType(type, tmp1 + INV_TEX_SIZE.xyy).zxyw;

	float4 lerp1 = lerp(sample1, sample2, delta.xxxx);
	float2 lerp2 = lerp(lerp1.xy, lerp1.zw, delta.yy);
	float lerp3 = lerp(lerp2.x, lerp2.y, delta.z);

	return lerp3;
}

float4 sampleNoiseMedium(float3 uvw, int type)
{
	float3 tmp1 = frac(uvw * TEX_SIZE + .5);
	float3 delta = (4 - 2 * tmp1) * tmp1 * tmp1;
	float3 sampleLoc = uvw + (delta - tmp1) / TEX_SIZE;

	return sampleType(type, sampleLoc);
}

float snap(float a, float b)
{
	float tmp = (.5 < a) ? 1 : 0;
	float tmp2 = 1.f - tmp * 2.f;

	return tmp * tmp2 * pow((tmp + tmp2 * a) * 2, b) * .5;
}

float shelfDensity(float3 pos)
{
	// scale down
	pos /= chunkSize;

	// grab some rand values
	float ran1 = saturate(sampleNoise(pos * .00834, 0) * 2.f - .5);
	float ran2 = sampleNoise(pos * .00742, 1);
	float ran3 = sampleNoise(pos * .00543, 2);

	float density = -pos.y;// +chunkSize - .5f;

						   // shelves
	density += lerp(
		density, SHELF_STRENGTH,
		.78 * saturate(SHELF_THICK - abs(pos.y - SHELF_UP))
		* saturate(ran2 * 1.5));

	density +=
		sampleNoiseMedium(pos.xyz, 0).x;

	density += ran3.x * 5.f;

	density *= 3.f;

	return density;
}

float littleBigPlanet(float3 pos)
{
	// scale down
	pos /= chunkSize;

	// grab some rand values
	float ran1 = saturate(sampleNoise(pos * .00834, 0) * 2.f - .5);
	float ran2 = sampleNoise(pos * .00742, 1);
	float ran3 = sampleNoise(pos * .00543, 2);

	float3 medRan1 = sampleNoiseMedium(pos * .02303, 0);

	float density = -pos.y;

	float3 rpos = 
		pos + float3(ran1, ran2, ran3) * 25.f * saturate(medRan1.y * 1.4 - .3);
	float rad = length(pos);
	float rrad = length(rpos);

	float combo = -lerp(rad, rrad, .1) * .2;
	float fCombo = frac(combo);
	float snapCombo = snap(fCombo, 16);

	density += (snapCombo - fCombo) * 10.f;

	return density;
}

float twistedWorld(float3 pos)
{
	// scale down
	pos /= chunkSize;

	// grab some rand values
	float ran1 = saturate(sampleNoise(pos * .00834, 0) * 2.f - .5);
	float ran2 = sampleNoise(pos * .00742, 1);
	float ran3 = sampleNoise(pos * .00543, 2);

	float density = -pos.y;

	density += ran1 * 25;
	density += ran2 * 5;
	density += ran3;

	density += snoise(pos);

	density *= 100.f;

	return density;
}

float floatingWorld(float3 pos)
{
	// scale down
	pos /= chunkSize;

	float ran1 = snoise(pos * 4.03).x * .25;
	float ran2 = snoise(pos * 1.96).x * .5;
	float ran3 = snoise(pos * 1.01).x;

	float3 medRan1 = snoise(pos * .02303);

	float density = -pos.y;

	float3 rpos =
		pos + float3(ran1, ran2, ran3) * 25.f * saturate(medRan1.y * 1.4 - .3);
	float rad = length(pos);
	float rrad = length(rpos);

	float combo = -lerp(rad, rrad, .1) * .2;
	float fCombo = frac(combo);
	float snapCombo = snap(fCombo, 16);

	density += (snapCombo - fCombo) * 10.f;

	return density;
}

float density(float3 pos)
{
	if (densityType == 1)
		return littleBigPlanet(pos);
	else if (densityType == 2)
		return twistedWorld(pos);
	else if (densityType == 3)
		return floatingWorld(pos);

	return shelfDensity(pos);
}


// ridges
//density +=
//sampleNoise(pos.xyz * float3(2.f, 32.f, 2.f) * .043, 0) * 2.f;


#endif