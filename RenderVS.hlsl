struct VS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

cbuffer WORLD_POS : register(b0)
{
	matrix worldViewProjection;
	matrix worldView;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = mul(input.position, worldViewProjection);
	output.normal = normalize(input.normal); // may need to fix

	return output;
}