#include <GenVertsH.hlsl>
#include <Debug.hlsl>

struct GS_INPUT
{
	uint bitPos : BITPOS;
};

struct GS_OUTPUT
{
	uint bitPos : BITPOS;
};

[maxvertexcount(3)]
void main(
	point GS_INPUT input[1],
	inout PointStream<GS_OUTPUT> output
)
{
	GS_OUTPUT element;

	uint reducedPos = input[0].bitPos & 0xFFFFFF00;

	// only allow edges 0, 3, and 8 to pass through
	// other voxels will gen the data
	if (input[0].bitPos & EDGE0)
	{
#ifdef DEBUG
		debug[0] = true;
#endif
		element.bitPos = reducedPos | 0;
		output.Append(element);
	}

	if (input[0].bitPos & EDGE3)
	{
#ifdef DEBUG
		debug[0] = true;
#endif
		element.bitPos = reducedPos | 3;
		output.Append(element);
	}

	if (input[0].bitPos & EDGE8)
	{
#ifdef DEBUG
		debug[0] = true;
#endif
		element.bitPos = reducedPos | 8;
		output.Append(element);
	}

	output.RestartStrip();
}