struct VS_INPUT
{
	float4 position : POSITION;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
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

	return output;
}