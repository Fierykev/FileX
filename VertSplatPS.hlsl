struct PS_INPUT
{
	float4 position : SV_POSITION;
	uint vertID : VERTEXID;
};

uint main(PS_INPUT input) : SV_TARGET0
{
	return input.vertID;
}