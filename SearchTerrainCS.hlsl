#include <ProceduralConstantsH.hlsl>
#include <DensityH.hlsl>

#define NUM_THREADS 128
#define DATA_SIZE (NUM_THREADS * 2)

static const float FINDY_SIZE_P1 = NUM_THREADS + 1;

Texture3D<float> sampleDensityTexture : register(t2);

SamplerState nearestSample : register(s0);

RWStructuredBuffer<int> yPos : register(u0);

static const float3 offsetVec = float3(1.f / 2.f, 1.f / FINDY_SIZE_P1, 0.f);

groupshared uint reduction[DATA_SIZE];

[numthreads(NUM_THREADS, 1, 1)]
void main(uint threadID : SV_DispatchThreadID, uint groupThreadID : SV_GroupIndex, uint3 groupID : SV_GroupID)
{
	bool positive;

	// load in the data (run sampling as well)
	for (uint loadi = 0; loadi < 2; loadi++)
	{
		uint index = (groupThreadID << 1) + loadi;

		uint data = 0;

		float3 position = float3(0, offsetVec.y * index, 0);

		data |= sampleDensityTexture.SampleLevel(nearestSample, position + offsetVec.zzz, 0).x > 0;
		data |= (sampleDensityTexture.SampleLevel(nearestSample, position + offsetVec.zyz, 0).x > 0) << 1;
		data |= (sampleDensityTexture.SampleLevel(nearestSample, position + offsetVec.xyz, 0).x > 0) << 2;
		data |= (sampleDensityTexture.SampleLevel(nearestSample, position + offsetVec.xzz, 0).x > 0) << 3;
		data |= (sampleDensityTexture.SampleLevel(nearestSample, position + offsetVec.zzx, 0).x > 0) << 4;
		data |= (sampleDensityTexture.SampleLevel(nearestSample, position + offsetVec.zyx, 0).x > 0) << 5;
		data |= (sampleDensityTexture.SampleLevel(nearestSample, position + offsetVec.xyx, 0).x > 0) << 6;
		data |= (sampleDensityTexture.SampleLevel(nearestSample, position + offsetVec.xzx, 0).x > 0) << 7;

		// used for if no pos neg in texture
		positive = data & 0x1;

		// TODO: eliminate branching
		if (0 < data && data < 255)
			data = index;
		else
			data = 1 << 30;

		reduction[(groupThreadID << 1) + loadi] = data;
	}

	// sync the group
	GroupMemoryBarrierWithGroupSync();

	for (uint iRed = 1; iRed < DATA_SIZE; iRed <<= 1)
	{
		uint index = iRed * 2 * groupThreadID;

		if (index < DATA_SIZE)
			reduction[index] =
				abs((int)reduction[index] - NUM_THREADS)
					< abs((int)reduction[index + iRed] - NUM_THREADS) ?
						reduction[index] : reduction[index + iRed];

		// wait for the other threads in this group to finish
		GroupMemoryBarrierWithGroupSync();
	}

	if (groupThreadID == 0)
	{
		yPos[0] = reduction[0];

		if (reduction[0] == (1 << 30))
			yPos[0] |= (positive << 31);

		float3 tmp = voxelPosF;
		
		yPos[0] = density(tmp / chunkSize) * 100.f;
			//sampleDensityTexture.SampleLevel(nearestSample, float3(0, 0.f, 0), 0).x;
	}
}