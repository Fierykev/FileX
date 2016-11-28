#include "EdgesConstantsH.hlsl"
#include "ProceduralConstantsH.hlsl"
#include "Debug.hlsl"

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float ambient : AMBIENT;
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

	float3 light = normalize(float3(0, 1000, 0) - input.position.xyz);

	float3 normal = input.normal;
	float4 color = float4(1, 1, 1, 1);

	color = saturate(lerp(.5, input.ambient, .2) * 2.1 - .1);

	float3 diff = color.xyz * max(dot(normal, light), 0.0);

	diff = clamp(diff, 0.0, 1.0);
	return float4(diff, 1);
	return float4(abs(normal), 1);//float4(diff.xyz, 1);
}