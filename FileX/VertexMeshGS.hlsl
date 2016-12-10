#include <Debug.hlsl>

struct GS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 texcoord : TEXCOORD;
};

struct GS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 texcoord : TEXCOORD;
};

[maxvertexcount(1)]
void main(
	point GS_INPUT input[1],
	inout PointStream<GS_OUTPUT> output
)
{
	GS_OUTPUT element;
	element.position = input[0].position;
	element.normal = input[0].normal;
	element.texcoord = input[0].texcoord;
	output.Append(element);

	output.RestartStrip();
}