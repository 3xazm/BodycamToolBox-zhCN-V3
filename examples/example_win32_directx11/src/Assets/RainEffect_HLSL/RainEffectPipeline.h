#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>

// 100% 原始 HLSL 脚本，不做任何修改
static const char* g_RainLiquidHLSL = R"(
cbuffer RainCB : register(b0)
{
    float  iTime;
    float2 iResolution;
    float2 iMouse;
    float3 padding; // C++ 端 16 字节内存对齐，防止崩溃
};

struct VS_INPUT { float2 pos : POSITION; float2 uv : TEXCOORD0; };
struct PS_INPUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.uv = float2(input.uv.x, 1.0 - input.uv.y);
    return output;
}

#define S(x, y, z) smoothstep(x, y, z)
#define sat(x) clamp(x, 0.0, 1.0)
#define HIGH_QUALITY

float N(float t) { return frac(sin(t * 10234.324) * 123423.23512); }
float3 N31(float p) {
    float3 p3 = frac(float3(p, p, p) * float3(0.1031, 0.11369, 0.13787));
    p3 += dot(p3, p3.yzx + 19.19);
    return frac(float3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
}
float SawTooth(float t) { return cos(t + cos(t)) + sin(2.0 * t) * 0.2 + sin(4.0 * t) * 0.02; }
float DeltaSawTooth(float t) { return 0.4 * cos(2.0 * t) + 0.08 * cos(4.0 * t) - (1.0 - sin(t)) * sin(t + cos(t)); }

float2 GetDrops(float2 uv, float seed, float m) {
    float t = iTime + m * 30.0;
    uv.y += t * 0.05;
    uv *= float2(10.0, 2.5) * 2.0;
    float2 id = floor(uv);
    float3 n = N31(id.x + (id.y + seed) * 546.3524);
    float2 bd = frac(uv) - 0.5;
    bd.y *= 4.0;
    bd.x += (n.x - 0.5) * 0.6;
    t += n.z * 6.28;
    float slide = SawTooth(t);
    float2 trailPos = float2(bd.x * 1.5, (frac(bd.y * 3.0 - t * 2.0) - 0.5) * 0.5);
    bd.y += slide * 2.0;
    #ifdef HIGH_QUALITY
    bd.y += bd.x * bd.x * DeltaSawTooth(t);
    #endif
    float d = length(bd);
    float trailMask = S(-0.2, 0.2, bd.y) * bd.y;
    float td = length(trailPos * max(0.5, trailMask));
    return lerp(bd * S(0.2, 0.1, d), trailPos, S(0.1, 0.02, td) * trailMask);
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.uv;
    float2 aspectUv = uv - 0.5;
    aspectUv.x *= iResolution.x / iResolution.y;
    
    // ==========================================
    // 像手电筒闪烁一样的极简高频闪光代码 (8行)
    // ==========================================
    // 1. 高频正弦波 (30.0/45.0) 形成频闪，frac 时间产生断续间隔
    float flashNoise = sin(iTime * 30.0) * cos(iTime * 45.0);
    float intervalMask = step(0.75, frac(iTime * 0.2)); // 每隔大约5秒才触发一次闪烁
    
    // 2. 最终手电筒闪烁强度 (0.05~0.15 之间的微弱白光)
    float flashlight = sat(flashNoise) * intervalMask * 0.12; 
    // ==========================================

    float2 dropOffs = GetDrops(aspectUv, 1.0, iMouse.x);
    dropOffs += GetDrops(aspectUv * 1.4, 10.0, iMouse.x);
    #ifdef HIGH_QUALITY
    dropOffs += GetDrops(aspectUv * 2.4, 25.0, iMouse.x);
    #endif
    
    float mag = length(dropOffs);
    
    // 没有雨滴的区域：直接透出微弱的闪光
    if (mag < 0.0001) {
        return float4(1.0, 1.0, 1.0, flashlight);
    }
    
    // 法线与光照计算
    float3 N = normalize(float3(-dropOffs.x * 2.0, -dropOffs.y * 2.0, 1.0));
    float3 lightDir = normalize(float3(-0.3, 0.8, 0.6));
    float diff = max(0.0, dot(N, lightDir));
    float fresnel = pow(1.0 - sat(N.z), 2.0);
    
    float3 viewDir = float3(0.0, 0.0, 1.0);
    float3 spec = pow(max(0.0, dot(N, normalize(lightDir + viewDir))), 32.0) * 1.2;
    
    // 将手电筒闪烁直接加到雨滴颜色和高光上
    float3 dropColor = float3(1.0, 1.0, 1.0) * (diff * 0.4 + fresnel * 0.8) + spec + flashlight * 2.0;
    float alpha = sat(mag * 4.0) * 0.35 + spec.x + flashlight;
    
    return float4(dropColor, alpha);
}
)";

struct RainSimpleVertex {
    float x, y;
    float u, v;
};

struct RainCB {
    float time;
    float resolution[2];
    float mouse[2];
    float padding[3];
};

class RainEffectPipeline {
public:
    ID3D11VertexShader* pVS = nullptr;
    ID3D11PixelShader* pPS = nullptr;
    ID3D11InputLayout* pInputLayout = nullptr;
    ID3D11Buffer* pVB = nullptr;
    ID3D11Buffer* pCB = nullptr;
    ID3D11Texture2D* pTexture = nullptr;
    ID3D11RenderTargetView* pRTV = nullptr;
    ID3D11ShaderResourceView* pSRV = nullptr;
    ID3D11BlendState* pBlendState = nullptr; // 专门的加法/亮白混合状态

    int texWidth = 0;
    int texHeight = 0;
    float gTime = 0.0f;

    bool Init(ID3D11Device* device) {
        if (!device) return false;

        size_t len = strlen(g_RainLiquidHLSL);
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;

        if (FAILED(D3DCompile(g_RainLiquidHLSL, len, nullptr, nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vsBlob, &errorBlob))) {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return false;
        }
        if (FAILED(D3DCompile(g_RainLiquidHLSL, len, nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob))) {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            if (vsBlob) vsBlob->Release();
            return false;
        }

        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &pVS);
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pPS);

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pInputLayout);
        vsBlob->Release();
        psBlob->Release();

        RainSimpleVertex vertices[] = {
            { -1.0f,  1.0f, 0.0f, 0.0f },
            {  1.0f,  1.0f, 1.0f, 0.0f },
            { -1.0f, -1.0f, 0.0f, 1.0f },
            {  1.0f,  1.0f, 1.0f, 0.0f },
            {  1.0f, -1.0f, 1.0f, 1.0f },
            { -1.0f, -1.0f, 0.0f, 1.0f },
        };
        D3D11_BUFFER_DESC vbd = {};
        vbd.Usage = D3D11_USAGE_DEFAULT;
        vbd.ByteWidth = sizeof(vertices);
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vData = { vertices, 0, 0 };
        device->CreateBuffer(&vbd, &vData, &pVB);

        D3D11_BUFFER_DESC cbd = {};
        cbd.ByteWidth = sizeof(RainCB);
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&cbd, nullptr, &pCB);

        // 创建正确支持白光的加法 Blend State
        D3D11_BLEND_DESC bDesc = {};
        bDesc.RenderTarget[0].BlendEnable = TRUE;
        bDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        bDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; // 支持白色正常叠加
        bDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        bDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        bDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        bDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        device->CreateBlendState(&bDesc, &pBlendState);

        return true;
    }

    void Resize(ID3D11Device* device, int width, int height) {
        if (!device || width <= 0 || height <= 0 || (width == texWidth && height == texHeight)) return;
        CleanupRenderTarget();

        texWidth = width;
        texHeight = height;

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = width;
        td.Height = height;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = device->CreateTexture2D(&td, nullptr, &pTexture);
        if (SUCCEEDED(hr) && pTexture) {
            device->CreateRenderTargetView(pTexture, nullptr, &pRTV);
            device->CreateShaderResourceView(pTexture, nullptr, &pSRV);
        }
    }

    void Render(ID3D11DeviceContext* ctx, float width, float height, float mouseX, float mouseY, float dt) {

        (void)mouseX;
        (void)mouseY;
        if (!ctx || !pCB || !pRTV || !pPS) return;
        gTime += dt;

        const float clearCol[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->ClearRenderTargetView(pRTV, clearCol);
        ctx->OMSetRenderTargets(1, &pRTV, nullptr);

        // 绑定专用 Blend State 保证白色闪烁正常
        if (pBlendState) {
            float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            ctx->OMSetBlendState(pBlendState, blendFactor, 0xffffffff);
        }

        D3D11_VIEWPORT vp = { 0.0f, 0.0f, width, height, 0.0f, 1.0f };
        ctx->RSSetViewports(1, &vp);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(ctx->Map(pCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            RainCB* cb = (RainCB*)mapped.pData;
            cb->time = gTime;
            cb->resolution[0] = width;
            cb->resolution[1] = height;
            cb->mouse[0] = 0.0f; // 设为 0 避免干扰闪烁和时间
            cb->mouse[1] = 0.0f;
            ctx->Unmap(pCB, 0);
        }

        UINT stride = sizeof(RainSimpleVertex);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &pVB, &stride, &offset);
        ctx->IASetInputLayout(pInputLayout);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ctx->VSSetShader(pVS, nullptr, 0);
        ctx->PSSetShader(pPS, nullptr, 0);
        ctx->PSSetConstantBuffers(0, 1, &pCB);

        ctx->Draw(6, 0);
    }

    void CleanupRenderTarget() {
        if (pSRV) { pSRV->Release(); pSRV = nullptr; }
        if (pRTV) { pRTV->Release(); pRTV = nullptr; }
        if (pTexture) { pTexture->Release(); pTexture = nullptr; }
    }

    void Shutdown() {
        CleanupRenderTarget();
        if (pBlendState) { pBlendState->Release(); pBlendState = nullptr; }
        if (pCB) { pCB->Release(); pCB = nullptr; }
        if (pVB) { pVB->Release(); pVB = nullptr; }
        if (pInputLayout) { pInputLayout->Release(); pInputLayout = nullptr; }
        if (pPS) { pPS->Release(); pPS = nullptr; }
        if (pVS) { pVS->Release(); pVS = nullptr; }
    }
};