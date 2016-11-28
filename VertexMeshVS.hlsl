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

VS_OUTPUT locateVertFromEdge(float3 position, float sampleArea, uint edgeNum)
{
	float samplePT0 = densityTexture.SampleLevel(
		nearestSample,
		sampleArea + occInvVecM1.xxx * edgeStartLoc[edgeNum],
		0).x;

	float samplePT1 = densityTexture.SampleLevel(
		nearestSample,
		sampleArea + occInvVecM1.xxx * edgeEndLoc[edgeNum],
		0).x;

	// saturate is needed for div 0
	float ratio = saturate(samplePT0 / (samplePT0 - samplePT1));
	float3 relPos = edgeStartLoc[edgeNum] + edgeDir[edgeNum] * ratio;
	
	// calculate normal gradient
	float3 uvw = sampleArea + relPos * occInvVecM1.xxx;
	float3 gradient;
	gradient.x = densityTexture.SampleLevel(
		nearestSample, uvw + occInvVecP1.xyy, 0).x
		- densityTexture.SampleLevel(
			nearestSample, uvw - occInvVecP1.xyy, 0).x;
	gradient.y = densityTexture.SampleLevel(
		nearestSample, uvw + voxelInvVecP1.yxy, 0).x;
		- densityTexture.SampleLevel(
			nearestSample, uvw - occInvVecP1.yxy, 0).x;
	gradient.z = densityTexture.SampleLevel(
		nearestSample, uvw + occInvVecP1.yyx, 0).x
		- densityTexture.SampleLevel(
			nearestSample, uvw - occInvVecP1.yyx, 0).x;

	VS_OUTPUT vout;
	vout.position =
		float4(position + relPos * voxelInvVecM1.xxx, 1);
	vout.normal = 
		-normalize(gradient);

	return vout;
}

VS_OUTPUT main(VS_INPUT input)
{
	// get the position
	uint3 position = getPos(input.bitPos);

	float3 sampleArea =
		((float3)position + extra) / occInvVecM1.xxx;

	sampleArea += occInvVecM1.xxx * .25f;

	float3 worldPos = (float3)position * chunkSize
		+ (float3)position * voxelInvVecM1.xxx * chunkSize;
	
	// get the edgenum
	uint edgeNum = input.bitPos & 0x0F;

	// get the vertex from the edge num
	return locateVertFromEdge(worldPos, sampleArea, edgeNum);
}