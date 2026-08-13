#include "pch.h"
#include"Graphic/D3D11Graphic.h"
//#include"ReplaceColor.h"
#include "LchColorControlContext.h"
#include"Graphic/CSPMOD_ColorConvert.h"

#include"Graphic/Shaders/FilterVS.h"
#include"Graphic/Shaders/LchColorControlPS.h"
#include"Graphic/Shaders/LchColorControlPrimaryRectanglePS.h"
#include"Graphic/Shaders/LchColorControlPrimaryTrianglePS.h"
#include"Graphic/Shaders/LchColorControlPrimaryHueCirclePS.h"
#include"Graphic/Shaders/AlphaBlurPS.h"





//#include <corecrt_math.h>
#include"SDL3/SDL.h"
#include <stdio.h>






static const float hueCircleWide = 0.2f;

//创建顶点结构
struct Vertex
{
    float posX;
    float posY;
    float u;
    float v;
};




bool LchColorControlContext::CreateContext()
{
    auto pdevice=D3D11Graphic::GetDevice();
    auto pcontext=D3D11Graphic::GetContext();
    
    //这种不需要对其他地方进行采样的后处理用像素着色器吧
    







    //创建矩形顶点
    {
        Vertex vertexs[4] =
        {
            {-1.f,1.f,  0.f,0.f},//左上
            {1.f,1.f,   1.f,0.f},//右上
            {-1.f,-1.f, 0.f,1.f},//左下
            {1.f,-1.f,  1.f,1.f}//右下
        };

        //创建顶点缓冲区
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT; // GPU 读写优化
        bd.ByteWidth = sizeof(vertexs); // 
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = vertexs;

        if (FAILED(pdevice->CreateBuffer(&bd, &initData, &pVertexBuffer_Filter)))
        {
            return false;
        }



        //索引缓冲
        {
            uint16_t index[] = { 0,1,2,   1,3,2 };

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = index;

            D3D11_BUFFER_DESC ibDesc = {};
            ibDesc.Usage = D3D11_USAGE_DEFAULT;
            ibDesc.ByteWidth = sizeof(index);
            ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            ibDesc.CPUAccessFlags = 0;
            if (FAILED(pdevice->CreateBuffer(&ibDesc, &initData, &pIndexBuffer)))
            {
                return false;
            }
        }
    }
    //矩形色盘顶点
    {
        float pos = (1 - hueCircleWide) / 1.4142135623731f;
        Vertex vertexs[4] =
        {
            {-pos,pos,  100.f,0.f},//左上 L C
            {pos,pos,   100.f,100.f},//右上
            {-pos,-pos, 0.f,0.f},//左下
            {pos,-pos,  0.f,100.f}//右下
        };
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT; // GPU 读写优化
        bd.ByteWidth = sizeof(vertexs); // 
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = vertexs;

        if (FAILED(pdevice->CreateBuffer(&bd, &initData, &pLchColorControlPrimaryRectangleVertextBuffer)))
        {
            return false;
        }
    }
    //三角形色盘顶点
    {
        float Lpos = (1 - hueCircleWide) *0.5f;
        float Ypos = (1 - hueCircleWide) *0.5f* 1.73205081f;
        Vertex vertexs[4] =
        {
            {-Lpos,-Ypos,  0.f,0.f},//下 L C
            {-Lpos,0,   50.f,0.f},//左中
            {1 - hueCircleWide,0, 50.f,100.f},//右中
            {-Lpos,Ypos,  100.f,0.f}//上
        };
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT; // GPU 读写优化
        bd.ByteWidth = sizeof(vertexs); // 
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = vertexs;

        if (FAILED(pdevice->CreateBuffer(&bd, &initData, &pLchColorControlPrimaryTriangleVertextBuffer)))
        {
            return false;
        }
    }










    //输入布局input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 } 
    };

    if (FAILED(pdevice->CreateInputLayout(layout, 2, FilterVS, sizeof(FilterVS), &pInputLayout)))
    {
        return false;
    }



    //着色器
    if (FAILED(pdevice->CreateVertexShader(FilterVS, sizeof(FilterVS), NULL, &pFilterVS)))
    {
        return false;
    }
    if (FAILED(pdevice->CreatePixelShader(LchColorControlPS, sizeof(LchColorControlPS), NULL, &pLchColorControlPS)))
    {
        return false;
    }
    if (FAILED(pdevice->CreatePixelShader(LchColorControlPrimaryRectanglePS, sizeof(LchColorControlPrimaryRectanglePS), NULL, &pLchColorControlPrimaryRectanglePS)))
    {
        return false;
    }
    if (FAILED(pdevice->CreatePixelShader(LchColorControlPrimaryTrianglePS, sizeof(LchColorControlPrimaryTrianglePS), NULL, &pLchColorControlPrimaryTrianglePS)))
    {
        return false;
    }
    if (FAILED(pdevice->CreatePixelShader(LchColorControlPrimaryHueCirclePS, sizeof(LchColorControlPrimaryHueCirclePS), NULL, &pLchColorControlPrimaryHueCirclePS)))
    {
        return false;
    }
    if (FAILED(pdevice->CreatePixelShader(AlphaBlurPS, sizeof(AlphaBlurPS), NULL, &pAlphaBlurPS)))
    {
        return false;
    }
    //采样器
    D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC{ CD3D11_DEFAULT{} };
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    if (FAILED(pdevice->CreateSamplerState(&samplerDesc, &pTableSampler)))
    {
        return false;
    }








    //着色器参数数
    {
        D3D11_BUFFER_DESC cbDesc;
        ZeroMemory(&cbDesc, sizeof(cbDesc));
        //对齐16字节
        auto padding = 16 - sizeof(PsUniform) % 16;
        if (padding == 16)padding = 0;
        cbDesc.ByteWidth = static_cast<UINT>(sizeof(PsUniform)+ padding);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;      // 允许CPU动态更新
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU可写入
        if (FAILED(pdevice->CreateBuffer(&cbDesc, nullptr, &pUniformBuffer)))
        {
            return false;
        }
        if (FAILED(pdevice->CreateBuffer(&cbDesc, nullptr, &pUniformBufferPrimaryRectangle)))
        {
            return false;
        }
        if (FAILED(pdevice->CreateBuffer(&cbDesc, nullptr, &pUniformBufferPrimaryTriangle)))
        {
            return false;
        }
        if (FAILED(pdevice->CreateBuffer(&cbDesc, nullptr, &pUniformBufferPrimaryHueCirclePS)))
        {
            return false;
        }
    }


#ifndef OK_COLOR
    //maxvalue表
    {

        uint32_t LightTotal;
        uint32_t HueTotal;
        float* pdata=CSPMOD_ColorConvert::GetMaxChromaOfHueTable(&LightTotal, &HueTotal);

        pMaxChromaOfHueTable.reset(new Texture(Texture::float32, HueTotal, LightTotal, pdata));

    }
#endif








	return true;
}

void LchColorControlContext::DrawLchColorControl(RenderTexture* renderTarget, float L)
{
    if (needInit)
    {
        if (CreateContext())
            needInit = false;
        else
            return;
    }

    auto pdevice = D3D11Graphic::GetDevice();
    auto pcontext = D3D11Graphic::GetContext();


    //设置参数
    PsUniform LchColorControlUniform = {L,(float)renderTarget ->GetSizeW()};






    //更新constantBuffer
    D3D11_MAPPED_SUBRESOURCE msr;
    pcontext->Map(pUniformBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr);
    memcpy(msr.pData, &LchColorControlUniform, sizeof(LchColorControlUniform));
    pcontext->Unmap(pUniformBuffer.Get(), 0u);


    //绑定并绘制
    {
        //auto rt = renderTarget->GetRT();
        //float clearColor[4] = { 0.f,0.f,0.f,0.f };
        //pcontext->OMSetRenderTargets(1, &rt,nullptr);
        //pcontext->ClearRenderTargetView(rt, clearColor);
        auto rt =renderTarget->GetRT();
        pcontext->OMSetRenderTargets(1, &rt, nullptr);

        UINT stride = sizeof(Vertex); 
        UINT offset = 0;
        pcontext->IASetVertexBuffers(0,1, pVertexBuffer_Filter.GetAddressOf(),&stride,&offset);
        pcontext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT,0);
        pcontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pcontext->IASetInputLayout(pInputLayout.Get());

        pcontext->VSSetShader(pFilterVS.Get(),nullptr,0);
        pcontext->PSSetShader(pLchColorControlPS.Get(),nullptr,0);


        pcontext->PSSetConstantBuffers(0, 1, pUniformBuffer.GetAddressOf());
        //pcontext->PSSetConstantBuffers(1, 1, pMaxChromaOfHueTable.GetAddressOf());


#ifndef OK_COLOR
        auto ptexTable=pMaxChromaOfHueTable->GetTex();
        pcontext->PSSetShaderResources(0,1, &ptexTable);
        pcontext->PSSetSamplers(0,1, pTableSampler.GetAddressOf());
#endif

        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<float>(renderTarget->GetSizeW());
        vp.Height = static_cast<float>(renderTarget->GetSizeH());
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        pcontext->RSSetViewports(1,&vp);

        pcontext->DrawIndexed(6,0,0);

    
    }




}

void LchColorControlContext::DrawLchColorControlPrimaryRectangle(RenderTexture* renderTarget, float h_rgb)
{

    if (needInit)
    {
        if (CreateContext())
            needInit = false;
        else
            return;
    }

    auto pdevice = D3D11Graphic::GetDevice();
    auto pcontext = D3D11Graphic::GetContext();


    //设置参数
    PsUniform LchColorControlRectangleUniform = { h_rgb * 0.01745329F };


    //更新constantBuffer
    D3D11_MAPPED_SUBRESOURCE msr;
    pcontext->Map(pUniformBufferPrimaryRectangle.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr);
    memcpy(msr.pData, &LchColorControlRectangleUniform, sizeof(LchColorControlRectangleUniform));
    pcontext->Unmap(pUniformBufferPrimaryRectangle.Get(), 0u);


    //绑定并绘制
    {

        auto rt = renderTarget->GetRT();
        pcontext->OMSetRenderTargets(1, &rt, nullptr);



        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pcontext->IASetVertexBuffers(0, 1, pLchColorControlPrimaryRectangleVertextBuffer.GetAddressOf(), &stride, &offset);
        pcontext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        pcontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pcontext->IASetInputLayout(pInputLayout.Get());

        pcontext->VSSetShader(pFilterVS.Get(), nullptr, 0);
        pcontext->PSSetShader(pLchColorControlPrimaryRectanglePS.Get(), nullptr, 0);

        pcontext->PSSetConstantBuffers(0, 1, pUniformBufferPrimaryRectangle.GetAddressOf());

#ifndef OK_COLOR
        auto ptexTable = pMaxChromaOfHueTable->GetTex();
        pcontext->PSSetShaderResources(0, 1, &ptexTable);
        pcontext->PSSetSamplers(0, 1, pTableSampler.GetAddressOf());
#endif


        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<float>(renderTarget->GetSizeW());
        vp.Height = static_cast<float>(renderTarget->GetSizeH());
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        pcontext->RSSetViewports(1, &vp);

        pcontext->DrawIndexed(6, 0, 0);

    }
}

void LchColorControlContext::DrawLchColorControlPrimaryHueCircle(RenderTexture* renderTarget)
{

    if (needInit)
    {
        if (CreateContext())
            needInit = false;
        else
            return;
    }

    auto pdevice = D3D11Graphic::GetDevice();
    auto pcontext = D3D11Graphic::GetContext();

    //设置参数
    PsUniform LchColorControlRectangleUniform = { (float)renderTarget->GetSizeW()};
    //更新constantBuffer
    D3D11_MAPPED_SUBRESOURCE msr;
    pcontext->Map(pUniformBufferPrimaryHueCirclePS.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr);
    memcpy(msr.pData, &LchColorControlRectangleUniform, sizeof(LchColorControlRectangleUniform));
    pcontext->Unmap(pUniformBufferPrimaryHueCirclePS.Get(), 0u);





    //绑定并绘制
    {

        auto rt = renderTarget->GetRT();
        pcontext->OMSetRenderTargets(1, &rt, nullptr);


        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pcontext->IASetVertexBuffers(0, 1, pVertexBuffer_Filter.GetAddressOf(), &stride, &offset);
        pcontext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        pcontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pcontext->IASetInputLayout(pInputLayout.Get());

        pcontext->VSSetShader(pFilterVS.Get(), nullptr, 0);
        pcontext->PSSetShader(pLchColorControlPrimaryHueCirclePS.Get(), nullptr, 0);

        pcontext->PSSetConstantBuffers(0, 1, pUniformBufferPrimaryHueCirclePS.GetAddressOf());

#ifndef OK_COLOR
        auto ptexTable = pMaxChromaOfHueTable->GetTex();
        pcontext->PSSetShaderResources(0, 1, &ptexTable);
        pcontext->PSSetSamplers(0, 1, pTableSampler.GetAddressOf());
#endif

        D3D11Graphic::BindDefaultBlend();

        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<float>(renderTarget->GetSizeW());
        vp.Height = static_cast<float>(renderTarget->GetSizeH());
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        pcontext->RSSetViewports(1, &vp);

        pcontext->DrawIndexed(6, 0, 0);

    }
}

void LchColorControlContext::DrawLchColorControlPrimaryTriangle(RenderTexture* renderTarget, float h_rgb)
{

    if (needInit)
    {
        if (CreateContext())
            needInit = false;
        else
            return;
    }

    auto pdevice = D3D11Graphic::GetDevice();
    auto pcontext = D3D11Graphic::GetContext();

    
    //设置参数
    PsUniform LchColorControlTriangleUniform = { h_rgb * 0.01745329F,(float)renderTarget->GetSizeW()};



    //更新constantBuffer
    D3D11_MAPPED_SUBRESOURCE msr;
    pcontext->Map(pUniformBufferPrimaryTriangle.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr);
    memcpy(msr.pData, &LchColorControlTriangleUniform, sizeof(LchColorControlTriangleUniform));
    pcontext->Unmap(pUniformBufferPrimaryTriangle.Get(), 0u);


    //绑定并绘制
    {
        {
            if (alphaBlurTex.GetSizeW()!= renderTarget->GetSizeW())
            {
                alphaBlurTex.Resize(2*renderTarget->GetSizeW(), 2*renderTarget->GetSizeW());
            }
        }
        alphaBlurTex.Clear();
        auto rt = alphaBlurTex.GetRT();
        pcontext->OMSetRenderTargets(1, &rt, nullptr);



        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pcontext->IASetVertexBuffers(0, 1, pLchColorControlPrimaryTriangleVertextBuffer.GetAddressOf(), &stride, &offset);
        pcontext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        pcontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pcontext->IASetInputLayout(pInputLayout.Get());

        pcontext->VSSetShader(pFilterVS.Get(), nullptr, 0);
        pcontext->PSSetShader(pLchColorControlPrimaryTrianglePS.Get(), nullptr, 0);


        pcontext->PSSetConstantBuffers(0, 1, pUniformBufferPrimaryTriangle.GetAddressOf());

#ifndef OK_COLOR
        auto ptexTable = pMaxChromaOfHueTable->GetTex();
        pcontext->PSSetShaderResources(0, 1, &ptexTable);
        pcontext->PSSetSamplers(0, 1, pTableSampler.GetAddressOf());
#endif

        D3D11_VIEWPORT vp = {};
        vp.Width = 2*static_cast<float>(renderTarget->GetSizeW());
        vp.Height = 2*static_cast<float>(renderTarget->GetSizeH());
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        pcontext->RSSetViewports(1, &vp);

        pcontext->DrawIndexed(6, 0, 0); 
    }
    {
        auto rt = renderTarget->GetRT();

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pcontext->IASetVertexBuffers(0, 1, pVertexBuffer_Filter.GetAddressOf(), &stride, &offset);
        pcontext->OMSetRenderTargets(1, &rt, nullptr);
        pcontext->PSSetShader(pAlphaBlurPS.Get(), nullptr, 0);
        auto ptex = alphaBlurTex.GetTex();


        pcontext->PSSetShaderResources(0, 1, &ptex);
        D3D11Graphic::BindDefaultBlend();


        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<float>(renderTarget->GetSizeW());
        vp.Height =  static_cast<float>(renderTarget->GetSizeH());
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        pcontext->RSSetViewports(1, &vp);
        pcontext->DrawIndexed(6, 0, 0);


    }
}
