#include <d3dcompiler.h>
#include <iostream>
#include "d3dx12.h"
#include "Graphics.h"
#include "Helper.h"
#include "Window.h"
#include "Shader.h"
#include "Parser.h"
#include "Image.h"
#include "Image2D.h"

#include <ctime>

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

	voxelViewport.Width = DENSITY_SIZE;
	voxelViewport.Height = DENSITY_SIZE;
	voxelViewport.MaxDepth = 1.0f;

	vertSplatViewport.Width = SPLAT_SIZE * 3;
	vertSplatViewport.Height = SPLAT_SIZE;
	vertSplatViewport.MaxDepth = 1.0f;

	viewYViewport.Width = 2;
	viewYViewport.Height = FINDY_SIZE_P1;
	viewYViewport.MaxDepth = 1.0f;

	scissorRect.right = static_cast<LONG>(width);
	scissorRect.bottom = static_cast<LONG>(height);

	voxelScissorRect.right = DENSITY_SIZE;
	voxelScissorRect.bottom = DENSITY_SIZE;

	vertSplatScissorRect.right = SPLAT_SIZE * 3;
	vertSplatScissorRect.bottom = SPLAT_SIZE;

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

void Graphics::genVoxel(XMINT3 pos)
{
	XMFLOAT3 realPos =
		{ pos.x * CHUNK_SIZE - startLoc.x,
		pos.y * CHUNK_SIZE - startLoc.y,
		pos.z * CHUNK_SIZE - startLoc.z };

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

void Graphics::phase1()
{
	// reset the command allocator
	ThrowIfFailed(commandAllocator[frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(commandAllocator[frameIndex].Get(),
		renderPipelineSolidState.Get()));

	setupProceduralDescriptors();

	renderDensity(); // NOTE: causes error
	renderOccupied();

	// run the commands
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// wait on the gpu
	waitForGpu();
}

UINT Graphics::phase2()
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
	ThrowIfFailed(commandAllocator[frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(commandAllocator[frameIndex].Get(),
		renderPipelineSolidState.Get()));

	setupProceduralDescriptors();

	// return the number of approximate verts
	// end early if no verts needed
	renderGenVerts(vertApprox);

	// run the commands
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// wait on the gpu
	waitForGpu();

	return vertApprox;
}

void Graphics::phase3(XMINT3 pos, UINT numOldVerts)
{
	// reset the command allocator
	ThrowIfFailed(commandAllocator[frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(commandAllocator[frameIndex].Get(),
		renderPipelineSolidState.Get()));

	setupProceduralDescriptors();

	// indices
	renderClearTex();
	UINT numVerts = renderVertSplat();
	renderGenIndices(pos, numOldVerts);

	// verts
	renderVertexMesh(pos, numVerts);

	// run the commands
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// wait on the gpu
	waitForGpu();
}

void Graphics::phase4(XMINT3 pos)
{
	// reset the command allocator
	ThrowIfFailed(commandAllocator[frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(commandAllocator[frameIndex].Get(),
		renderPipelineSolidState.Get()));

	setupProceduralDescriptors();

	getIndexData(pos);

	// run the commands
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// wait on the gpu
	waitForGpu();
}

void Graphics::updateTerrain()
{
	// check if need to generate terrain
	XMFLOAT3 curPos{
		at.m128_f32[0],
		at.m128_f32[1],
		at.m128_f32[2]
	};

	XMFLOAT3 voxelPos;
	
	// neg x
	if (curPos.x < currentMid.x * CHUNK_SIZE - startLoc.x)
	{
		XMINT3 curEdge{
			currentMid.x + NUM_VOXELS_X / 2 - 1,
			currentMid.y - NUM_VOXELS_Y / 2,
			currentMid.z - NUM_VOXELS_Z / 2
		};

		XMINT3 newEdge{
			currentMid.x - NUM_VOXELS_X / 2 - 1,
			currentMid.y - NUM_VOXELS_Y / 2,
			currentMid.z - NUM_VOXELS_Z / 2
		};

		// remake terrain
		for (INT y = 0; y < NUM_VOXELS_Y; y++)
		{
			for (INT z = 0; z < NUM_VOXELS_Z; z++)
			{
				XMINT3 oldLoc{
					curEdge.x,
					curEdge.y + y,
					curEdge.z + z
				};

				XMINT3 newLoc{
					newEdge.x,
					newEdge.y + y,
					newEdge.z + z
				};

				// erase the old position
				computedPos.erase(oldLoc);

				// create the new position
				genVoxel(newLoc);
			}
		}

		// set new mid
		currentMid.x--;
	}
	
	// neg y
	if (curPos.y < currentMid.y * CHUNK_SIZE - startLoc.y)
	{
		XMINT3 curEdge{
			currentMid.x - NUM_VOXELS_X / 2,
			currentMid.y + NUM_VOXELS_Y / 2 - 1,
			currentMid.z - NUM_VOXELS_Z / 2
		};

		XMINT3 newEdge{
			currentMid.x - NUM_VOXELS_X / 2,
			currentMid.y - NUM_VOXELS_Y / 2 - 1,
			currentMid.z - NUM_VOXELS_Z / 2
		};

		// remake terrain
		for (INT x = 0; x < NUM_VOXELS_X; x++)
		{
			for (INT z = 0; z < NUM_VOXELS_Z; z++)
			{
				XMINT3 oldLoc{
					curEdge.x + x,
					curEdge.y,
					curEdge.z + z
				};

				XMINT3 newLoc{
					newEdge.x + x,
					newEdge.y,
					newEdge.z + z
				};

				// erase the old position
				computedPos.erase(oldLoc);

				// create the new position
				genVoxel(newLoc);
			}
		}

		// set new mid
		currentMid.y--;
	}
	
	// neg z
	if (curPos.z < currentMid.z * CHUNK_SIZE - startLoc.z)
	{
		XMINT3 curEdge{
			currentMid.x - NUM_VOXELS_X / 2,
			currentMid.y - NUM_VOXELS_Y / 2,
			currentMid.z + NUM_VOXELS_Z / 2 - 1
		};

		XMINT3 newEdge{
			currentMid.x - NUM_VOXELS_X / 2,
			currentMid.y - NUM_VOXELS_Y / 2,
			currentMid.z - NUM_VOXELS_Z / 2 - 1
		};

		// remake terrain
		for (INT y = 0; y < NUM_VOXELS_Y; y++)
		{
			for (INT x = 0; x < NUM_VOXELS_X; x++)
			{
				XMINT3 oldLoc{
					curEdge.x + x,
					curEdge.y + y,
					curEdge.z
				};

				XMINT3 newLoc{
					newEdge.x + x,
					newEdge.y + y,
					newEdge.z
				};

				// erase the old position
				computedPos.erase(oldLoc);

				// create the new position
				genVoxel(newLoc);
			}
		}

		// set new mid
		currentMid.z--;
	}
	
	// pos x
	if ((currentMid.x + 1) * CHUNK_SIZE - startLoc.x < curPos.x)
	{
		XMINT3 curEdge{
			currentMid.x - NUM_VOXELS_X / 2,
			currentMid.y - NUM_VOXELS_Y / 2,
			currentMid.z - NUM_VOXELS_Z / 2
		};

		XMINT3 newEdge{
			currentMid.x + NUM_VOXELS_X / 2,
			currentMid.y - NUM_VOXELS_Y / 2,
			currentMid.z - NUM_VOXELS_Z / 2
		};

		// remake terrain
		for (INT y = 0; y < NUM_VOXELS_Y; y++)
		{
			for (INT z = 0; z < NUM_VOXELS_Z; z++)
			{
				XMINT3 oldLoc{
					curEdge.x,
					curEdge.y + y,
					curEdge.z + z
				};

				XMINT3 newLoc{
					newEdge.x,
					newEdge.y + y,
					newEdge.z + z
				};

				// erase the old position
				computedPos.erase(oldLoc);

				// create the new position
				genVoxel(newLoc);
			}
		}

		// set new mid
		currentMid.x++;
	}

	// pos y
	if ((currentMid.y + 1) * CHUNK_SIZE - startLoc.y < curPos.y)
	{
		XMINT3 curEdge{
			currentMid.x - NUM_VOXELS_X / 2,
			currentMid.y - NUM_VOXELS_Y / 2,
			currentMid.z - NUM_VOXELS_Z / 2
		};

		XMINT3 newEdge{
			currentMid.x - NUM_VOXELS_X / 2,
			currentMid.y + NUM_VOXELS_Y / 2,
			currentMid.z - NUM_VOXELS_Z / 2
		};

		// remake terrain
		for (INT x = 0; x < NUM_VOXELS_X; x++)
		{
			for (INT z = 0; z < NUM_VOXELS_Z; z++)
			{
				XMINT3 oldLoc{
					curEdge.x + x,
					curEdge.y,
					curEdge.z + z
				};

				XMINT3 newLoc{
					newEdge.x + x,
					newEdge.y,
					newEdge.z + z
				};

				// erase the old position
				computedPos.erase(oldLoc);

				// create the new position
				genVoxel(newLoc);
			}
		}

		// set new mid
		currentMid.y++;
	}

	// pos y
	if ((currentMid.z + 1) * CHUNK_SIZE - startLoc.z < curPos.z)
	{
		XMINT3 curEdge{
			currentMid.x - NUM_VOXELS_X / 2,
			currentMid.y - NUM_VOXELS_Y / 2,
			currentMid.z - NUM_VOXELS_Z / 2
		};

		XMINT3 newEdge{
			currentMid.x - NUM_VOXELS_X / 2,
			currentMid.y - NUM_VOXELS_Y / 2,
			currentMid.z + NUM_VOXELS_Z / 2
		};

		// remake terrain
		for (INT y = 0; y < NUM_VOXELS_Y; y++)
		{
			for (INT x = 0; x < NUM_VOXELS_X; x++)
			{
				XMINT3 oldLoc{
					curEdge.x + x,
					curEdge.y + y,
					curEdge.z
				};

				XMINT3 newLoc{
					newEdge.x + x,
					newEdge.y + y,
					newEdge.z
				};

				// erase the old position
				computedPos.erase(oldLoc);

				// create the new position
				genVoxel(newLoc);
			}
		}

		// set new mid
		currentMid.z++;
	}
}

void Graphics::drawPhase()
{
	float y = findY();

	eyeDelta.x = origDelta.x * cos(yAngle)
		- origDelta.z * sin(yAngle);

	eyeDelta.z = origDelta.z * cos(yAngle)
		+ origDelta.x * sin(yAngle);

	at.m128_f32[1] = y;

	eye.m128_f32[0] = at.m128_f32[0] + eyeDelta.x;
	eye.m128_f32[1] = at.m128_f32[1] + eyeDelta.y;
	eye.m128_f32[2] = at.m128_f32[2] + eyeDelta.z;

	updateTerrain();
	
	//regenTerrain();

	// reset the command allocator
	ThrowIfFailed(commandAllocator[frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(commandAllocator[frameIndex].Get(),
		renderPipelineSolidState.Get()));

	setupProceduralDescriptors();

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
	rtvHeapDesc.NumDescriptors = numFrames + SRV_COUNT;
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
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&voxelTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr,
		IID_PPV_ARGS(&intermediateTarget[DENSITY_TEXTURE])
	));

	device->CreateRenderTargetView(intermediateTarget[DENSITY_TEXTURE].Get(), &rtvDesc, rtvHandle);
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
		intermediateTarget[DENSITY_TEXTURE].Get(),
		&srvDesc, srvHandle0);
	srvHandle0.Offset(csuDescriptorSize);

	// SRV 2
	
	voxelTextureDesc.Format = INDEX_FORMAT;
	voxelTextureDesc.Width = SPLAT_SIZE * 3;
	voxelTextureDesc.Height = SPLAT_SIZE;
	voxelTextureDesc.DepthOrArraySize = SPLAT_SIZE;

	rtvDesc.Format = INDEX_FORMAT;
	rtvDesc.Texture3D.WSize = voxelTextureDesc.DepthOrArraySize;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&voxelTextureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr,
		IID_PPV_ARGS(&intermediateTarget[INDEX_TEXTURE])
	));
	
	device->CreateRenderTargetView(intermediateTarget[INDEX_TEXTURE].Get(), &rtvDesc, rtvHandle);
	rtvHandle.Offset(1, rtvDescriptorSize);

	srvDesc.Format = INDEX_FORMAT;

	// bind texture to srv
	device->CreateShaderResourceView(
		intermediateTarget[INDEX_TEXTURE].Get(),
		&srvDesc, srvHandle0);
	srvHandle0.Offset(csuDescriptorSize);

	// SRV 3

	voxelTextureDesc.Format = DENSITY_FORMAT;
	voxelTextureDesc.Width = 2;
	voxelTextureDesc.Height = FINDY_SIZE_P1;
	voxelTextureDesc.DepthOrArraySize = 2;

	rtvDesc.Format = DENSITY_FORMAT;
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
		IID_PPV_ARGS(&intermediateTarget[FINDY_TEXTURE])
	));

	device->CreateRenderTargetView(intermediateTarget[FINDY_TEXTURE].Get(), &rtvDesc, rtvHandle);
	rtvHandle.Offset(1, rtvDescriptorSize);

	srvDesc.Format = DENSITY_FORMAT;

	// bind texture to srv
	device->CreateShaderResourceView(
		intermediateTarget[FINDY_TEXTURE].Get(),
		&srvDesc, srvHandle0);
	srvHandle0.Offset(csuDescriptorSize);

	// Instance SRV
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = 1;
	srvDesc.Buffer.StructureByteStride = sizeof(UINT);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(UINT) + 255) & ~255),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&bufferSRV[INSTANCE])));

	// bind texture to srv
	device->CreateShaderResourceView(
		bufferSRV[INSTANCE].Get(),
		&srvDesc, srvHandle0);
	srvHandle0.Offset(csuDescriptorSize);

	// setup the image loader
	Image::initDevil();
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
	
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&dsvTex,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		nullptr,
		IID_PPV_ARGS(&bufferDSV[DSV_TEX])));

	device->CreateDepthStencilView(bufferDSV[DSV_TEX].Get(),
		nullptr, dsvHandle);
	dsvHandle.Offset(dsvDescriptorSize);

	// create sampler
	CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle0(samplerHeap->GetCPUDescriptorHandleForHeapStart(), 0, samplerDescriptorSize);
	
	// create sampler
	D3D12_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
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

	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

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
	bufferCB[CB_WORLD_POS]->Map(0, &readRange, (void**)&worldPosCB);

	cdesc.BufferLocation = bufferCB[CB_WORLD_POS]->GetGPUVirtualAddress();
	cdesc.SizeInBytes = (sizeof(WORLD_POS) + 255) & ~255;

	device->CreateConstantBufferView(&cdesc, cbvHandle0);
	cbvHandle0.Offset(csuDescriptorSize);

	// IS NOT UNMAPPED

	bufferCB[CB_VOXEL_POS]->Map(0, &readRange, (void**)&voxelPosData);

	voxelPosData->densityType = 0;
	voxelPosData->renderType = 0;

	// IS NOT UNMAPPED

	cdesc.BufferLocation = bufferCB[CB_VOXEL_POS]->GetGPUVirtualAddress();
	cdesc.SizeInBytes = (sizeof(VOXEL_POS) + 255) & ~255;

	device->CreateConstantBufferView(&cdesc, cbvHandle0);
	cbvHandle0.Offset(csuDescriptorSize);

	// setup YPOS var for UAV

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT)),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&yposMap)));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT),
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&bufferUAV[UAV_YPOS])));

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
	uavDesc.Buffer.StructureByteStride = sizeof(UINT);

	device->CreateUnorderedAccessView(bufferUAV[UAV_YPOS].Get(), nullptr, &uavDesc, uavHandle0);
	uavHandle0.Offset(1, csuDescriptorSize);

	uavDesc.Buffer.NumElements = 1;
	uavDesc.Buffer.StructureByteStride = sizeof(BOOL);

	device->CreateUnorderedAccessView(bufferUAV[DEBUG_VAR].Get(), nullptr, &uavDesc, uavHandle0);
	uavHandle0.Offset(1, csuDescriptorSize);

	// create tmp buffers for voxel generation

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(MAX_BUFFER_SIZE + COUNTER_SIZE),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr,
		IID_PPV_ARGS(&vertexFrontBuffer)));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(MAX_BUFFER_SIZE + COUNTER_SIZE),
		D3D12_RESOURCE_STATE_STREAM_OUT,
		nullptr,
		IID_PPV_ARGS(&vertexBackBuffer)));

	// setup index
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT)),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&indexCount)));

	// setup vertex count
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT)),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vertexCount)));

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
	string uploadVS, uploadPS, uploadGS,
		sampleDataDensityVS,
		dataDensityVS, dataDensityPS, dataDensityGS,
		dataOccupiedVS, dataOccupiedGS,
		dataGenVertsVS, dataGenVertsGS,
		dataVertexMeshVS, dataVertexMeshGS,
		dataClearTexVS, dataClearTexPS, dataClearTexGS,
		dataVertSplatVS, dataVertSplatPS, dataVertSplatGS,
		dataGenIndicesMeshVS, dataGenIndicesMeshGS,
		dataVS, dataPS, dataCS[CS_COUNT];

	// release optimizations currently break shaders currently so prefer debug versions
#ifndef DEBUG_
	ThrowIfFailed(ReadCSO("../x64/Debug/SampleDensityVS.cso", sampleDataDensityVS));

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

	ThrowIfFailed(ReadCSO("../x64/Debug/RenderVS.cso", dataVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/RenderPS.cso", dataPS));

	ThrowIfFailed(ReadCSO("../x64/Debug/UploadVS.cso", uploadVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/UploadPS.cso", uploadPS));
	ThrowIfFailed(ReadCSO("../x64/Debug/UploadGS.cso", uploadGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/SearchTerrainCS.cso", dataCS[CS_SEARCH_TERRAIN]));
#else
	ThrowIfFailed(ReadCSO("../x64/Release/SampleDensityVS.cso", sampleDataDensityVS));

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
	
	// vertex input layout

	const D3D12_INPUT_ELEMENT_DESC densityLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { densityLayout, _countof(densityLayout) };
	psoDesc.pRootSignature = rootSignature.Get();
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
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&densityPipelineState)));

	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)sampleDataDensityVS.c_str()), sampleDataDensityVS.length() };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&sampleDensityPipelineState)));

	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataClearTexVS.c_str()), dataClearTexVS.length() };
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)dataClearTexPS.c_str()), dataClearTexPS.length() };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataClearTexGS.c_str()), dataClearTexGS.length() };
	psoDesc.RTVFormats[0] = INDEX_FORMAT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&dataClearTexPipelineState)));

	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)uploadVS.c_str()), uploadVS.length() };
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)uploadPS.c_str()), uploadPS.length() };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)uploadGS.c_str()), uploadGS.length() };
	psoDesc.RTVFormats[0] = Image::TEX_FORMAT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&uploadTexPipelineState)));

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
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&dataVertSplatPipelineState)));

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

	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataGenIndicesMeshVS.c_str()), dataGenIndicesMeshVS.length() };
	psoDesc.PS = { 0, 0 };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)dataGenIndicesMeshGS.c_str()), dataGenIndicesMeshGS.length() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&dataGenIndicesPipelineState)));

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
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&vertexMeshPipelineState)));
	
	// setup for render pipeline
	const D3D12_INPUT_ELEMENT_DESC layoutRender[] =
	{
		{ "SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT , 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 }
	};

	psoDesc.InputLayout = { layoutRender, _countof(layoutRender) };
	psoDesc.StreamOutput.NumEntries = 0;
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)dataVS.c_str()), dataVS.length() };
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)dataPS.c_str()), dataPS.length() };
	psoDesc.GS = { 0, 0 };
	
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

	// create the compute pipeline (radix sort)
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};

	computePsoDesc.pRootSignature = computeRootSignature.Get();

	for (unsigned int i = 0; i < CS_COUNT; i++)
	{
		computePsoDesc.CS = { reinterpret_cast<UINT8*>((void*)dataCS[i].c_str()), dataCS[i].length() };

		ThrowIfFailed(device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&computeStateCS[i])));
	}

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

	// create command list

	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex].Get(), nullptr, IID_PPV_ARGS(&commandList)));

	// load in the 2D images
	altitude.loadImage(device.Get(), L"Altitude/Mountain Texture 4.bmp");
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

	// load in the volume images
	noise0.setPipeline(uploadTexPipelineState.Get());
	noise1.setPipeline(uploadTexPipelineState.Get());
	noise2.setPipeline(uploadTexPipelineState.Get());

	noise0.loadImage(device.Get(), L"Noise/noise0.raw");
	noise1.loadImage(device.Get(), L"Noise/noise1.raw");
	noise2.loadImage(device.Get(), L"Noise/noise2.raw");

	// upload textures
	noise0.uploadTexture(this, commandList.Get());
	noise1.uploadTexture(this, commandList.Get());
	noise2.uploadTexture(this, commandList.Get());

	regenTerrain();
}

void Graphics::regenTerrain()
{
	// reset camera
	yAngle = 0;
	eyeDelta = origDelta;
	at = { eyeDelta.x, eyeDelta.y, eyeDelta.z };

	// clear the hashmap
	computedPos.clear();

	// generate the terrain

	// record the render commands

	XMFLOAT3 voxelPos;
	startLoc =
	{ at.m128_f32[0] + NUM_VOXELS_X * CHUNK_SIZE / 2.f,
		at.m128_f32[1] + NUM_VOXELS_Y * CHUNK_SIZE / 2.f,
		at.m128_f32[2] + NUM_VOXELS_Z *  CHUNK_SIZE / 2.f };

	clock_t startGen = std::clock_t();

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

	currentMid = XMINT3(
		NUM_VOXELS_X / 2,
		NUM_VOXELS_Y / 2,
		NUM_VOXELS_Z / 2
	);

	double delta = (std::clock() - startGen) / (double)CLOCKS_PER_SEC;
	//cout << "CREATION TIME: " << delta << endl;
}

void Graphics::setupProceduralDescriptors()
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

	commandList->RSSetViewports(1, &voxelViewport);
	commandList->RSSetScissorRects(1, &voxelScissorRect);
}

void Graphics::renderDensity()
{
	// set the pipeline
	commandList->SetPipelineState(densityPipelineState.Get());

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), numFrames + DENSITY_TEXTURE, rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &plainVB);
	commandList->DrawInstanced(_countof(plainVerts), OCC_SIZE, 0, 0);

	// TODO: CHECK IF THIS IS RIGHT
	// wait for shader
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(NULL));
}

void Graphics::renderOccupied()
{
	// set the pipeline state
	commandList->SetPipelineState(occupiedPipelineState.Get());

	// clear out the filled size location
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexFrontBuffer.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST));

	commandList->CopyBufferRegion(vertexFrontBuffer.Get(),
		MAX_BUFFER_SIZE,
		zeroBuffer.Get(),
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

void Graphics::renderGenVerts(UINT vertApprox)
{
	// set the pipeline state
	commandList->SetPipelineState(genVertsPipelineState.Get());

	// clear out the filled size location
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBackBuffer.Get(),
		D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST));

	commandList->CopyBufferRegion(vertexBackBuffer.Get(),
		MAX_BUFFER_SIZE,
		zeroBuffer.Get(),
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

void Graphics::renderVertexMesh(XMINT3 pos, UINT numVerts)
{
	// set the pipeline state
	commandList->SetPipelineState(vertexMeshPipelineState.Get());

	D3D12_VERTEX_BUFFER_VIEW vertBuffer;
	vertBuffer.BufferLocation = vertexBackBuffer->GetGPUVirtualAddress();
	vertBuffer.StrideInBytes = sizeof(BITPOS);
	vertBuffer.SizeInBytes = MAX_BUFFER_SIZE;

	// create out buffer
	ThrowIfFailed(device->CreateCommittedResource(
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

void Graphics::renderClearTex()
{
	// set the pipeline
	commandList->SetPipelineState(dataClearTexPipelineState.Get());

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), numFrames + INDEX_TEXTURE, rtvDescriptorSize);
	commandList->SOSetTargets(0, 0, 0);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &plainVB);
	commandList->RSSetViewports(1, &vertSplatViewport);
	commandList->RSSetScissorRects(1, &vertSplatScissorRect);
	commandList->DrawInstanced(_countof(plainVerts), SPLAT_SIZE, 0, 0);

	// TODO: CHECK IF THIS IS RIGHT
	// wait for shader
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(NULL));
}

UINT Graphics::renderVertSplat()
{
	// set the pipeline
	commandList->SetPipelineState(dataVertSplatPipelineState.Get());

	D3D12_VERTEX_BUFFER_VIEW vertBuffer;
	vertBuffer.BufferLocation = vertexBackBuffer->GetGPUVirtualAddress();
	vertBuffer.StrideInBytes = sizeof(BITPOS);
	vertBuffer.SizeInBytes = MAX_BUFFER_SIZE;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		numFrames + INDEX_TEXTURE, rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList->IASetVertexBuffers(0, 1, &vertBuffer);

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

void Graphics::renderGenIndices(XMINT3 pos, UINT minIndices)
{
	// set the pipeline
	commandList->SetPipelineState(dataGenIndicesPipelineState.Get());

	// create out buffer
	ThrowIfFailed(device->CreateCommittedResource(
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

// TODO: remove :)
void Graphics::getIndexData(XMINT3 pos)
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

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vertBuffer);
		commandList->IASetIndexBuffer(&indexBufferView);
		commandList->SOSetTargets(0, 0, nullptr);
		commandList->DrawIndexedInstanced(p.second.numIndices, 1, 0, 0, 0);
	}

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
	case 'R':
		voxelPosData->renderType = (voxelPosData->renderType + 1) % NUM_RENDER_TYPES;
		break;
	case 'T':

		voxelPosData->densityType = (voxelPosData->densityType + 1) % NUM_DENSITY_TYPES;
		regenTerrain();

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