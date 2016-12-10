#ifndef GRPAHICS_H
#define GRAPHICS_H

#include <unordered_map>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "Manager.h"
#include "Image.h"
#include "Image2D.h"

#define NUM_VOXELS_X 8
#define NUM_VOXELS_Y 8
#define NUM_VOXELS_Z 14
#define NUM_VOXELS (NUM_VOXELS_X * NUM_VOXELS_Y * NUM_VOXELS_Z)

#define NUM_RENDER_TYPES 4
#define NUM_DENSITY_TYPES 4

using namespace Microsoft::WRL;
using namespace DirectX;

constexpr float PERSON_HEIGHT = 0.f;
constexpr float CHUNK_SIZE = 80.f;

constexpr float EXTRA = 4.f;
constexpr float VOXEL_SIZE = 33.f;
constexpr float VOXEL_SIZE_M1 = VOXEL_SIZE - 1.f;
constexpr float VOXEL_SIZE_M2 = VOXEL_SIZE_M1 - 1.f;
constexpr float VOXEL_SIZE_P1 = VOXEL_SIZE + 1.f;
constexpr float OCC_SIZE = VOXEL_SIZE + EXTRA * 2.f;
constexpr float OCC_SIZE_P1 = OCC_SIZE + 1;
constexpr float OCC_SIZE_M1 = OCC_SIZE - 1;

/*
#define EXTRA 4.f
#define VOXEL_SIZE_M2 63.f
#define VOXEL_SIZE_M1 64.f
#define VOXEL_SIZE 65.f
#define VOXEL_SIZE_P1 66.f
#define OCC_SIZE 73.f
#define OCC_SIZE_P1 74.f
#define OCC_SIZE_M1 72.f*/

constexpr float FINDY_SIZE = 256.f;
constexpr float FINDY_SIZE_P1 = 257.f;
constexpr float SAMP_EXPANSION = VOXEL_SIZE / CHUNK_SIZE;

#define SRVS_PER_FRAME 2

#define DENSITY_FORMAT DXGI_FORMAT_R32_FLOAT
#define INDEX_FORMAT DXGI_FORMAT_R32_UINT
#define DSV_FORMAT DXGI_FORMAT_D32_FLOAT

#define CAM_DELTA .1f

namespace std
{
	template <> struct hash<XMINT3>
	{
		size_t operator()(const XMINT3& data) const
		{
			return std::hash<INT>{}(data.x)
				^ std::hash<INT>{}(data.y)
				^ std::hash<INT>{}(data.z);
		}
	};

	inline bool operator== (const XMINT3& a, const XMINT3& b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}
}

constexpr UINT COUNTER_SIZE = sizeof(UINT64);
constexpr float speed = 4.f, angleSpeed = .03f;

class Graphics : public Manager
{
public:
	// use Manager's constructor
	Graphics(std::wstring title, unsigned int width, unsigned int height);

	virtual void onInit();
	virtual void onUpdate();
	virtual void onRender();
	virtual void onDestroy();
	virtual void onKeyDown(UINT8 key);
	virtual void onKeyUp(UINT8 key);

	const UINT DENSITY_SIZE = OCC_SIZE;

	const UINT OCCUPIED_SIZE = VOXEL_SIZE_P1;
	const UINT NUM_POINTS = OCCUPIED_SIZE * OCCUPIED_SIZE;
	const UINT BYTES_POINTS = sizeof(OCCUPIED_POINT) * sizeof(OCCUPIED_POINT) *	NUM_POINTS;

	const UINT SPLAT_SIZE = VOXEL_SIZE_P1;

	// TODO: check if need to throw more memory at gpu
	const UINT MAX_BUFFER_SIZE =
		((((UINT)VOXEL_SIZE_P1 * (UINT)VOXEL_SIZE_P1 * (UINT)VOXEL_SIZE_P1) >> 2) << 2) * sizeof(VERT_OUT);

	const UINT MAX_INDEX_SIZE =
		((((UINT)VOXEL_SIZE_P1 * (UINT)VOXEL_SIZE_P1 * (UINT)VOXEL_SIZE_P1) >> 2) << 2) * sizeof(UINT);

//private:
	// methods
	void loadPipeline();
	void loadAssets();
	void populateCommandList();
	void moveToNextFrame();
	void waitForGpu();

	// added methods
	void regenTerrain();
	void setupProceduralDescriptors();
	void renderDensity(UINT index);
	void renderOccupied(UINT index);
	bool renderGenVerts(UINT index);
	void renderVertexMesh(UINT index);
	void renderClearTex(UINT index);
	void renderVertSplat(UINT index);
	void renderGenIndices(UINT index);
	void getVertIndexData(UINT index);
	float findY();
	void findYRender();
	void searchTerrain();
	void sampleDensity();
	void genVoxel(XMFLOAT3 pos, UINT index);
	void phase1(UINT index);
	bool phase2(UINT index);
	void phase3(UINT index);
	void phase4(UINT index);
	void updateTerrain();
	void drawPhase();

	enum ComputeShader : UINT32
	{
		CS_SEARCH_TERRAIN = 0,
		CS_COUNT
	};

	// setup the srv and uav enums
	enum BVCB : UINT32
	{
		CB_WORLD_POS = 0,
		CB_VOXEL_POS,
		CBV_COUNT
	};

	enum BVSRV : UINT32
	{
		DENSITY_TEXTURE = 0,
		INDEX_TEXTURE,
		FINDY_TEXTURE,
		INSTANCE,
		NOISE_0,
		NOISE_1,
		NOISE_2,
		UPLOAD_TEX,
		ALTITUDE,
		BUMPMAP,
		SRV_COUNT
	};

	enum BVUAV : UINT32
	{
		UAV_YPOS = 0,
		DEBUG_VAR,
		UAV_COUNT
	};

	enum BVDSV : UINT32
	{
		DSV_TEX = 0,
		DSV_COUNT
	};

	enum BVSAMPLER :UINT32
	{
		NEAREST_SAMPLER = 0,
		REPEATE_SAMPLER,
		SAMPLER_COUNT
	};

	// static globals
	static const UINT numFrames = 2;

	// buffers
	ComPtr<ID3D12Resource> bufferCB[CBV_COUNT],
		bufferSRV[SRV_COUNT],
		bufferUAV[UAV_COUNT],
		bufferDSV[DSV_COUNT],
		zeroBuffer, plainVCB, pointVCB,
		vertexBuffer[NUM_VOXELS], vertexBackBuffer,
		indexBuffer[NUM_VOXELS],
		vertexCount,
		indexCount, yposMap;

	unordered_map<XMINT3, UINT> computedPos;
	XMFLOAT3 startLoc;
	XMINT3 currentMid;

	D3D12_VERTEX_BUFFER_VIEW plainVB, pointVB;

	struct PLAIN_VERTEX
	{
		XMFLOAT3 position;
		XMFLOAT2 texcoord;
	};

	struct OCCUPIED_POINT
	{
		XMFLOAT2 position;
		XMUINT2 uv;
	};

	struct BITPOS
	{
		UINT bitpos;
	};

	struct POLY_CONSTANTS
	{
		INT numberPolygons[256];
	};

	struct EDGE_CONSTANTS
	{
		XMINT4 edgeNumber[256][5];
	};

	struct VERT_OUT
	{
		XMFLOAT4 position;
		XMFLOAT3 normal;
		XMFLOAT3 texcoord;
	};

	PLAIN_VERTEX plainVerts[6] = {
		PLAIN_VERTEX{ XMFLOAT3(1, 1, 0), XMFLOAT2(1, 1) },
		PLAIN_VERTEX{ XMFLOAT3(1, -1, 0), XMFLOAT2(1, 0) },
		PLAIN_VERTEX{ XMFLOAT3(-1, 1, 0), XMFLOAT2(0, 1) },
		PLAIN_VERTEX{ XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 0) },
		PLAIN_VERTEX{ XMFLOAT3(-1, 1, 0), XMFLOAT2(0, 1) },
		PLAIN_VERTEX{ XMFLOAT3(1, -1, 0), XMFLOAT2(1, 0) }
	};

	struct INTERMEDIATE_VB
	{
		XMFLOAT4 position;
		UINT bitPoints;
	};

	// verts
	UINT vertCount[NUM_VOXELS],
		indCount[NUM_VOXELS];

	// fences
	UINT frameIndex;
	UINT64 fenceVal[numFrames];

	// pipeline objects
	D3D12_VIEWPORT viewport, voxelViewport, vertSplatViewport, viewYViewport;
	ComPtr<ID3D12Device> device;
	D3D12_RECT scissorRect, voxelScissorRect, vertSplatScissorRect, viewYScissorRect;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12RootSignature> computeRootSignature;
	ComPtr<ID3D12PipelineState> densityPipelineState,
		occupiedPipelineState,
		genVertsPipelineState,
		renderPipelineSolidState,
		renderPipelineWireframeState,
		vertexMeshPipelineState,
		dataVertSplatPipelineState,
		dataGenIndicesPipelineState,
		dataClearTexPipelineState,
		sampleDensityPipelineState,
		uploadTexPipelineState;
	ComPtr<ID3D12PipelineState> computeStatePR;
	ComPtr<ID3D12PipelineState> computeStateMC;
	ComPtr<ID3D12PipelineState> computeStateCS[CS_COUNT];
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<IDXGISwapChain3> swapChain;

	// per frame vars
	ComPtr<ID3D12Resource> renderTarget[numFrames], intermediateTarget[SRV_COUNT];
	ComPtr<ID3D12CommandAllocator> commandAllocator[numFrames];

	// heaps
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ComPtr<ID3D12DescriptorHeap> csuHeap;
	ComPtr<ID3D12DescriptorHeap> samplerHeap;
	UINT rtvDescriptorSize;
	UINT dsvDescriptorSize;
	UINT csuDescriptorSize;
	UINT samplerDescriptorSize;

	// synchronized objects
	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent;
	HANDLE swapChainEvent;

	// Images
	Image noise0, noise1, noise2;
	Image2D altitude, bumpMap;

	struct WORLD_POS
	{
		XMMATRIX world;
		XMMATRIX worldView;
		XMMATRIX worldViewProjection;
	};

	struct VOXEL_POS
	{
		XMFLOAT4 voxelPos;
		UINT densityType;
		UINT renderType;
	};

	bool isSolid = false;

	// constant buffers
	VOXEL_POS* voxelPosData;
	WORLD_POS* worldPosCB;

	// view params
	XMMATRIX world, view, projection, worldViewProjection;

	XMFLOAT3 origDelta{ 0.0f, 100.0f, 200.0f };
	XMFLOAT3 eyeDelta = origDelta;
	XMVECTOR at{ eyeDelta.x, eyeDelta.y, eyeDelta.z };
	XMVECTOR eye = { 0, 0, 0 };
	XMVECTOR up{ 0.0f, 1.f, 0.0f };

	float yAngle = 0, xAngle = 0;

	// locations in the root parameter table
	enum RootParameters : UINT32
	{
		rpCB = 0,
		rpSRV,
		rpUAV,
		rpSAMPLER,
		rpCount
	};
};

#endif