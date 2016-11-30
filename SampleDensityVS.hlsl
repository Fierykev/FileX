#include <ProceduralConstantsH.hlsl>

static const float TEX_Y = 257;
static const float SAMP_EXPANSION = 5.f;
static const float SAMP_EXPANSION_P1 = SAMP_EXPANSION + 1;
static const float Y_EXPANSION = SAMP_EXPANSION_P1 / TEX_Y;
static const float Z_EXPANSION = SAMP_EXPANSION_P1 / 2.f;

struct VS_INPUT
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
	uint instanceID : SV_InstanceID;
};

struct VS_OUTPUT
{
	float4 position : POSITION;
	float4 worldPosition : TEXCOORD;
	uint instanceID : SV_InstanceID;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = float4(
		input.position.xy, 0, 1
		);
	
	output.worldPosition =
		float4(
			voxelPos.xyz +
			float3(
				float2(input.texcoord.x, input.texcoord.y - .5) * Y_EXPANSION,
				input.instanceID * Z_EXPANSION)
			, 1);

	output.instanceID = input.instanceID;

	return output;
}