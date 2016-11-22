#include <ProceduralConstantsH.hlsl>

struct VS_INPUT
{
	float2 position : POSITION;
	uint instanceID : SV_InstanceID;
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
	
	uint3 position = (uint3)getVoxelLoc(
		input.position, input.instanceID
	);

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
	
	output.bitPos |= position.x << 24
		| position.y << 16
		| position.z << 8;
	
	return output;
}