#ifndef PROCEDURAL_CONSTANTSH_H
#define PROCEDURAL_CONSTANTSH_H

cbuffer VOXEL_POS : register(b1)
{
	float3 voxelPos;
	uint densityType;
	uint renderType;
};

cbuffer GENERATION_CONSTANTS : register(b2)
{
	float chunkSize : packoffset(c0);
	float extra : packoffset(c0.y);
	float voxelExpansion : packoffset(c0.z);
	float voxelM1 : packoffset(c0.w);

	float voxelP1 : packoffset(c1);
	float occExpansion : packoffset(c1.y);
	float occM1 : packoffset(c1.z);
	float occP1 : packoffset(c1.w);

	float2 voxelInv : packoffset(c2);
	float2 voxelInvVecM1 : packoffset(c2.z);

	float2 voxelInvVecP1 : packoffset(c3);
	float2 occInv : packoffset(c3.z);

	float2 occInvVecM1 : packoffset(c4);
	float2 occInvVecP1 : packoffset(c4.z);
};

static const float densStep = 100.f;

inline float4 getVoxelLoc(float2 texcoord, uint instanceID)
{
	return float4(
		voxelPos.xyz + float3(texcoord, instanceID) * voxelExpansion, 1);
}

inline float3 getRelLoc(float2 texcoord, uint instanceID)
{
	return float3(texcoord, instanceID * voxelInv.x);
}

inline float3 getRelLocP1(float2 texcoord, uint instanceID)
{
	return float3(texcoord, instanceID * voxelInvVecP1.x);
}

inline float3 getRelLocM1(float2 texcoord, uint instanceID)
{
	return float3(texcoord, instanceID * voxelInvVecM1.x);
}

inline float3 getPosOffset(float2 texcoord, uint instanceID)
{
	return (texcoord * voxelM1 + extra, instanceID + extra) * occInvVecM1.x;
}

inline uint3 getPos(uint bitPos)
{
	return uint3(
		(bitPos & 0xFF000000) >> 24,
		(bitPos & 0x00FF0000) >> 16,
		(bitPos & 0x0000FF00) >> 8
	);
}

#endif