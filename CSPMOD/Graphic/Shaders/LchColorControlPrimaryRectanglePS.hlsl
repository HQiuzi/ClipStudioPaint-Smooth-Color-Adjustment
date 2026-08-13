#include"Tolerance.hlsli"


#ifndef OK_COLOR
cbuffer _ConstantBuffer : register(b0)
{
    float h_rad;
    float padding1;
    float padding2;
    float padding3;
};


struct PS_IN
{
    float4 Position : SV_POSITION;
    float2 lc : TEXCOORD0;
};


Texture2D<float> L_MaxC_Table : register(t0);
SamplerState TableSampler : register(s0);



float4 main(PS_IN psIN) : SV_TARGET
{
    
    //float maxC=L_MaxC_Table.Sample(TableSampler, float2(degrees(h_rad) * 10, psIN.lc.x));
    float maxC = SAMPLER_FROM_TABLE(h_rad, psIN.lc.x);
    return float4(LCH_to_sRGB(float3(psIN.lc.x,maxC *psIN.lc.y, h_rad)), 1.f);
}
#else



cbuffer _ConstantBuffer : register(b0)
{
    float h_rad;
    float padding1;
    float padding2;
    float padding3;
};


struct PS_IN
{
    float4 Position : SV_POSITION;
    float2 lc : TEXCOORD0;
};




float4 main(PS_IN psIN) : SV_TARGET
{
    return float4(LCH_to_sRGB(float3(psIN.lc, h_rad)), 1.f);
}
#endif