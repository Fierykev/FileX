#include "FindY.h"
#include "Graphics.h"

using namespace std;

static float pos2Voxel = 1.f / CHUNK_SIZE;

void FindY::setup()
{
	string sampleDataDensityVS,
		sampleDataDensityPS,
		sampleDataDensityGS,
		searchTerrainCS;

	// load shaders
#ifdef _DEBUG
	ThrowIfFailed(ReadCSO("../x64/Debug/SampleDensityVS.cso", sampleDataDensityVS));
	ThrowIfFailed(ReadCSO("../x64/Debug/SampleDensityPS.cso", sampleDataDensityPS));
	ThrowIfFailed(ReadCSO("../x64/Debug/SampleDensityGS.cso", sampleDataDensityGS));

	ThrowIfFailed(ReadCSO("../x64/Debug/SearchTerrainCS.cso", searchTerrainCS));
#else
	ThrowIfFailed(ReadCSO("../x64/Release/SampleDensityVS.cso", sampleDataDensityVS));
	ThrowIfFailed(ReadCSO("../x64/Release/SampleDensityPS.cso", sampleDataDensityPS));
	ThrowIfFailed(ReadCSO("../x64/Release/SampleDensityGS.cso", sampleDataDensityGS));

	ThrowIfFailed(ReadCSO("../x64/Release/SearchTerrainCS.cso", searchTerrainCS));
#endif

	// setup render stages
	const D3D12_INPUT_ELEMENT_DESC findyLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { findyLayout, _countof(findyLayout) };
	psoDesc.pRootSignature = Graphics::rootSignature.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.RTVFormats[0] = DENSITY_FORMAT;
	psoDesc.PS = { reinterpret_cast<UINT8*>((void*)sampleDataDensityPS.c_str()), sampleDataDensityPS.length() };
	psoDesc.GS = { reinterpret_cast<UINT8*>((void*)sampleDataDensityGS.c_str()), sampleDataDensityGS.length() };
	psoDesc.VS = { reinterpret_cast<UINT8*>((void*)sampleDataDensityVS.c_str()), sampleDataDensityVS.length() };
	ThrowIfFailed(Graphics::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&sampleDensityPipelineState)));

	// create compute stage
	// create the compute pipeline
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};

	computePsoDesc.pRootSignature = Graphics::computeRootSignature.Get();
	computePsoDesc.CS = { reinterpret_cast<UINT8*>((void*)searchTerrainCS.c_str()), searchTerrainCS.length() };
	ThrowIfFailed(Graphics::device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&searchTerrainPipelineState)));

	// setup YPOS var for UAV
	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT)),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&yposMap)));

	ThrowIfFailed(Graphics::device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT),
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&Graphics::bufferUAV[Graphics::UAV_YPOS])));

	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle0(Graphics::csuHeap->GetCPUDescriptorHandleForHeapStart(),
		Graphics::CBV_COUNT + Graphics::SRV_COUNT, Graphics::csuDescriptorSize);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	uavDesc.Buffer.NumElements = 1;
	uavDesc.Buffer.StructureByteStride = sizeof(UINT);

	Graphics::device->CreateUnorderedAccessView(Graphics::bufferUAV[Graphics::UAV_YPOS].Get(), nullptr, &uavDesc, uavHandle0);
	uavHandle0.Offset(1, Graphics::csuDescriptorSize);

	// setup commandlist
	commandList = Graphics::commandList;
}

void FindY::sampleDensity()
{
	// set the pipeline (NOT NEEDED)
	//commandList->SetPipelineState(sampleDensityPipelineState.Get());

	commandList->RSSetViewports(1, &Graphics::viewYViewport);
	commandList->RSSetScissorRects(1, &Graphics::viewYScissorRect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(Graphics::rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		Graphics::numFrames + Graphics::FINDY_TEXTURE, Graphics::rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &Graphics::plainVB);
	commandList->DrawInstanced(_countof(Graphics::plainVerts), 2, 0, 0);

	// TODO: CHECK IF THIS IS RIGHT
	// wait for shader
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(NULL));
}

void FindY::searchTerrain()
{
	commandList->SetComputeRootSignature(Graphics::computeRootSignature.Get());

	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(Graphics::csuHeap->GetGPUDescriptorHandleForHeapStart(), 0, Graphics::csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(Graphics::csuHeap->GetGPUDescriptorHandleForHeapStart(), Graphics::CBV_COUNT, Graphics::csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(Graphics::csuHeap->GetGPUDescriptorHandleForHeapStart(), Graphics::CBV_COUNT + Graphics::SRV_COUNT, Graphics::csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE samplerHandle(Graphics::samplerHeap->GetGPUDescriptorHandleForHeapStart(), 0, Graphics::samplerDescriptorSize);

	commandList->SetComputeRootDescriptorTable(Graphics::rpCB, cbvHandle);
	commandList->SetComputeRootDescriptorTable(Graphics::rpSRV, srvHandle);
	commandList->SetComputeRootDescriptorTable(Graphics::rpUAV, uavHandle);
	commandList->SetComputeRootDescriptorTable(Graphics::rpSAMPLER, samplerHandle);

	commandList->SetPipelineState(searchTerrainPipelineState.Get());
	commandList->Dispatch(1, 1, 1);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Graphics::bufferUAV[Graphics::UAV_YPOS].Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	// copy UAV
	commandList->CopyBufferRegion(yposMap.Get(),
		0,
		Graphics::bufferUAV[Graphics::UAV_YPOS].Get(),
		0, sizeof(UINT));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Graphics::bufferUAV[Graphics::UAV_YPOS].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

void FindY::findYRender()
{
	// reset the command allocator
	ThrowIfFailed(Graphics::commandAllocator[Graphics::frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(Graphics::commandAllocator[Graphics::frameIndex].Get(),
		sampleDensityPipelineState.Get()));

	Graphics::setupDescriptors();

	sampleDensity();
	searchTerrain();

	// run the commands
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	Graphics::commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// wait on the gpu
	Graphics::waitForGpu();
}

float FindY::findY(XMVECTOR pos)
{
	UINT* tmpPos;
	UINT ypos;
	int type = 0;

	while (true)
	{
		ProcGen::voxelPosData->voxelPos =
			XMFLOAT3(pos.m128_f32[0],
				pos.m128_f32[1] - PERSON_HEIGHT,
				pos.m128_f32[2]);

		// render the voxel
		findYRender();

		CD3DX12_RANGE readRange(0, sizeof(UINT));
		yposMap->Map(0, &readRange, (void**)&tmpPos);

		ypos = *tmpPos;

		yposMap->Unmap(0, nullptr);

		if ((ypos >> 30) & 0x1)
		{
			if ((ypos >> 31) & 0x1)
			{			
				/*if (type == 1)
				{
					ypos = 0;
					break;
				}*/

				pos.m128_f32[1] -= ((FINDY_SIZE - 1)* SAMP_EXPANSION) / 2.f;
				type = 1;
			}
			else
			{/*
				if (type == 1)
				{
					ypos = 128;
					break;
				}*/

				pos.m128_f32[1] += ((FINDY_SIZE - 1) * SAMP_EXPANSION) / 2.f;
				type = 2;
			}
		}
		else // done
			break;

		//break;
	}
	/*
	// fetch the data
	cout << ypos << " "
		<< pos.m128_f32[0] << " "
		<< pos.m128_f32[1] << " "
		<< pos.m128_f32[2]
		<< endl;*/

	return pos.m128_f32[1] + ypos * SAMP_EXPANSION / 2.f + PERSON_HEIGHT;
}