#include "Debug.hlsl"

Texture2D<float4> uploadTex : register(t7);
SamplerState nearestSample : register(s0);

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	return uploadTex.Sample(nearestSample, input.uv);
}