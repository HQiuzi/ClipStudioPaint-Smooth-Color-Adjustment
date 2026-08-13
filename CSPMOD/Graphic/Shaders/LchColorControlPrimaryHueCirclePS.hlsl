#include"Tolerance.hlsli"
#ifndef OK_COLOR
cbuffer _ConstantBuffer : register(b0)
{
    float pixPerUV;
    float ____uvPerPix;
    float padding2;
    float padding3;
};



Texture2D<float> L_MaxC_Table : register(t0);
SamplerState TableSampler : register(s0);

struct PS_IN
{
    float4 Position : SV_POSITION;
    float2 uv : TEXCOORD0;
};


static const float hueCircleWide = 0.2f;
static const float hueCircleLight = 50.f;
static const float hueOffset = 150.f;

float4 main(PS_IN psIN) : SV_TARGET
{
    float d = distance(psIN.uv, float2(0.5f,0.5f));
    if (d >= 0.5 || d <= 0.5 - hueCircleWide*0.5)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);

    float alpha = clamp(min(0.5 - d, d - (0.5 - hueCircleWide * 0.5)) *0.4* pixPerUV, 0, 1);
    
    
    //hrad红色色相渲染会有跳变的问题
    float h_rad = atan2(psIN.uv.y-0.5f, psIN.uv.x-0.5f);
    h_rad += radians(150);
    if (h_rad < 0)
        h_rad += 2 * PI;
    else if (h_rad >= 2 * PI)
        h_rad -= 2 * PI;
    //h_rad = 2 * PI;
    //h_rad=0和2PI的时候RGBH2LCHH返回了不同的值
    //上面改成>= 2 * PI就好了，神经.
    float realHue = RGBH2LCHH(h_rad);
    //realHue = 2 * PI;
    //float val = realHue / (2 * PI);
    //float val = h_rad / (2 * PI);
    //return float4(val, val, val, 1);
    return float4(LCH_to_sRGB(float3(hueCircleLight, 
    
    SAMPLER_FROM_TABLE(realHue, hueCircleLight) * 100.f
    
    , realHue)), alpha);
    //return float4(LCH_to_sRGB(float3(hueCircleLight, 100.f, (h_rad))), alpha);
}


#else



cbuffer _ConstantBuffer : register(b0)
{
    float pixPerUV;
    float ____uvPerPix;
    float padding2;
    float padding3;
};


struct PS_IN
{
    float4 Position : SV_POSITION;
    float2 uv : TEXCOORD0;
};


static const float hueCircleWide = 0.2f;
static const float hueCircleLight = 50.f;
static const float hueOffset = 150.f;

float4 main(PS_IN psIN) : SV_TARGET
{
    float d = distance(psIN.uv, float2(0.5f, 0.5f));
    if (d >= 0.5 || d <= 0.5 - hueCircleWide * 0.5)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);

    float alpha = clamp(min(0.5 - d, d - (0.5 - hueCircleWide * 0.5)) * 0.4 * pixPerUV, 0, 1);
    
    
    //hrad红色色相渲染会有跳变的问题
    float h_rad = atan2(psIN.uv.y - 0.5f, psIN.uv.x - 0.5f);
    h_rad += radians(150);
    if (h_rad < 0)
        h_rad += 2 * PI;
    else if (h_rad >= 2 * PI)
        h_rad -= 2 * PI;
    
    //OK Color下不区分颜色色相
    //float realHue = RGBH2LCHH(h_rad);
    float realHue = h_rad;
    
    
    
    
    //realHue = 2 * PI;
    //float val = realHue / (2 * PI);
    //float val = h_rad / (2 * PI);
    //return float4(val, val, val, 1);
    return float4(LCH_to_sRGB(float3(hueCircleLight,
    
    100.f
    
    , realHue)), alpha);
    //return float4(LCH_to_sRGB(float3(hueCircleLight, 100.f, (h_rad))), alpha);
}


#endif