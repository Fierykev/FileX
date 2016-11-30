#include <ProceduralConstantsH.hlsl>
#include <EdgesConstantsH.hlsl>
#include <PolyEdgeH.hlsl>
#include <Debug.hlsl>

struct GS_INPUT
{
	uint bitPos : BITPOS;
};

struct GS_OUTPUT
{
	uint index : BITPOS;
};

Texture3D<uint> indexTex: register(t1);

[maxvertexcount(15)]
void main(
	point GS_INPUT input[1],
	inout TriangleStream<GS_INPUT> output
)
{
	GS_OUTPUT element[3];
	bool valid[3];

	// get the value to index into the lookup tables
	uint edgeIndex = input[0].bitPos & 0x0FF;

	// get the position
	uint3 position = getPos(input[0].bitPos);

	// get the number of polygons
	uint numPolygons = numberPolygons[edgeIndex];

	// check that everything is whithin the cell
	// NOTE: this will most likely use cmove so no branch
	if ((uint)occM1 <= max(max(position.x, position.y), position.z))
		numPolygons = 0;

	// generate the indices for each poly
	for (uint i = 0; i < numPolygons; i++)
	{
		// get the edge number
		int3 triEdges = edgeNumber[edgeIndex * 5 + i].xyz;

		int3 edgeTMP;

		int3 triIndices;

		// load in the data
		[unroll(3)]
		for (uint i = 0; i < 3; i++)
		{
			// get the starting edge
			edgeTMP = position + edgeStartLoc[triEdges[i]].xyz;
			
			// check if runs off texture
			valid[i] = edgeTMP.x < voxelExpansion
				&& edgeTMP.y < voxelExpansion
				&& edgeTMP.z < voxelExpansion;

			// expand x
			edgeTMP.x = edgeTMP.x * 3 + edgeAlignment[triEdges[i]];
			
			// load and output the index
			element[i].index = indexTex.Load(int4(edgeTMP, 0)).x;
		}

		if (valid[0] &&
			valid[1] &&
			valid[2])
		{
			[unroll(3)]
			for (uint i = 0; i < 3; i++)
			{
				output.Append(element[i]);
			}
		}

		// reset the strip
		output.RestartStrip();
	}
}