#ifndef _PROC_GEN_H_
#define _PROC_GEN_H_

#include <DirectXMath.h>
#include <wrl.h>
#include <Windows.h>
#include <unordered_map>

#include "d3dx12.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace std;

#define DENSITY_FORMAT DXGI_FORMAT_R32_FLOAT
#define INDEX_FORMAT DXGI_FORMAT_R32_UINT

constexpr float EXTRA = 5.f;
constexpr float VOXEL_SIZE = 65.f;
constexpr float VOXEL_SIZE_M1 = VOXEL_SIZE - 1.f;
constexpr float VOXEL_SIZE_M2 = VOXEL_SIZE_M1 - 1.f;
constexpr float VOXEL_SIZE_P1 = VOXEL_SIZE + 1.f;
constexpr float OCC_SIZE = VOXEL_SIZE + EXTRA * 2.f;
constexpr float OCC_SIZE_P1 = OCC_SIZE + 1;
constexpr float OCC_SIZE_M1 = OCC_SIZE - 1;

constexpr float CHUNK_SIZE = 4.f;

constexpr float INV_OCC_SIZE_M1 = 1.f / OCC_SIZE_M1;

class Graphics;

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

class ProcGen
{
public:
	ProcGen();

#pragma pack(push, 1)
	struct BITPOS
	{
		UINT bitpos;
	};

	struct VOXEL_POS
	{
		XMFLOAT3 voxelPos;
		UINT densityType;
		UINT renderType;
	};

	struct DRAW_DATA
	{
		ComPtr<ID3D12Resource> vertices, indices;
		UINT numIndices, numVertices;
	};

	struct OCCUPIED_POINT
	{
		XMFLOAT2 position;
		XMUINT2 uv;
	};

	struct VERT_OUT
	{
		XMFLOAT4 position;
		XMFLOAT3 normal;
		XMFLOAT3 texcoord;
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

	struct DENSITY_CONSTANTS
	{
		XMFLOAT4X4 rotMatrix0;
		XMFLOAT4X4 rotMatrix1;
		XMFLOAT4X4 rotMatrix2;
	};
#pragma pack(pop)

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

	void setup();

	void genVoxel(XMINT3 pos);
	void regenTerrain();
	void drawTerrain();

	static VOXEL_POS* voxelPosData;

	ComPtr<ID3D12GraphicsCommandList> commandList;

private:
	unordered_map<XMINT3, DRAW_DATA> computedPos;

	D3D12_VIEWPORT voxelViewport, vertSplatViewport;
	D3D12_RECT voxelScissorRect, vertSplatScissorRect;

	//Graphics* g;

	ComPtr<ID3D12PipelineState> densityPipelineState,
		occupiedPipelineState,
		genVertsPipelineState,
		vertexMeshPipelineState,
		dataVertSplatPipelineState,
		dataGenIndicesPipelineState,
		dataClearTexPipelineState;

	D3D12_VERTEX_BUFFER_VIEW pointVB;

	ComPtr<ID3D12Resource> pointVCB;

	ComPtr<ID3D12Resource> vertexFrontBuffer,
		vertexBackBuffer;

	ComPtr<ID3D12Resource> vertexCount,
		indexCount;

	void setupBuffers();
	void setupCBV();
	void setupRTV();

	void renderDensity();
	void renderOccupied();
	void renderGenVerts(UINT vertApprox);
	void renderClearTex();
	UINT renderVertSplat();
	void renderGenIndices(XMINT3 pos, UINT minIndices);
	void renderVertexMesh(XMINT3 pos, UINT numVerts);
	void setupShaders();

	void phase1();
	UINT phase2();
	void phase3(XMINT3 pos, UINT numOldVerts);
	void phase4(XMINT3 pos);
	void getIndexData(XMINT3 pos);
};

#endif