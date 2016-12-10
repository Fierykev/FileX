#include <ProceduralConstantsH.hlsl>

static const float TEX_Y = 257;
static const float SAMP_EXPANSION = voxelInv * chunkSize;
static const float Y_EXPANSION = TEX_Y * SAMP_EXPANSION;
static const float Z_EXPANSION = Y_EXPANSION;

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

	float3 tmpPos = float3(voxelPos.x, -voxelPos.y, voxelPos.z);

	output.worldPosition =
		float4(
			tmpPos +
			float3(
				float2(input.texcoord.x, input.texcoord.y - .5) * Y_EXPANSION / 2.f,
				input.instanceID * Z_EXPANSION)
			, 1);

	output.instanceID = input.instanceID;

	return output;
}