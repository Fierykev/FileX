#ifndef _FEATURES_H_
#define _FEATURES_H_

#include "NoiseH.hlsl"

// low freq
Texture3D noise0 : register(t4);
Texture3D noise1 : register(t5);
Texture3D noise2 : register(t6);

// high freq
Texture3D noiseH0 : register(t7);
Texture3D noiseH1 : register(t8);
Texture3D noiseH2 : register(t9);

struct Random
{
	float4 ulfVal[3];
	float3 shfVal;
	float3 pos, posWarp;
	float3 zone;
};

float4 evalTexID(uint texID, float3 uvw, SamplerState s)
{
	switch (texID)
	{
	case 0:
		return noise0.SampleLevel(s, uvw, 0);
	case 1:
		return noise1.SampleLevel(s, uvw, 0);
	case 2:
		return noise2.SampleLevel(s, uvw, 0);
	case 3:
		return noiseH0.SampleLevel(s, uvw, 0);
	case 4:
		return noiseH1.SampleLevel(s, uvw, 0);
	case 5:
		return noiseH2.SampleLevel(s, uvw, 0);
	}

	return float4(0, 0, 0, 0);
}

inline Random getRandom(float3 pos)
{
	Random ran;

	// unsigned low freq
	ran.ulfVal[0] = mediumUnsigned(0, pos * .00832);
	ran.ulfVal[1] = mediumUnsigned(1, pos * .00742);
	ran.ulfVal[2] = mediumUnsigned(2, pos * .00543);

	// signed high freq
	ran.shfVal = float3(
		highSigned(3, pos * .0123) * .66
		+ highSigned(4, pos * .0456) * .33,
		highSigned(4, pos * .0348) * .66
		+ highSigned(5, pos * .0144) * .33,
		highSigned(3, pos * .0255) * .66
		+ highSigned(5, pos * .00934) * .33
	);

	// store the position
	ran.pos = pos;

	// store the modified position
	const float warpStrength = 25.f;
	ran.posWarp = pos + ran.ulfVal[0].xyz * warpStrength *
		saturate(ran.ulfVal[2].w * 1.4f - .3f);

	// store the zone
	ran.zone = mediumUnsigned(2, pos * .0000931);

	return ran;
}

inline float groundPlain(Random ran)
{
	float density = 0;

	density += -ran.pos.y;

	return density;
}

inline float shelf(Random ran)
{
	float density = 0;

	// shelf
	const float shelfThickness = 3.f;
	float shelfUp = -1;
	float shelfStrength = 9.5f;

	// shelf
	density += lerp(
		density, shelfStrength,
		.9 * saturate(shelfThickness - abs(ran.posWarp.y - shelfUp))
		* saturate(ran.ulfVal[1].z * 10.5 - .5));

	return density;
}

inline float terrace(Random ran)
{
	float density = 0;

	const float warp = .5f * ran.ulfVal[0].x;
	const float yFreq = .13f;
	const float terraceStrength = 30.f * saturate(ran.ulfVal[1].y * 20.f - 1.f);
	const float overhangStrength = 10.f * saturate(ran.ulfVal[2].z * 20.f - 1.f);

	float yVal = -lerp(ran.pos.y, ran.posWarp.y, warp) * yFreq;
	float fracY = frac(yVal);
	float smoothFracY = snap(fracY, 16);
	float fY = floor(yVal) + smoothFracY;

	density += fY * terraceStrength;
	density += (smoothFracY - fracY) * overhangStrength;

	return density;
}

inline float mountain(Random ran)
{
	float density = 0;

	density += ran.ulfVal[1].w * 80.f;

	return density;
}

inline float highNoise(Random ran)
{
	float density = 0;

	[unroll(3)]
	for (uint i = 0; i < 3; i++)
		density += ran.shfVal[i];

	return density;
}

#endif