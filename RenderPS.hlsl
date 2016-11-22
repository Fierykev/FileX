#include "EdgesConstantsH.hlsl"
#include "ProceduralConstantsH.hlsl"
#include "Debug.hlsl"

struct PS_INPUT
{
	float4 position : SV_POSITION;
};

// TMP
Texture3D<float> densityTexture : register(t0);
SamplerState nearestSample : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
	// depth render
	float ratio;

	ratio = densityTexture.SampleLevel(nearestSample, float3(input.position.xy / 800.f, .4), 0).x;
	return float4(ratio, ratio, ratio, 1.0f);
	/*
	float3 position = float3(.5, .4, 0);
	// sample the texture where needed
	uint bitPos = 0;

	bitPos |= densityTexture.SampleLevel(nearestSample, position, 0).x > 0;
	bitPos |= (densityTexture.SampleLevel(nearestSample, position + float3(.0, .2, .0), 0).x > 0) << 1;
	/*
	bitPos |= densityTexture.SampleLevel(nearestSample, position + voxelInvMinP1.yyy, 0).x > 0;
	bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvMinP1.yxy, 0).x > 0) << 1;
	bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvMinP1.xxy, 0).x > 0) << 2;
	bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvMinP1.xyy, 0).x > 0) << 3;
	bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvMinP1.yyx, 0).x > 0) << 4;
	bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvMinP1.yxx, 0).x > 0) << 5;
	bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvMinP1.xxx, 0).x > 0) << 6;
	bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvMinP1.xyx, 0).x > 0) << 7;*/
/*
	if (0 < bitPos && bitPos < 255)
		return float4(0.f, 1.f, 0.f, 1.f);
	else
		return float4(1.f, 0.f, 0.f, 1.f);*/

#ifdef DEBUG
	return !debug[0] ? float4(1, 0, 0, 1) : float4(0, 1, 0, 1);
#endif
	//ratio = ratio <= 0 ? 1 : 0;
	return float4(ratio, ratio, ratio, 1.0f);

	//voxelPos.z != 10
	//ratio = ratio == 0.f ? 1.f : 0.f;
	//numberPolygons[18] != 2
	//return edgeNumber[3][0].y != 8 ? float4(1.f, 0.f, 0.f, 1.f) : float4(0.f, 1.f, 0.f, 1.f);// float4(ratio, ratio, ratio, 1.0f);
}