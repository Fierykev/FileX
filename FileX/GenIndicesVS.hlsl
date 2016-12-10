struct VS_INPUT
{
	uint bitPos : BITPOS;
};

struct VS_OUTPUT
{
	uint bitPos : BITPOS;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.bitPos = input.bitPos;

	return output;
}