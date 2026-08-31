#pragma once

#include <d3d9.h>

#ifndef D3DX_PI
#define D3DX_PI (3.141592654f)
#endif

#ifndef D3DX_DEFAULT
#define D3DX_DEFAULT ((UINT)-1)
#endif

#ifndef D3DX_DEFAULT_NONPOW2
#define D3DX_DEFAULT_NONPOW2 ((UINT)-2)
#endif

#ifndef D3DX_FILTER_NONE
#define D3DX_FILTER_NONE     (1 << 0)
#define D3DX_FILTER_POINT    (2 << 0)
#define D3DX_FILTER_LINEAR   (3 << 0)
#define D3DX_FILTER_TRIANGLE (4 << 0)
#define D3DX_FILTER_BOX      (5 << 0)
#endif

typedef enum _D3DXIMAGE_FILEFORMAT
{
	D3DXIFF_BMP = 0,
	D3DXIFF_JPG = 1,
	D3DXIFF_TGA = 2,
	D3DXIFF_PNG = 3,
	D3DXIFF_DDS = 4,
	D3DXIFF_PPM = 5,
	D3DXIFF_DIB = 6,
	D3DXIFF_HDR = 7,
	D3DXIFF_PFM = 8,
	D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT;

typedef struct _D3DXIMAGE_INFO
{
	UINT                 Width;
	UINT                 Height;
	UINT                 Depth;
	UINT                 MipLevels;
	D3DFORMAT            Format;
	D3DRESOURCETYPE      ResourceType;
	D3DXIMAGE_FILEFORMAT ImageFileFormat;
} D3DXIMAGE_INFO;

typedef D3DMATRIX D3DXMATRIX;

D3DXMATRIX* D3DXMatrixIdentity(D3DXMATRIX* pOut);

D3DXMATRIX* D3DXMatrixOrthoOffCenterLH(D3DXMATRIX* pOut, float l, float r,
	float b, float t, float zn, float zf);

HRESULT D3DXCreateTextureFromFileInMemoryEx(
	LPDIRECT3DDEVICE9   pDevice,
	LPCVOID             pSrcData,
	UINT                SrcDataSize,
	UINT                Width,
	UINT                Height,
	UINT                MipLevels,
	DWORD               Usage,
	D3DFORMAT           Format,
	D3DPOOL             Pool,
	DWORD               Filter,
	DWORD               MipFilter,
	D3DCOLOR            ColorKey,
	D3DXIMAGE_INFO*     pSrcInfo,
	PALETTEENTRY*       pPalette,
	LPDIRECT3DTEXTURE9* ppTexture);

HRESULT D3DXCreateTextureFromFileInMemory(
	LPDIRECT3DDEVICE9   pDevice,
	LPCVOID             pSrcData,
	UINT                SrcDataSize,
	LPDIRECT3DTEXTURE9* ppTexture);
