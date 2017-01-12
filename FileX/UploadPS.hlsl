#include <SamplersH.hlsl>
#include "Debug.hlsl"

Texture2D<float4> uploadTex : register(t3);
SamplerState nearestSample : register(s0);

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	return uploadTex.Sample(nearestClampSample, input.uv);
}