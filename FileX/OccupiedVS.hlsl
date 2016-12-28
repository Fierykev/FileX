#include <ProceduralConstantsH.hlsl>

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

SamplerState nearestSample : register(s0);

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	/*
	float3 position = float3(input.position,
		(input.instanceID + extra) * occInvVecM1.x);

	position.xy = float2(
		(input.uv + extra.xx)
		* occInvVecM1.xx
		);

	//position.xyz *= voxelExpansion * voxelInvVecM1.x;

	// for float error
	position.xyz += occInvVecM1.xxx * .125;

	//position.xyz *= (occM1.x * occInv.x).xxx;
	*/
	// sample the texture where needed
	output.bitPos = 0;

	uint2 tmpDelta = uint2(1, 0);
	uint3 tmpPos = uint3(input.uv, input.instanceID + extra);
	
	// TODO:TMP (will switch back)
	output.bitPos |= densityTexture.Load(int4(tmpPos + tmpDelta.yyy, 0)).x > 0;
	output.bitPos |= (densityTexture.Load(int4(tmpPos + tmpDelta.yxy, 0)).x > 0) << 1;
	output.bitPos |= (densityTexture.Load(int4(tmpPos + tmpDelta.xxy, 0)).x > 0) << 2;
	output.bitPos |= (densityTexture.Load(int4(tmpPos + tmpDelta.xyy, 0)).x > 0) << 3;
	output.bitPos |= (densityTexture.Load(int4(tmpPos + tmpDelta.yyx, 0)).x > 0) << 4;
	output.bitPos |= (densityTexture.Load(int4(tmpPos + tmpDelta.yxx, 0)).x > 0) << 5;
	output.bitPos |= (densityTexture.Load(int4(tmpPos + tmpDelta.xxx, 0)).x > 0) << 6;
	output.bitPos |= (densityTexture.Load(int4(tmpPos + tmpDelta.xyx, 0)).x > 0) << 7;

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
	uint3 coord = uint3(input.uv, input.instanceID);

	output.bitPos |=
		coord.x << 24
		| coord.y << 16
		| coord.z << 8;
	
	return output;
}