#include <ProceduralConstantsH.hlsl>

struct VS_INPUT
{
	uint bitPos : BITPOS;
	uint vertID : SV_VERTEXID;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	uint vertID : VERTEXID; // strip of ID status
	uint renderTarget : SV_RenderTargetArrayIndex;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	// get the position
	int3 position = getPos(input.bitPos);

	// get edge number
	uint edgeNum = input.bitPos & 0x0000000F;

	// expand x by factor of 3 since 0, 3, and 8 are
	// the possibilities
	position.x *= 3;

	// do not use if else so flow does not alter too much
	// cmove will probably be used by compiler to prevent divergence
	
	if (edgeNum == 3)
		position.x += 1;
	if (edgeNum == 8)
		position.x += 2;

	// store the z coord and vertex ID
	output.renderTarget = position.z;
	output.vertID = input.vertID;

	// set the output position
	output.position.x =
		(float)position.x * voxelInv / 3.0;
	output.position.y =
		(float)position.y * voxelInv;
	output.position.z = 0;

	// restrict to [-1, 1] range
	output.position = output.position * 2 - 1;

	output.position.w = 1;

	return output;
}