#include "Debug.hlsl"

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 worldPosition : TEXCOORD;
};

float main(PS_INPUT input) : SV_TARGET0
{
#ifdef DEBUG
	//debug[0] = true;
#endif

	// eval the density function
	// TODO: add better density function
	return .5;// input.worldPosition.y;
}