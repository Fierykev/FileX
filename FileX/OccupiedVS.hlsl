#include <ProceduralConstantsH.hlsl>
#include <SamplersH.hlsl>

struct VS_INPUT
{
	float2 position : POSITION;
	uint2 uv : UVPOSITION;
	uint instanceID : SV_InstanceID;
};

struct VS_OUTPUT
{
	uint bitPos : BITPOS;
};

Texture3D<float> densityTexture : register(t0);

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float3 position = float3(input.position,
		(input.instanceID + extra) * occInvVecM1.x);

	// for float error
	position.xyz += occInvVecM1.xxx * .5;

	// sample the texture where needed
	output.bitPos = 0;

	/*
	output.bitPos |= densityTexture.SampleLevel(nearestSample, position + occInvVecM1.yyy, 0).x > 0;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.yxy, 0).x > 0) << 1;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.xxy, 0).x > 0) << 2;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.xyy, 0).x > 0) << 3;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.yyx, 0).x > 0) << 4;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.yxx, 0).x > 0) << 5;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.xxx, 0).x > 0) << 6;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.xyx, 0).x > 0) << 7;
	*/

	// get the bits that compose bitPos
	// this is done instead of using | to allow for simd
	float4 bits[2];
	bits[0].x = densityTexture.SampleLevel(nearestClampSample, position + occInvVecM1.yyy, 0).x;
	bits[0].y = densityTexture.SampleLevel(nearestClampSample, position + occInvVecM1.yxy, 0).x;
	bits[0].z = densityTexture.SampleLevel(nearestClampSample, position + occInvVecM1.xxy, 0).x;
	bits[0].w = densityTexture.SampleLevel(nearestClampSample, position + occInvVecM1.xyy, 0).x;
	bits[1].x = densityTexture.SampleLevel(nearestClampSample, position + occInvVecM1.yyx, 0).x;
	bits[1].y = densityTexture.SampleLevel(nearestClampSample, position + occInvVecM1.yxx, 0).x;
	bits[1].z = densityTexture.SampleLevel(nearestClampSample, position + occInvVecM1.xxx, 0).x;
	bits[1].w = densityTexture.SampleLevel(nearestClampSample, position + occInvVecM1.xyx, 0).x;

	// use full registers to find the sign
	uint4 part1 = saturate(sign(bits[0]));
	uint4 part2 = saturate(sign(bits[1]));

	output.bitPos = part1.x | (part1.y << 1) | (part1.z << 2) | (part1.w << 3)
		| (part2.x << 4) | (part2.y << 5) | (part2.z << 6) | (part2.w << 7);

	uint3 coord = uint3(input.uv, input.instanceID);

	output.bitPos |=
		coord.x << 24
		| coord.y << 16
		| coord.z << 8;
	
	return output;
}