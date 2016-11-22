struct GS_INPUT
{
	float4 position : SV_POSITION;
};

struct GS_OUTPUT
{
	float4 position : SV_POSITION;
};

[maxvertexcount(1)]
void main(
	point GS_INPUT input[1],
	inout PointStream<GS_OUTPUT> output
)
{
	GS_OUTPUT element;
	element.position = input[0].position;
	output.Append(element);

	output.RestartStrip();
}