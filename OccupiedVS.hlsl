#include <ProceduralConstantsH.hlsl>

struct VS_INPUT
{
	float2 position : POSITION;
	uint instanceID : IID;
};

struct VS_OUTPUT
{
	uint bitPos : BITPOS;
};

Texture3D<float> densityTexture : register(t0);

SamplerState nearestSample : register(s0);

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	
	float3 position = getPosOffset(
		input.position, input.instanceID
	);

	// for float error
	position.xy += voxelInvVecP1.xx * .125;

	// sample the texture where needed
	output.bitPos = 0;
	output.bitPos |= densityTexture.SampleLevel(nearestSample, position + voxelInvVecP1.yyy, 0).x > 0;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvVecP1.yxy, 0).x > 0) << 1;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvVecP1.xxy, 0).x > 0) << 2;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvVecP1.xyy, 0).x > 0) << 3;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvVecP1.yyx, 0).x > 0) << 4;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvVecP1.yxx, 0).x > 0) << 5;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvVecP1.xxx, 0).x > 0) << 6;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + voxelInvVecP1.xyx, 0).x > 0) << 7;
	
	uint3 coord = uint3(input.position * voxelP1, input.instanceID);

	output.bitPos |= coord.x << 24
		| coord.y << 16
		| coord.z << 8;
	
	return output;
}