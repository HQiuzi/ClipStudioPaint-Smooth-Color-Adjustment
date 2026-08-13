#ifndef _CSPMOD_ColorConvert_h
#define _CSPMOD_ColorConvert_h


#include<SDL3/SDL.h>

//这个宏要和着色器Tolerance.hlsli中对应的宏一起开关
#define OK_COLOR

namespace ui
{
	class UiColor;
}

class CSPMOD_ColorConvert
{

public:

	static inline float UInt32ToFloat(uint32_t val) { return  (float)((double)val / (double)UINT32_MAX); };


	//r g b:0~1
// L 0~100;
// C 0~100;
// H 0~360;

#ifndef OK_COLOR


	static void RGB2Lch(float r,float g,float b,float* L,float* c,float* lchH);
	static void RGB2HSV(float r,float g,float b,float* H,float* S,float* V);
	static void Lch2RGB(float L,float c,float h,float* r,float* g,float* b);


	static float*  GetMaxChromaOfHueTable(uint32_t* outLCount, uint32_t* outHueCount);


	//色相重映射，这个是用于UI的，和具体的颜色转换无关
	static float RGBH2LCHH(float h);
	static float LCHH2UIH(float h);




	static void Color96ToUIColor(struct Color96* pCol96, ui::UiColor* uiColor);
#else

	//使用OKHSL空间



	static void RGB2Lch(float r, float g, float b, float* L, float* c, float* lchH);


	//这个在代码中用于色相计算，改用OK COLOR中不再手动计算色相
	//static void RGB2HSV(float r, float g, float b, float* H, float* S, float* V);
	
	
	static void Lch2RGB(float L, float c, float h, float* r, float* g, float* b);

	//色相重映射，这个是用于UI的，和具体的颜色转换无关
	//使用OK COLOR时，不需要进行任何转换
	static float RGBH2LCHH(float h) { return h; };
	static float LCHH2UIH(float h) { return h; };






#endif // OLDCSPMODCOLORVERSION
};




























#endif // !_CSPMOD_ColorConvert_h
