#include <ProceduralConstantsH.hlsl>

struct VS_INPUT
{
	float3 position : POSITION;
	float2 texcoord : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
	uint instanceID : SV_InstanceID;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = float4(
		input.position.xy, 0, 1
		);
	output.texcoord = float2(input.texcoord.x, 1.f - input.texcoord.y);
	output.instanceID = voxelPos.x;

	return output;
}