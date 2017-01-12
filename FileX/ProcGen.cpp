#include <time.h>

#include "ProcGen.h"
#include "Helper.h"
#include "Shader.h"
#include "Graphics.h"

ProcGen::VOXEL_POS* ProcGen::voxelPosData;

ProcGen::ProcGen()
{
	voxelViewport.Width = DENSITY_SIZE;
	voxelViewport.Height = DENSITY_SIZE;
	voxelViewport.MaxDepth = 1.0f;

	vertSplatViewport.Width = SPLAT_SIZE * 3;
	vertSplatViewport.Height = SPLAT_SIZE;
	vertSplatViewport.MaxDepth = 1.0f;

	voxelScissorRect.right = DENSITY_SIZE;
	voxelScissorRect.bottom = DENSITY_SIZE;

	vertSplatScissorRect.right = SPLAT_SIZE * 3;
	vertSplatScissorRect.bottom = SPLAT_SIZE;
}

void ProcGen::setupShaders()
{
	string dataDensityVS, dataDensityPS, dataDensityGS,
		dataOccupiedVS, dataOccupiedGS,
		dataGenVertsVS, dataGenVertsGS,
		dataVertexMeshVS, dataVertexMeshGS,
		dataClearTexVS, dataClearTexPS, dataClearTexGS,
		dataVertSplatVS, dataVertSplatPS, dataVertSplatGS,
		dataGenIndicesMeshVS, dataGenIndicesMeshGS;

#ifdef _DEBUG
	ThrowIfFailed(ReadCSO("../x64/Debug/DensityVS.cso", dataDensityVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/DensityPS.cso", dataDensityPS));
	ThrowIfFailed(ReadCSO("../x64/Debug/DensityGS.cso", dataDensityGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/OccupiedVS.cso", dataOccupiedVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/OccupiedGS.cso", dataOccupiedGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/GenVertsVS.cso", dataGenVertsVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/GenVertsGS.cso", dataGenVertsGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/VertexMeshVS.cso", dataVertexMeshVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/VertexMeshGS.cso", dataVertexMeshGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/ClearTexVS.cso", dataClearTexVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/ClearTexPS.cso", dataClearTexPS));
	ThrowIfFailed(ReadCSO("../x64/Debug/ClearTexGS.cso", dataClearTexGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/VertSplatVS.cso", dataVertSplatVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/VertSplatPS.cso", dataVertSplatPS));
	ThrowIfFailed(ReadCSO("../x64/Debug/VertSplatGS.cso", dataVertSplatGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/GenIndicesVS.cso", dataGenIndicesMeshVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/GenIndicesGS.cso", dataGenIndicesMeshGS));
#else
	ThrowIfFailed(ReadCSO("../x64/Release/DensityVS.cso", dataDensityVS));
	ThrowIfFailed(ReadCSO("../x64/Release/DensityPS.cso", dataDensityPS));
	ThrowIfFailed(ReadCSO("../x64/Release/DensityGS.cso", dataDensityGS));

	ThrowIfFailed(ReadCSO("../x64/Release/OccupiedVS.cso", dataOccupiedVS));
	ThrowIfFailed(ReadCSO("../x64/Release/OccupiedGS.cso", dataOccupiedGS));

	ThrowIfFailed(ReadCSO("../x64/Release/GenVertsVS.cso", dataGenVertsVS));
	ThrowIfFailed(ReadCSO("../x64/Release/GenVertsGS.cso", dataGenVertsGS));

	ThrowIfFailed(ReadCSO("../x64/Release/VertexMeshVS.cso", dataVertexMeshVS));
	ThrowIfFailed(ReadCSO("../x64/Release/VertexMeshGS.cso", dataVertexMeshGS));

	ThrowIfFailed(ReadCSO("../x64/Release/ClearTexVS.cso", dataClearTexVS));
	ThrowIfFailed(ReadCSO("../x64/Release/ClearTexPS.cso", dataClearTexPS));
	ThrowIfFailed(ReadCSO("../x64/Release/ClearTexGS.cso", dataClearTexGS));

	ThrowIfFailed(ReadCSO("../x64/Release/VertSplatVS.cso", dataVertSplatVS));
	ThrowIfFailed(ReadCSO("../x64/Release/VertSplatPS.cso", dataVertSplatPS));
	ThrowIfFailed(ReadCSO("../x64/Release/VertSplatGS.cso", dataVertSplatGS));

	ThrowIfFailed(ReadCSO("../x64/Release/GenIndicesVS.cso", dataGenIndicesMeshVS));
	ThrowIfFailed(ReadCSO("../x64/Release/GenIndicesGS.cso", dataGenIndicesMeshGS));
#endif

	// vertex input layout

	const D3D12_INPUT_ELEMENT_DESC densityLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { densityLayout, _countof(densityLayout) };
	psoDesc.pRootSignature = Graphics::rootSignature.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DENSITY_FORMAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataDensityVS.c_str()), dataDensityVS.length() };
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)dataDensityPS.c_str()), dataDensityPS.length() };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataDensityGS.c_str()), dataDensityGS.length() };
	ThrowIfFailed(Graphics::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&densityPipelineState)));
	densityPipelineState->SetName(L"Density Pipeline State");

	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataClearTexVS.c_str()), dataClearTexVS.length() };
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)dataClearTexPS.c_str()), dataClearTexPS.length() };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataClearTexGS.c_str()), dataClearTexGS.length() };
	psoDesc.RTVFormats[0] = INDEX_FORMAT;
	ThrowIfFailed(Graphics::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&dataClearTexPipelineState)));
	dataClearTexPipelineState->SetName(L"Clear Tex Pipeline State");

	const D3D12_INPUT_ELEMENT_DESC vertSplatLayout[] =
	{
		{ "BITPOS", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	psoDesc.InputLayout = { vertSplatLayout, _countof(vertSplatLayout) };
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataVertSplatVS.c_str()), dataVertSplatVS.length() };
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)dataVertSplatPS.c_str()), dataVertSplatPS.length() };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataVertSplatGS.c_str()), dataVertSplatGS.length() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.RTVFormats[0] = INDEX_FORMAT;
	ThrowIfFailed(Graphics::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&dataVertSplatPipelineState)));
	dataVertSplatPipelineState->SetName(L"Vert Splat Pipeline State");

	const D3D12_INPUT_ELEMENT_DESC occupiedLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UVPOSITION", 0, DXGI_FORMAT_R32G32_UINT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 }
	};

	const D3D12_SO_DECLARATION_ENTRY occupiedOut[] =
	{
		{ 0, "BITPOS", 0, 0, 1, 0 }
	};

	UINT occupiedStride = sizeof(UINT);

	psoDesc.InputLayout = { occupiedLayout, _countof(occupiedLayout) };
	psoDesc.StreamOutput.NumEntries = _countof(occupiedOut);
	psoDesc.StreamOutput.pBufferStrides = &occupiedStride;
	psoDesc.StreamOutput.NumStrides = 1;
	psoDesc.StreamOutput.RasterizedStream = 0;
	psoDesc.StreamOutput.pSODeclaration = occupiedOut;
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataOccupiedVS.c_str()), dataOccupiedVS.length() };
	psoDesc.PS = { 0, 0 };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataOccupiedGS.c_str()), dataOccupiedGS.length() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	ThrowIfFailed(Graphics::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&occupiedPipelineState)));
	occupiedPipelineState->SetName(L"Occupied Pipeline State");

	const D3D12_INPUT_ELEMENT_DESC bitPosLayout[] =
	{
		{ "BITPOS", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	psoDesc.InputLayout = { bitPosLayout, _countof(bitPosLayout) };
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataGenVertsVS.c_str()), dataGenVertsVS.length() };
	psoDesc.PS = { 0, 0 };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataGenVertsGS.c_str()), dataGenVertsGS.length() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	ThrowIfFailed(Graphics::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&genVertsPipelineState)));
	genVertsPipelineState->SetName(L"Gen Verts Pipeline State");

	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataGenIndicesMeshVS.c_str()), dataGenIndicesMeshVS.length() };
	psoDesc.PS = { 0, 0 };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataGenIndicesMeshGS.c_str()), dataGenIndicesMeshGS.length() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	ThrowIfFailed(Graphics::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&dataGenIndicesPipelineState)));
	dataGenIndicesPipelineState->SetName(L"Gen Indices Pipeline State");

	const D3D12_SO_DECLARATION_ENTRY vertexMeshOut[] =
	{
		{ 0, "SV_POSITION", 0, 0, 4, 0 },
		{ 0, "NORMAL", 0, 0, 3, 0 },
		{ 0, "TEXCOORD", 0, 0, 3, 0 }
	};

	UINT vertexMeshStride = sizeof(FLOAT) * 10;

	psoDesc.StreamOutput.NumEntries = _countof(vertexMeshOut);
	psoDesc.StreamOutput.pBufferStrides = &vertexMeshStride;
	psoDesc.StreamOutput.NumStrides = 1;
	psoDesc.StreamOutput.RasterizedStream = 0;
	psoDesc.StreamOutput.pSODeclaration = vertexMeshOut;
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataVertexMeshVS.c_str()), dataVertexMeshVS.length() };
	psoDesc.PS = { 0, 0 };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataVertexMeshGS.c_str()), dataVertexMeshGS.length() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	ThrowIfFailed(Graphics::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&vertexMeshPipelineState)));
	vertexMeshPipelineState->SetName(L"Vertex Mesh Pipeline State");
}

void ProcGen::setup()
{
	setupShaders();
	setupBuffers();
	setupCBV();
	setupRTV();

	// set command list
	commandList = Graphics::commandList;
}

void ProcGen::setupBuffers()
{
	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(MAX_BUFFER_SIZE + COUNTER_SIZE),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr,
		IID_PPV_ARGS(&vertexFrontBuffer)));

	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(MAX_BUFFER_SIZE + COUNTER_SIZE),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr,
		IID_PPV_ARGS(&vertexBackBuffer)));

	// setup index
	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT)),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&indexCount)));

	// setup vertex count
	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT)),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vertexCount)));


	// setup points
	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(BYTES_POINTS),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pointVCB)));

	OCCUPIED_POINT* pointBufferData;
	CD3DX12_RANGE readRange(0, 0);
	pointVCB->Map(0, &readRange, (void**)&pointBufferData);

	for (UINT j = 0; j < OCCUPIED_SIZE; j++)
	{
		for (UINT k = 0; k < OCCUPIED_SIZE; k++)
		{
			OCCUPIED_POINT* p = &pointBufferData[k + j * (UINT)OCCUPIED_SIZE];
			p->position = XMFLOAT2(
				(k + EXTRA) * INV_OCC_SIZE_M1,
				(j + EXTRA) * INV_OCC_SIZE_M1
			);
			p->uv = XMUINT2(
				k, j
			);
		}
	}

	pointVCB->Unmap(0, nullptr);

	// setup vertex and index buffer structs
	pointVB.BufferLocation = pointVCB->GetGPUVirtualAddress();
	pointVB.StrideInBytes = sizeof(OCCUPIED_POINT);
	pointVB.SizeInBytes = BYTES_POINTS;
}

void ProcGen::setupCBV()
{
	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(VOXEL_POS) + 255) & ~255),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&Graphics::bufferCB[Graphics::CB_VOXEL_POS])));

	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(GENERATION_CONSTANTS) + 255) & ~255),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&Graphics::bufferCB[Graphics::CB_GENERATION_CONSTANTS])));

	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(DENSITY_CONSTANTS) + 255) & ~255),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&Graphics::bufferCB[Graphics::CB_DENSITY_CONSTANTS])));

	CD3DX12_RANGE readRange(0, 0);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cdesc;

	Graphics::bufferCB[Graphics::CB_VOXEL_POS]->Map(0, &readRange, (void**)&voxelPosData);

	voxelPosData->densityType = 0;
	voxelPosData->renderType = 0;

	// IS NOT UNMAPPED

	cdesc.BufferLocation = Graphics::bufferCB[Graphics::CB_VOXEL_POS]->GetGPUVirtualAddress();
	cdesc.SizeInBytes = (sizeof(VOXEL_POS) + 255) & ~255;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(
		Graphics::csuHeap->GetCPUDescriptorHandleForHeapStart(),
		Graphics::CB_VOXEL_POS, Graphics::csuDescriptorSize);

	Graphics::device->CreateConstantBufferView(&cdesc, cbvHandle0);
	cbvHandle0.Offset(Graphics::csuDescriptorSize);


	// set generation to default
	GENERATION_CONSTANTS* gc;
	Graphics::bufferCB[Graphics::CB_GENERATION_CONSTANTS]->Map(0, &readRange, (void**)&gc);

	*gc = GENERATION_CONSTANTS();

	Graphics::bufferCB[Graphics::CB_GENERATION_CONSTANTS]->Unmap(0, nullptr);

	cdesc.BufferLocation = Graphics::bufferCB[Graphics::CB_GENERATION_CONSTANTS]->GetGPUVirtualAddress();
	cdesc.SizeInBytes = (sizeof(GENERATION_CONSTANTS) + 255) & ~255;

	Graphics::device->CreateConstantBufferView(&cdesc, cbvHandle0);
	cbvHandle0.Offset(Graphics::csuDescriptorSize);

	// set density constants
	DENSITY_CONSTANTS* dc;
	Graphics::bufferCB[Graphics::CB_DENSITY_CONSTANTS]->Map(0, &readRange, (void**)&dc);
	XMStoreFloat4x4(
		&dc->rotMatrix0,
		XMMatrixRotationRollPitchYaw(.213, 1.23, 4.24));
	XMStoreFloat4x4(
		&dc->rotMatrix1,
		XMMatrixRotationRollPitchYaw(1.87, 2.33, 2.334));
	XMStoreFloat4x4(
		&dc->rotMatrix2,
		XMMatrixRotationRollPitchYaw(3.22, 1.44, .98));

	Graphics::bufferCB[Graphics::CB_DENSITY_CONSTANTS]->Unmap(0, nullptr);

	cdesc.BufferLocation = Graphics::bufferCB[Graphics::CB_DENSITY_CONSTANTS]->GetGPUVirtualAddress();
	cdesc.SizeInBytes = (sizeof(DENSITY_CONSTANTS) + 255) & ~255;

	Graphics::device->CreateConstantBufferView(&cdesc, cbvHandle0);
	cbvHandle0.Offset(Graphics::csuDescriptorSize);
}

void ProcGen::setupRTV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		Graphics::rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		Graphics::RTV_DENSITY_TEXTURE, Graphics::rtvDescriptorSize);

	// setup RTV and command allocator for the frames
	D3D12_RESOURCE_DESC voxelTextureDesc = {};
	voxelTextureDesc.MipLevels = 1;
	voxelTextureDesc.Format = DENSITY_FORMAT;
	voxelTextureDesc.Width = DENSITY_SIZE;
	voxelTextureDesc.Height = DENSITY_SIZE;
	voxelTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	voxelTextureDesc.DepthOrArraySize = OCC_SIZE;
	voxelTextureDesc.SampleDesc.Count = 1;
	voxelTextureDesc.SampleDesc.Quality = 0;
	voxelTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = voxelTextureDesc.Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
	rtvDesc.Texture3D.MipSlice = 0;
	rtvDesc.Texture3D.FirstWSlice = 0;
	rtvDesc.Texture3D.WSize = voxelTextureDesc.DepthOrArraySize;

	// density 3D texture SRV 1
	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&voxelTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr,
		IID_PPV_ARGS(&Graphics::intermediateTarget[Graphics::RTV_DENSITY_TEXTURE])
	));

	Graphics::device->CreateRenderTargetView(Graphics::intermediateTarget[Graphics::RTV_DENSITY_TEXTURE].Get(), &rtvDesc, rtvHandle);
	rtvHandle.Offset(1, Graphics::rtvDescriptorSize);

	// bind the SRV to t0
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(
		Graphics::csuHeap->GetCPUDescriptorHandleForHeapStart(),
		Graphics::CBV_COUNT, Graphics::csuDescriptorSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DENSITY_FORMAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MipLevels = 1;

	// bind texture to srv
	Graphics::device->CreateShaderResourceView(
		Graphics::intermediateTarget[Graphics::RTV_DENSITY_TEXTURE].Get(),
		&srvDesc, srvHandle0);
	srvHandle0.Offset(Graphics::csuDescriptorSize);

	// SRV 2

	voxelTextureDesc.Format = INDEX_FORMAT;
	voxelTextureDesc.Width = SPLAT_SIZE * 3;
	voxelTextureDesc.Height = SPLAT_SIZE;
	voxelTextureDesc.DepthOrArraySize = SPLAT_SIZE;

	rtvDesc.Format = INDEX_FORMAT;
	rtvDesc.Texture3D.WSize = voxelTextureDesc.DepthOrArraySize;

	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&voxelTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr,
		IID_PPV_ARGS(&Graphics::intermediateTarget[Graphics::RTV_INDEX_TEXTURE])
	));

	Graphics::device->CreateRenderTargetView(Graphics::intermediateTarget[Graphics::RTV_INDEX_TEXTURE].Get(), &rtvDesc, rtvHandle);
	rtvHandle.Offset(1, Graphics::rtvDescriptorSize);

	srvDesc.Format = INDEX_FORMAT;

	// bind texture to srv
	Graphics::device->CreateShaderResourceView(
		Graphics::intermediateTarget[Graphics::RTV_INDEX_TEXTURE].Get(),
		&srvDesc, srvHandle0);
	srvHandle0.Offset(Graphics::csuDescriptorSize);
}

void ProcGen::renderDensity()
{
	// set the pipeline (NOT NEEDED)
	//commandList->SetPipelineState(densityPipelineState.Get());

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(Graphics::rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		Graphics::RTV_DENSITY_TEXTURE, Graphics::rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &Graphics::plainVB);
	commandList->RSSetViewports(1, &voxelViewport);
	commandList->RSSetScissorRects(1, &voxelScissorRect);
	commandList->DrawInstanced(_countof(Graphics::plainVerts), OCC_SIZE, 0, 0);

	// TODO: CHECK IF THIS IS RIGHT
	// wait for shader
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(NULL));
}

void ProcGen::renderOccupied()
{
	// set the pipeline state
	commandList->SetPipelineState(occupiedPipelineState.Get());

	// clear out the filled size location
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexFrontBuffer.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST));

	commandList->CopyBufferRegion(vertexFrontBuffer.Get(),
		MAX_BUFFER_SIZE,
		Graphics::zeroBuffer.Get(),
		0, sizeof(UINT));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexFrontBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT));

	D3D12_STREAM_OUTPUT_BUFFER_VIEW sobv;
	sobv.BufferLocation = vertexFrontBuffer->GetGPUVirtualAddress();
	sobv.SizeInBytes = MAX_BUFFER_SIZE;
	sobv.BufferFilledSizeLocation = sobv.BufferLocation + MAX_BUFFER_SIZE;

	// set the target
	commandList->SOSetTargets(0, 1, &sobv);

	// draw the object
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
	commandList->IASetVertexBuffers(0, 1, &pointVB);
	commandList->DrawInstanced(NUM_POINTS, OCCUPIED_SIZE, 0, 0);

	// wait for the buffer to be written to
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexFrontBuffer.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE));

	// copy the output data to a buffer
	commandList->CopyBufferRegion(vertexCount.Get(), 0,
		vertexFrontBuffer.Get(),
		MAX_BUFFER_SIZE, sizeof(UINT));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexFrontBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
}

void ProcGen::renderGenVerts(UINT vertApprox)
{
	// set the pipeline state (NOT NEEDED)
	//commandList->SetPipelineState(genVertsPipelineState.Get());

	// clear out the filled size location
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBackBuffer.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST));

	commandList->CopyBufferRegion(vertexBackBuffer.Get(),
		MAX_BUFFER_SIZE,
		Graphics::zeroBuffer.Get(),
		0, sizeof(UINT));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBackBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT));

	D3D12_VERTEX_BUFFER_VIEW vertBuffer;
	vertBuffer.BufferLocation = vertexFrontBuffer->GetGPUVirtualAddress();
	vertBuffer.StrideInBytes = sizeof(BITPOS);
	vertBuffer.SizeInBytes = MAX_BUFFER_SIZE;

	D3D12_STREAM_OUTPUT_BUFFER_VIEW sobv;
	sobv.BufferLocation = vertexBackBuffer->GetGPUVirtualAddress();
	sobv.SizeInBytes = MAX_BUFFER_SIZE;
	sobv.BufferFilledSizeLocation = sobv.BufferLocation + MAX_BUFFER_SIZE;

	// set the target
	commandList->SOSetTargets(0, 1, &sobv);

	// draw the object
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->IASetVertexBuffers(0, 1, &vertBuffer);

	commandList->DrawInstanced(vertApprox, 1, 0, 0);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBackBuffer.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE));

	// copy the output data to a buffer
	commandList->CopyBufferRegion(vertexCount.Get(), 0,
		vertexBackBuffer.Get(),
		MAX_BUFFER_SIZE, sizeof(UINT));

	// TODO: combine barriers
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBackBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer[index].Get(),
	//D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT));
}

void ProcGen::renderClearTex()
{
	// set the pipeline
	commandList->SetPipelineState(dataClearTexPipelineState.Get());

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(Graphics::rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		Graphics::RTV_INDEX_TEXTURE, Graphics::rtvDescriptorSize);
	commandList->SOSetTargets(0, 0, 0);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &Graphics::plainVB);
	commandList->RSSetViewports(1, &vertSplatViewport);
	commandList->RSSetScissorRects(1, &vertSplatScissorRect);
	commandList->DrawInstanced(_countof(Graphics::plainVerts), SPLAT_SIZE, 0, 0);

	// TODO: CHECK IF THIS IS RIGHT
	// wait for shader
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(NULL));
}

UINT ProcGen::renderVertSplat()
{
	// set the pipeline (NOT NEEDED)
	//commandList->SetPipelineState(dataVertSplatPipelineState.Get());

	D3D12_VERTEX_BUFFER_VIEW vertBuffer;
	vertBuffer.BufferLocation = vertexBackBuffer->GetGPUVirtualAddress();
	vertBuffer.StrideInBytes = sizeof(BITPOS);
	vertBuffer.SizeInBytes = MAX_BUFFER_SIZE;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(Graphics::rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		Graphics::numFrames + Graphics::INDEX_TEXTURE, Graphics::rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->IASetVertexBuffers(0, 1, &vertBuffer);
	commandList->RSSetViewports(1, &vertSplatViewport);
	commandList->RSSetScissorRects(1, &vertSplatScissorRect);

	// store old verts
	UINT numVerts;

	UINT* readVert;
	CD3DX12_RANGE readRange(0, sizeof(UINT));
	vertexCount->Map(0, &readRange, (void**)&readVert);
	numVerts = *readVert / sizeof(BITPOS);
	vertexCount->Unmap(0, nullptr);
	//cout << "2 " << vertCount[index] << endl;

	commandList->DrawInstanced(numVerts, 1, 0, 0);

	// TODO: CHECK IF THIS IS RIGHT
	// wait for shader
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(NULL));

	return numVerts;
}

void ProcGen::renderGenIndices(XMINT3 pos, UINT minIndices)
{
	// set the pipeline
	commandList->SetPipelineState(dataGenIndicesPipelineState.Get());

	// create out buffer
	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(minIndices * 15 * sizeof(UINT) + COUNTER_SIZE),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr,
		IID_PPV_ARGS(&computedPos[pos].indices)));

	D3D12_STREAM_OUTPUT_BUFFER_VIEW sobv;
	sobv.BufferLocation = computedPos[pos].indices->GetGPUVirtualAddress();
	sobv.SizeInBytes = minIndices * 15 * sizeof(UINT);
	sobv.BufferFilledSizeLocation = sobv.BufferLocation + sobv.SizeInBytes;

	D3D12_VERTEX_BUFFER_VIEW vertBuffer;
	vertBuffer.BufferLocation = vertexFrontBuffer->GetGPUVirtualAddress();
	vertBuffer.StrideInBytes = sizeof(BITPOS);
	vertBuffer.SizeInBytes = MAX_BUFFER_SIZE;

	// set the target
	commandList->SOSetTargets(0, 1, &sobv);

	commandList->OMSetRenderTargets(0, 0, FALSE, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->IASetVertexBuffers(0, 1, &vertBuffer);
	commandList->DrawInstanced(minIndices, 1, 0, 0);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(computedPos[pos].indices.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE));

	// copy the output data to a buffer
	commandList->CopyBufferRegion(indexCount.Get(), 0,
		computedPos[pos].indices.Get(),
		sobv.SizeInBytes, sizeof(UINT));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(computedPos[pos].indices.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexFrontBuffer.Get(),
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT));
}

void ProcGen::renderVertexMesh(XMINT3 pos, UINT numVerts)
{
	// set the pipeline state
	commandList->SetPipelineState(vertexMeshPipelineState.Get());

	D3D12_VERTEX_BUFFER_VIEW vertBuffer;
	vertBuffer.BufferLocation = vertexBackBuffer->GetGPUVirtualAddress();
	vertBuffer.StrideInBytes = sizeof(BITPOS);
	vertBuffer.SizeInBytes = MAX_BUFFER_SIZE;

	// create out buffer
	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(numVerts * sizeof(VERT_OUT) + COUNTER_SIZE),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr,
		IID_PPV_ARGS(&computedPos[pos].vertices)));

	// store the number of vertices that will be produced
	computedPos[pos].numVertices = numVerts;

	D3D12_STREAM_OUTPUT_BUFFER_VIEW sobv;
	sobv.BufferLocation = computedPos[pos].vertices->GetGPUVirtualAddress();
	sobv.SizeInBytes = numVerts * sizeof(VERT_OUT);
	sobv.BufferFilledSizeLocation = sobv.BufferLocation + sobv.SizeInBytes;

	// set the target
	commandList->SOSetTargets(0, 1, &sobv);

	// draw the object
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->IASetVertexBuffers(0, 1, &vertBuffer);

	// read in the vertex count
	commandList->DrawInstanced(numVerts, 1, 0, 0);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexFrontBuffer.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBackBuffer.Get(),
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT));
}

void ProcGen::phase1()
{
	// reset the command allocator
	ThrowIfFailed(Graphics::commandAllocator[Graphics::frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(Graphics::commandAllocator[Graphics::frameIndex].Get(),
		densityPipelineState.Get()));

	Graphics::setupDescriptors();

	renderDensity();
	renderOccupied();

	// run the commands
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	Graphics::commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// wait on the gpu
	Graphics::waitForGpu();
}

UINT ProcGen::phase2()
{
	// read in the vertex count
	UINT* readVert;

	// get the number of verts needed (if at all)
	UINT vertApprox;
	CD3DX12_RANGE readRange(0, sizeof(UINT));
	vertexCount->Map(0, &readRange, (void**)&readVert);
	vertApprox = *readVert / sizeof(BITPOS);
	vertexCount->Unmap(0, nullptr);

	// do not run the command list
	if (vertApprox == 0)
		return 0;

	// reset the command allocator
	ThrowIfFailed(Graphics::commandAllocator[Graphics::frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(Graphics::commandAllocator[Graphics::frameIndex].Get(),
		genVertsPipelineState.Get()));

	Graphics::setupDescriptors();

	// return the number of approximate verts
	// end early if no verts needed
	renderGenVerts(vertApprox);

	// run the commands
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	Graphics::commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// wait on the gpu
	Graphics::waitForGpu();

	return vertApprox;
}

void ProcGen::phase3(XMINT3 pos, UINT numOldVerts)
{
	// reset the command allocator
	ThrowIfFailed(Graphics::commandAllocator[Graphics::frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(Graphics::commandAllocator[Graphics::frameIndex].Get(),
		dataVertSplatPipelineState.Get()));

	Graphics::setupDescriptors();

	// indices
	//renderClearTex();
	UINT numVerts = renderVertSplat();
	renderGenIndices(pos, numOldVerts);

	// verts
	renderVertexMesh(pos, numVerts);

	// run the commands
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	Graphics::commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// wait on the gpu
	Graphics::waitForGpu();
}

void ProcGen::phase4(XMINT3 pos)
{
	getIndexData(pos);
}

void ProcGen::getIndexData(XMINT3 pos)
{
	// read in the vertex count
	UINT* readData;
	CD3DX12_RANGE readRange(0, sizeof(UINT));

	// read in the index count
	indexCount->Map(0, &readRange, (void**)&readData);
	computedPos[pos].numIndices = *readData / sizeof(UINT);
	indexCount->Unmap(0, nullptr);

	//cout << "IND " << computedPos[pos].numIndices << endl;
}

void ProcGen::genVoxel(XMINT3 pos)
{
	XMFLOAT3 realPos =
		{ pos.x * CHUNK_SIZE,
			pos.y * CHUNK_SIZE,
			pos.z * CHUNK_SIZE };

	// set the voxel pos
	voxelPosData->voxelPos =
		XMFLOAT3(realPos.x, realPos.y, realPos.z);

	//cout << "POS " << pos.x << " " << pos.y << " " << pos.z << endl;

	// run phase 1
	phase1();

	UINT numOldVerts;

	if ((numOldVerts = phase2()) == 0) // no verts
	{
		//cout << "NO VERTS" << endl;
		return;
	}

	phase3(pos, numOldVerts);
	phase4(pos);
}

void ProcGen::regenTerrain()
{
	clock_t startGen = clock();

	// TODO: change to vector
	// clear the hashmap
	computedPos.clear();

	for (UINT z = 0; z < NUM_VOXELS_Z; z++)
	{
		for (UINT y = 0; y < NUM_VOXELS_Y; y++)
		{
			for (UINT x = 0; x < NUM_VOXELS_X; x++)
			{
				genVoxel(XMINT3(x, y, z));
			}
		}
	}

	double delta = (clock() - startGen) / (double)CLOCKS_PER_SEC;
	//cout << "CREATION TIME: " << delta << endl;
}

void ProcGen::drawTerrain()
{
	for (auto p : computedPos)
	{
		// draw the object
		D3D12_VERTEX_BUFFER_VIEW vertBuffer;
		vertBuffer.BufferLocation = p.second.vertices->GetGPUVirtualAddress();
		vertBuffer.StrideInBytes = sizeof(VERT_OUT);
		vertBuffer.SizeInBytes = p.second.numVertices * vertBuffer.StrideInBytes;

		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		indexBufferView.BufferLocation = p.second.indices->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = p.second.numIndices * sizeof(UINT);

		Graphics::commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Graphics::commandList->IASetVertexBuffers(0, 1, &vertBuffer);
		Graphics::commandList->IASetIndexBuffer(&indexBufferView);
		Graphics::commandList->SOSetTargets(0, 0, nullptr);
		Graphics::commandList->DrawIndexedInstanced(p.second.numIndices, 1, 0, 0, 0);
	}
}