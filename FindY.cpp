#include <iostream>
#include "Graphics.h"
#include "Helper.h"
#include "Window.h"
#include "d3dx12.h"

using namespace std;

static float pos2Voxel = 1.f / CHUNK_SIZE;

void Graphics::sampleDensity()
{
	// set the pipeline
	commandList->SetPipelineState(sampleDensityPipelineState.Get());

	commandList->RSSetViewports(1, &viewYViewport);
	commandList->RSSetScissorRects(1, &viewYScissorRect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		numFrames + FINDY_TEXTURE, rtvDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &plainVB);
	commandList->DrawInstanced(_countof(plainVerts), 2, 0, 0);

	// TODO: CHECK IF THIS IS RIGHT
	// wait for shader
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(NULL));
}

void Graphics::searchTerrain()
{
	commandList->SetComputeRootSignature(computeRootSignature.Get());

	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), 0, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), CBV_COUNT, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(csuHeap->GetGPUDescriptorHandleForHeapStart(), CBV_COUNT + SRV_COUNT, csuDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE samplerHandle(samplerHeap->GetGPUDescriptorHandleForHeapStart(), 0, samplerDescriptorSize);

	commandList->SetComputeRootDescriptorTable(rpCB, cbvHandle);
	commandList->SetComputeRootDescriptorTable(rpSRV, srvHandle);
	commandList->SetComputeRootDescriptorTable(rpUAV, uavHandle);
	commandList->SetComputeRootDescriptorTable(rpSAMPLER, samplerHandle);

	commandList->SetPipelineState(computeStateCS[CS_SEARCH_TERRAIN].Get());
	commandList->Dispatch(1, 1, 1);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(bufferUAV[UAV_YPOS].Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	// copy UAV
	commandList->CopyBufferRegion(yposMap.Get(),
		0,
		bufferUAV[UAV_YPOS].Get(),
		0, sizeof(UINT));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(bufferUAV[UAV_YPOS].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

void Graphics::findYRender()
{
	// reset the command allocator
	ThrowIfFailed(commandAllocator[frameIndex]->Reset());

	// reset the command list
	ThrowIfFailed(commandList->Reset(commandAllocator[frameIndex].Get(),
		renderPipelineSolidState.Get()));

	setupProceduralDescriptors();

	sampleDensity();
	searchTerrain();

	// run the commands
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// wait on the gpu
	waitForGpu();
}

float Graphics::findY()
{
	UINT* tmpPos;
	UINT ypos;
	XMVECTOR pos = at;
	int type = 0;

	while (true)
	{
		voxelPosData->voxelPos =
			XMFLOAT4(pos.m128_f32[0],
				pos.m128_f32[1],
				pos.m128_f32[2], 0);

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
				if (type == 1)
				{
					ypos = 0;
					break;
				}

				pos.m128_f32[1] -= SAMP_EXPANSION;
				type = 1;
			}
			else
			{
				if (type == 1)
				{
					ypos = 128;
					break;
				}

				pos.m128_f32[1] += SAMP_EXPANSION;
				type = 2;
			}
		}
		else // done
			break;

		//break;
	}

	// fetch the data
	cout << ypos << " "
		<< pos.m128_f32[0] << " "
		<< pos.m128_f32[1] << " "
		<< pos.m128_f32[2]
		<< endl;

	return pos.m128_f32[1] + ypos * SAMP_EXPANSION / 2.f + PERSON_HEIGHT;
}