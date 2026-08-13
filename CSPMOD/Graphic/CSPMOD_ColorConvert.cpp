#include "CSPMOD_ColorConvert.h"
#include"DUI/DuiCommon.h"
#include<SDL3/SDL.h>



#ifndef OK_COLOR




static float GetTValue(float t)
{
	if (t > 0.008856f)
	{
		return powf( t, 1.f / 3.f);
	}

	return 7.787037f * t + 0.137931f;
}


#ifndef M_PI
#define M_PI 3.1415926535898F
#endif // !M_PI

void OKLCH_to_OKLab(float L, float C, float H, float& a, float& b) {
	// H 从 0-1 转换为 0-2*PI 弧度
	float h_rad = H * 2.0f * static_cast<float>(M_PI);
	//float h_rad = H  * static_cast<float>(M_PI)/180.f;
	a = C * SDL_cosf (h_rad);
	b = C * SDL_sinf(h_rad);
}

// --- 步骤 2: OKLab 转 Linear sRGB ---
// 使用 OKLab 到 Linear sRGB 的转换矩阵
void OKLab_to_Linear_sRGB(float L, float a, float b, float& r, float& g, float& bl) {
	// OKLab 到 Linear sRGB 的转换矩阵
	// 来源：https://bottosson.github.io/posts/oklab/
	float l_val = L + 0.3963377774f * a + 0.2158037573f * b;
	float m_val = L - 0.1055613458f * a - 0.0638541728f * b;
	float s_val = L - 0.0894841775f * a - 1.2914855480f * b;

	// 对 l, m, s 分量进行立方运算
	float l_cubed = l_val * l_val * l_val;
	float m_cubed = m_val * m_val * m_val;
	float s_cubed = s_val * s_val * s_val;

	// Linear sRGB 到 RGB 的转换矩阵
	r = +4.0767416621f * l_cubed - 3.3077115913f * m_cubed + 0.2309699292f * s_cubed;
	g = -1.2684380046f * l_cubed + 2.6097574011f * m_cubed - 0.3413193965f * s_cubed;
	bl = -0.0041960863f * l_cubed - 0.3505336262f * m_cubed + 1.3547297126f * s_cubed;
}


void OKLCH_to_Linear_sRGB(float L, float C, float H, float& r, float& g, float& bl)
{
	float a, b;
	OKLCH_to_OKLab(L,C,H,a,b);
	OKLab_to_Linear_sRGB(L,a,b,r,g,bl);

	auto gamma_correct = [](float c) -> float {
		if (c <= 0.0031308f) {
			return 12.92f * c;
		}
		else {
			return 1.055f * SDL_powf(c, 1.0f / 2.4f) - 0.055f;
		}
		};
	r = gamma_correct(r);
	g = gamma_correct(g);
	bl = gamma_correct(bl);
}

void LCH_to_Linear_sRGB(float L, float C, float H, float& r, float& g, float& bl)
{
	float h_rad = H * 2.0f * static_cast<float>(M_PI);
	float lab_a = C * cosf(h_rad)*128.f;
	float lab_b = C * sinf(h_rad)*128.f;


	//参考白点 0.95047  1 1.08883
	const float Xn = 0.95047f;
	const float Yn = 1.00000f;
	const float Zn = 1.08883f;

	float fy = (L*100.f + 16.0f) / 116.0f;
	float fx = lab_a / 500.0f + fy;
	float fz = fy - lab_b / 200.0f;


	float x = Xn * ((fx * fx * fx > 0.008856f) ? (fx * fx * fx) : ((fx - 16.0f / 116.0f) / 7.787f));
	float y = Yn * ((fy * fy * fy > 0.008856f) ? (fy * fy * fy) : ((fy - 16.0f / 116.0f) / 7.787f));
	float z = Zn * ((fz * fz * fz > 0.008856f) ? (fz * fz * fz) : ((fz - 16.0f / 116.0f) / 7.787f));


	r = x * 3.2404542f - y * 1.5371385f - z * 0.4985314f;
	g = x * -0.9692660f + y * 1.8760108f + z * 0.0415560f;
	bl = x * 0.0556434f - y * 0.2040259f + z * 1.0572252f;
	//*r = SDL_clamp(R, 0.f, 1.f);
	//*g = SDL_clamp(G, 0.f, 1.f);
	//*b = SDL_clamp(B, 0.f, 1.f);

	auto gamma_correct = [](float c) -> float {
		if (c <= 0.0031308f) {
			return 12.92f * c;
		}
		else {
			return 1.055f *SDL_powf(c, 1.0f / 2.4f) - 0.055f;
		}
		};
	r = gamma_correct(r);
	g = gamma_correct(g);
	bl = gamma_correct(bl);

}







//r g b:0~1;
// L 0~100;
// C 0~100;
// H 0~360;
float maxChromaForL_Hue(float, float);
void CSPMOD_ColorConvert::RGB2Lch(float r, float g, float b, float* L, float* c, float* lchH)
{



	auto gamma_decorrect = [](float c) -> float {
		if (c <= 0.04045f) {
			return  c/ 12.92f;
		}
		else {
			return SDL_powf((c+ 0.055f)/ 1.055f,  2.4f);
		}
		};
	r = gamma_decorrect(r);
	g = gamma_decorrect(g);
	b = gamma_decorrect(b);


	float x= 0.412453f *r + 0.357580f *g + 0.180423f *b;
	float y=0.212671f * r + 0.715160f * g + 0.072169f *b;
	float z= 0.019334f *r + 0.119193f * g + 0.950227f * b;


	//参考白点 0.95047  1 1.08883
	x /= 0.95047f;
	//y /= 1;
	z /= 1.08883f;

	//T
	x = GetTValue(x);
	y = GetTValue(y);
	z = GetTValue(z);

	*L=  SDL_clamp( ((116 *y - 16)),0.f,100.f);
	float lab_a = 500 * (x - y);
	float lab_b= 200 * (y - z);




	//float h= degrees(SDL_atan2f(b, a));
	float _h= (SDL_atan2f(lab_b, lab_a))*180.f/3.1415927f;
	if (_h < 0.f)
		_h += 360.f;
	//*h= _h/360.f;
	*lchH = _h;




	float curC = sqrtf(lab_a * lab_a + lab_b * lab_b) / 1.28f;

	float CMax = maxChromaForL_Hue(*L, _h);//最大实际彩度，达到这个彩度时显示C为100
	if (CMax < 1e-6f)
	{
		curC = 0.f;
	}
	else
	{
		curC /= CMax;
	}

	*c = SDL_clamp(curC, 0.f, 100.f);
}

void CSPMOD_ColorConvert::RGB2HSV(float r, float g, float b, float* H, float* S, float* V)
{

	float fMax, fMin;
	float cuR = r;
	float cuG = g;
	float cuB = b;
	
	fMax = SDL_max(SDL_max(cuR, cuG), cuB);
	fMin = SDL_min(SDL_min(cuR, cuG), cuB);

	float fD = fMax - fMin;

	float fH;
	float fS;
	//    float fV;

	if (fMax == 0)
		fS = 0;
	else
		fS = fD / fMax;

	if (fS == 0)
		fH = 0;
	else
	{
		if (fMax == cuR)
		{
			fH = (cuG - cuB) / fD; //s不为0的时候 D一定不为0
			if (fH < 0)
				fH += 6;
		}
		else if (fMax == cuG)
		{
			fH = 2 + (cuB - cuR) / fD;
		}
		else
		{
			fH = 4 + (cuR - cuG) / fD;
		}
	}
	*H = fH;
	*S = fS;
	*V = fMax;
	//return float3(fH, fS, fMax);
}




#define __LCH
//#define OKLCH

//打表
static float maxChroma[101];
float maxChromaForL(float L) {

	static bool maxChromaInited = false;
	if (!maxChromaInited)
	{
		for (int index = 0; index < 101; index++)
		{
			float curL = float(index);


			float minC = 1e9f;
			for (int i = 0; i < 360; ++i) {
				//float h = i * M_PI / 180.0F;
				float h = i / 360.f;
				// 二分搜索
				float low = 0.0f, high = 1.5f; // 初始范围
				for (int iter = 0; iter < 20; ++iter) {
					float mid = (low + high) * 0.5f;
					float rgb[3];

#ifdef OKLCH
					//OKLCH_to_OKLab();
					OKLCH_to_Linear_sRGB(curL/100.f, mid, h, rgb[0], rgb[1], rgb[2]);
#endif

#ifdef __LCH
					LCH_to_Linear_sRGB(curL / 100.f, mid, h, rgb[0], rgb[1], rgb[2]);
#endif
					if (rgb[0] >= 0 && rgb[0] <= 1 &&
						rgb[1] >= 0 && rgb[1] <= 1 &&
						rgb[2] >= 0 && rgb[2] <= 1) {
						low = mid; // 合法，尝试更大
					}
					else {
						high = mid; // 超出，缩小范围
					}
				}
				minC = SDL_min(minC, low);
			}

			maxChroma[index] = minC;
		}

		maxChromaInited = true;
	}


	return maxChroma[SDL_clamp(static_cast<uint32_t>(L*100.f), 0, 100)];
}








#define HUETOTAL 3600
#define LIGHTTOTAL 101
static float maxChroma_Hue[101][HUETOTAL];


float maxChromaForL_Hue(float L,float Hue) {

	static bool maxChromaInited = false;
	if (!maxChromaInited)
	{
		for (int index = 0; index < 101; index++)
		{
			float curL = float(index);


			for (int i = 0; i < HUETOTAL; ++i) {
				//float h = i * M_PI / 180.0F;
				float h =float(i) / HUETOTAL;
				// 二分搜索
				float low = 0.0f, high = 1.5f; // 初始范围
				for (int iter = 0; iter < 20; ++iter) {
					float mid = (low + high) * 0.5f;
					float rgb[3];

#ifdef OKLCH
					//OKLCH_to_OKLab();
					OKLCH_to_Linear_sRGB(curL/100.f, mid, h, rgb[0], rgb[1], rgb[2]);
#endif

#ifdef __LCH
					LCH_to_Linear_sRGB(curL / 100.f, mid, h, rgb[0], rgb[1], rgb[2]);
#endif
					if (rgb[0] >= 0 && rgb[0] <= 1 &&
						rgb[1] >= 0 && rgb[1] <= 1 &&
						rgb[2] >= 0 && rgb[2] <= 1) {
						low = mid; // 合法，尝试更大
					}
					else {
						high = mid; // 超出，缩小范围
					}
				}

				maxChroma_Hue[index][i] = low;

				//minC = SDL_min(minC, low);
			}

			//maxChroma[index] = minC;
		}

		maxChromaInited = true;
	}


	//return maxChroma[SDL_clamp(static_cast<uint32_t>(L * 100.f), 0, 100)];
	return maxChroma_Hue[SDL_clamp(static_cast<uint32_t>(L), 0, 100)][SDL_clamp(static_cast<uint32_t>(HUETOTAL*Hue/360.f), 0, HUETOTAL-1)];
}



















float GetP1(float cuV, float cuS, float cuF)
{
	return cuV * (1 - cuS);
}
float GetP2(float cuV, float cuS, float cuF)
{
	return cuV * (1 - cuS * cuF);
}
float GetP3(float cuV, float cuS, float cuF)
{
	return cuV * (1 - cuS * (1 - cuF));
}
void HSVtoRGB(float H,float S,float V,float& r,float& g,float& b)
{
	//float H;
	//float S ;
	//float V ;


	float cuF = H - floorf(H);




	//float3 result;
	if (H < 1)
	{
		r = V;
		g = GetP3(V, S, cuF);
		b = GetP1(V, S, cuF);
	}
	else if (H < 2)
	{
		r = GetP2(V, S, cuF);
		g = V;
		b = GetP1(V, S, cuF);
	}
	else if (H < 3)
	{
		r = GetP1(V, S, cuF);
		g = V;
		b = GetP3(V, S, cuF);
	}
	else if (H < 4)
	{
		r = GetP1(V, S, cuF);
		g = GetP2(V, S, cuF);
		b = V;
	}
	else if (H < 5)
	{
		r = GetP3(V, S, cuF);
		g = GetP1(V, S, cuF);
		b = V;
	}
	else
	{
		r = V;
		g = GetP1(V, S, cuF);
		b = GetP2(V, S, cuF);
	}
	r = SDL_clamp(r, 0.f, 1.f);
	g = SDL_clamp(g, 0.f, 1.f);
	b = SDL_clamp(b, 0.f, 1.f);
}




//RGB色相(UI色相)转为LCH色相
//这个也可以打表
static float UIH2LCHH_map[HUETOTAL];
static float LCHH2UIH_map[HUETOTAL];
static bool UIH_LCHH_map_Inited = false;
//其实应该是UIH2LCHH
float CSPMOD_ColorConvert::RGBH2LCHH(float h_deg)
{
	if (UIH_LCHH_map_Inited)
	{
		//线性采样
		float hsample = (h_deg / 360.f) * HUETOTAL ;
		int index =(int) SDL_floorf(hsample);
		hsample = hsample - index;

		if (index == HUETOTAL)index = 0;
		float LValue = UIH2LCHH_map[index];
		float RValue = UIH2LCHH_map[index + 1 == HUETOTAL ? 0 : (index + 1)];
		if (LValue > 180.f && RValue < 180.f)
		{
			RValue += 360.f;
		}
			float result= LValue * (1 - hsample) + RValue * hsample;
			if (result > 360.f)result -= 360.f;
			return result;
	}
	
	//打表
	for (int i = 0; i < HUETOTAL; i++)
	{
		float r, g, b;
		float h =   360.f*i / HUETOTAL;
		HSVtoRGB(h / 60.f, 1.f, 0.5f, r, g, b);
		float L, C, H;
		CSPMOD_ColorConvert::RGB2Lch(r, g, b, &L, &C, &H);
		UIH2LCHH_map[i] = H;
		
		//保证单增
		if (i > 0)
		{
			if ((UIH2LCHH_map[i] < UIH2LCHH_map[i - 1]&& UIH2LCHH_map[i - 1]- UIH2LCHH_map[i]<180.f)
				|| (UIH2LCHH_map[i] > UIH2LCHH_map[i - 1] && UIH2LCHH_map[i] -UIH2LCHH_map[i - 1]  > 180.f))
			{
				UIH2LCHH_map[i] = UIH2LCHH_map[i - 1];
			}
		}
	}




	//逆向表
	//先寻找LCHH为0时的索引值
	//int lchStartIndex = 0;
	int curFindIndex = 0;
	//for (int i = 0; i < HUETOTAL; i++)
	//{
	//	float L = UIH2LCHH_map[i];
	//	float R = UIH2LCHH_map[(i + 1)%HUETOTAL];
	//	//先转为-pi到pi
	//	if (L >= 180.f)L -= 360.f;
	//	if (R >= 180.f)R -= 360.f;
	//	if (L <= 0.f && R >= 0.f)
	//	{
	//		lchStartIndex = i;
	//		curFindIndex = lchStartIndex;
	//		break;
	//	}
	//}
	for (int i=0;i< HUETOTAL; i++)
	{
		float h_lch = 360.f * i / HUETOTAL;
		//从curFindIndex开始寻找目标索引
		for (int j = 0; j < HUETOTAL; j++)
		{
			int testIndex = (j + curFindIndex)% HUETOTAL;
			float L = UIH2LCHH_map[testIndex];
			float R = UIH2LCHH_map[(testIndex + 1) % HUETOTAL];
			//先转为以h_lch为中心的-pi到pi数值
			L -= h_lch;
			R -= h_lch;
			if (L >= 180.f)L -= 360.f;
			else if (L < -180.f)L += 360.f;
			if (R >= 180.f)R -= 360.f;
			else if (R < -180.f)R += 360.f;

			if (L <= 0.f && R >= 0.f)
			{
				curFindIndex = testIndex;
				float lert;
				if (R - L < 1e-6f)lert = 0;
				else
					lert = (0.f - L) / (R - L);
				lert = SDL_clamp(lert,0,1);
				LCHH2UIH_map[i] = (testIndex + lert) * 360.F / HUETOTAL;
				break;
			}
		}





	}

	//调试发现Release下，1800之后LCHH2UIH_map的数值为0。DEBUG没有问题 
	for (int i = 0; i < HUETOTAL; i++)
	{
		SDL_Log("%d:%f-%f", i, UIH2LCHH_map[i], LCHH2UIH_map[i]);
	}

	UIH_LCHH_map_Inited = true;
	return RGBH2LCHH(h_deg);
}

float CSPMOD_ColorConvert::LCHH2UIH(float h_lchDeg)
{



	if (UIH_LCHH_map_Inited)
	{
		//线性采样
		float hsample = (h_lchDeg / 360.f) * HUETOTAL;
		int index = (int)SDL_floorf(hsample);
		hsample = hsample - index;

		if (index == HUETOTAL)index = 0;
		float LValue = LCHH2UIH_map[index];
		float RValue = LCHH2UIH_map[index + 1 == HUETOTAL ? 0 : (index + 1)];
		if (LValue > 180.f && RValue < 180.f)
		{
			RValue += 360.f;
		}
		float result = LValue * (1 - hsample) + RValue * hsample;
		if (result > 360.f)result -= 360.f;
		return result;
	}
	RGBH2LCHH(0);//触发打表
	return LCHH2UIH(h_lchDeg);
}

//void CSPMOD_ColorConvert::Color96ToRGB(Color96* pCol96, float*r,float* g,float*b)
//{
//
//}















//r g b:0~1;
// L 0~100;
// C 0~100;
// H 0~360;

//3.14/180;
#define DEG2RAD 0.0174532925f
void CSPMOD_ColorConvert::Lch2RGB(float L, float c, float h, float* r, float* g, float* b)
{


	//OKLCH范围为0~1
//#define OKLCH
#ifdef OKLCH


	//float _OKLCH_L = L / 100.f;
	////float _OKLCH_C = c  / 100.f;
	//float _OKLCH_C = c*maxChromaForL_Hue(L,h) / 100.f;
	//float _OKLCH_H = h / 360.f;
	//float R ;
	//float G ;
	//float B ;
	//OKLCH_to_Linear_sRGB(_OKLCH_L, _OKLCH_C, _OKLCH_H, R, G, B);
	////LCH_to_Linear_sRGB(_OKLCH_L, _OKLCH_C, _OKLCH_H, R, G, B);
	//if (R >= 0 && R <= 1 &&
	//	G >= 0 &&G <= 1 &&
	//	B >= 0 && B <= 1) {
	//	*r = R; *g = G;*b=B; // 合法
	//}
	//else
	//{
	//	*r = 0;
	//	*g = 0;
	//	*b = 0;
	//}





	float _OKLCH_L = L / 100.f;
	//float _OKLCH_C = c* maxChromaForL(_OKLCH_L) / 100.f;
	float _OKLCH_C = c* maxChromaForL_Hue(L, h) / 100.f;
	float _OKLCH_H = h / 360.f;

	float _OKLab_a;
	float _OKLab_b;
	OKLCH_to_OKLab(_OKLCH_L, _OKLCH_C, _OKLCH_H, _OKLab_a, _OKLab_b);
	float R ;
	float G ;
	float B ;
	OKLab_to_Linear_sRGB(_OKLCH_L, _OKLab_a, _OKLab_b,R,G,B);

	*r = SDL_clamp(R, 0.f, 1.f);
	*g = SDL_clamp(G, 0.f, 1.f);
	*b = SDL_clamp(B, 0.f, 1.f);
#endif



#ifdef __LCH
	float realL = L / 100.f;
	//float realC= c * maxChromaForL(realL) / 100.f;
	float realC= c * maxChromaForL_Hue(L,h) / 100.f;
	//float realC=SDL_min( c / 100.f ,maxChromaForL_Hue(L,h)) ;
	float realH = h / 360.f;
	float R;
	float G;
	float B;
	LCH_to_Linear_sRGB(realL, realC, realH, R, G, B);
//
//	if (R >= 0 && R <= 1 &&
//	G >= 0 &&G <= 1 &&
//	B >= 0 && B <= 1) {
//	*r = R; *g = G;*b=B; // 合法
//}
//else
//{
//	*r = 0;
//	*g = 0;
//	*b = 0;
//}

	*r = SDL_clamp(R, 0.f, 1.f);
	*g = SDL_clamp(G, 0.f, 1.f);
	*b = SDL_clamp(B, 0.f, 1.f);

#endif // __LCH


#if 0
	float lab_a = c * cosf(h* DEG2RAD);
	float lab_b = c * sinf(h* DEG2RAD);


	//参考白点 0.95047  1 1.08883
	const float Xn = 0.95047f;
	const float Yn = 1.00000f;
	const float Zn = 1.08883f;

	float fy = (L + 16.0f) / 116.0f;
	float fx = lab_a / 500.0f + fy;
	float fz = fy - lab_b / 200.0f;


	float x = Xn * ((fx * fx * fx > 0.008856f) ? (fx * fx * fx) : ((fx - 16.0f / 116.0f) / 7.787f));
	float y = Yn * ((fy * fy * fy > 0.008856f) ? (fy * fy * fy) : ((fy - 16.0f / 116.0f) / 7.787f));
	float z = Zn * ((fz * fz * fz > 0.008856f) ? (fz * fz * fz) : ((fz - 16.0f / 116.0f) / 7.787f));


	float R = x * 3.2404542f - y * 1.5371385f - z * 0.4985314f;
	float G = x * -0.9692660f + y * 1.8760108f + z * 0.0415560f;
	float B = x * 0.0556434f - y * 0.2040259f + z * 1.0572252f;
	*r = SDL_clamp( R,0.f,1.f);
	*g = SDL_clamp(G, 0.f, 1.f);
	*b = SDL_clamp(B, 0.f, 1.f);
#endif
}

float* CSPMOD_ColorConvert::GetMaxChromaOfHueTable(uint32_t* outLCount, uint32_t* outHueCount)
{
	//打表
	maxChromaForL_Hue(0.f,0.f);
	*outLCount = LIGHTTOTAL;
	*outHueCount = HUETOTAL;
	return (float*)maxChroma_Hue;
}




#else



#include"ok_color.h"




//inline float srgb_transfer_function_inv(float a)
//{
//	return .04045f < a ? powf((a + .055f) / 1.055f, 2.4f) : a / 12.92f;
//}


//r g b:0~1
// L 0~100;
// C 0~100;
// H 0~360;
void CSPMOD_ColorConvert::RGB2Lch(float r, float g, float b, float* L, float* c, float* lchH)
{
	ok_color::HSL result = ok_color::srgb_to_okhsl({r,g,b});


	//检查lsh范围
	*L = result.l*100.f;
	*c = result.s * 100.f;
	*lchH = result.h*360.f;

}




void CSPMOD_ColorConvert::Lch2RGB(float L, float c, float h, float* r, float* g, float* b)
{
	ok_color::RGB result = ok_color::okhsl_to_srgb({ h / 360.f,c/100.f,L / 100.f});


	*r = result.r;
	*g = result.g;
	*b = result.b;
}




















































#endif