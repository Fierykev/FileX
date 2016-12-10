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

StructuredBuffer<uint> instance : register(t3);

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = float4(
		input.position.xy, 0, 1
		);
	output.texcoord = input.texcoord;
	output.instanceID = instance[0];

	return output;
}