#include <d3dcompiler.h>
#include <iostream>
#include "d3dx12.h"
#include "Graphics.h"
#include "Helper.h"
#include "Window.h"
#include "Shader.h"
#include "Parser.h"

#include <ctime>

#define DATA_SIZE 256

// TMP
std::clock_t start;
double fps = 0, frames = 0;

Graphics::Graphics(std::wstring title, unsigned int width, unsigned int height)
	: Manager(title, width, height),
	frameIndex(0)
{
	ZeroMemory(fenceVal, sizeof(fenceVal));
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MaxDepth = 1.0f;

	voxelViewport.Width = VOXEL_SIZE_P1;
	voxelViewport.Height = VOXEL_SIZE_P1;
	voxelViewport.MaxDepth = 1.0f;

	scissorRect.right = static_cast<LONG>(width);
	scissorRect.bottom = static_cast<LONG>(height);

	voxelScissorRect.right = VOXEL_SIZE_P1;
	voxelScissorRect.bottom = VOXEL_SIZE_P1;
}

void Graphics::onInit()
{
	loadPipeline();
	loadAssets();
}

void Graphics::onUpdate()
{
	// wait for the last present

	WaitForSingleObjectEx(swapChainEvent, 100, FALSE);
}

void Graphics::onRender()
{
	double delta = (std::clock() - start) / (double)CLOCKS_PER_SEC;

	if (delta > 1.0)
	{
		fps = frames / delta;

		frames = 0;

		start = std::clock();
	}

	// record the render commands

	// reset the command allocator
	ThrowIfFailed(commandAllocator[frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(commandAllocator[frameIndex].Get(),
		renderPipelineState.Get()));

	setupProceduralDescriptors();

	XMUINT3 voxelPos = { 0, 0, 0 };
	voxelPosData->voxelPos = voxelPos;

	UINT index = voxelPos.x * NUM_VOXELS_X +
		voxelPos.y * NUM_VOXELS_Y +
		voxelPos.z * NUM_VOXELS_Z;

	renderDensity(voxelPos); // NOTE: causes error
	//renderOccupied(voxelPos, index);
	//renderGenVerts(voxelPos, index);
	//renderVertexMesh(voxelPos, index);
	populateCommandList();


	// close the commands
	ThrowIfFailed(commandList->Close());

	// run the commands

	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// show the frame

	ThrowIfFailed(swapChain->Present(1, 0));

	if (frames == 0)
		std::cout << "FPS: " << fps << '\n';

	frames++;

	// go to the next frame

	moveToNextFrame();
}

void Graphics::onDestroy()
{
	// wait for the GPU to finish

	waitForGpu();

	// close the elements

	CloseHandle(fenceEvent);
}

void Graphics::loadPipeline()
{
#if defined(_DEBUG)

	// Enable debug in direct 3D
	ComPtr<ID3D12Debug> debugController;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		debugController->EnableDebugLayer();

#endif

	// create the graphics infrastructure
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

	// check for wraper device

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
		));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
		));
	}

	// create the command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	// create the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = numFrames;
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = Window::getWindow();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	ComPtr<IDXGISwapChain> swapChaintmp;
	ThrowIfFailed(factory->CreateSwapChain(
		commandQueue.Get(),
		&swapChainDesc,
		&swapChaintmp
	));

	ThrowIfFailed(swapChaintmp.As(&swapChain));

	// does not support full screen transitions for now TODO: fix this

	ThrowIfFailed(factory->MakeWindowAssociation(Window::getWindow(), DXGI_MWA_NO_ALT_ENTER));

	// store the frame and swap chain

	frameIndex = swapChain->GetCurrentBackBufferIndex();
	swapChainEvent = swapChain->GetFrameLatencyWaitableObject();

	// create descriptor heaps (RTV and CBV / SRV / UAV)

	// RTV

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = numFrames + 1;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// CBV / SRV / UAV

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors = CBV_COUNT + SRV_COUNT + UAV_COUNT;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&csuHeap)));

	csuDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Sampler

	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
	samplerHeapDesc.NumDescriptors = SAMPLER_COUNT;
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerHeap)));

	// create sampler
	samplerDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// frame resource

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT n = 0; n < numFrames; n++)
	{
		// render texture
		ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTarget[n])));
		device->CreateRenderTargetView(renderTarget[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);

		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[n])));
	}

	// setup RTV and command allocator for the frames
	voxelTextureDesc.MipLevels = 1;
	voxelTextureDesc.Format = DENSITY_FORMAT;
	voxelTextureDesc.Width = VOXEL_SIZE_P1;
	voxelTextureDesc.Height = VOXEL_SIZE_P1;
	voxelTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	voxelTextureDesc.DepthOrArraySize = VOXEL_SIZE_P1;
	voxelTextureDesc.SampleDesc.Count = 1;
	voxelTextureDesc.SampleDesc.Quality = 0;
	voxelTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = DENSITY_FORMAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
	rtvDesc.Texture3D.MipSlice = 0;
	rtvDesc.Texture3D.FirstWSlice = 0;
	rtvDesc.Texture3D.WSize = VOXEL_SIZE_P1;

	// density 3D texture
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&voxelTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr,
		IID_PPV_ARGS(&intermediateTarget[0])
	));

	device->CreateRenderTargetView(intermediateTarget[0].Get(), nullptr, rtvHandle);
	rtvHandle.Offset(1, rtvDescriptorSize);

	// bind the SRV to t0
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(
		csuHeap->GetCPUDescriptorHandleForHeapStart(),
		CBV_COUNT, csuDescriptorSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DENSITY_FORMAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MipLevels = 1;

	// bind texture to srv
	device->CreateShaderResourceView(
		intermediateTarget[0].Get(),
		&srvDesc, srvHandle0);

	// create sampler
	CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle0(samplerHeap->GetCPUDescriptorHandleForHeapStart(), 0, samplerDescriptorSize);
	device->CreateSampler(&samplerDesc, samplerHandle0);

	// create constant buffer

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(VOXEL_POS) + 255) & ~255),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferCB[CB_VOXEL_POS])));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cdesc;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(
		csuHeap->GetCPUDescriptorHandleForHeapStart(),
		0, csuDescriptorSize);

	// setup map
	CD3DX12_RANGE readRange(0, 0);
	bufferCB[CB_VOXEL_POS]->Map(0, &readRange, (void**)&voxelPosData);

	// DO NOT EVER UNMAP
	//bufferCB[0]->Unmap(0, nullptr);

	cdesc.BufferLocation = bufferCB[CB_VOXEL_POS]->GetGPUVirtualAddress();
	cdesc.SizeInBytes = (sizeof(VOXEL_POS) + 255) & ~255;

	device->CreateConstantBufferView(&cdesc, cbvHandle0);
	cbvHandle0.Offset(csuDescriptorSize);

	// setup poly and edge constants

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(POLY_CONSTANTS) + 255) & ~255),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferCB[CB_POLY_CONST])));

	cdesc.BufferLocation = bufferCB[CB_POLY_CONST]->GetGPUVirtualAddress();
	cdesc.SizeInBytes = (sizeof(POLY_CONSTANTS) + 255) & ~255;

	device->CreateConstantBufferView(&cdesc, cbvHandle0);
	cbvHandle0.Offset(csuDescriptorSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(EDGE_CONSTANTS) + 255) & ~255),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferCB[CB_EDGE_CONST])));

	cdesc.BufferLocation = bufferCB[CB_EDGE_CONST]->GetGPUVirtualAddress();
	cdesc.SizeInBytes = (sizeof(EDGE_CONSTANTS) + 255) & ~255;

	device->CreateConstantBufferView(&cdesc, cbvHandle0);
	cbvHandle0.Offset(csuDescriptorSize);

	POLY_CONSTANTS* polyConst;
	bufferCB[CB_POLY_CONST]->Map(0, &readRange, (void**)&polyConst);

	EDGE_CONSTANTS* edgeConst;
	bufferCB[CB_EDGE_CONST]->Map(0, &readRange, (void**)&edgeConst);

	parsePoly("Polys.txt", "Edges.txt", polyConst->numberPolygons, edgeConst->edgeNumber);
	
	bufferCB[CB_POLY_CONST]->Unmap(0, nullptr);
	bufferCB[CB_EDGE_CONST]->Unmap(0, nullptr);

	// setup DEBUG var for UAV

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(BOOL),
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&bufferUAV[DEBUG_VAR])));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle0(csuHeap->GetCPUDescriptorHandleForHeapStart(),
		CBV_COUNT + SRV_COUNT, csuDescriptorSize);

	uavDesc.Buffer.NumElements = 1;
	uavDesc.Buffer.StructureByteStride = sizeof(BOOL);

	device->CreateUnorderedAccessView(bufferUAV[DEBUG_VAR].Get(), nullptr, &uavDesc, uavHandle0);
	uavHandle0.Offset(1, csuDescriptorSize);
	
	// create vertex buffer for each voxel

	for (UINT i = 0; i < NUM_VOXELS; i++)
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(MAX_BUFFER_SIZE + COUNTER_SIZE),
			D3D12_RESOURCE_STATE_STREAM_OUT,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer[i])));
	}
	
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(MAX_BUFFER_SIZE + COUNTER_SIZE),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr,
		IID_PPV_ARGS(&vertexBackBuffer)));
}

void Graphics::loadAssets()
{
	// load the shaders
	string dataDensityVS, dataDensityPS, dataDensityGS,
		dataOccupiedVS, dataOccupiedGS,
		dataGenVertsVS, dataGenVertsGS,
		dataVertexMeshVS, dataVertexMeshGS,
		dataVS, dataPS;

#ifdef DEBUG
	ThrowIfFailed(ReadCSO("../x64/Debug/DensityVS.cso", dataDensityVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/DensityPS.cso", dataDensityPS));
	ThrowIfFailed(ReadCSO("../x64/Debug/DensityGS.cso", dataDensityGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/OccupiedVS.cso", dataOccupiedVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/OccupiedGS.cso", dataOccupiedGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/GenVertsVS.cso", dataGenVertsVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/GenVertsGS.cso", dataGenVertsGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/VertexMeshVS.cso", dataVertexMeshVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/VertexMeshGS.cso", dataVertexMeshGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/RenderVS.cso", dataVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/RenderPS.cso", dataPS));
#else
	ThrowIfFailed(ReadCSO("../x64/Release/DensityVS.cso", dataVS));
	ThrowIfFailed(ReadCSO("../x64/Release/DensityPS.cso", dataVS));
	ThrowIfFailed(ReadCSO("../x64/Release/DensityGS.cso", dataVS));

	ThrowIfFailed(ReadCSO("../x64/Release/OccupiedVS.cso", dataOccupiedVS));
	ThrowIfFailed(ReadCSO("../x64/Release/OccupiedGS.cso", dataOccupiedGS));

	ThrowIfFailed(ReadCSO("../x64/Release/GenVertsVS.cso", dataGenVertsVS));
	ThrowIfFailed(ReadCSO("../x64/Release/GenVertsGS.cso", dataGenVertsGS));

	ThrowIfFailed(ReadCSO("../x64/Release/VertexMeshVS.cso", dataVertexMeshVS));
	ThrowIfFailed(ReadCSO("../x64/Release/VertexMeshGS.cso", dataVertexMeshGS));

	ThrowIfFailed(ReadCSO("../x64/Release/RenderVS.cso", dataVS));
	ThrowIfFailed(ReadCSO("../x64/Release/RenderPS.cso", dataPS));

#endif

	// setup constant buffer and descriptor tables

	CD3DX12_DESCRIPTOR_RANGE ranges[rpCount];
	CD3DX12_ROOT_PARAMETER rootParameters[rpCount];
	/*
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CBV_COUNT, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRV_COUNT, 0);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UAV_COUNT, 0);
	ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, SAMPLER_COUNT, 0);
	//rootParameters[rpCB].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[rpSRV].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[rpUAV].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[rpSAMPLER].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);*/

	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CBV_COUNT, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRV_COUNT, 0);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UAV_COUNT, 0);
	ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, SAMPLER_COUNT, 0);
	rootParameters[rpCB].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[rpSRV].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[rpUAV].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[rpSAMPLER].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);

	// empty root signature

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	//rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
	/*
	// create compute signiture
	CD3DX12_ROOT_SIGNATURE_DESC computeRootSignatureDesc(_countof(rootParameters), rootParameters, 0, nullptr);
	ThrowIfFailed(D3D12SerializeRootSignature(&computeRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&computeRootSignature)));
	*/
	// vertex input layout

	const D3D12_INPUT_ELEMENT_DESC densityLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { densityLayout, _countof(densityLayout) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataDensityVS.c_str()), dataDensityVS.length() };
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)dataDensityPS.c_str()), dataDensityPS.length() };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataDensityGS.c_str()), dataDensityGS.length() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DENSITY_FORMAT;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&densityPipelineState)));
	
	const D3D12_INPUT_ELEMENT_DESC occupiedLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	psoDesc.InputLayout = { occupiedLayout, _countof(occupiedLayout) };
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataOccupiedVS.c_str()), dataOccupiedVS.length() };
	psoDesc.PS = { 0, 0 };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataOccupiedGS.c_str()), dataOccupiedGS.length() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	//psoDesc.NumRenderTargets = 0; TODO: This may be an issue

	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&occupiedPipelineState)));

	const D3D12_INPUT_ELEMENT_DESC bitPosLayout[] =
	{
		{ "BITPOS", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	psoDesc.InputLayout = { bitPosLayout, _countof(bitPosLayout) };
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataGenVertsVS.c_str()), dataGenVertsVS.length() };
	psoDesc.PS = { 0, 0 };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataGenVertsGS.c_str()), dataGenVertsGS.length() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&genVertsPipelineState)));

	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataVertexMeshVS.c_str()), dataVertexMeshVS.length() };
	psoDesc.PS = { 0, 0 };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataVertexMeshGS.c_str()), dataVertexMeshGS.length() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&vertexMeshPipelineState)));

	// setup for render pipeline
	const D3D12_INPUT_ELEMENT_DESC layoutRender[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	psoDesc.InputLayout = { layoutRender, _countof(layoutRender) };
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataVS.c_str()), dataVS.length() };
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)dataPS.c_str()), dataPS.length() };
	psoDesc.GS = { 0, 0 };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&renderPipelineState)));

	// create the compute pipeline (radix sort)
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};

	//computePsoDesc.pRootSignature = computeRootSignature.Get();

	// setup data

	// setup plane to trick PS into rendering the ray tracer output

	// setup verts and indices

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(PLAIN_VERTEX) * _countof(plainVerts) * VOXEL_SIZE_P1),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&plainVCB)));

	// map vertex plain data for upload
	CD3DX12_RANGE readRange(0, 0);

	PLAIN_VERTEX* vertexBufferData;
	plainVCB->Map(0, &readRange, (void**)&vertexBufferData);

	// copy the data with instance IDs added
	for (UINT i = 0; i < VOXEL_SIZE_P1; i++)
	{
		for (UINT j = 0; j < _countof(plainVerts); j++)
		{
			vertexBufferData[i * _countof(plainVerts) + j].position = plainVerts[j].position;
			vertexBufferData[i * _countof(plainVerts) + j].texcoord = plainVerts[j].texcoord;
			vertexBufferData[i * _countof(plainVerts) + j].svInstance = i;
		}
	}

	plainVCB->Unmap(0, nullptr);

	// setup vertex and index buffer structs
	plainVB.BufferLocation = plainVCB->GetGPUVirtualAddress();
	plainVB.StrideInBytes = sizeof(PLAIN_VERTEX);
	plainVB.SizeInBytes = sizeof(PLAIN_VERTEX) * _countof(plainVerts) * VOXEL_SIZE_P1;

	// setup points
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(BYTES_POINTS),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pointVCB)));

	OCCUPIED_POINT* pointBufferData;
	pointVCB->Map(0, &readRange, (void**)&pointBufferData);

	for (UINT i = 0; i < VOXEL_SIZE; i++)
	{
		for (UINT j = 0; j < VOXEL_SIZE; j++)
		{
			for (UINT k = 0; k < VOXEL_SIZE; k++)
			{
				OCCUPIED_POINT* p = &pointBufferData[k + j * VOXEL_SIZE + i * VOXEL_SIZE * VOXEL_SIZE];
				p->position = XMFLOAT2((float)k / VOXEL_SIZE, (float)j / VOXEL_SIZE);
				p->instanceID = i;
			}
		}
	}

	pointVCB->Unmap(0, nullptr);

	// setup vertex and index buffer structs
	pointVB.BufferLocation = pointVCB->GetGPUVirtualAddress();
	pointVB.StrideInBytes = sizeof(OCCUPIED_POINT);
	pointVB.SizeInBytes = BYTES_POINTS;

	// create command list

	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex].Get(), nullptr, IID_PPV_ARGS(&commandList)));

	// close the command list until things are added

	ThrowIfFailed(commandList->Close());

	// run the commands

	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// synchronize objects and wait until assets are uploaded to the GPU

	ThrowIfFailed(device->CreateFence(fenceVal[frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	fenceVal[frameIndex]++;

	// create an event handler

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (fenceEvent == nullptr)
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));

	// wait for the command list to finish being run on the GPU

	waitForGpu();
}

void Graphics::setupProceduralDescriptors()
{
	// set the state
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { csuHeap.Get(), samplerHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// setup the resources
	/*
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), 0, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), CBV_COUNT, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), CBV_COUNT + SRV_COUNT, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE samplerHandle(samplerHeap->GetGPUDescriptorHandleForHeapStart(), 0, samplerDescriptorSize);
	commandList->SetGraphicsRootDescriptorTable(rpCB, cbvHandle);
	commandList->SetGraphicsRootDescriptorTable(rpSRV, srvHandle);
	commandList->SetGraphicsRootDescriptorTable(rpUAV, uavHandle);
	commandList->SetGraphicsRootDescriptorTable(rpSAMPLER, samplerHandle);*/

	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), 0, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), CBV_COUNT, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), CBV_COUNT + SRV_COUNT, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE samplerHandle(samplerHeap->GetGPUDescriptorHandleForHeapStart(), 0, samplerDescriptorSize);

	commandList->SetGraphicsRootDescriptorTable(rpCB, cbvHandle);
	commandList->SetGraphicsRootDescriptorTable(rpSRV, srvHandle);
	commandList->SetGraphicsRootDescriptorTable(rpUAV, uavHandle);
	commandList->SetGraphicsRootDescriptorTable(rpSAMPLER, samplerHandle);

	commandList->RSSetViewports(1, &voxelViewport);
	commandList->RSSetScissorRects(1, &voxelScissorRect);
}

void Graphics::renderDensity(XMUINT3 voxelPos)
{
	// set the pipeline
	commandList->SetPipelineState(densityPipelineState.Get());

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), numFrames, rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &plainVB);
	commandList->DrawInstanced(_countof(plainVerts) * VOXEL_SIZE_P1, 1, 0, 0);
}

void Graphics::renderOccupied(XMUINT3 voxelPos, UINT index)
{
	// set the pipeline state
	commandList->SetPipelineState(occupiedPipelineState.Get());

	D3D12_STREAM_OUTPUT_BUFFER_VIEW sobv;
	sobv.BufferLocation = vertexBuffer[index]->GetGPUVirtualAddress();
	sobv.SizeInBytes = MAX_BUFFER_SIZE;
	sobv.BufferFilledSizeLocation = sobv.BufferLocation + MAX_BUFFER_SIZE;
	
	// set the target
	commandList->SOSetTargets(0, 1, &sobv);

	// draw the object
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->IASetVertexBuffers(0, 1, &pointVB);
	commandList->DrawInstanced(_countof(plainVerts) * VOXEL_SIZE_P1, 1, 0, 0);
}

void Graphics::renderGenVerts(XMUINT3 voxelPos, UINT index)
{
	// set the pipeline state
	commandList->SetPipelineState(genVertsPipelineState.Get());

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer[index].Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	D3D12_VERTEX_BUFFER_VIEW vertBuffer;
	vertBuffer.BufferLocation = vertexBuffer[index]->GetGPUVirtualAddress();
	vertBuffer.StrideInBytes = sizeof(BITPOS);
	vertBuffer.SizeInBytes = MAX_BUFFER_SIZE;

	D3D12_STREAM_OUTPUT_BUFFER_VIEW sobv;
	sobv.BufferLocation = vertexBackBuffer->GetGPUVirtualAddress();
	sobv.SizeInBytes = MAX_BUFFER_SIZE;
	sobv.BufferFilledSizeLocation = sobv.BufferLocation + MAX_BUFFER_SIZE;

	// set the target
	commandList->SOSetTargets(0, 1, &sobv);
	//(sobv.BufferLocation + MAX_BUFFER_SIZE)
	// draw the object
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->IASetVertexBuffers(0, 1, &vertBuffer);
	commandList->DrawInstanced(100, 1, 0, 0); // FIX THIS

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer[index].Get(),
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT));
}

void Graphics::renderVertexMesh(XMUINT3 voxelPos, UINT index)
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBackBuffer.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// set the pipeline state
	commandList->SetPipelineState(vertexMeshPipelineState.Get());

	D3D12_VERTEX_BUFFER_VIEW vertBuffer;
	vertBuffer.BufferLocation = vertexBackBuffer->GetGPUVirtualAddress();
	vertBuffer.StrideInBytes = sizeof(BITPOS);
	vertBuffer.SizeInBytes = MAX_BUFFER_SIZE;

	D3D12_STREAM_OUTPUT_BUFFER_VIEW sobv;
	sobv.BufferLocation = vertexBuffer[index]->GetGPUVirtualAddress();
	sobv.SizeInBytes = MAX_BUFFER_SIZE;
	sobv.BufferFilledSizeLocation = sobv.BufferLocation + MAX_BUFFER_SIZE;

	// set the target
	commandList->SOSetTargets(0, 1, &sobv);

	// draw the object
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->IASetVertexBuffers(0, 1, &vertBuffer);
	commandList->DrawInstanced(100, 1, 0, 0); // FIX THIS

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBackBuffer.Get(),
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT));
}

void Graphics::populateCommandList()
{
	// set the pipeline state
	commandList->SetPipelineState(renderPipelineState.Get());

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	// set the back buffer as the render target

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget[frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// set the render target view

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// set the background

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// draw the object

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &plainVB);
	commandList->SOSetTargets(0, 0, nullptr);
	commandList->DrawInstanced(_countof(plainVerts), 1, 0, 0);
	//commandList->DrawInstanced(100, 1, 0, 0);

	// do not present the back buffer
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget[frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void Graphics::waitForGpu()
{
	// add a signal to the command queue
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceVal[frameIndex]));

	// wait until the signal is processed

	ThrowIfFailed(fence->SetEventOnCompletion(fenceVal[frameIndex], fenceEvent));
	WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

	// update the fence value

	fenceVal[frameIndex]++;
}

void Graphics::moveToNextFrame()
{
	// write signal command to the queue

	const UINT64 currentFenceValue = fenceVal[frameIndex];
	ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));

	// update the index

	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// wait if the frame has not been rendered yet

	if (fence->GetCompletedValue() < fenceVal[frameIndex])
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceVal[frameIndex], fenceEvent));
		WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
	}

	// set the fence value for the new frame

	fenceVal[frameIndex] = currentFenceValue + 1;
}

// keyboard

void Graphics::onKeyDown(UINT8 key)
{

}

void Graphics::onKeyUp(UINT8 key)
{

}