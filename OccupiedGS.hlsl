#include <EdgesConstantsH.hlsl>
#include <Debug.hlsl>

struct GS_INPUT
{
	uint bitPos : BITPOS;
};

struct GS_OUTPUT
{
	uint bitPos : BITPOS;
};

[maxvertexcount(1)]
void main(
	point GS_INPUT input[1],
	inout PointStream<GS_OUTPUT> output
)
{
	GS_OUTPUT element;

	// filter out position portion
	uint edgesOnly = input[0].bitPos & 0x0FF;

	// filter out air blocks
	if (0 < edgesOnly && edgesOnly < 255)
	{
		debug[0] = true;
		element.bitPos = input[0].bitPos;
		output.Append(element);
	}

	output.RestartStrip();
}