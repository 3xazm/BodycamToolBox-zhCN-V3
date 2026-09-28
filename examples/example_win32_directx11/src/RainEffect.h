#pragma once

// 暴露 Shader 代码字符串给 RainEffect.cpp 使用
//extern const char* g_RainEffectHLSL;

// 优化后的苹果液态玻璃 + 雨滴折射 HLSL Shader
const char* g_RainEffectHLSL = R"(
cbuffer RainCB : register(b0)
{
    float  iTime;
    float2 iResolution;
    float  padding;
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

float N(float t) { return frac(sin(t * 10234.324) * 123423.23512); }
float3 N31(float p) {
    float3 p3 = frac(float3(p, p, p) * float3(0.1031, 0.11369, 0.13787));
    p3 += dot(p3, p3.yzx + 19.19);
    return frac(float3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
}
float SawTooth(float t) { return cos(t + cos(t)) + sin(2.0 * t) * 0.2 + sin(4.0 * t) * 0.02; }

// 计算雨滴位置与形状
float2 GetDrops(float2 uv, float seed) {
    float t = iTime * 0.8 + seed * 10.0;
    uv.y += t * 0.05;
    uv *= float2(8.0, 2.0) * 2.0;
    float2 id = floor(uv);
    float3 n = N31(id.x + (id.y + seed) * 546.3524);
    float2 bd = frac(uv) - 0.5;
    bd.y *= 4.0;
    bd.x += (n.x - 0.5) * 0.6;
    t += n.z * 6.28;
    
    float slide = SawTooth(t);
    float2 trailPos = float2(bd.x * 1.5, (frac(bd.y * 3.0 - t * 2.0) - 0.5) * 0.5);
    bd.y += slide * 1.8;
    
    float d = length(bd);
    float trailMask = S(-0.2, 0.2, bd.y) * bd.y;
    float td = length(trailPos * max(0.5, trailMask));
    
    return lerp(bd * S(0.25, 0.05, d), trailPos, S(0.1, 0.02, td) * trailMask);
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.uv;
    float2 aspectUv = uv - 0.5;
    aspectUv.x *= iResolution.x / iResolution.y;

    // 1. 多层雨滴计算
    float2 dropOffs = GetDrops(aspectUv, 1.0);
    dropOffs += GetDrops(aspectUv * 1.5, 12.34);
    dropOffs += GetDrops(aspectUv * 2.3, 45.67);

    float mag = length(dropOffs);

    // 2. 雨滴法线与高级反射/高光计算
    // 当存在雨滴时生成 3D 法线矢量
    float3 N = normalize(float3(-dropOffs.x * 4.0, -dropOffs.y * 4.0, 1.0));
    
    // 主光源角度（顶部倾斜光）
    float3 lightDir = normalize(float3(-0.4, 0.7, 0.6));
    float diff = max(0.0, dot(N, lightDir));
    
    // 菲涅尔效应（水滴边缘的高亮）
    float fresnel = pow(1.0 - sat(N.z), 3.0);
    
    // 高光 (Specular Light)
    float3 viewDir = float3(0.0, 0.0, 1.0);
    float3 halfVec = normalize(lightDir + viewDir);
    float spec = pow(max(0.0, dot(N, halfVec)), 48.0) * 2.5;
    
    // 水滴边缘阴影（增强立体感与折射厚度感）
    float dropShadow = S(0.2, 0.0, mag) * 0.3;

    // 3. 液态玻璃体整体暗角与高光边缘
    float glassEdge = pow(1.0 - sat(1.0 - length(uv - 0.5) * 0.8), 2.0);

    // 4. 最终色彩合成
    // RGB 用于表现水滴的高光与边缘光，Alpha 通道控制透光与折射强度
    float3 waterHighlight = float3(1.0, 1.0, 1.0) * (diff * 0.2 + fresnel * 0.9) + float3(0.9, 0.95, 1.0) * spec;
    
    // 基础雨滴覆盖 Alpha + 强高光 Alpha
    float alpha = sat(mag * 3.5) * 0.45 + spec * 0.8 + fresnel * 0.2;
    
    return float4(waterHighlight, sat(alpha));
}
)";