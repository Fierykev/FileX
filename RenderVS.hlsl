struct VS_INPUT
{
	float4 position : POSITION;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = input.position;
	float tmp = output.position.y;
	output.position.y = input.position.z;
	output.position.z = 0;// tmp;
	//output.position.x = 0;

	return output;
}