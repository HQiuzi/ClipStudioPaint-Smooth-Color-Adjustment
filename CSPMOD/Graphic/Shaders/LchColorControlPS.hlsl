#include"Tolerance.hlsli"
#ifndef OK_COLOR
cbuffer _ConstantBuffer : register(b0)
{
    float L;
    float pixPerUV;
    float padding2;
    float padding3;
};


struct PS_IN
{
    float4 Position : SV_POSITION;
    float2 uv : TEXCOORD0;
};


Texture2D<float> L_MaxC_Table : register(t0);
SamplerState TableSampler : register(s0);


float4 main(PS_IN psIN) : SV_TARGET
{
    float d = distance(psIN.uv, float2(0.5f, 0.5f));
    if (d >0.5 )
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    
    float alpha = clamp((0.5 - d) * 0.4 * pixPerUV, 
    0, 1);
    
    float h_rad = atan2(psIN.uv.y - 0.5f, psIN.uv.x - 0.5f);
    h_rad += radians(150);
    if (h_rad < 0)
        h_rad += 2 * PI;
    else if (h_rad >= 2 * PI)
        h_rad -= 2 * PI;
    //h_rad = 2 * PI;
    //h_rad=0和2PI的时候RGBH2LCHH返回了不同的值
    //上面改成>= 2 * PI就好了，神经.
    float realHue = RGBH2LCHH(h_rad);
    
    
    
    float cFactor = SAMPLER_FROM_TABLE(realHue, L);
    
    

    
    return float4(LCH_to_sRGB(float3(L, cFactor * d * 200.f, realHue)), alpha);
    
    
    
    
    //return float4(LCH_to_sRGB(float3(L, min(cFactor, d * 2)*100.f, realHue)),alpha);
    
    //float minC = cFactor;
    //for (int i = 0; i < 100; i++)
    //{
    //    float a= SAMPLER_FROM_TABLE(i*2*3.1415926f/100, L);
    //    if (minC > a)
    //        minC = a;
    //}
    //return float4(LCH_to_sRGB(float3(L, minC * d * 200.f, realHue)), alpha);
    //return float4(LCH_to_sRGB(float3(L, d * 200.f, realHue)), alpha);
}


#else




cbuffer _ConstantBuffer : register(b0)
{
    float L;
    float pixPerUV;
    float padding2;
    float padding3;
};


struct PS_IN
{
    float4 Position : SV_POSITION;
    float2 uv : TEXCOORD0;
};



float4 main(PS_IN psIN) : SV_TARGET
{
    float d = distance(psIN.uv, float2(0.5f, 0.5f));
    if (d > 0.5)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    
    float alpha = clamp((0.5 - d) * 0.4 * pixPerUV,
    0, 1);
    
    float h_rad = atan2(psIN.uv.y - 0.5f, psIN.uv.x - 0.5f);
    h_rad += radians(150);
    if (h_rad < 0)
        h_rad += 2 * PI;
    else if (h_rad >= 2 * PI)
        h_rad -= 2 * PI;
    //h_rad = 2 * PI;
    //h_rad=0和2PI的时候RGBH2LCHH返回了不同的值
    //上面改成>= 2 * PI就好了，神经.
    
    //OK Color下不区分颜色色相
    //float realHue = RGBH2LCHH(h_rad);
    float realHue = h_rad;
    
    
    
    
    return float4(LCH_to_sRGB(float3(L,  d * 200.f, realHue)), alpha);
    
    
}




#endif