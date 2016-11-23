struct GS_INPUT
{
	float4 position : SV_POSITION;
	uint vertID : VERTEXID;
	uint renderTarget : SV_RenderTargetArrayIndex;
};

struct GS_OUTPUT
{
	float4 position : SV_POSITION;
	uint vertID : VERTEXID;
	uint renderTarget : SV_RenderTargetArrayIndex;
};

[maxvertexcount(1)]
void main(
	point GS_INPUT input[1] : SV_POSITION,
	inout PointStream<GS_OUTPUT> output
)
{
	GS_OUTPUT element;
	element.position = input[0].position;
	element.vertID = input[0].vertID;
	element.renderTarget = input[0].renderTarget;
	output.Append(element);
}