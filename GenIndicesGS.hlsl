#include <ProceduralConstantsH.hlsl>
#include <EdgesConstantsH.hlsl>

struct GS_INPUT
{
	uint bitPos : BITPOS;
};

struct GS_OUTPUT
{
	uint index : INDEX;
};

texture3D<uint> indexTex: register(t1);

[maxvertexcount(15)]
void main(
	point GS_INPUT input[1] : SV_POSITION,
	inout TriangleStream<GS_INPUT> output
)
{
	GS_OUTPUT element;

	// get the value to index into the lookup tables
	uint edgeIndex = input[0].bitPos & 0x000000FF;

	// get the position
	uint3 position = getPos(input[0].bitPos);

	// get the number of polygons
	uint numPolygons = numberPolygons[edgeIndex];

	// check that everything is whithin the cell
	// NOTE: this will most likely use cmove so no branch
	if (voxelM1 <= max(max(position.x, position.y), position.z))
		numPolygons = 0;

	// generate the indices for each poly
	for (uint i = 0; i < numPolygons; i++)
	{
		// get the edge number
		int3 triEdges = edgeNumber[edgeIndex][i];

		int3 edgeTMP;

		int3 triIndices;

		// load in the data
		[unroll(3)]
		for (uint i = 0; i < 3; i++)
		{
			// get the starting edge
			edgeTMP = position + (uint3)edgeStartLoc[triEdges[i]];

			// expand x
			edgeTMP.x = edgeTMP.x * 3 + edgeAlignment[triEdges[i]];

			// load and output the index
			element.index = indexTex.Load(int4(edgeTMP, 0)).x;

			// add to the strip
			output.Append(element);
		}

		// reset the strip
		output.RestartStrip();
	}
}