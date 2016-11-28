#include <GenVertsH.hlsl>

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

	// only allow the first byte through
	uint reducedPos = input.bitPos & 0x0FF;

	// get the needed bits for comparison
	bool b0 = reducedPos & 0x1;
	bool b1 = (reducedPos >> 1) & 0x1;
	bool b3 = (reducedPos >> 3) & 0x1;
	bool b4 = (reducedPos >> 4) & 0x1;
	
	// remove the first byte
	reducedPos = input.bitPos & 0xFFFFFF00;

	// look for differing signs to see if another
	// cell will create the vertex
	if (b1 != b0)
		reducedPos |= EDGE0;

	if (b3 != b0)
		reducedPos |= EDGE3;

	if (b4 != b0)
		reducedPos |= EDGE8;

	output.bitPos = reducedPos;

	return output;
}