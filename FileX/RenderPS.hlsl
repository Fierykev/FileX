#include "EdgesConstantsH.hlsl"
#include "ProceduralConstantsH.hlsl"
#include "AltitudeH.hlsl"
#include "MarbleH.hlsl"

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float ambient : AMBIENT;
	float3 texcoord : TEXCOORD;
	float3 normal : TEXCOORD1;
	float3 worldPos : TEXCOORD2;
};

SamplerState nearestSample : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
	// perturbation
	float3 off = float3(0, 0, 0);
	off += noise0.Sample(nearestSample, input.worldPos * 3.97).xyz;
	off += noise1.Sample(nearestSample, input.worldPos * 8.06).xyz * .5;
	off += noise2.Sample(nearestSample, input.worldPos * 15.96).xyz * .25;

	float3 normal = normalize(input.normal + off);

	float3 light = normalize(float3(0, -1000, 0) - input.position.xyz);

	float3 color = float3(.5, .5, .5);

	color = float3(0, 1, 0);//genBaseCol(input.worldPos, normal);

	//color = saturate(lerp(.8, input.ambient, .1) * 2.1 - .1);

	//color = genMarble(color, input.worldPos);

	float ambient = input.ambient;
	float3 lightColor = light * ambient * color;
	
	float3 diff = color.xyz * ambient;// max(dot(normal, light), 0.0);

	diff = clamp(diff, 0.0, 1.0);
	
	return float4(diff, 1);// float4(ambient, ambient, ambient, 1);
}