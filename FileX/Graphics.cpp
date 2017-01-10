#include <d3dcompiler.h>
#include <iostream>
#include <IL/il.h>
#include "d3dx12.h"
#include "Graphics.h"
#include "Helper.h"
#include "Window.h"
#include "Shader.h"
#include "Parser.h"

#include <ctime>

// static fields

// fences
UINT Graphics::frameIndex;
UINT64 Graphics::fenceVal[Graphics::numFrames];

// pipeline objects
D3D12_VIEWPORT Graphics::viewport, Graphics::viewYViewport;
ComPtr<ID3D12Device> Graphics::device;
D3D12_RECT Graphics::scissorRect, Graphics::viewYScissorRect;
ComPtr<ID3D12CommandQueue> Graphics::commandQueue;
ComPtr<ID3D12RootSignature> Graphics::rootSignature;
ComPtr<ID3D12RootSignature> Graphics::computeRootSignature;
ComPtr<ID3D12PipelineState> Graphics::renderPipelineSolidState,
	Graphics::renderPipelineWireframeState;
ComPtr<ID3D12GraphicsCommandList> Graphics::commandList;
ComPtr<IDXGISwapChain3> Graphics::swapChain;

// per frame vars
ComPtr<ID3D12Resource> Graphics::renderTarget[Graphics::numFrames], Graphics::intermediateTarget[Graphics::RTV_COUNT];
ComPtr<ID3D12CommandAllocator> Graphics::commandAllocator[Graphics::numFrames];

// heaps
ComPtr<ID3D12DescriptorHeap> Graphics::rtvHeap;
ComPtr<ID3D12DescriptorHeap> Graphics::dsvHeap;
ComPtr<ID3D12DescriptorHeap> Graphics::csuHeap;
	ComPtr<ID3D12DescriptorHeap> Graphics::samplerHeap;
UINT Graphics::rtvDescriptorSize;
UINT Graphics::dsvDescriptorSize;
UINT Graphics::csuDescriptorSize;
UINT Graphics::samplerDescriptorSize;

// synchronized objects
ComPtr<ID3D12Fence> Graphics::fence;
HANDLE Graphics::fenceEvent;
HANDLE Graphics::swapChainEvent;

// buffers
ComPtr<ID3D12Resource> Graphics::bufferCB[CBV_COUNT],
	Graphics::bufferSRV[Graphics::SRV_COUNT],
	Graphics::bufferUAV[Graphics::UAV_COUNT],
	Graphics::bufferDSV[Graphics::DSV_COUNT],
	Graphics::plainVCB;

ComPtr<ID3D12Resource> Graphics::zeroBuffer;

D3D12_VERTEX_BUFFER_VIEW Graphics::plainVB;

// TMP
std::clock_t start;
double fps = 0, frames = 0;

Graphics::Graphics(std::wstring title, unsigned int width, unsigned int height)
	: Manager(title, width, height)
{
	// init DevIL
	ilInit();

	frameIndex = 0;

	ZeroMemory(fenceVal, sizeof(fenceVal));
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MaxDepth = 1.0f;

	viewYViewport.Width = 2;
	viewYViewport.Height = FINDY_SIZE_P1;
	viewYViewport.MaxDepth = 1.0f;

	scissorRect.right = static_cast<LONG>(width);
	scissorRect.bottom = static_cast<LONG>(height);

	viewYScissorRect.right = 2;
	viewYScissorRect.bottom = FINDY_SIZE_P1;
}

void Graphics::onInit()
{
	loadPipeline();
	loadAssets();
}

void Graphics::onUpdate()
{
	// update world, view, proj
	
	world = XMMatrixIdentity();
	view = XMMatrixLookAtLH(eye, at, up);
	projection = XMMatrixPerspectiveFovLH(XM_PI / 16,
		(float)height / width, .1, 1000.0);
	worldViewProjection = world * view * projection;

	worldPosCB->world = XMMatrixTranspose(world);
	worldPosCB->worldView =
		XMMatrixTranspose(world * view);
	worldPosCB->worldViewProjection =
		XMMatrixTranspose(worldViewProjection);

	// wait for the last present

	WaitForSingleObjectEx(swapChainEvent, 100, FALSE);
}

void Graphics::drawPhase()
{
	float y = findY.findY(at);
	cout << y << endl;
	eyeDelta.x = origDelta.x * cos(yAngle)
		- origDelta.z * sin(yAngle);

	eyeDelta.z = origDelta.z * cos(yAngle)
		+ origDelta.x * sin(yAngle);

	at.m128_f32[1] = y;
	
	eye.m128_f32[0] = at.m128_f32[0] + eyeDelta.x;
	eye.m128_f32[1] = at.m128_f32[1] + eyeDelta.y;
	eye.m128_f32[2] = at.m128_f32[2] + eyeDelta.z;

	//regenTerrain();

	// reset the command allocator
	ThrowIfFailed(commandAllocator[frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(commandAllocator[frameIndex].Get(),
		renderPipelineSolidState.Get()));

	setupDescriptors();

	populateCommandList();

	// run the commands
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// wait on the gpu
	waitForGpu();
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

	drawPhase();

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
	rtvHeapDesc.NumDescriptors = RTV_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// DSV

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = DSV_COUNT;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));

	dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

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

	// SRV 3
	D3D12_RESOURCE_DESC voxelTextureDesc = {};
	voxelTextureDesc.MipLevels = 1;
	voxelTextureDesc.Format = DENSITY_FORMAT;
	voxelTextureDesc.Width = 2;
	voxelTextureDesc.Height = FINDY_SIZE_P1;
	voxelTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	voxelTextureDesc.DepthOrArraySize = 2;
	voxelTextureDesc.SampleDesc.Count = 1;
	voxelTextureDesc.SampleDesc.Quality = 0;
	voxelTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = voxelTextureDesc.Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
	rtvDesc.Texture3D.MipSlice = 0;
	rtvDesc.Texture3D.FirstWSlice = 0;
	rtvDesc.Texture3D.WSize = voxelTextureDesc.DepthOrArraySize;

	D3D12_CLEAR_VALUE clear;
	clear.Format = DENSITY_FORMAT;
	clear.DepthStencil.Depth = 1.0f;
	clear.DepthStencil.Stencil = 0;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&voxelTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clear,
		IID_PPV_ARGS(&intermediateTarget[RTV_FINDY_TEXTURE])
	));

	// change offset
	rtvHandle.InitOffsetted(rtvHeap->GetCPUDescriptorHandleForHeapStart(), RTV_FINDY_TEXTURE, rtvDescriptorSize);

	device->CreateRenderTargetView(intermediateTarget[RTV_FINDY_TEXTURE].Get(), &rtvDesc, rtvHandle);
	rtvHandle.Offset(1, rtvDescriptorSize);
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DENSITY_FORMAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MipLevels = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(
		csuHeap->GetCPUDescriptorHandleForHeapStart(),
		CBV_COUNT + FINDY_TEXTURE, Graphics::csuDescriptorSize);

	// bind texture to srv
	device->CreateShaderResourceView(
		intermediateTarget[RTV_FINDY_TEXTURE].Get(),
		&srvDesc, srvHandle0);
	srvHandle0.Offset(csuDescriptorSize);

	// setup the image loader
	Image::setBase(srvHandle0, csuDescriptorSize, rtvHandle, rtvDescriptorSize);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandleALT(
		csuHeap->GetCPUDescriptorHandleForHeapStart(),
		CBV_COUNT + ALTITUDE, csuDescriptorSize);
	Image2D::setSRVBase(srvHandleALT, csuDescriptorSize);

	// depth stencil
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart(),
		0, dsvDescriptorSize);

	CD3DX12_RESOURCE_DESC dsvTex(D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0,
		static_cast< UINT >(viewport.Width), static_cast< UINT >(viewport.Height), 1, 1,
		DSV_FORMAT, 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
	
	clear.Format = DSV_FORMAT;
	clear.DepthStencil.Depth = 1.0f;
	clear.DepthStencil.Stencil = 0;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&dsvTex,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clear,
		IID_PPV_ARGS(&bufferDSV[DSV_TEX])));

	device->CreateDepthStencilView(bufferDSV[DSV_TEX].Get(),
		nullptr, dsvHandle);
	dsvHandle.Offset(dsvDescriptorSize);

	// create sampler
	CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle0(samplerHeap->GetCPUDescriptorHandleForHeapStart(), 0, samplerDescriptorSize);
	
	// create sampler
	D3D12_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	device->CreateSampler(&samplerDesc, samplerHandle0);
	samplerHandle0.Offset(samplerDescriptorSize);

	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

	device->CreateSampler(&samplerDesc, samplerHandle0);
	samplerHandle0.Offset(samplerDescriptorSize);

	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	device->CreateSampler(&samplerDesc, samplerHandle0);
	samplerHandle0.Offset(samplerDescriptorSize);

	// create constant buffer

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(WORLD_POS) + 255) & ~255),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferCB[CB_WORLD_POS])));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cdesc;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(
		csuHeap->GetCPUDescriptorHandleForHeapStart(),
		0, csuDescriptorSize);

	// setup map
	CD3DX12_RANGE readRange(0, 0);
	bufferCB[CB_WORLD_POS]->Map(0, &readRange, (void**)&worldPosCB);

	cdesc.BufferLocation = bufferCB[CB_WORLD_POS]->GetGPUVirtualAddress();
	cdesc.SizeInBytes = (sizeof(WORLD_POS) + 255) & ~255;

	device->CreateConstantBufferView(&cdesc, cbvHandle0);
	cbvHandle0.Offset(csuDescriptorSize);

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

	// setup vertex count
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&zeroBuffer)));

	// setup map
	UINT* zeroPtr;

	readRange.Begin = 0;
	readRange.End = 0;
	zeroBuffer->Map(0, &readRange, (void**)&zeroPtr);
	*zeroPtr = 0;
	zeroBuffer->Unmap(0, nullptr);
}

void Graphics::loadAssets()
{
	// load the shaders
	string dataVS, dataPS;

	// release optimizations currently break shaders currently so prefer debug versions
#ifdef _DEBUG
	ThrowIfFailed(ReadCSO("../x64/Debug/RenderVS.cso", dataVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/RenderPS.cso", dataPS));
#else
	ThrowIfFailed(ReadCSO("../x64/Release/RenderVS.cso", dataVS));
	ThrowIfFailed(ReadCSO("../x64/Release/RenderPS.cso", dataPS));
#endif

	// setup constant buffer and descriptor tables

	CD3DX12_DESCRIPTOR_RANGE ranges[rpCount];
	CD3DX12_ROOT_PARAMETER rootParameters[rpCount];

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
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
	
	// create compute signiture
	CD3DX12_ROOT_SIGNATURE_DESC computeRootSignatureDesc(_countof(rootParameters), rootParameters, 0, nullptr);
	ThrowIfFailed(D3D12SerializeRootSignature(&computeRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&computeRootSignature)));

	// setup for render pipeline
	const D3D12_INPUT_ELEMENT_DESC layoutRender[] =
	{
		{ "SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT , 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { layoutRender, _countof(layoutRender) };
	psoDesc.StreamOutput.NumEntries = 0;
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataVS.c_str()), dataVS.length() };
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)dataPS.c_str()), dataPS.length() };
	psoDesc.GS = { 0, 0 };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.SampleDesc.Count = 1;
	
	// enable depth buffer
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// add in stencil buffer
	psoDesc.DepthStencilState.StencilEnable = TRUE;
	psoDesc.DepthStencilState.StencilReadMask = 0xFF;
	psoDesc.DepthStencilState.StencilWriteMask = 0xFF;

	// front facing operations
	psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
	psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// back facing operations
	psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
	psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	
	psoDesc.DSVFormat = DSV_FORMAT;
	
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RasterizerState = 
	{
		D3D12_FILL_MODE_SOLID,
		D3D12_CULL_MODE_FRONT,
		FALSE,
		D3D12_DEFAULT_DEPTH_BIAS,
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		TRUE,
		FALSE,
		FALSE,
		0,
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
	};
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&renderPipelineSolidState)));

	psoDesc.RasterizerState =
	{
		D3D12_FILL_MODE_WIREFRAME,
		D3D12_CULL_MODE_NONE,
		FALSE,
		D3D12_DEFAULT_DEPTH_BIAS,
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		TRUE,
		FALSE,
		FALSE,
		0,
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
	};
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&renderPipelineWireframeState)));

	// setup data

	// setup plane to trick PS into rendering the ray tracer output

	// setup verts and indices

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(PLAIN_VERTEX) * _countof(plainVerts)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&plainVCB)));

	// map vertex plain data for upload
	CD3DX12_RANGE readRange(0, 0);

	PLAIN_VERTEX* vertexBufferData;
	plainVCB->Map(0, &readRange, (void**)&vertexBufferData);

	// copy the data with instance IDs added
	for (UINT j = 0; j < _countof(plainVerts); j++)
	{
		vertexBufferData[j].position = plainVerts[j].position;
		vertexBufferData[j].texcoord = plainVerts[j].texcoord;
	}

	plainVCB->Unmap(0, nullptr);

	// setup vertex and index buffer structs
	plainVB.BufferLocation = plainVCB->GetGPUVirtualAddress();
	plainVB.StrideInBytes = sizeof(PLAIN_VERTEX);
	plainVB.SizeInBytes = sizeof(PLAIN_VERTEX) * _countof(plainVerts);

	// create command list

	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex].Get(), nullptr, IID_PPV_ARGS(&commandList)));

	// load in the 2D images
	altitude.loadImage(device.Get(), L"Altitude/Altitude.png");
	altitude.uploadTexture(commandList.Get());

	bumpMap.loadImage(device.Get(), L"Altitude/Mountain Texture 3.bmp");
	bumpMap.uploadTexture(commandList.Get());

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

	// run setup work
	// setup proc gen
	procGen.setup();

	// setup findY
	findY.setup();

	// setup image
	Image::setup();

	// load in the volume images
	noise0.loadImage(device.Get(), L"Noise/noise0.raw");
	noise1.loadImage(device.Get(), L"Noise/noise1.raw");
	noise2.loadImage(device.Get(), L"Noise/noise2.raw");

	noiseH0.loadImage(device.Get(), L"Noise/noiseH0.raw");
	noiseH1.loadImage(device.Get(), L"Noise/noiseH1.raw");
	noiseH2.loadImage(device.Get(), L"Noise/noiseH2.raw");

	// upload textures
	noise0.uploadTexture();
	noise1.uploadTexture();
	noise2.uploadTexture();

	noiseH0.uploadTexture();
	noiseH1.uploadTexture();
	noiseH2.uploadTexture();

	procGen.regenTerrain();
}

void Graphics::setupDescriptors()
{
	// set the state
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { csuHeap.Get(), samplerHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// setup the resources
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), 0, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), CBV_COUNT, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), CBV_COUNT + SRV_COUNT, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE samplerHandle(samplerHeap->GetGPUDescriptorHandleForHeapStart(), 0, samplerDescriptorSize);

	commandList->SetGraphicsRootDescriptorTable(rpCB, cbvHandle);
	commandList->SetGraphicsRootDescriptorTable(rpSRV, srvHandle);
	commandList->SetGraphicsRootDescriptorTable(rpUAV, uavHandle);
	commandList->SetGraphicsRootDescriptorTable(rpSAMPLER, samplerHandle);
}

void Graphics::populateCommandList()
{
	// set the pipeline state
	if (isSolid)
		commandList->SetPipelineState(renderPipelineSolidState.Get());
	else
		commandList->SetPipelineState(renderPipelineWireframeState.Get());

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	// set the back buffer as the render target

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget[frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	// setup stencil buffer

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart(), DSV_TEX, dsvDescriptorSize);
	commandList->ClearDepthStencilView(
		dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	// set the render target view

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	// set the background

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// draw the terrain
	procGen.drawTerrain();

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
	switch (key)
	{
	case VK_LEFT:

		yAngle += angleSpeed;

		break;
	case VK_RIGHT:

		yAngle -= angleSpeed;

		break;
	case VK_DOWN:

		at.m128_f32[0] -= sin(yAngle) * speed;
		at.m128_f32[2] += cos(yAngle) * speed;

		break;
	case VK_UP:

		at.m128_f32[0] += sin(yAngle) * speed;
		at.m128_f32[2] -= cos(yAngle) * speed;

		break;
	case VK_TAB:

		isSolid = !isSolid;

		break;
	case VK_SPACE:

		system("PAUSE");

		break;
	}
}

void Graphics::onKeyUp(UINT8 key)
{

}