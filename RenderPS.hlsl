#include "EdgesConstantsH.hlsl"
#include "ProceduralConstantsH.hlsl"
#include "MarbleH.hlsl"

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float ambient : AMBIENT;
	float3 texcoord : TEXCOORD;
	float3 normal : TEXCOORD1;
	float3 worldPos : TEXCOORD2;
};

// TMP
Texture3D<float> densityTexture : register(t0);
SamplerState nearestSample : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
	float3 light = normalize(float3(0, -1000, 0) - input.position.xyz);

	float3 normal = input.normal;
	float4 color = float4(.5, .5, .5, 1);

	color = saturate(lerp(.8, input.ambient, .1) * 2.1 - .1);

	color = float4(genMarble(color.xyz, input.worldPos), 1);
		
	float3 diff = color.xyz * max(dot(normal, light), 0.0);

	diff = clamp(diff, 0.0, 1.0);

	return float4(diff, 1);
}