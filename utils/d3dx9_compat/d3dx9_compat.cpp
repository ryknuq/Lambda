#include "d3dx9.h"

#include <wincodec.h>
#include <objbase.h>
#include <vector>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

D3DXMATRIX* D3DXMatrixIdentity(D3DXMATRIX* pOut)
{
	if (!pOut)
		return nullptr;

	ZeroMemory(pOut, sizeof(*pOut));

	pOut->m[0][0] = 1.0f;
	pOut->m[1][1] = 1.0f;
	pOut->m[2][2] = 1.0f;
	pOut->m[3][3] = 1.0f;

	return pOut;
}

D3DXMATRIX* D3DXMatrixOrthoOffCenterLH(D3DXMATRIX* pOut, float l, float r,
	float b, float t, float zn, float zf)
{
	if (!pOut)
		return nullptr;

	ZeroMemory(pOut, sizeof(*pOut));

	pOut->m[0][0] = 2.0f / (r - l);
	pOut->m[1][1] = 2.0f / (t - b);
	pOut->m[2][2] = 1.0f / (zf - zn);
	pOut->m[3][0] = (l + r) / (l - r);
	pOut->m[3][1] = (t + b) / (b - t);
	pOut->m[3][2] = zn / (zn - zf);
	pOut->m[3][3] = 1.0f;

	return pOut;
}

namespace
{
	IWICImagingFactory* wic_factory()
	{
		static IWICImagingFactory* factory = []() -> IWICImagingFactory*
		{
			CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

			IWICImagingFactory* result = nullptr;

			if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
				CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&result))))
				return nullptr;

			return result;
		}();

		return factory;
	}

	template <typename T>
	void release(T*& object)
	{
		if (object)
		{
			object->Release();
			object = nullptr;
		}
	}

	bool decode_to_bgra(LPCVOID data, UINT size, UINT& width, UINT& height,
		std::vector <unsigned char>& pixels, D3DXIMAGE_FILEFORMAT& file_format)
	{
		auto* factory = wic_factory();

		if (!factory || !data || !size)
			return false;

		IWICStream* stream = nullptr;
		IWICBitmapDecoder* decoder = nullptr;
		IWICBitmapFrameDecode* frame = nullptr;
		IWICFormatConverter* converter = nullptr;
		IWICBitmapScaler* scaler = nullptr;

		auto ok = false;

		do
		{
			if (FAILED(factory->CreateStream(&stream)))
				break;

			if (FAILED(stream->InitializeFromMemory(
				static_cast<BYTE*>(const_cast<void*>(data)), size)))
				break;

			if (FAILED(factory->CreateDecoderFromStream(stream, nullptr,
				WICDecodeMetadataCacheOnDemand, &decoder)))
				break;

			if (FAILED(decoder->GetFrame(0, &frame)))
				break;

			UINT source_width = 0, source_height = 0;

			if (FAILED(frame->GetSize(&source_width, &source_height))
				|| !source_width || !source_height)
				break;

			if (width == 0 || width == D3DX_DEFAULT)
				width = source_width;

			if (height == 0 || height == D3DX_DEFAULT)
				height = source_height;

			if (FAILED(factory->CreateFormatConverter(&converter)))
				break;

			if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA,
				WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
				break;

			IWICBitmapSource* source = converter;

			if (width != source_width || height != source_height)
			{
				if (FAILED(factory->CreateBitmapScaler(&scaler)))
					break;

				if (FAILED(scaler->Initialize(converter, width, height,
					WICBitmapInterpolationModeFant)))
					break;

				source = scaler;
			}

			const auto stride = width * 4;

			pixels.resize(static_cast<size_t>(stride) * height);

			if (FAILED(source->CopyPixels(nullptr, stride,
				static_cast<UINT>(pixels.size()), pixels.data())))
				break;

			GUID container = {};

			file_format = D3DXIFF_PNG;

			if (SUCCEEDED(decoder->GetContainerFormat(&container)))
			{
				if (container == GUID_ContainerFormatBmp)
					file_format = D3DXIFF_BMP;
				else if (container == GUID_ContainerFormatJpeg)
					file_format = D3DXIFF_JPG;
			}

			ok = true;
		} while (false);

		release(scaler);
		release(converter);
		release(frame);
		release(decoder);
		release(stream);

		return ok;
	}

	void apply_color_key(std::vector <unsigned char>& pixels, D3DCOLOR color_key)
	{
		if (!color_key)
			return;

		const unsigned char key[4] =
		{
			static_cast<unsigned char>(color_key & 0xFF),
			static_cast<unsigned char>((color_key >> 8) & 0xFF),
			static_cast<unsigned char>((color_key >> 16) & 0xFF),
			static_cast<unsigned char>((color_key >> 24) & 0xFF)
		};

		for (size_t i = 0; i + 3 < pixels.size(); i += 4)
		{
			if (pixels[i] == key[0] && pixels[i + 1] == key[1]
				&& pixels[i + 2] == key[2] && pixels[i + 3] == key[3])
				pixels[i + 3] = 0;
		}
	}

	bool fill_surface(IDirect3DTexture9* texture, const std::vector <unsigned char>& pixels,
		UINT width, UINT height)
	{
		D3DLOCKED_RECT locked = {};

		if (FAILED(texture->LockRect(0, &locked, nullptr, 0)))
			return false;

		const auto stride = static_cast<size_t>(width) * 4;

		for (UINT y = 0; y < height; ++y)
		{
			memcpy(static_cast<unsigned char*>(locked.pBits) + static_cast<size_t>(y) * locked.Pitch,
				pixels.data() + static_cast<size_t>(y) * stride, stride);
		}

		texture->UnlockRect(0);

		return true;
	}
}

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
	LPDIRECT3DTEXTURE9* ppTexture)
{
	(void)Filter;
	(void)MipFilter;
	(void)pPalette;
	(void)MipLevels;

	if (!pDevice || !ppTexture)
		return D3DERR_INVALIDCALL;

	*ppTexture = nullptr;

	UINT width = Width, height = Height;
	std::vector <unsigned char> pixels;
	auto file_format = D3DXIFF_PNG;

	if (!decode_to_bgra(pSrcData, SrcDataSize, width, height, pixels, file_format))
		return D3DERR_INVALIDCALL;

	apply_color_key(pixels, ColorKey);

	const auto format = Format == D3DFMT_UNKNOWN ? D3DFMT_A8R8G8B8 : Format;

	IDirect3DTexture9* texture = nullptr;

	if (FAILED(pDevice->CreateTexture(width, height, 1, Usage, format, Pool,
		&texture, nullptr)) || !texture)
		return D3DERR_INVALIDCALL;

	auto uploaded = false;

	if (Pool == D3DPOOL_DEFAULT && !(Usage & D3DUSAGE_DYNAMIC))
	{
		IDirect3DTexture9* staging = nullptr;

		if (SUCCEEDED(pDevice->CreateTexture(width, height, 1, 0, format,
			D3DPOOL_SYSTEMMEM, &staging, nullptr)) && staging)
		{
			uploaded = fill_surface(staging, pixels, width, height)
				&& SUCCEEDED(pDevice->UpdateTexture(staging, texture));

			staging->Release();
		}
	}
	else
	{
		uploaded = fill_surface(texture, pixels, width, height);
	}

	if (!uploaded)
	{
		texture->Release();
		return D3DERR_INVALIDCALL;
	}

	if (pSrcInfo)
	{
		pSrcInfo->Width = width;
		pSrcInfo->Height = height;
		pSrcInfo->Depth = 1;
		pSrcInfo->MipLevels = 1;
		pSrcInfo->Format = format;
		pSrcInfo->ResourceType = D3DRTYPE_TEXTURE;
		pSrcInfo->ImageFileFormat = file_format;
	}

	*ppTexture = texture;

	return D3D_OK;
}

HRESULT D3DXCreateTextureFromFileInMemory(
	LPDIRECT3DDEVICE9   pDevice,
	LPCVOID             pSrcData,
	UINT                SrcDataSize,
	LPDIRECT3DTEXTURE9* ppTexture)
{
	return D3DXCreateTextureFromFileInMemoryEx(pDevice, pSrcData, SrcDataSize,
		D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
		D3DX_DEFAULT, D3DX_DEFAULT, 0, nullptr, nullptr, ppTexture);
}
