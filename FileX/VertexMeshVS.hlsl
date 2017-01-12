#include <ProceduralConstantsH.hlsl>
#include <EdgesConstantsH.hlsl>
#include <RaysH.hlsl>
#include <DensityH.hlsl>
#include <SamplersH.hlsl>
#include "Debug.hlsl"

#define NUM_STEPS 16
#define FAR_SAMPLES 6

struct VS_INPUT
{
	uint bitPos : BITPOS;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 texcoord : TEXCOORD;
};

Texture3D<float> densityTexture : register(t0);

float ambientOcclusion(float3 position)
{
	//return snoise(position);
	float vis = 0;

	const float skipCells = 1.53;

	for (uint i = 0; i < 32; i++)
	{
		float3 direction = ray32[i];
		float isVis = 1;

		float3 rays =
			position + direction * occInv.xxx * skipCells;
		float3 delta =
			direction * occExpansion.xxx / 180.f / NUM_STEPS;
		
		for (uint j = 1; j < NUM_STEPS; j++)
		{
			rays += delta;

			float den = densityTexture.SampleLevel(linearClampSample,
				rays, 0);

			isVis = lerp(isVis, 0, saturate(den * 7.f));
		}
		
		for (uint k = 0; k < FAR_SAMPLES; k++)
		{
			float range = 10.f * k + 1.f;
			range = pow(range, 2.1);
			range *= 45.f;

			float den = density(position + direction * range);
			isVis *=
				.08 + .92 * saturate(-den * .21 + 24);
		}
		
		vis += isVis;
	}

	return (1 - vis / 32.f);
}

VS_OUTPUT locateVertFromEdge(float3 position, float3 sampleArea, uint edgeNum)
{
	float samplePT0 = densityTexture.SampleLevel(
		nearestClampSample,
		sampleArea + occInvVecM1.xxx * edgeStartLoc[edgeNum],
		0).x;

	float samplePT1 = densityTexture.SampleLevel(
		nearestClampSample,
		sampleArea + occInvVecM1.xxx * edgeEndLoc[edgeNum],
		0).x;

	// saturate is needed for div 0
	float ratio = saturate(samplePT0 / (samplePT0 - samplePT1));
	float3 relPos = edgeStartLoc[edgeNum] + edgeDir[edgeNum] * ratio;
	
	// calculate normal gradient
	float3 uvw = sampleArea + relPos * voxelSize.xxx; // TODO: change occInvVecM1 to LOD
	float3 gradient;
	gradient.x = densityTexture.SampleLevel(
		linearClampSample, uvw + occInv.xyy, 0).x
		- densityTexture.SampleLevel(
			linearClampSample, uvw - occInv.xyy, 0).x;
	gradient.y = densityTexture.SampleLevel(
		linearClampSample, uvw + occInv.yxy, 0).x;
		- densityTexture.SampleLevel(
			linearClampSample, uvw - occInv.yxy, 0).x;
	gradient.z = densityTexture.SampleLevel(
		linearClampSample, uvw + occInv.yyx, 0).x
		- densityTexture.SampleLevel(
			linearClampSample, uvw - occInv.yyx, 0).x;

	VS_OUTPUT vout;
	vout.position =
		float4(position + relPos * voxelInvVecM1.xxx, 1);
	vout.position.w = ambientOcclusion(uvw);

	vout.normal = 
		-normalize(gradient);

	vout.texcoord = uvw;

	return vout;
}

VS_OUTPUT main(VS_INPUT input)
{
	// get the position
	uint3 position = getPos(input.bitPos);

	float3 sampleArea =
		((float3)position + extra) * occInvVecM1.xxx;

	sampleArea += occInvVecM1.xxx * .25f;

	float3 worldPos = voxelPos.xyz
		+ (float3)position * voxelInvVecM1.xxx * chunkSize;
	
	// get the edgenum
	uint edgeNum = input.bitPos & 0x0F;

	// get the vertex from the edge num
	return locateVertFromEdge(worldPos, sampleArea, edgeNum);
}