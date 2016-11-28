struct VS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL_WRONG;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float ambient : AMBIENT;
	float3 normal : NORMAL_WRONG;
};

cbuffer WORLD_POS : register(b0)
{
	matrix worldViewProjection;
	matrix worldView;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.ambient = input.position.w;
	input.position.w = 1.f;
	output.position = mul(input.position, worldViewProjection);
	output.normal = normalize(input.normal); // may need to fix

	return output;
}