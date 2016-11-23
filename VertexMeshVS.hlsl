#include <ProceduralConstantsH.hlsl>
#include <EdgesConstantsH.hlsl>

struct VS_INPUT
{
	uint bitPos : BITPOS;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
};

Texture3D<float> densityTexture : register(t0);

SamplerState nearestSample : register(s0);

float3 locateVertFromEdge(float3 position, uint edgeNum)
{
	float samplePT0 = densityTexture.SampleLevel(
		nearestSample,
		position + voxelInv.xxx * edgeStartLoc[edgeNum],
		0).x;

	float samplePT1 = densityTexture.SampleLevel(
		nearestSample,
		position + voxelInv.xxx * edgeEndLoc[edgeNum],
		0).x;

	// saturate is needed for div 0
	float ratio = saturate(samplePT0 / (samplePT0 - samplePT1));
	float relPos = edgeStartLoc[edgeNum] + edgeDir[edgeNum] * ratio;

	// TODO: add normals
	return position + relPos * voxelInv;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	// get the position
	uint3 position = uint3(
		(input.bitPos & 0xFF000000) >> 24,
		(input.bitPos & 0x00FF0000) >> 16,
		(input.bitPos & 0x0000FF00) >> 8
		);

	// get the edgenum
	uint edgeNum = position & 0x0000000F;

	// get the vertex from the edge num
	output.pos = float4(
		locateVertFromEdge(
		position, edgeNum), 1);

	return output;
}