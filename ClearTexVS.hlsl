#include <ProceduralConstantsH.hlsl>

struct VS_INPUT
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
	uint instanceID : IID;// : SV_InstanceID;
};

struct VS_OUTPUT
{
	float4 position : POSITION;
	float4 worldPosition : TEXCOORD;
	uint instanceID : IID;// : SV_InstanceID;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = float4(
		input.position.xy, 0, 1
		);

	output.worldPosition =
		float4(getRelLoc(
			input.texcoord, input.instanceID
		), 1);

	output.instanceID = input.instanceID;

	return output;
}