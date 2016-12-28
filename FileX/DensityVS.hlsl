#include <ProceduralConstantsH.hlsl>

struct VS_INPUT
{
	float3 position : POSITION;
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

	float3 samplePos = float3(input.texcoord.xy,
		input.instanceID * occInv.x);

	samplePos *= voxelExpansion * voxelInvVecM1.x;
	//samplePos.xy *= occExpansion.x * occInvVecM1.x;

	samplePos = (samplePos * occExpansion.xxx - extra) * voxelInv.x;
	samplePos *= chunkSize;
	
	float3 tmpPos = float3(voxelPos.x, -voxelPos.y, voxelPos.z);
	samplePos += tmpPos;

	output.worldPosition = 
		float4(samplePos, 1);

	output.instanceID = input.instanceID;

	return output;
}