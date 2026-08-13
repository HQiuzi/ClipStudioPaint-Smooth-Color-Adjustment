
//这个宏要配合代码中CSPMOD_ColorConvert.h对应的宏进行开关
#define OK_COLOR
#ifndef OK_COLOR



#include"HSVConvert.hlsli"

//先简单使用距离的方式，后续转为使用Lab色差
//float CalcPixWeight(float3 color1, float3 color2, float tolerance)
//{
//    if (tolerance > distance(color1, color2) / 1.732051f)
//    {
//        return 1.f;
//    }
//    else
//    {
//        return 0.f;
//    }
//    return 1.f;
//}


//简单计算RGB距离,tolerance参数为半透明选区范围，线性
//会出现明显的高亮条带
//float CalcPixWeight(float3 color1, float3 color2, float tolerance)
//{
//    float d = distance(color1, color2) / 1.732051f;
//    if (tolerance > d)
//    {
//        return (tolerance - d) / tolerance;
//    }
//    else
//    {
//        return 0.f;
//    }
//    return 1.f;
//}

//简单计算RGB距离,tolerance参数为半透明选区范围，高斯函数
//容差高的时候范围巨大，容差低的时候边缘巨硬
//float CalcPixWeight(float3 color1, float3 color2, float tolerance)
//{
//    float d = distance(color1, color2) / 1.732051f;//sqrt3
//    return 1 / (2 * 3.1415927f * tolerance) * exp(-d * d / (2 * tolerance));
//    //return 0.5f*cos(3.1415927f*d/ tolerance)+0.5f;
//}

//简单计算RGB距离,tolerance参数为半透明选区范围，线性
//会出现明显的高亮条带
//float CalcPixWeight(float3 color1, float3 color2, float tolerance)
//{
//    float d = distance(color1, color2) / 1.732051f;
//    if (tolerance > d*d)
//    {
//        return (tolerance - d*d) / tolerance;
//    }
//    else
//    {
//        return 0.f;
//    }
//    return 1.f;
//}


//简单计算RGB距离,tolerance参数为半透明选区范围，正弦
//截断功能欠缺，容差稍大会导致选区过大
//float CalcPixWeight(float3 color1, float3 color2, float tolerance)
//{
//    float d = distance(color1, color2) / 1.732051f; //sqrt3
//    //return 1 / (2 * 3.1415927f * tolerance) * exp(-d * d / (2 * tolerance));
//    tolerance += 0.01F;
//    if (d > 2 * tolerance)
//        return 0.f;
//     return 0.5f * cos(3.1415927f * 0.5f * d / tolerance) + 0.5f;
//}









//LAB D65

float GetTValue(float t)
{
    if (t>0.008856)
    {
        return pow(t,1.f/3.f);
    }
    
    return 7.787037 * t + 0.137931;
    
}

float3 RGB2Lab(float3 rgb)
{
    float3 xyz;
    xyz.x = 0.412453 * rgb.r + 0.357580f * rgb.g + 0.180423 * rgb.b;
    xyz.y = 0.212671 * rgb.r + 0.715160 * rgb.g + 0.072169 * rgb.b;
    xyz.z = 0.019334 * rgb.r + 0.119193 * rgb.g + 0.950227 * rgb.b;
    
    xyz.x /= 0.95047;
    //xyz.y /= 1;
    xyz.z /= 1.08883;
    
    //T
    xyz.x = GetTValue(xyz.x);
    xyz.y = GetTValue(xyz.y);
    xyz.z = GetTValue(xyz.z);
    
    float3 Lab;
    Lab.x = 116 * xyz.y - 16;//L
    Lab.y = 500 * (xyz.x-xyz.y);//a
    Lab.b = 200 * (xyz.y-xyz.z);//b
    
    return Lab;
}

#define DEG2RAD 0.0174532925
#define PI 3.14159265358
float DistanceLab(float3 lab1, float3 lab2)
{
    //deltaE00
    
     // 1. 提取分量
    float L1 = lab1.x, a1 = lab1.y, b1 = lab1.z;
    float L2 = lab2.x, a2 = lab2.y, b2 = lab2.z;
    
    
    // 2. 计算平均值和初步差值
    float L_bar = 0.5 * (L1 + L2);
    float C1 = sqrt(a1 * a1 + b1 * b1);
    float C2 = sqrt(a2 * a2 + b2 * b2);
    float C_bar = 0.5 * (C1 + C2);
    
    // 3. 计算 G 因子 (彩度加权)
    float C_bar_pow7 = pow(C_bar, 7.0);
    float G = 0.5 * (1.0 - sqrt(C_bar_pow7 / (C_bar_pow7 + 25.0 * 25.0 * 25.0 * 25.0 * 25.0 * 25.0 * 25.0))); // 25^7

     // 4. 计算修正后的 a'
    float a1_prime = a1 * (1.0 + G);
    float a2_prime = a2 * (1.0 + G);
    
        // 5. 重新计算彩度 C' 和 色相 h'
    float C1_prime = sqrt(a1_prime * a1_prime + b1 * b1);
    float C2_prime = sqrt(a2_prime * a2_prime + b2 * b2);
    float C_bar_prime = 0.5 * (C1_prime + C2_prime);
    
        // 计算 h' (注意象限处理，使用 atan2)
    float h1_prime = degrees(atan2(b1, a1_prime));
    if (h1_prime < 0.0)
        h1_prime += 360.0;
    
    float h2_prime = degrees(atan2(b2, a2_prime));
    if (h2_prime < 0.0)
        h2_prime += 360.0;

    // 6. 计算差值 Delta
    float dL_prime = L2 - L1;
    float dC_prime = C2_prime - C1_prime;
    
    float dh_prime;
    float dH_prime;
    
    if (abs(h2_prime - h1_prime) <= 180.0)
    {
        dh_prime = h2_prime - h1_prime;
    }
    else
    {
        if (h2_prime <= h1_prime)
            dh_prime = h2_prime - h1_prime + 360.0;
        else
            dh_prime = h2_prime - h1_prime - 360.0;
    }
    
    dH_prime = 2.0 * sqrt(C1_prime * C2_prime) * sin(0.5 * dh_prime * DEG2RAD);

    // 7. 计算平均值 H'
    float H_bar_prime;
    if (abs(h2_prime - h1_prime) <= 180.0)
        H_bar_prime = 0.5 * (h1_prime + h2_prime);
    else
    {
        if (h1_prime + h2_prime < 360.0)
            H_bar_prime = 0.5 * (h1_prime + h2_prime + 360.0);
        else
            H_bar_prime = 0.5 * (h1_prime + h2_prime - 360.0);
    }

    // 8. 计算权重函数 T, S_L, S_C, S_H
    float H_bar_rad = H_bar_prime * DEG2RAD;
    float T = 1.0
            - 0.17 * cos(H_bar_rad - 30.0 * DEG2RAD)
            + 0.24 * cos(2.0 * H_bar_rad)
            + 0.32 * cos(3.0 * H_bar_rad + 6.0 * DEG2RAD)
            - 0.20 * cos(4.0 * H_bar_rad - 63.0 * DEG2RAD);

    float dTheta = 30.0 * exp(-pow((H_bar_prime - 275.0) / 25.0, 2.0));
    float R_C = 2.0 * sqrt(pow(C_bar_prime, 7.0) / (pow(C_bar_prime, 7.0) + 25.0 * 25.0 * 25.0 * 25.0 * 25.0 * 25.0 * 25.0));
    float S_L = 1.0 + (0.015 * pow(L_bar - 50.0, 2.0)) / sqrt(20.0 + pow(L_bar - 50.0, 2.0));
    float S_C = 1.0 + 0.045 * C_bar_prime;
    float S_H = 1.0 + 0.015 * C_bar_prime * T;
    float R_T = -sin(2.0 * dTheta * DEG2RAD) * R_C;

    // 9. 最终计算 (假设 kL=kC=kH=1)
    float term1 = dL_prime / S_L;
    float term2 = dC_prime / S_C;
    float term3 = dH_prime / S_H;
    
    float deltaE = sqrt(term1 * term1 + term2 * term2 + term3 * term3 + R_T * term2 * term3);

    return deltaE;
}

float CalcPixWeight(float3 color1, float3 color2, float tolerance)
{
    float t = tolerance * 200*3+0.001;
    float d = DistanceLab(RGB2Lab(color1), RGB2Lab(color2));
    d = d * d;
    if (t > d)
    {
        return (t - d) / t;
    }
    else
    {
        return 0.f;
    }
    return 1.f;
}




float _LinearToGamma_(float c)
{
    if (c <= 0.0031308f)
    {
        return 12.92f * c;
    }
    else
    {
        return 1.055f * pow(c, 1.0f / 2.4f) - 0.055f;
    }
}
float3 LinearToGamma(float3 c)
{
    return float3(
    _LinearToGamma_(c.r),
    _LinearToGamma_(c.g),
    _LinearToGamma_(c.b));

}

float _GammaToLinear_(float c)
{
    if (c <= 0.04045f)
    {
        //return c / 12.92f;
        return c * 0.077399381;
    }
    else
    {
        return pow((c + 0.055f) / 1.055f, 2.4f);
    }
}
float3 GammaToLinear(float3 c)
{
    return float3(
    _GammaToLinear_(c.r),
    _GammaToLinear_(c.g),
    _GammaToLinear_(c.b));

}




//0~100 0~100 0~360
float3 LCH_to_sRGB(float3 LCH)
{
    //float h_rad = LCH.z * DEG2RAD;
    float h_rad = LCH.z ;
    float lab_a = LCH.y * cos(h_rad)*1.28f;//a实际是-128~128,软件实现中把C也设定为了100
    float lab_b = LCH.y * sin(h_rad)*1.28f;
    
    //参考白点 0.95047  1 1.08883
    const float Xn = 0.95047f;
    const float Yn = 1.00000f;
    const float Zn = 1.08883f;
    
    
    float fy = (LCH.x + 16.0f) / 116.0f;
    float fx = lab_a / 500.0f + fy;
    float fz = fy - lab_b / 200.0f;
    
    float x = Xn * ((fx * fx * fx > 0.008856f) ? (fx * fx * fx) : ((fx - 16.0f / 116.0f) / 7.787f));
    float y = Yn * ((fy * fy * fy > 0.008856f) ? (fy * fy * fy) : ((fy - 16.0f / 116.0f) / 7.787f));
    float z = Zn * ((fz * fz * fz > 0.008856f) ? (fz * fz * fz) : ((fz - 16.0f / 116.0f) / 7.787f));
    
    float3 resultCol;
    resultCol.r = x * 3.2404542f - y * 1.5371385f - z * 0.4985314f;
    resultCol.g = x * -0.9692660f + y * 1.8760108f + z * 0.0415560f;
    resultCol.b = x * 0.0556434f - y * 0.2040259f + z * 1.0572252f;
    return clamp(LinearToGamma(resultCol),0,1);
}



//因为这个函数目前只用来计算色相，这里移除一些内容
float3 sRGB_to_LCH(float3 srgb)
{
    float3 rgb = GammaToLinear(srgb);
    
    float x = 0.412453f * rgb.r + 0.357580f * rgb.g + 0.180423f * rgb.b;
    float y = 0.212671f * rgb.r + 0.715160f * rgb.g + 0.072169f * rgb.b;
    float z = 0.019334f * rgb.r + 0.119193f * rgb.g + 0.950227f * rgb.b;
    
    //参考白点 0.95047  1 1.08883
    x /= 0.95047f;
	//y /= 1;
    z /= 1.08883f;
    
    x = GetTValue(x);
    y = GetTValue(y);
    z = GetTValue(z);
    
    float3 lch;
    //lch.x = clamp(((116 * y - 16)), 0.f, 100.f);
    float lab_a = 500 * (x - y);
    float lab_b = 200 * (y - z);
    //lch.y = clamp(sqrt(lab_a * lab_a + lab_b * lab_b), 0.f, 100.f);
    lch.z = atan2(lab_b, lab_a);
    if (lch.z < 0)
        lch.z = lch.z+ 
    2 * 3.14159265359f;
    return lch;

}




float RGBH2LCHH(float h)
{
    float hInRgb = h * 0.95493f; //0~2pi 转为0~6;
    float3 rgb=HSVtoRGB(float3(hInRgb, 1.f, 0.5f));
    //float L, C, H;
    return sRGB_to_LCH(rgb).z;
}


#define SAMPLER_FROM_TABLE(h_rad,light) L_MaxC_Table.Sample(TableSampler, float2(h_rad*0.1591549431f,light*0.01f))


float DistancePointToLine2D(float2 P, float2 A, float2 B)
{
    float2 AB = B - A;
    float2 AP = P - A;
    
    // 2D 叉乘公式：AB.x * AP.y - AB.y * AP.x
    // 这计算的是平行四边形的面积
    float cross = AB.x * AP.y - AB.y * AP.x;
    
    // 距离 = 面积 / 底边长度
    float lenAB = length(AB);
    
    // 防止除以 0 (如果 A 和 B 重合)
    if (lenAB < 1e-6)
        return length(AP);
    
    return abs(cross) / lenAB;
}

#else

//见ok_color.h

//LAB D65

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062f
#define pi PI
#define FLT_MAX 3.402823466e+38f

#define DEG2RAD 0.0174532925

#define sgn sign
#define fmax max
#define fmin min
#define sqrtf sqrt
#define fabs abs


#define srgb_transfer_function _LinearToGamma_
#define srgb_transfer_function_inv _GammaToLinear_

typedef float3 Lab;
typedef float3 RGB;
typedef float3 HSV;
typedef float3 HSL;
typedef float2 LC;

typedef float2 ST;
typedef float3 Cs;


float GetTValue(float t)
{
    if (t > 0.008856)
    {
        return pow(t, 1.f / 3.f);
    }
    
    return 7.787037 * t + 0.137931;
    
}
float3 RGB2Lab(float3 rgb)
{
    float3 xyz;
    xyz.x = 0.412453 * rgb.r + 0.357580f * rgb.g + 0.180423 * rgb.b;
    xyz.y = 0.212671 * rgb.r + 0.715160 * rgb.g + 0.072169 * rgb.b;
    xyz.z = 0.019334 * rgb.r + 0.119193 * rgb.g + 0.950227 * rgb.b;
    
    xyz.x /= 0.95047;
    //xyz.y /= 1;
    xyz.z /= 1.08883;
    
    //T
    xyz.x = GetTValue(xyz.x);
    xyz.y = GetTValue(xyz.y);
    xyz.z = GetTValue(xyz.z);
    
    float3 Lab;
    Lab.x = 116 * xyz.y - 16; //L
    Lab.y = 500 * (xyz.x - xyz.y); //a
    Lab.b = 200 * (xyz.y - xyz.z); //b
    
    return Lab;
}



float DistanceLab(float3 lab1, float3 lab2)
{
    //deltaE00
    
     // 1. 提取分量
    float L1 = lab1.x, a1 = lab1.y, b1 = lab1.z;
    float L2 = lab2.x, a2 = lab2.y, b2 = lab2.z;
    
    
    // 2. 计算平均值和初步差值
    float L_bar = 0.5 * (L1 + L2);
    float C1 = sqrt(a1 * a1 + b1 * b1);
    float C2 = sqrt(a2 * a2 + b2 * b2);
    float C_bar = 0.5 * (C1 + C2);
    
    // 3. 计算 G 因子 (彩度加权)
    float C_bar_pow7 = pow(C_bar, 7.0);
    float G = 0.5 * (1.0 - sqrt(C_bar_pow7 / (C_bar_pow7 + 25.0 * 25.0 * 25.0 * 25.0 * 25.0 * 25.0 * 25.0))); // 25^7

     // 4. 计算修正后的 a'
    float a1_prime = a1 * (1.0 + G);
    float a2_prime = a2 * (1.0 + G);
    
        // 5. 重新计算彩度 C' 和 色相 h'
    float C1_prime = sqrt(a1_prime * a1_prime + b1 * b1);
    float C2_prime = sqrt(a2_prime * a2_prime + b2 * b2);
    float C_bar_prime = 0.5 * (C1_prime + C2_prime);
    
        // 计算 h' (注意象限处理，使用 atan2)
    float h1_prime = degrees(atan2(b1, a1_prime));
    if (h1_prime < 0.0)
        h1_prime += 360.0;
    
    float h2_prime = degrees(atan2(b2, a2_prime));
    if (h2_prime < 0.0)
        h2_prime += 360.0;

    // 6. 计算差值 Delta
    float dL_prime = L2 - L1;
    float dC_prime = C2_prime - C1_prime;
    
    float dh_prime;
    float dH_prime;
    
    if (abs(h2_prime - h1_prime) <= 180.0)
    {
        dh_prime = h2_prime - h1_prime;
    }
    else
    {
        if (h2_prime <= h1_prime)
            dh_prime = h2_prime - h1_prime + 360.0;
        else
            dh_prime = h2_prime - h1_prime - 360.0;
    }
    
    dH_prime = 2.0 * sqrt(C1_prime * C2_prime) * sin(0.5 * dh_prime * DEG2RAD);

    // 7. 计算平均值 H'
    float H_bar_prime;
    if (abs(h2_prime - h1_prime) <= 180.0)
        H_bar_prime = 0.5 * (h1_prime + h2_prime);
    else
    {
        if (h1_prime + h2_prime < 360.0)
            H_bar_prime = 0.5 * (h1_prime + h2_prime + 360.0);
        else
            H_bar_prime = 0.5 * (h1_prime + h2_prime - 360.0);
    }

    // 8. 计算权重函数 T, S_L, S_C, S_H
    float H_bar_rad = H_bar_prime * DEG2RAD;
    float T = 1.0
            - 0.17 * cos(H_bar_rad - 30.0 * DEG2RAD)
            + 0.24 * cos(2.0 * H_bar_rad)
            + 0.32 * cos(3.0 * H_bar_rad + 6.0 * DEG2RAD)
            - 0.20 * cos(4.0 * H_bar_rad - 63.0 * DEG2RAD);

    float dTheta = 30.0 * exp(-pow((H_bar_prime - 275.0) / 25.0, 2.0));
    float R_C = 2.0 * sqrt(pow(C_bar_prime, 7.0) / (pow(C_bar_prime, 7.0) + 25.0 * 25.0 * 25.0 * 25.0 * 25.0 * 25.0 * 25.0));
    float S_L = 1.0 + (0.015 * pow(L_bar - 50.0, 2.0)) / sqrt(20.0 + pow(L_bar - 50.0, 2.0));
    float S_C = 1.0 + 0.045 * C_bar_prime;
    float S_H = 1.0 + 0.015 * C_bar_prime * T;
    float R_T = -sin(2.0 * dTheta * DEG2RAD) * R_C;

    // 9. 最终计算 (假设 kL=kC=kH=1)
    float term1 = dL_prime / S_L;
    float term2 = dC_prime / S_C;
    float term3 = dH_prime / S_H;
    
    float deltaE = sqrt(term1 * term1 + term2 * term2 + term3 * term3 + R_T * term2 * term3);

    return deltaE;
}

float CalcPixWeight(float3 color1, float3 color2, float tolerance)
{
    float t = tolerance * 200 * 3 + 0.001;
    float d = DistanceLab(RGB2Lab(color1), RGB2Lab(color2));
    d = d * d;
    if (t > d)
    {
        return (t - d) / t;
    }
    else
    {
        return 0.f;
    }
    return 1.f;
}




float _LinearToGamma_(float c)
{
    if (c <= 0.0031308f)
    {
        return 12.92f * c;
    }
    else
    {
        return 1.055f * pow(c, 1.0f / 2.4f) - 0.055f;
    }
}
float3 LinearToGamma(float3 c)
{
    return float3(
    _LinearToGamma_(c.r),
    _LinearToGamma_(c.g),
    _LinearToGamma_(c.b));

}

float _GammaToLinear_(float c)
{
    if (c <= 0.04045f)
    {
        //return c / 12.92f;
        return c * 0.077399381;
    }
    else
    {
        return pow((c + 0.055f) / 1.055f, 2.4f);
    }
}
float3 GammaToLinear(float3 c)
{
    return float3(
    _GammaToLinear_(c.r),
    _GammaToLinear_(c.g),
    _GammaToLinear_(c.b));

}


float cbrtf(float x)
{
    return sign(x) * pow(abs(x), 1.0f / 3.0f);
}



Lab linear_srgb_to_oklab(RGB c)
{
    float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
    float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
    float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;

    float l_ = cbrtf(l);
    float m_ = cbrtf(m);
    float s_ = cbrtf(s);
    
    return float3(
        0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
			1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
			0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_
);
}

RGB oklab_to_linear_srgb(Lab c)
{
    float l_ = c.x + 0.3963377774f * c.y + 0.2158037573f * c.z;
    float m_ = c.x - 0.1055613458f * c.y - 0.0638541728f * c.z;
    float s_ = c.x - 0.0894841775f * c.y - 1.2914855480f * c.z;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    return float3
    (
        +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
			-1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
			-0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s
		);
}



float compute_max_saturation(float a, float b)
{
		// Max saturation will be when one of r, g or b goes below zero.

		// Select different coefficients depending on which component goes below zero first
    float k0, k1, k2, k3, k4, wl, wm, ws;

    if (-1.88170328f * a - 0.80936493f * b > 1)
    {
			// Red component
        k0 = +1.19086277f;
        k1 = +1.76576728f;
        k2 = +0.59662641f;
        k3 = +0.75515197f;
        k4 = +0.56771245f;
        wl = +4.0767416621f;
        wm = -3.3077115913f;
        ws = +0.2309699292f;
    }
    else if (1.81444104f * a - 1.19445276f * b > 1)
    {
			// Green component
        k0 = +0.73956515f;
        k1 = -0.45954404f;
        k2 = +0.08285427f;
        k3 = +0.12541070f;
        k4 = +0.14503204f;
        wl = -1.2684380046f;
        wm = +2.6097574011f;
        ws = -0.3413193965f;
    }
    else
    {
			// Blue component
        k0 = +1.35733652f;
        k1 = -0.00915799f;
        k2 = -1.15130210f;
        k3 = -0.50559606f;
        k4 = +0.00692167f;
        wl = -0.0041960863f;
        wm = -0.7034186147f;
        ws = +1.7076147010f;
    }

		// Approximate max saturation using a polynomial:
    float S = k0 + k1 * a + k2 * b + k3 * a * a + k4 * a * b;

		// Do one step Halley's method to get closer
		// this gives an error less than 10e6, except for some blue hues where the dS/dh is close to infinite
		// this should be sufficient for most applications, otherwise do two/three steps 

    float k_l = +0.3963377774f * a + 0.2158037573f * b;
    float k_m = -0.1055613458f * a - 0.0638541728f * b;
    float k_s = -0.0894841775f * a - 1.2914855480f * b;

		{
        float l_ = 1.f + S * k_l;
        float m_ = 1.f + S * k_m;
        float s_ = 1.f + S * k_s;

        float l = l_ * l_ * l_;
        float m = m_ * m_ * m_;
        float s = s_ * s_ * s_;

        float l_dS = 3.f * k_l * l_ * l_;
        float m_dS = 3.f * k_m * m_ * m_;
        float s_dS = 3.f * k_s * s_ * s_;

        float l_dS2 = 6.f * k_l * k_l * l_;
        float m_dS2 = 6.f * k_m * k_m * m_;
        float s_dS2 = 6.f * k_s * k_s * s_;

        float f = wl * l + wm * m + ws * s;
        float f1 = wl * l_dS + wm * m_dS + ws * s_dS;
        float f2 = wl * l_dS2 + wm * m_dS2 + ws * s_dS2;

        S = S - f * f1 / (f1 * f1 - 0.5f * f * f2);
    }

    return S;
}

LC find_cusp(float a, float b)
{
		// First, find the maximum saturation (saturation S = C/L)
    float S_cusp = compute_max_saturation(a, b);

		// Convert to linear sRGB to find the first point where at least one of r,g or b >= 1:
    RGB rgb_at_max = oklab_to_linear_srgb(float3(
        1, S_cusp * a, S_cusp * b
    ));
    
    float L_cusp = cbrtf(1.f / max(max(rgb_at_max.r, rgb_at_max.g), rgb_at_max.b));
    float C_cusp = L_cusp * S_cusp;

    return float2(
        L_cusp, C_cusp);

}


float find_gamut_intersection(float a, float b, float L1, float C1, float L0, LC cusp)
{
		// Find the intersection for upper and lower half seprately
    float t;
    if (((L1 - L0) * cusp.y - (cusp.x - L0) * C1) <= 0.f)
    {
			// Lower half

        t = cusp.y * L0 / (C1 * cusp.x + cusp.y * (L0 - L1));
    }
    else
    {
			// Upper half

			// First intersect with triangle
        t = cusp.y * (L0 - 1.f) / (C1 * (cusp.x - 1.f) + cusp.y * (L0 - L1));

			// Then one step Halley's method
			{
            float dL = L1 - L0;
            float dC = C1;

            float k_l = +0.3963377774f * a + 0.2158037573f * b;
            float k_m = -0.1055613458f * a - 0.0638541728f * b;
            float k_s = -0.0894841775f * a - 1.2914855480f * b;

            float l_dt = dL + dC * k_l;
            float m_dt = dL + dC * k_m;
            float s_dt = dL + dC * k_s;


				// If higher accuracy is required, 2 or 3 iterations of the following block can be used:
				{
                float L = L0 * (1.f - t) + t * L1;
                float C = t * C1;

                float l_ = L + C * k_l;
                float m_ = L + C * k_m;
                float s_ = L + C * k_s;

                float l = l_ * l_ * l_;
                float m = m_ * m_ * m_;
                float s = s_ * s_ * s_;

                float ldt = 3 * l_dt * l_ * l_;
                float mdt = 3 * m_dt * m_ * m_;
                float sdt = 3 * s_dt * s_ * s_;

                float ldt2 = 6 * l_dt * l_dt * l_;
                float mdt2 = 6 * m_dt * m_dt * m_;
                float sdt2 = 6 * s_dt * s_dt * s_;

                float r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s - 1;
                float r1 = 4.0767416621f * ldt - 3.3077115913f * mdt + 0.2309699292f * sdt;
                float r2 = 4.0767416621f * ldt2 - 3.3077115913f * mdt2 + 0.2309699292f * sdt2;

                float u_r = r1 / (r1 * r1 - 0.5f * r * r2);
                float t_r = -r * u_r;

                float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s - 1;
                float g1 = -1.2684380046f * ldt + 2.6097574011f * mdt - 0.3413193965f * sdt;
                float g2 = -1.2684380046f * ldt2 + 2.6097574011f * mdt2 - 0.3413193965f * sdt2;

                float u_g = g1 / (g1 * g1 - 0.5f * g * g2);
                float t_g = -g * u_g;

                float b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s - 1;
                float b1 = -0.0041960863f * ldt - 0.7034186147f * mdt + 1.7076147010f * sdt;
                float b2 = -0.0041960863f * ldt2 - 0.7034186147f * mdt2 + 1.7076147010f * sdt2;

                float u_b = b1 / (b1 * b1 - 0.5f * b * b2);
                float t_b = -b * u_b;

                t_r = u_r >= 0.f ? t_r : FLT_MAX;
                t_g = u_g >= 0.f ? t_g : FLT_MAX;
                t_b = u_b >= 0.f ? t_b : FLT_MAX;

                t += fmin(t_r, fmin(t_g, t_b));
            }
        }
    }

    return t;
}

float find_gamut_intersection(float a, float b, float L1, float C1, float L0)
{
		// Find the cusp of the gamut triangle
    LC cusp = find_cusp(a, b);

    return find_gamut_intersection(a, b, L1, C1, L0, cusp);
}


RGB gamut_clip_preserve_chroma(RGB rgb)
{
    if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
        return rgb;

    Lab lab = linear_srgb_to_oklab(rgb);

    float L = lab.x;
    float eps = 0.00001f;
    float C = fmax(eps, sqrtf(lab.y * lab.y + lab.z * lab.z));
    float a_ = lab.y / C;
    float b_ = lab.z / C;

    float L0 = clamp(L, 0, 1);

    float t = find_gamut_intersection(a_, b_, L, C, L0);
    float L_clipped = L0 * (1 - t) + t * L;
    float C_clipped = t * C;
    
    return oklab_to_linear_srgb(float3(
        L_clipped, C_clipped * a_, C_clipped * b_
    ));
}

RGB gamut_clip_project_to_0_5(RGB rgb)
{
    if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
        return rgb;

    Lab lab = linear_srgb_to_oklab(rgb);
    
    float L = lab.x;
    float eps = 0.00001f;
    float C = fmax(eps, sqrtf(lab.y * lab.y + lab.z * lab.z));
    float a_ = lab.y / C;
    float b_ = lab.z / C;

    float L0 = 0.5;

    float t = find_gamut_intersection(a_, b_, L, C, L0);
    float L_clipped = L0 * (1 - t) + t * L;
    float C_clipped = t * C;

    return oklab_to_linear_srgb(float3(
        L_clipped, C_clipped * a_, C_clipped * b_
    ));
}

RGB gamut_clip_project_to_L_cusp(RGB rgb)
{
    if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
        return rgb;

    Lab lab = linear_srgb_to_oklab(rgb);

    float L = lab.x;
    float eps = 0.00001f;
    float C = fmax(eps, sqrtf(lab.y * lab.y + lab.z * lab.z));
    float a_ = lab.y / C;
    float b_ = lab.z / C;

		// The cusp is computed here and in find_gamut_intersection, an optimized solution would only compute it once.
    LC cusp = find_cusp(a_, b_);

    float L0 = cusp.x;

    float t = find_gamut_intersection(a_, b_, L, C, L0);

    float L_clipped = L0 * (1 - t) + t * L;
    float C_clipped = t * C;

    return oklab_to_linear_srgb(float3(
        L_clipped, C_clipped * a_, C_clipped * b_
    ));
}

RGB gamut_clip_adaptive_L0_0_5(RGB rgb, float alpha = 0.05f)
{
    if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
        return rgb;

    Lab lab = linear_srgb_to_oklab(rgb);

    float L = lab.y;
    float eps = 0.00001f;
    float C = fmax(eps, sqrtf(lab.y * lab.y + lab.z * lab.z));
    float a_ = lab.y / C;
    float b_ = lab.z / C;

    float Ld = L - 0.5f;
    float e1 = 0.5f + fabs(Ld) + alpha * C;
    float L0 = 0.5f * (1.f + sgn(Ld) * (e1 - sqrtf(e1 * e1 - 2.f * fabs(Ld))));

    float t = find_gamut_intersection(a_, b_, L, C, L0);
    float L_clipped = L0 * (1.f - t) + t * L;
    float C_clipped = t * C;

    return oklab_to_linear_srgb(float3(
        L_clipped, C_clipped * a_, C_clipped * b_
    ));
}

RGB gamut_clip_adaptive_L0_L_cusp(RGB rgb, float alpha = 0.05f)
{
    if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
        return rgb;

    Lab lab = linear_srgb_to_oklab(rgb);

    float L = lab.x;
    float eps = 0.00001f;
    float C = fmax(eps, sqrtf(lab.y * lab.y + lab.z * lab.z));
    float a_ = lab.y / C;
    float b_ = lab.z / C;

		// The cusp is computed here and in find_gamut_intersection, an optimized solution would only compute it once.
    LC cusp = find_cusp(a_, b_);

    float Ld = L - cusp.x;
    float k = 2.f * (Ld > 0 ? 1.f - cusp.x : cusp.x);

    float e1 = 0.5f * k + fabs(Ld) + alpha * C / k;
    float L0 = cusp.x + 0.5f * (sgn(Ld) * (e1 - sqrtf(e1 * e1 - 2.f * k * fabs(Ld))));

    float t = find_gamut_intersection(a_, b_, L, C, L0);
    float L_clipped = L0 * (1.f - t) + t * L;
    float C_clipped = t * C;

    return oklab_to_linear_srgb(float3(
        L_clipped, C_clipped * a_, C_clipped * b_
    ));
}

float toe(float x)
{
    const float k_1 = 0.206f;
    const float k_2 = 0.03f;
    const float k_3 = (1.f + k_1) / (1.f + k_2);
    return 0.5f * (k_3 * x - k_1 + sqrtf((k_3 * x - k_1) * (k_3 * x - k_1) + 4 * k_2 * k_3 * x));
}

float toe_inv(float x)
{
    const float k_1 = 0.206f;
    const float k_2 = 0.03f;
    const float k_3 = (1.f + k_1) / (1.f + k_2);
    return (x * x + k_1 * x) / (k_3 * (x + k_2));
}

ST to_ST(LC cusp)
{
    float L = cusp.x;
    float C = cusp.y;
    return float2(
        C / L, C / (1 - L));
}

ST get_ST_mid(float a_, float b_)
{
    float S = 0.11516993f + 1.f / (
			+7.44778970f + 4.15901240f * b_
			+ a_ * (-2.19557347f + 1.75198401f * b_
				+ a_ * (-2.13704948f - 10.02301043f * b_
					+ a_ * (-4.24894561f + 5.38770819f * b_ + 4.69891013f * a_
						)))
			);

    float T = 0.11239642f + 1.f / (
			+1.61320320f - 0.68124379f * b_
			+ a_ * (+0.40370612f + 0.90148123f * b_
				+ a_ * (-0.27087943f + 0.61223990f * b_
					+ a_ * (+0.00299215f - 0.45399568f * b_ - 0.14661872f * a_
						)))
			);

    return float2(
        S, T);
}
    
    


Cs get_Cs(float L, float a_, float b_)
{
    LC cusp = find_cusp(a_, b_);

    float C_max = find_gamut_intersection(a_, b_, L, 1, L, cusp);
    ST ST_max = to_ST(cusp);

		// Scale factor to compensate for the curved part of gamut shape:
    float k = C_max / fmin((L * ST_max.x), (1 - L) * ST_max.y);

    float C_mid;
		{
        ST ST_mid = get_ST_mid(a_, b_);

			// Use a soft minimum function, instead of a sharp triangle shape to get a smooth value for chroma.
        float C_a = L * ST_mid.x;
        float C_b = (1.f - L) * ST_mid.y;
        C_mid = 0.9f * k * sqrtf(sqrtf(1.f / (1.f / (C_a * C_a * C_a * C_a) + 1.f / (C_b * C_b * C_b * C_b))));
    }

    float C_0;
		{
			// for C_0, the shape is independent of hue, so ST are constant. Values picked to roughly be the average values of ST.
        float C_a = L * 0.4f;
        float C_b = (1.f - L) * 0.8f;

			// Use a soft minimum function, instead of a sharp triangle shape to get a smooth value for chroma.
        C_0 = sqrtf(1.f / (1.f / (C_a * C_a) + 1.f / (C_b * C_b)));
    }

    return float3(C_0, C_mid, C_max);
}




RGB okhsl_to_srgb(HSL hsl)
{
    float h = hsl.x;
    float s = hsl.y;
    float l = hsl.z;

    if (l == 1.0f)
    {
        return float3(
            1.f, 1.f, 1.f);
    }

		else
        if (l == 0.f)
        {
            return float3(
                0.f, 0.f, 0.f);
        }

            float a_ = cos(2.f * pi * h);
            float b_ = sin(2.f * pi * h);
            float L = toe_inv(l);

            Cs cs = get_Cs(L, a_, b_);
            //float C_0 = cs.C_0;
            //float C_mid = cs.C_mid;
            //float C_max = cs.C_max;
            float C_0 = cs.x;
            float C_mid = cs.y;
            float C_max = cs.z;

            float mid = 0.8f;
            float mid_inv = 1.25f;

            float C, t, k_0, k_1, k_2;

            if (s < mid)
            {
                t = mid_inv * s;

                k_1 = mid * C_0;
                k_2 = (1.f - k_1 / C_mid);

                C = t * k_1 / (1.f - k_2 * t);
            }
            else
            {
                t = (s - mid) / (1 - mid);

                k_0 = C_mid;
                k_1 = (1.f - mid) * C_mid * C_mid * mid_inv * mid_inv / C_0;
                k_2 = (1.f - (k_1) / (C_max - C_mid));

                C = k_0 + t * k_1 / (1.f - k_2 * t);
            }

    RGB rgb = oklab_to_linear_srgb(float3(
                L, C * a_, C * b_
            ));
            return float3(
			srgb_transfer_function(rgb.r),
			srgb_transfer_function(rgb.g),
			srgb_transfer_function(rgb.b)
		);
        }



HSL srgb_to_okhsl(RGB rgb)
{
    Lab lab = linear_srgb_to_oklab(float3(
			srgb_transfer_function_inv(rgb.r),
			srgb_transfer_function_inv(rgb.g),
			srgb_transfer_function_inv(rgb.b)

   ));

    float C = sqrtf(lab.y * lab.y + lab.z * lab.z);
    float a_ = lab.y / C;
    float b_ = lab.z / C;

    float L = lab.x;
    float h = 0.5f + 0.5f * atan2(-lab.z, -lab.y) / pi;

    Cs cs = get_Cs(L, a_, b_);
    float C_0 = cs.x;
    float C_mid = cs.y;
    float C_max = cs.z;

		// Inverse of the interpolation in okhsl_to_srgb:

    float mid = 0.8f;
    float mid_inv = 1.25f;

    float s;
    if (C < C_mid)
    {
        float k_1 = mid * C_0;
        float k_2 = (1.f - k_1 / C_mid);

        float t = C / (k_1 + k_2 * C);
        s = t * mid;
    }
    else
    {
        float k_0 = C_mid;
        float k_1 = (1.f - mid) * C_mid * C_mid * mid_inv * mid_inv / C_0;
        float k_2 = (1.f - (k_1) / (C_max - C_mid));

        float t = (C - k_0) / (k_1 + k_2 * (C - k_0));
        s = mid + (1.f - mid) * t;
    }

    float l = toe(L);
    return float3(
        h, s, l);
}



RGB okhsv_to_srgb(HSV hsv)
{
    float h = hsv.x;
    float s = hsv.y;
    float v = hsv.z;

    float a_ = cos(2.f * pi * h);
    float b_ = sin(2.f * pi * h);

    LC cusp = find_cusp(a_, b_);
    ST ST_max = to_ST(cusp);
    float S_max = ST_max.x;
    float T_max = ST_max.y;
    float S_0 = 0.5f;
    float k = 1 - S_0 / S_max;

		// first we compute L and V as if the gamut is a perfect triangle:

		// L, C when v==1:
    float L_v = 1 - s * S_0 / (S_0 + T_max - T_max * k * s);
    float C_v = s * T_max * S_0 / (S_0 + T_max - T_max * k * s);

    float L = v * L_v;
    float C = v * C_v;

		// then we compensate for both toe and the curved top part of the triangle:
    float L_vt = toe_inv(L_v);
    float C_vt = C_v * L_vt / L_v;

    float L_new = toe_inv(L);
    C = C * L_new / L;
    L = L_new;

    RGB rgb_scale = oklab_to_linear_srgb(float3(
        L_vt, a_ * C_vt, b_ * C_vt
    ));
    float scale_L = cbrtf(1.f / fmax(fmax(rgb_scale.r, rgb_scale.g), fmax(rgb_scale.b, 0.f)));

    L = L * scale_L;
    C = C * scale_L;

    RGB rgb = oklab_to_linear_srgb(float3(
        L, C * a_, C * b_
    ));
    return 
    float3(
			srgb_transfer_function(rgb.r),
			srgb_transfer_function(rgb.g),
			srgb_transfer_function(rgb.b)
		);
}


HSV srgb_to_okhsv(RGB rgb)
{
    Lab lab = linear_srgb_to_oklab(float3(
			srgb_transfer_function_inv(rgb.r),
			srgb_transfer_function_inv(rgb.g),
			srgb_transfer_function_inv(rgb.b)

    ));

    float C = sqrtf(lab.y * lab.y + lab.z * lab.z);
    float a_ = lab.y / C;
    float b_ = lab.z / C;

    float L = lab.x;
    float h = 0.5f + 0.5f * atan2(-lab.z, -lab.y) / pi;

    LC cusp = find_cusp(a_, b_);
    ST ST_max = to_ST(cusp);
    float S_max = ST_max.x;
    float T_max = ST_max.y;
    float S_0 = 0.5f;
    float k = 1 - S_0 / S_max;

		// first we find L_v, C_v, L_vt and C_vt

    float t = T_max / (C + L * T_max);
    float L_v = t * L;
    float C_v = t * C;

    float L_vt = toe_inv(L_v);
    float C_vt = C_v * L_vt / L_v;

		// we can then use these to invert the step that compensates for the toe and the curved top part of the triangle:
    RGB rgb_scale = oklab_to_linear_srgb(float3(
        L_vt, a_ * C_vt, b_ * C_vt
    ));
    float scale_L = cbrtf(1.f / fmax(fmax(rgb_scale.r, rgb_scale.g), fmax(rgb_scale.b, 0.f)));

    L = L / scale_L;
    C = C / scale_L;

    C = C * toe(L) / L;
    L = toe(L);

		// we can now compute v and s:

    float v = L / L_v;
    float s = (S_0 + T_max) * C_v / ((T_max * S_0) + T_max * k * C_v);

    return 
    float3(
        h, s, v);
}


//0~100 0~100 0~360
float3 LCH_to_sRGB(float3 LCH)
{
    return okhsl_to_srgb(float3(LCH.z / (2*pi), LCH.y * 0.01f, LCH.x * 0.01f));

}



float3 sRGB_to_LCH(float3 srgb)
{
    float3 hsl = srgb_to_okhsl(srgb);
    return float3(hsl.z * 100.f, hsl.y * 100.f, hsl.x * 2 * pi);

}












float DistancePointToLine2D(float2 P, float2 A, float2 B)
{
    float2 AB = B - A;
    float2 AP = P - A;
    
    // 2D 叉乘公式：AB.x * AP.y - AB.y * AP.x
    // 这计算的是平行四边形的面积
    float cross = AB.x * AP.y - AB.y * AP.x;
    
    // 距离 = 面积 / 底边长度
    float lenAB = length(AB);
    
    // 防止除以 0 (如果 A 和 B 重合)
    if (lenAB < 1e-6)
        return length(AP);
    
    return abs(cross) / lenAB;
}



#endif