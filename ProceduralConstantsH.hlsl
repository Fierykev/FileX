#ifndef PROCEDURAL_CONSTANTSH_H
#define PROCEDURAL_CONSTANTSH_H

cbuffer VOXEL_POS : register(b0)
{
	uint3 voxelPos;
};

static const float voxelExpansion = 32.f;

static const float voxelM1 = 31.f;

static const float voxelInv = 1 / 32.f;
static const float2 voxelInvVecP1 = float2(
	1 / 33.f, 0
	);
static const float2 voxelInvVecM1 = float2(
	1 / 32.f, 0
	);

inline float4 getVoxelLoc(float2 texcoord, uint instanceID)
{
	return float4(
		voxelPos + float3(texcoord, instanceID) * voxelExpansion, 1);
}

inline float3 getRelLoc(float2 texcoord, uint instanceID)
{
	return float3(texcoord, instanceID * voxelInv);
}

inline float3 getRelLocP1(float2 texcoord, uint instanceID)
{
	return float3(texcoord, instanceID * voxelInvVecP1.y);
}

inline float3 getRelLocM1(float2 texcoord, uint instanceID)
{
	return float3(texcoord, instanceID * voxelInvVecP1.y) + voxelInvVecM1.xxx;
}

inline uint3 getPos(uint bitPos)
{
	return int3(
			(bitPos >> 24) & 0x0FF,
			(bitPos >> 16) & 0x0FF,
			(bitPos >> 8) & 0x0FF
		);
}

#endif