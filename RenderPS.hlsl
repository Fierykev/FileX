#include "EdgesConstantsH.hlsl"
#include "ProceduralConstantsH.hlsl"
#include "Debug.hlsl"

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

// TMP
Texture3D<float> densityTexture : register(t0);
SamplerState nearestSample : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
#ifdef DEBUG
	return !debug[0] ? float4(1, 0, 0, 1) : float4(0, 1, 0, 1);
#endif
	//float3 ratio = float3(input.position.x, input.position.y, input.position.z);
	//ratio /= 2000.f;

	//return float4(ratio.x, ratio.y, ratio.z, 1.0f);

	float3 light = normalize(float3(0, 1000, 3) - input.position.xyz);

	float3 normal = input.normal;
	float4 color = float4(1, 1, 1, 1);

	float3 diff = color.xyz * max(dot(normal, light), 0.0);

	diff = clamp(diff, 0.0, 1.0);

	return float4(diff.xyz, color.w);
}