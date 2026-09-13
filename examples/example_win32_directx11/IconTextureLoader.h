#pragma once
#include <d3d11.h>
#include <vector>
#include <algorithm>
#include "imgui.h"
#include "stb_image.h" 

// 像素点 3x3 膨胀算法（加粗 Alpha 通道线条）
inline void DilateRGBAPixels(unsigned char* pixels, int width, int height, int strokeThickness = 1) {
    if (!pixels || width <= 0 || height <= 0 || strokeThickness <= 0) return;

    std::vector<unsigned char> src(pixels, pixels + (width * height * 4));

    for (int cycle = 0; cycle < strokeThickness; ++cycle) {
        std::vector<unsigned char> temp = src;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 4;
                unsigned char maxAlpha = temp[idx + 3];
                unsigned char r = temp[idx + 0];
                unsigned char g = temp[idx + 1];
                unsigned char b = temp[idx + 2];

                // 遍历 3x3 邻域，找到最大的 Alpha 值和颜色
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int nIdx = (ny * width + nx) * 4;
                            if (temp[nIdx + 3] > maxAlpha) {
                                maxAlpha = temp[nIdx + 3];
                                r = temp[nIdx + 0];
                                g = temp[nIdx + 1];
                                b = temp[nIdx + 2];
                            }
                        }
                    }
                }

                src[idx + 0] = r;
                src[idx + 1] = g;
                src[idx + 2] = b;
                src[idx + 3] = maxAlpha;
            }
        }
    }

    memcpy(pixels, src.data(), src.size());
}

// 从内存中的 PNG 字节数组生成 Direct3D11 纹理指针，支持加粗参数 boldStroke
inline ImTextureID LoadTextureFromMemory(ID3D11Device* device, const unsigned char* data, size_t size, int boldStroke = 0) {
    if (!device || !data || size == 0) return (ImTextureID)0;

    int width = 0, height = 0, channels = 0;
    unsigned char* rgbaPixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 4);
    if (!rgbaPixels) return (ImTextureID)0;

    // 如果指定了加粗像素，进行形态学膨胀处理
    if (boldStroke > 0) {
        DilateRGBAPixels(rgbaPixels, width, height, boldStroke);
    }

    // 1. 创建 D3D11 2D 纹理结构
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = (UINT)width;
    desc.Height = (UINT)height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = rgbaPixels;
    initData.SysMemPitch = (UINT)width * 4;

    ID3D11Texture2D* pTexture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &initData, &pTexture);
    stbi_image_free(rgbaPixels);

    if (FAILED(hr) || !pTexture) return (ImTextureID)0;

    // 2. 创建 SRV 视图
    ID3D11ShaderResourceView* outSRV = nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(pTexture, &srvDesc, &outSRV);
    pTexture->Release();

    if (FAILED(hr) || !outSRV) return (ImTextureID)0;

    return (ImTextureID)outSRV;
}