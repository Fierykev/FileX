#ifndef PROCEDURAL_CONSTANTSH_H
#define PROCEDURAL_CONSTANTSH_H

cbuffer VOXEL_POS : register(b1)
{
	uint3 voxelPos;
};
/*
static const float extra = 4.f;
static const float voxelExpansion = 65.f;
static const float voxelM1 = 64.f;
static const float voxelP1 = 66.f;

static const float2 voxelInv = float2(
	1.f / 65.f, 0
	);
static const float2 voxelInvVecP1 = float2(
	1.f / 66.f, 0
	);
static const float2 voxelInvVecM1 = float2(
	1.f / 64.f, 0
	);

static const float occExpansion = 73.f;
static const float occM1 = 72.f;
static const float occP1 = 74.f;

static const float2 occInv = float2(
	1.f / 73.f, 0
	);
static const float2 occInvVecP1 = float2(
	1.f / 74.f, 0
	);
static const float2 occInvVecM1 = float2(
	1.f / 72.f, 0
	);*/

static const float chunkSize = 4.f;

static const float extra = 4.f;
static const float voxelExpansion = 33.f;
static const float voxelM1 = 32.f;
static const float voxelP1 = 34.f;

static const float2 voxelInv = float2(
	1.f / 33.f, 0
	);
static const float2 voxelInvVecP1 = float2(
	1.f / 34.f, 0
	);
static const float2 voxelInvVecM1 = float2(
	1.f / 32.f, 0
	);

static const float occExpansion = 41;
static const float occM1 = 40.f;
static const float occP1 = 42.f;

static const float2 occInv = float2(
	1.f / 41.f, 0
	);
static const float2 occInvVecP1 = float2(
	1.f / 42.f, 0
	);
static const float2 occInvVecM1 = float2(
	1.f / 40.f, 0
	);

inline float4 getVoxelLoc(float2 texcoord, uint instanceID)
{
	return float4(
		voxelPos + float3(texcoord, instanceID) * voxelExpansion, 1);
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