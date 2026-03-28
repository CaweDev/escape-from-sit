/**
 * @file TextureUtils.cpp
 * @brief TextureUtils implementation — WIC-based CPU pixel loading
 */

#include "TextureUtils.hpp"

// Windows Imaging Component for CPU-side pixel loading
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>

namespace Engine
{
    bool TextureUtils::LoadCpuPixels(const std::string& path, CpuTexture& out)
    {
        out.pixels.clear();
        out.width = out.height = 0;

        // Ensure COM is initialised for this thread (safe to call multiple times).
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        // Create the WIC factory — our entry point for all WIC operations.
        IWICImagingFactory* factory = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory));
        if (FAILED(hr) || !factory) return false;

        // Convert the UTF-8 file path to a wide string for the WIC API.
        int wideLen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
        std::wstring widePath(wideLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &widePath[0], wideLen);

        // Decode the image file into a WIC bitmap.
        IWICBitmapDecoder* decoder = nullptr;
        hr = factory->CreateDecoderFromFilename(
            widePath.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, &decoder);
        if (FAILED(hr) || !decoder) { factory->Release(); return false; }

        // Grab frame 0 (PNG files have exactly one frame).
        IWICBitmapFrameDecode* frame = nullptr;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr) || !frame) { decoder->Release(); factory->Release(); return false; }

        // Convert whatever pixel format the file uses into 32bpp RGBA.
        IWICFormatConverter* converter = nullptr;
        hr = factory->CreateFormatConverter(&converter);
        if (FAILED(hr) || !converter) { frame->Release(); decoder->Release(); factory->Release(); return false; }

        hr = converter->Initialize(
            frame, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0,
            WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) { converter->Release(); frame->Release(); decoder->Release(); factory->Release(); return false; }

        // Copy the converted pixel data into our CPU buffer.
        UINT w = 0, h = 0;
        converter->GetSize(&w, &h);
        out.width = (int)w;
        out.height = (int)h;
        out.pixels.resize(w * h * 4);

        hr = converter->CopyPixels(nullptr, w * 4, (UINT)out.pixels.size(), out.pixels.data());

        // Release all WIC COM objects (order: converter -> frame -> decoder -> factory).
        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();

        if (FAILED(hr)) { out.pixels.clear(); out.width = out.height = 0; return false; }

        //printf("[TextureUtils] Loaded CPU texture: %s (%dx%d)\n", path.c_str(), out.width, out.height);
        return true;
    }

} // namespace Engine
