#define HIGH_QUALITY

#include <DensityH.hlsl>
#include "Debug.hlsl"

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 worldPosition : TEXCOORD;
};

float main(PS_INPUT input) : SV_TARGET0
{
	return density(input.worldPosition.xyz);
}