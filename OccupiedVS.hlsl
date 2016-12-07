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

	float3 position = float3(input.position,
		(input.instanceID + extra) * occInvVecM1.x);

	// for float error
	position += occInvVecM1.xxx * .125;
	//position.xy *= (occM1.x * occInv.x).xx;

	// sample the texture where needed
	output.bitPos = 0;
	
	output.bitPos |= densityTexture.SampleLevel(nearestSample, position + occInvVecM1.yyy, 0).x > 0;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.yxy, 0).x > 0) << 1;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.xxy, 0).x > 0) << 2;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.xyy, 0).x > 0) << 3;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.yyx, 0).x > 0) << 4;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.yxx, 0).x > 0) << 5;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.xxx, 0).x > 0) << 6;
	output.bitPos |= (densityTexture.SampleLevel(nearestSample, position + occInvVecM1.xyx, 0).x > 0) << 7;
	
	uint3 coord = uint3(input.uv, input.instanceID);

	output.bitPos |=
		coord.x << 24
		| coord.y << 16
		| coord.z << 8;
	
	return output;
}