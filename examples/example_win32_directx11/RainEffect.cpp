#include "RainEffect.h"

// 将 HLSL Shader 嵌入在 C++ 字符串中，无需外部 .hlsl 文件
const char* g_RainEffectHLSL = R"(
cbuffer RainCB : register(b0)
{
    float  iTime;
    float2 iResolution;
    float2 iMouse;
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
    float alpha = sat(mag * 4.0) * 0.35 + spec + flashlight;
    
    return float4(dropColor, alpha);
}
)";