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