#include "ProceduralConstantsH.hlsl"

struct VS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 texcoord : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float ambient : AMBIENT;
	float3 texcoord : TEXCOORD;
	float3 normal : TEXCOORD1;
	float3 worldPos : TEXCOORD2;
};

cbuffer WORLD_POS : register(b0)
{
	matrix world;
	matrix worldView;
	matrix worldViewProjection;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.ambient = input.position.w;
	input.position.w = 1.f;
	output.position = mul(input.position, worldViewProjection);
	output.texcoord = input.texcoord;
	output.normal = input.normal;
	output.worldPos = input.position.xyz;

	return output;
}