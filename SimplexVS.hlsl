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
	output.position = input.position;

	return output;
}