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
	//output.position.y = -output.position.y;
	
	//if (output.position.z != 0)
		//output.position.y = -output.position.y;

	output.worldPosition = 
		float4(
			float3(input.texcoord.xy,
				input.instanceID * voxelInv.x)
			+ (float3)voxelPos
			, 1);

	output.instanceID = input.instanceID;

	return output;
}