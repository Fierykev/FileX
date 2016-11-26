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
		position + voxelInvVecP1.xxx * edgeStartLoc[edgeNum],
		0).x;

	float samplePT1 = densityTexture.SampleLevel(
		nearestSample,
		position + voxelInvVecP1.xxx * edgeEndLoc[edgeNum],
		0).x;

	// saturate is needed for div 0
	float ratio = saturate(samplePT0 / (samplePT0 - samplePT1));
	float relPos = edgeStartLoc[edgeNum] + edgeDir[edgeNum] * ratio;

	// TODO: add normals
	return position + relPos * voxelInvVecP1.xxx;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	// get the position
	uint3 position = getPos(input.bitPos);

	// get the edgenum
	uint edgeNum = position & 0x0F;

	// get the vertex from the edge num
	output.pos = float4(
		locateVertFromEdge(
		position, edgeNum), 1);

	return output;
}