#ifndef EDGES_CONSTANTS_H
#define EDGES_CONSTANTS_H

cbuffer POLY_CONSTANTS : register(b1)
{
	int numberPolygons[256];
};

cbuffer EDGE_CONSTANTS : register(b2)
{
	int4 edgeNumber[256][5];
};

float3 edgeStartLoc[12] =
{
	float3(0, 0, 0),
	float3(0, 1, 0),
	float3(1, 1, 0),
	float3(1, 0, 0),
	float3(0, 0, 1),
	float3(0, 1, 1),
	float3(1, 1, 1),
	float3(1, 0, 1),
	float3(0, 0, 0),
	float3(0, 1, 0),
	float3(1, 1, 0),
	float3(1, 0, 0)
};
float3 edgeEndLoc[12] = 
{
	float3(0, 1, 0),
	float3(1, 1, 0),
	float3(1, 0, 0),
	float3(0, 0, 0),
	float3(0, 1, 1),
	float3(1, 1, 1),
	float3(1, 0, 1),
	float3(0, 0, 1),
	float3(0, 0, 1),
	float3(0, 1, 1),
	float3(1, 1, 1),
	float3(1, 0, 1)
};

float3 edgeDir[12] = 
{
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(0, -1, 0),
	float3(-1, 0, 0),
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(0, -1, 0),
	float3(-1, 0, 0),
	float3(0, 0, 1),
	float3(0, 0, 1),
	float3(0, 0, 1),
	float3(0, 0, 1)
};

uint edgeAlignment[12] =
{
	1, 0, 1, 0,
	1, 0, 1, 0,
	2, 2, 2, 2
};

#endif