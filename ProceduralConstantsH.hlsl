#ifndef PROCEDURAL_CONSTANTSH_H
#define PROCEDURAL_CONSTANTSH_H

cbuffer VOXEL_POS : register(b1)
{
	float4 voxelPos;
};

static const float chunkSize = 10.f;

static const float extra = 4.f;
static const float voxelExpansion = 33.f;
static const float voxelM2 = voxelExpansion - 2.f;
static const float voxelM1 = voxelExpansion - 1.f;
static const float voxelP1 = voxelExpansion + 1.f;

static const float2 voxelInv = float2(
	1.f / voxelExpansion, 0
	);
static const float2 voxelInvVecP1 = float2(
	1.f / voxelP1, 0
	);
static const float2 voxelInvVecM1 = float2(
	1.f / voxelM1, 0
	);

static const float2 voxelInvVecM2 = float2(
	1.f / voxelM2, 0
	);

static const float occExpansion = voxelExpansion + extra * 2.f;
static const float occM1 = occExpansion - 1.f;
static const float occM2 = occM1 - 1.f;
static const float occP1 = occExpansion + 1.f;

static const float2 occInv = float2(
	1.f / occExpansion, 0
	);
static const float2 occInvVecP1 = float2(
	1.f / occP1, 0
	);
static const float2 occInvVecM1 = float2(
	1.f / occM1, 0
	);

static const float2 occInvVecM2 = float2(
	1.f / occM2, 0
	);
	
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