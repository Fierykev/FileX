#ifndef EDGES_CONSTANTS_H
#define EDGES_CONSTANTS_H

static const float3 edgeStartLoc[12] =
{
	float3(0, 0, 0),
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(0, 0, 0),
	float3(0, 0, 1),
	float3(0, 1, 1),
	float3(1, 0, 1),
	float3(0, 0, 1),
	float3(0, 0, 0),
	float3(0, 1, 0),
	float3(1, 1, 0),
	float3(1, 0, 0)
};

static const float3 edgeEndLoc[12] =
{
	float3(0, 1, 0),
	float3(1, 1, 0),
	float3(1, 1, 0),
	float3(1, 0, 0),
	float3(0, 1, 1),
	float3(1, 1, 1),
	float3(1, 1, 1),
	float3(1, 0, 1),
	float3(0, 0, 1),
	float3(0, 1, 1),
	float3(1, 1, 1),
	float3(1, 0, 1)
};

static const float3 edgeDir[12] =
{
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(0, 0, 1),
	float3(0, 0, 1),
	float3(0, 0, 1),
	float3(0, 0, 1)
};

static const uint edgeAlignment[12] =
{
	1, 0, 1, 0,
	1, 0, 1, 0,
	2, 2, 2, 2
};

#endif