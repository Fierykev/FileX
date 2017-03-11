#ifndef _FEATURES_H_
#define _FEATURES_H_

#include "NoiseH.hlsl"

struct Random
{
	float4 ulfVal[3];
	float3 pos, posWarp;
	float3 zone;
	float3 rot[3];
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
	ran.ulfVal[0] = mediumUnsigned(0, pos * .0832);
	ran.ulfVal[1] = mediumUnsigned(1, pos * .0742);
	ran.ulfVal[2] = mediumUnsigned(2, pos * .0543);

	// signed high freq
	float3 shfVal = float3(
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
	ran.posWarp = pos + shfVal.xyz * warpStrength *
		ran.ulfVal[2].w;

	// store the zone
	ran.zone = mediumUnsigned(2, pos * .0000931).xyz;

	// store the rotated coords
	ran.rot[0] = rot(ran.posWarp, rotMatrix0);
	ran.rot[1] = rot(ran.posWarp, rotMatrix1);
	ran.rot[2] = rot(ran.posWarp, rotMatrix2);

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
	
	// get if need high level detail
#ifdef HIGH_QUALITY
	// add in 4 octaves of noise
	density += lowSigned(2, ran.posWarp * .123).x * .231;
	density += lowSigned(0, ran.posWarp * .897).x * .2993;
	density += lowSigned(1, ran.posWarp * .07234).x * 1.233;
	density += lowSigned(2, ran.posWarp * .0332).x * 2.344;
#endif
	
	// add in last 4 octaves of noise
	density += lowSigned(1, ran.posWarp * .02231).x * 4.34f;
	density += highSigned(0, ran.rot[0] * .001232).x * 5.22f;
	density += highSigned(2, ran.rot[0] * .00311).x * 6.12f;
	density += highSigned(1, ran.rot[0] * .003422).x * 7.34f;
	
	return density;
}

#endif