struct GS_INPUT
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
	uint instanceID : SV_InstanceID;
};

struct GS_OUTPUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD;
	uint instanceID : SV_RenderTargetArrayIndex;
};

[maxvertexcount(3)]
void main(
	triangle GS_INPUT input[3],
	inout TriangleStream<GS_OUTPUT> output
)
{
	GS_OUTPUT element;

	[unroll(3)]
	for (uint i = 0; i < 3; i++)
	{
		element.position = input[i].position;
		element.texcoord = input[i].texcoord;
		element.instanceID = input[i].instanceID;
		output.Append(element);
	}

	output.RestartStrip();
}