#include <ProceduralConstantsH.hlsl>
#include <EdgesConstantsH.hlsl>

struct VS_INPUT
{
	uint bitPos : BITPOS;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

Texture3D<float> densityTexture : register(t0);

SamplerState nearestSample : register(s0);

VS_OUTPUT locateVertFromEdge(float3 position, uint edgeNum)
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
	float3 relPos = edgeStartLoc[edgeNum] + edgeDir[edgeNum] * ratio;
	
	// calculate normal gradient
	float3 uvw = position;// +relPos * voxelInvVecP1.xxx;
	float3 gradient = float3(0,0,0);
	gradient.x = densityTexture.SampleLevel(
		nearestSample, uvw + voxelInvVecP1.xyy, 0).x
		- densityTexture.SampleLevel(
			nearestSample, uvw - voxelInvVecP1.xyy, 0).x;
	gradient.y = densityTexture.SampleLevel(
		nearestSample, uvw + voxelInvVecP1.yxy, 0).x;
		- densityTexture.SampleLevel(
			nearestSample, uvw - voxelInvVecP1.yxy, 0).x;
	
	gradient.z = densityTexture.SampleLevel(
		nearestSample, uvw + voxelInvVecP1.yyx, 0).x
		- densityTexture.SampleLevel(
			nearestSample, uvw - voxelInvVecP1.yyx, 0).x;

	VS_OUTPUT vout;
	vout.position =
		float4(position + relPos * voxelInvVecP1.xxx + voxelPos * voxelM1, 1);
	vout.normal = 
		-normalize(gradient);

	return vout;
}

VS_OUTPUT main(VS_INPUT input)
{
	// get the position
	uint3 position = getPos(input.bitPos);
	
	// get the edgenum
	uint edgeNum = input.bitPos & 0x0F;

	// get the vertex from the edge num
	return locateVertFromEdge(position, edgeNum);
}