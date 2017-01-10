#ifndef _DENSITY_H_
#define _DENSITY_H_

#include <ProceduralConstantsH.hlsl>
#include <SampleTypes.hlsl>
#include "FeaturesH.hlsl"

float density(float3 pos)
{
	// start with nothing
	float density = 0;
	
	// get random vals
	Random ran = getRandom(pos);

	// place the ground plain
	float densityGroundPlain = groundPlain(ran);

	// create shelves
	float densityShelf = shelf(ran);

	// create terraces
	float densityTerrace = terrace(ran);

	// create mountains (TODO: FIX)
	float densityMountain = 0.f;// mountain(ran);

	// high noise
	float densityHighNoise = highNoise(ran);

	density = densityGroundPlain;

	float comboDensity = 0.f;
	comboDensity = lerp(densityMountain,
		densityTerrace, ran.zone.x);
	comboDensity += lerp(comboDensity,
		densityHighNoise, ran.zone.y);

	density += comboDensity;
	//density = densityGroundPlain + densityHighNoise;

	return density; // TODO: UNCOMMENT
	return densityGroundPlain;
}

#endif