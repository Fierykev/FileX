#include "Debug.hlsl"

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 worldPosition : TEXCOORD;
};

uint main(PS_INPUT input) : SV_TARGET0
{
	// clear the value
	return 0;// MAX_INT;
}