#ifndef LchColorControlContext_h
#define LchColorControlContext_h


#include <d3d11.h>
#include <cstdint>
#include"Graphic/Texture.h"
#include<memory>


class LchColorControlContext
{
public:
	inline static LchColorControlContext* pthis=nullptr;
	static LchColorControlContext& GetIns() { if(!pthis)pthis=new LchColorControlContext; return *pthis; }
	static void ShutDown() {
		delete pthis;
		pthis = nullptr;
	};
	


	bool CreateContext();


	void DrawLchColorControl(RenderTexture* renderTarget, float L);
	void DrawLchColorControlPrimaryRectangle(RenderTexture* renderTarget, float h_rgb);
	void DrawLchColorControlPrimaryTriangle(RenderTexture* renderTarget, float h_rgb);
	void DrawLchColorControlPrimaryHueCircle(RenderTexture* renderTarget);


private:


	bool needInit = true;

	struct PsUniform
	{
		float var1;
		float var2;
		float var3;
		float var4;
	};

	Microsoft::WRL::ComPtr <ID3D11Buffer> pVertexBuffer_Filter;
	Microsoft::WRL::ComPtr <ID3D11Buffer> pIndexBuffer;

	Microsoft::WRL::ComPtr <ID3D11Buffer> pLchColorControlPrimaryRectangleVertextBuffer;
	//Microsoft::WRL::ComPtr <ID3D11Buffer> pLchColorControlPrimaryRectangleIndexBuffer;//用上面的

	Microsoft::WRL::ComPtr <ID3D11Buffer> pLchColorControlPrimaryTriangleVertextBuffer;
	//Microsoft::WRL::ComPtr <ID3D11Buffer> pLchColorControlPrimaryTriangleIndexBuffer;


	Microsoft::WRL::ComPtr <ID3D11InputLayout> pInputLayout;
	//Microsoft::WRL::ComPtr <id3d11topo> pInputLayout;

	Microsoft::WRL::ComPtr <ID3D11VertexShader> pFilterVS;
	//Microsoft::WRL::ComPtr <ID3D11VertexShader> pLCHVS;
	Microsoft::WRL::ComPtr <ID3D11PixelShader> pLchColorControlPS;
	Microsoft::WRL::ComPtr <ID3D11PixelShader> pLchColorControlPrimaryRectanglePS;
	Microsoft::WRL::ComPtr <ID3D11PixelShader> pLchColorControlPrimaryTrianglePS;
	Microsoft::WRL::ComPtr <ID3D11PixelShader> pLchColorControlPrimaryHueCirclePS;
	Microsoft::WRL::ComPtr <ID3D11PixelShader> pAlphaBlurPS;

	Microsoft::WRL::ComPtr <ID3D11Buffer> pUniformBuffer;//像素着色器参数
	Microsoft::WRL::ComPtr <ID3D11Buffer> pUniformBufferPrimaryRectangle;//像素着色器参数
	Microsoft::WRL::ComPtr <ID3D11Buffer> pUniformBufferPrimaryTriangle;//像素着色器参数
	Microsoft::WRL::ComPtr <ID3D11Buffer> pUniformBufferPrimaryHueCirclePS;//像素着色器参数

	Microsoft::WRL::ComPtr <ID3D11SamplerState> pTableSampler;//像素着色器参数

#ifndef OK_COLOR
	std::unique_ptr< Texture> pMaxChromaOfHueTable;
#endif
	RenderTexture alphaBlurTex;


};


#endif // !1
