#ifndef _DENSITY_H_
#define _DENSITY_H_

#include <ProceduralConstantsH.hlsl>
#include <SampleTypes.hlsl>
#include "NoiseH.hlsl"

// low freq
Texture3D noise0 : register(t4);
Texture3D noise1 : register(t5);
Texture3D noise2 : register(t6);

// high freq
Texture3D noiseH0 : register(t7);
Texture3D noiseH1 : register(t8);
Texture3D noiseH2 : register(t9);

// shelf
static const float SHELF_THICK = 3.f;
static const float SHELF_UP = -1;
static const float SHELF_STRENGTH = 80.f;

float4 evalTexID(uint texID, float3 uvw)
{
	switch(texID)
	{
	case 0:
		return noise0.SampleLevel(repeatSampler, uvw, 0);
	case 1:
		return noise1.SampleLevel(repeatSampler, uvw, 0);
	case 2:
		return noise2.SampleLevel(repeatSampler, uvw, 0);
	case 3:
		return noiseH0.SampleLevel(repeatSampler, uvw, 0);
	case 4:
		return noiseH1.SampleLevel(repeatSampler, uvw, 0);
	case 5:
		return noiseH2.SampleLevel(repeatSampler, uvw, 0);
	}

	return float4(0, 0, 0, 0);
}

float density(float3 pos)
{
	// start with nothing
	float density = 0;
	
	// get random vals:

	// unsigned low freq
	float4 ulfVal0 = mediumUnsigned(0, pos * .000832);
	float4 ulfVal1 = mediumUnsigned(1, pos * .000742);
	float4 ulfVal2 = mediumUnsigned(2, pos * .000543);

	// signed high freq
	float3 shfVal = {
		highSigned(3, pos * .0123) * .66
			+ highSigned(4, pos * .0456) * .33,
		highSigned(3, pos * .0348) * .66
			+ highSigned(5, pos * .0144) * .33,
		highSigned(3, pos * .0255) * .66
			+ highSigned(5, pos * .00934) * .33	
	};

	density = -pos.y;// + ulfVal0.x * 16.f;
	
	return density;
}

#endif