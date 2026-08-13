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
    
    //float tem = L_MaxC_Table.Sample(TableSampler, float2(psIN.lc.y, psIN.lc.x));
    //return float4(tem, tem, tem,1);
    
    
    
    float CFactor = SAMPLER_FROM_TABLE(h_rad, psIN.lc.x);
    
    float cMax = (1 - abs(psIN.lc.x - 50.f) * 0.02f)*100.f;
    float cRemap = 100.f*psIN.lc.y  / cMax;
    return float4(LCH_to_sRGB(float3(psIN.lc.x, CFactor * cRemap, h_rad)), 1.f);
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