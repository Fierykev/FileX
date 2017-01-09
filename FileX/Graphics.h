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
#define NUM_VOXELS_Z 8
#define NUM_VOXELS (NUM_VOXELS_X * NUM_VOXELS_Y * NUM_VOXELS_Z)

#define NUM_RENDER_TYPES 4
#define NUM_DENSITY_TYPES 4

using namespace Microsoft::WRL;
using namespace DirectX;

constexpr float PERSON_HEIGHT = 0.f;
constexpr float CHUNK_SIZE = 4.f;

constexpr float EXTRA = 5.f;
constexpr float VOXEL_SIZE = 65.f;
constexpr float VOXEL_SIZE_M1 = VOXEL_SIZE - 1.f;
constexpr float VOXEL_SIZE_M2 = VOXEL_SIZE_M1 - 1.f;
constexpr float VOXEL_SIZE_P1 = VOXEL_SIZE + 1.f;
constexpr float OCC_SIZE = VOXEL_SIZE + EXTRA * 2.f;
constexpr float OCC_SIZE_P1 = OCC_SIZE + 1;
constexpr float OCC_SIZE_M1 = OCC_SIZE - 1;

constexpr float INV_OCC_SIZE_M1 = 1.f / OCC_SIZE_M1;

constexpr float FINDY_SIZE = 256.f;
constexpr float FINDY_SIZE_P1 = 257.f;
constexpr float SAMP_EXPANSION = .1f;// VOXEL_SIZE / CHUNK_SIZE;

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
constexpr float speed = 1.f, angleSpeed = .3f;

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

	// static globals
	static const UINT numFrames = 2;

	const UINT DENSITY_SIZE = OCC_SIZE;

	const UINT OCCUPIED_SIZE = VOXEL_SIZE;
	const UINT NUM_POINTS = OCCUPIED_SIZE * OCCUPIED_SIZE;
	const UINT BYTES_POINTS = sizeof(OCCUPIED_POINT) *	NUM_POINTS;

	const UINT SPLAT_SIZE = VOXEL_SIZE;

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
	void renderDensity();
	void renderOccupied();
	void renderGenVerts(UINT vertApprox);
	void renderVertexMesh(XMINT3 pos, UINT numVerts);
	void renderClearTex();
	UINT renderVertSplat();
	void renderGenIndices(XMINT3 pos, UINT minIndices);
	void getIndexData(XMINT3 pos);
	float findY();
	void findYRender();
	void searchTerrain();
	void sampleDensity();
	void genVoxel(XMINT3 pos);
	void phase1();
	UINT phase2();
	void phase3(XMINT3 pos, UINT numOldVerts);
	void phase4(XMINT3 pos);
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
		CB_GENERATION_CONSTANTS,
		CBV_COUNT
	};

	enum BVSRV : UINT32
	{
		DENSITY_TEXTURE = 0,
		INDEX_TEXTURE,
		FINDY_TEXTURE,
		UPLOAD_TEX,
		NOISE_0,
		NOISE_1,
		NOISE_2,
		NOISEH_0,
		NOISEH_1,
		NOISEH_2,
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

	enum BVRTV : UINT32
	{
		RTV_FRAMES = numFrames - 1, // not to be used
		RTV_DENSITY_TEXTURE,
		RTV_INDEX_TEXTURE,
		RTV_FINDY_TEXTURE,
		RTV_UPLOAD_TEXTURE,
		RTV_COUNT
	};

	enum BVSAMPLER :UINT32
	{
		NEAREST_SAMPLER = 0,
		LINEAR_REPEAT_SAMPLER,
		NEAREST_REPEAT_SAMPLER,
		SAMPLER_COUNT
	};

	// buffers
	ComPtr<ID3D12Resource> bufferCB[CBV_COUNT],
		bufferSRV[SRV_COUNT],
		bufferUAV[UAV_COUNT],
		bufferDSV[DSV_COUNT],
		zeroBuffer, plainVCB, pointVCB,
		vertexFrontBuffer, vertexBackBuffer,
		vertexCount,
		indexCount, yposMap;

	struct DRAW_DATA
	{
		ComPtr<ID3D12Resource> vertices, indices;
		UINT numIndices, numVertices;
	};

	unordered_map<XMINT3, DRAW_DATA> computedPos;


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
	ComPtr<ID3D12Resource> renderTarget[numFrames], intermediateTarget[RTV_COUNT];
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
	Image noise0, noise1, noise2,
		noiseH0, noiseH1, noiseH2;
	Image2D altitude, bumpMap;

#pragma pack(push, 1)
	struct WORLD_POS
	{
		XMMATRIX world;
		XMMATRIX worldView;
		XMMATRIX worldViewProjection;
	};

	struct VOXEL_POS
	{
		XMFLOAT3 voxelPos;
		UINT densityType;
		UINT renderType;
	};

	struct GENERATION_CONSTANTS
	{
		float chunkSize = CHUNK_SIZE;
		float extra = EXTRA;

		float voxelExpansion = VOXEL_SIZE;
		float voxelM1 = voxelExpansion - 1.f;
		float voxelP1 = voxelExpansion + 1.f;

		float occExpansion = voxelExpansion + extra * 2.f;
		float occM1 = occExpansion - 1.f;
		float occP1 = occExpansion + 1.f;

		XMFLOAT2 voxelInv = XMFLOAT2(
			1.f / voxelExpansion, 0
		);
		XMFLOAT2 voxelInvVecM1 = XMFLOAT2(
			1.f / voxelM1, 0
		);
		XMFLOAT2 voxelInvVecP1 = XMFLOAT2(
			1.f / voxelP1, 0
		);

		XMFLOAT2 occInv = XMFLOAT2(
			1.f / occExpansion, 0
		);
		XMFLOAT2 occInvVecM1 = XMFLOAT2(
			1.f / occM1, 0
		);
		XMFLOAT2 occInvVecP1 = XMFLOAT2(
			1.f / occP1, 0
		);

		XMFLOAT2 voxelSize = XMFLOAT2(
			1.f / (chunkSize * 16.f), 0
		);
	};
#pragma pack(pop)

	bool isSolid = false;

	// constant buffers
	VOXEL_POS* voxelPosData;
	WORLD_POS* worldPosCB;

	// view params
	XMMATRIX world, view, projection, worldViewProjection;

	XMFLOAT3 origDelta{ 0.0f, 10.0f, 20.0f };
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