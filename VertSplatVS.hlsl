#include <ProceduralConstantsH.hlsl>

struct VS_INPUT
{
	uint bitPos : BITPOS; // TODO: change label
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
	uint edgeNum = input.bitPos & 0x0F;

	// expand x by factor of 3 since 0, 3, and 8 are
	// the possibilities
	position.x *= 3;

	// do not use if else so flow does not alter too much
	// cmove will probably be used by compiler to prevent divergence
	
	if (edgeNum == 0)
		position.x += 1;
	if (edgeNum == 8)
		position.x += 2;

	// store the z coord and vertex ID
	output.renderTarget = position.z;
	output.vertID = input.vertID;

	float2 uv = position.xy;
	
	// TODO: alignment fix

	// set the output position
	output.position.w = 1;
	output.position.x =
		((float)uv.x * voxelInv / 3.0) * 2.f - 1.f;
	output.position.y =
		((float)uv.y * voxelInv) * 2.f;
	output.position.z = 0;

	// restrict to [-1, 1] range
	output.position = output.position * 2 - 1;

	return output;
}