#include "LchColorControl_Primary.h"
#include "duilib/Control/ColorConvert.h"
#include "duilib/Core/Window.h"
#include "duilib/Core/GlobalManager.h"
#include"Graphic/CSPMOD_ColorConvert.h"
#include"SDL3/SDL.h"
#include"CspData.h"
#include"Graphic/Geometry.h"
#include"LchColorControlContext.h"
using namespace ui;


//色环内部矩形边长
const constexpr uint32_t baseSquareSideLen = 256;
//色环宽度
//const constexpr uint32_t hueCircleWide = 20;
const constexpr float _hueCircleWide = 0.2f;
//sqrt(2)
//4:色环两侧1像素冗余
const constexpr uint32_t imageWidth = uint32_t(baseSquareSideLen * 1.414213562373f) + 40 + 4+1;

//色环统一明度？
const constexpr float  hueCircleLight = 66.f;





    LchColorControl_Primary::LchColorControl_Primary(Window* pWindow) :
        Control(pWindow)
    {
        m_lastPt.x = -1;
        m_lastPt.y = -1;
        //m_bMouseDown = false;
        m_bMouseHueCircle = false;
        m_bMouseColorPan = false;
        AttachSizeChanged([this](const ui::EventArgs&) { 
            this->needResetPos=true; 
            this->needRepaint = true;
            return true; });
    }

    DString LchColorControl_Primary::GetType() const { return L"LchColorControl_Primary"; }

    void LchColorControl_Primary::GetCurrentColor96(Color96* pColor)
    {
        float r, g, b;
       //float degree = h_rgb+150.f;//与CSP色相环同步
       //  if (degree >= 360.f)degree -= 360.0f;
        CSPMOD_ColorConvert::Lch2RGB(L,c_percent, CSPMOD_ColorConvert::RGBH2LCHH(h_rgb),&r,&g,&b);
        double dR = r;
        double dG = g;
        double dB = b;
        pColor->R=static_cast<uint32_t>(dR* UINT32_MAX);
        pColor->G=static_cast<uint32_t>(dG* UINT32_MAX);
        pColor->B =static_cast<uint32_t>(dB* UINT32_MAX);

        //return Color96();
    }

    //void LchColorControl_Primary::SetL(float LCH_L)
    //{
    //    if (L != LCH_L)
    //    {
    //        L = LCH_L;
    //        needResetPos = true;
    //        Invalidate();
    //    }
    //}

    //void LchColorControl_Primary::SetC(float LCH_C)
    //{
    //    if (c_percent != LCH_C)
    //    {
    //        c_percent = LCH_C;
    //        
    //        needResetPos = true;
    //        Invalidate();
    //    }
    //}

    //void LchColorControl_Primary::SetH(float LCH_H)
    //{
    //    if (h_rgb != LCH_H)
    //    {
    //        h_rgb = LCH_H;
    //        if (h_rgb < 0)h_rgb += 360.f;
    //        else if (h_rgb >= 360.f)h_rgb -= 360.f;
    //        needRepaint = true;
    //        Invalidate();
    //    }
    //}

    void LchColorControl_Primary::SetLCHrgb(float LCH_L, float LCH_C, float LCH_H)
    {

        if (L != LCH_L || c_percent != LCH_C || h_rgb != LCH_H)
        {

            needRepaint = h_rgb != LCH_H;
            needResetPos = c_percent != LCH_C || L != LCH_L;

            L = LCH_L;
            c_percent = LCH_C;
            h_rgb = LCH_H;

            needResetPos = true;
            Invalidate();

        }
    }


    void LchColorControl_Primary::SetWheelType(WheelType _wheelType)
    {
        wheelType = _wheelType;
        needRepaint = true;
        needResetPos = true;
        this->Invalidate();
    }

    void LchColorControl_Primary::PaintBkImage(IRender* pRender)
    {
        BaseClass::PaintBkImage(pRender);
        if (pRender == nullptr) {
            return;
        }
        UiRect rc = GetRect();
        
        UiRect rcPaint = GetPaintRect();
        UiPadding padding = GetPadding();

        UiRect rcDest;
        rcDest.left =rc.left+ padding.left;
        rcDest.right = rc.right- padding.right;
        rcDest.top = rc.top+ padding.top;
        rcDest.bottom = rc.bottom- padding.bottom;

        rcHueCircle= rcDest;
        if (rcDest.Width() > rcDest.Height())
        {
            rcHueCircle.left = rcDest.left + (rcDest.Width() - rcDest.Height()) / 2;
            rcHueCircle.right = rcHueCircle.left + rcDest.Height();
        }
        else
        {
            rcHueCircle.top = rcDest.top + (rcDest.Height() - rcDest.Width()) / 2;
            rcHueCircle.bottom = rcHueCircle.top + rcDest.Width();
        }


        //IBitmap* pBitmap_HueCircle = GetHueCircleBitmap();
        //uint32_t hueCirclePadding=static_cast<uint32_t>(hueCircleWide+2);
        uint32_t hueCirclePadding=static_cast<uint32_t>(_hueCircleWide * rcHueCircle.Width()*0.5f);

        rcColorPan = rcHueCircle;
        rcColorPan.left += hueCirclePadding;
        rcColorPan.top += hueCirclePadding;
        rcColorPan.right -= hueCirclePadding;
        rcColorPan.bottom -= hueCirclePadding;

        IBitmap* pBitmap_ColorPan = GetColorBitmap(rcHueCircle);


        uint8_t uFade = 255;
        IMatrix* pMatrix = nullptr;

        //计算位置
        if (needResetPos)
        {
            OnSizeChanged({});
            needResetPos = false;
        }









        if (pBitmap_ColorPan != nullptr) {

            if (needRepaint)_RePaintWheel();
            //pRender->DrawImageRect(rcPaint, pBitmap, rcDest, rcSource, uFade, pMatrix);
            pRender->DrawImage(rcPaint, pBitmap_ColorPan, rcHueCircle, {0,0, (int)pBitmap_ColorPan->GetWidth(),(int)pBitmap_ColorPan->GetHeight() }, uFade);

        }

        //pRender->DrawImage(rcPaint, pBitmap_HueCircle, rcHueCircle, { 0,0, (int)pBitmap_HueCircle->GetWidth(),(int)pBitmap_HueCircle->GetHeight() }, uFade);





        //画选择位置的显示标志
        if ((m_lastPt.x >= 0) && m_lastPt.y >= 0) {
            UiPoint centerPt = m_lastPt;
            int32_t radius = Dpi().GetScaleInt(4);
            UiColor penColor = UiColor(UiColors::White);
            int32_t nWidth = Dpi().GetScaleInt(2);
            pRender->DrawCircle(centerPt, radius, penColor, nWidth);
            int32_t radius2 = Dpi().GetScaleInt(5);
            UiColor penColor2 = UiColor(UiColors::DarkSlateGray);
            int32_t nWidth2 = Dpi().GetScaleInt(1);
            pRender->DrawCircle(centerPt, radius2, penColor2, nWidth2);
        }
        //画色相框
        {
        
            IRenderFactory* pRenderFactory = GlobalManager::Instance().GetRenderFactory();
            if (pRenderFactory != nullptr) {
                std::unique_ptr<IPen> pen(pRenderFactory->CreatePen(ui::UiColor(0,0,0), 3.5F));
                std::unique_ptr<IPath> path(pRenderFactory->CreatePath());
                //std::unique_ptr<IMatrix> path(pRenderFactory->CreateMatrix());
                if (pen && path) {

                    //pointList
                    //h_argb为0的时候的path
                    float scaleFactor = 0.005f;
                    float baseWidth = 4.f;

                    float H= baseWidth + rcHueCircle.Width() * scaleFactor*0.5f;
                    float radOff = asinf(H/ (0.5f*rcHueCircle.Width()));

                    //float currentRad = h_rgb+150.f;为什么变成210了。计算的时候 +150=-210吗
                    float currentRad = h_rgb+210.f;
                    if (currentRad >= 360.f)currentRad -= 360.f;
                    currentRad =currentRad*3.1415927535f / 180.f;

                    ui::UiPointF pointList[4];
                    float _R = rcHueCircle.Width() * 0.5f;
                    float _r = rcHueCircle.Width() * 0.5f*(1-_hueCircleWide);
                    float R = _R - (_R - _r) * 0.05f;
                    float r = _r + (_R - _r) * 0.05f;
                    //R *= 0.9f; r *= 1.1f;
                    pointList[0].x = R * cosf(currentRad- radOff);
                    pointList[0].y = R* sinf(currentRad- radOff);

                    pointList[1].x = R * cosf(currentRad+ radOff);
                    pointList[1].y = R * sinf(currentRad+ radOff);

                    pointList[2].x =r * cosf(currentRad+ radOff);
                    pointList[2].y = r * sinf(currentRad+ radOff);

                    pointList[3].x =r * cosf(currentRad- radOff);
                    pointList[3].y =r * sinf(currentRad- radOff);

                    for (int i = 0; i < 4; i++)
                    {
                        pointList[i].x += rcHueCircle.left+ rcHueCircle.Width() * 0.5f;
                        pointList[i].y += rcHueCircle.top+ rcHueCircle.Width() * 0.5f;
                    }


                    path->AddPolygon(pointList,4);
                    path->Close();
                    pRender->DrawPath(path.get(), pen.get());


                    pen->SetColor(ui::UiColor(255, 255, 255));
                    pen->SetWidth(2.5f);
                    pRender->DrawPath(path.get(), pen.get());
                }
            }



        
        }
    }

    bool LchColorControl_Primary::ButtonDown(const EventArgs& msg)
    {
        bool bRet = BaseClass::ButtonDown(msg);
        if (msg.IsSenderExpired()) {
            return false;
        }
        bool isInHueCircle = false;
        bool isInWheel = false;
        {
            float mPosInRectX = msg.ptMouse.x - rcHueCircle.left + 0.5f;
            float mPosInRectY = msg.ptMouse.y - rcHueCircle.top + 0.5f;
            float invertDistance =sqrtf( (0.5f * rcHueCircle.Width() - mPosInRectX) * (0.5f * rcHueCircle.Width() - mPosInRectX)
                + (0.5f * rcHueCircle.Width() - mPosInRectY) * (0.5f * rcHueCircle.Width() - mPosInRectY));
        
            invertDistance = (0.5f*rcHueCircle.Width()-invertDistance)  /( rcHueCircle.Width()*0.5f);
            isInHueCircle = (invertDistance< _hueCircleWide)&& invertDistance>0.f;
            //SDL_Log("(%f,%f)%f,%d", mPosInRectX,invertDistance, isInHueCircle);
        }
        if (!isInHueCircle)
        {
            if (wheelType == WheelType_Rectangle)
            {
                UiRect rcHitTest;
                int squarPadding =static_cast<int> (rcColorPan.Width() *0.5f * 0.2928932188f);//1-根号2分之1
                rcHitTest.left=rcColorPan.left+ squarPadding-8;
                rcHitTest.right=rcColorPan.right- squarPadding+8;
                rcHitTest.top = rcColorPan.top + squarPadding - 8;
                rcHitTest.bottom= rcColorPan.bottom - squarPadding + 8;

                isInWheel = rcHitTest.ContainsPt(msg.ptMouse);
            }
            else if (wheelType == WheelType_Triangle)
            {
                float mPosInRectX = msg.ptMouse.x - rcColorPan.left + 0.5f;
                float mPosInRectY = msg.ptMouse.y - rcColorPan.top + 0.5f;
                float pt1x = rcColorPan.Width() * 0.25f;
                float pt1y = rcColorPan.Width() * 0.5f- rcColorPan.Width() * 0.25f* 1.7320508f;//根号3
                float pt2x = pt1x;
                float pt2y = rcColorPan.Width() * 0.5f + rcColorPan.Width() * 0.25f * 1.7320508f;//根号3;
                float pt3x = (float)rcColorPan.Width();
                float pt3y=0.5f* rcColorPan.Width();
                
                auto cross = [] (float px,float py,float x1,float y1,float x2,float y2)->float{
                    return (px - x1) * (y2 - y1) - (py - y1) * (x2 - x1);
                    };
                float d1 = cross(mPosInRectX, mPosInRectY,pt1x,pt1y,pt2x,pt2y);
                float d2 = cross(mPosInRectX, mPosInRectY,pt2x,pt2y,pt3x,pt3y);
                float d3 = cross(mPosInRectX, mPosInRectY,pt3x,pt3y,pt1x,pt1y);

                isInWheel = (d1 >= 0 && d2 >= 0 && d3 >= 0) || (d1 <= 0 && d2 <= 0 && d3 <= 0);

            }
            else
            {
                ASSERT(false);
            }

        }


        if (isInHueCircle)
        {
            //m_bMouseDown = true;
            m_bMouseHueCircle = true;
            SetMouseCapture(true);
            OnHuePosChanged(msg.ptMouse);
        }
        else if (isInWheel) {
            //m_bMouseDown = true;
            m_bMouseColorPan = true;
            SetMouseCapture(true);
            OnSelectPosChanged( msg.ptMouse);
        }
        return bRet;
    }

    bool LchColorControl_Primary::MouseMove(const EventArgs& msg)
    {
        bool bRet = BaseClass::MouseMove(msg);
        if (msg.IsSenderExpired()) {
            return false;
        }
        if (m_bMouseHueCircle) {

            OnHuePosChanged(msg.ptMouse);
        }
        else if (m_bMouseColorPan)
        {

            OnSelectPosChanged(msg.ptMouse);
        }

        if (!m_bMouseHueCircle)
        {
            if (m_bMouseColorPan)
            {
                ui::GlobalManager::Instance().Cursor().SetImageCursor(GetWindow(), ui::FilePath(L"Eyedropper.cur"));
            }
            else
            {
                bool isInHueCircle = false;
                bool isInWheel = false;
                bool isInCurWindow = false;
                {
                    float mPosInRectX = msg.ptMouse.x - rcHueCircle.left + 0.5f;
                    float mPosInRectY = msg.ptMouse.y - rcHueCircle.top + 0.5f;
                    float invertDistance = sqrtf((0.5f * rcHueCircle.Width() - mPosInRectX) * (0.5f * rcHueCircle.Width() - mPosInRectX)
                        + (0.5f * rcHueCircle.Width() - mPosInRectY) * (0.5f * rcHueCircle.Width() - mPosInRectY));

                    invertDistance = (0.5f * rcHueCircle.Width() - invertDistance) / (rcHueCircle.Width() * 0.5f);
                    isInHueCircle = (invertDistance < _hueCircleWide) && invertDistance > 0.f;
                    //SDL_Log("(%f,%f)%f,%d", mPosInRectX,invertDistance, isInHueCircle);
                }
                if (!isInHueCircle)
                {
                    if (wheelType == WheelType_Rectangle)
                    {
                        UiRect rcHitTest;
                        int squarPadding = static_cast<int> (rcColorPan.Width() * 0.5f * 0.2928932188f);//1-根号2分之1
                        rcHitTest.left = rcColorPan.left + squarPadding - 4;
                        rcHitTest.right = rcColorPan.right - squarPadding + 4;
                        rcHitTest.top = rcColorPan.top + squarPadding - 4;
                        rcHitTest.bottom = rcColorPan.bottom - squarPadding + 4;

                        isInWheel = rcHitTest.ContainsPt(msg.ptMouse);
                    }
                    else if (wheelType == WheelType_Triangle)
                    {
                        float mPosInRectX = msg.ptMouse.x - rcColorPan.left + 0.5f;
                        float mPosInRectY = msg.ptMouse.y - rcColorPan.top + 0.5f;
                        float pt1x = rcColorPan.Width() * 0.25f;
                        float pt1y = rcColorPan.Width() * 0.5f - rcColorPan.Width() * 0.25f * 1.7320508f;//根号3
                        float pt2x = pt1x;
                        float pt2y = rcColorPan.Width() * 0.5f + rcColorPan.Width() * 0.25f * 1.7320508f;//根号3;
                        float pt3x = (float)rcColorPan.Width();
                        float pt3y = 0.5f * rcColorPan.Width();

                        auto cross = [](float px, float py, float x1, float y1, float x2, float y2)->float {
                            return (px - x1) * (y2 - y1) - (py - y1) * (x2 - x1);
                            };
                        float d1 = cross(mPosInRectX, mPosInRectY, pt1x, pt1y, pt2x, pt2y);
                        float d2 = cross(mPosInRectX, mPosInRectY, pt2x, pt2y, pt3x, pt3y);
                        float d3 = cross(mPosInRectX, mPosInRectY, pt3x, pt3y, pt1x, pt1y);

                        isInWheel = (d1 >= 0 && d2 >= 0 && d3 >= 0) || (d1 <= 0 && d2 <= 0 && d3 <= 0);

                    }
                    else
                    {
                        ASSERT(false);
                    }
                }
                isInCurWindow;
                {
                    POINT currenCursorPos = {};
                    ::GetCursorPos(&currenCursorPos);
                    isInCurWindow= GetWindow()->GetWindowHandle() == ::WindowFromPoint(currenCursorPos);
                
                }
                if (isInWheel&& isInCurWindow) {
                    ui::GlobalManager::Instance().Cursor().SetImageCursor(GetWindow(), ui::FilePath(L"Eyedropper.cur"));
                    SetMouseCapture(true);
                }
                else
                    SetMouseCapture(false);

            }
        }

        return bRet;
    }

    bool LchColorControl_Primary::MouseLeave(const ui::EventArgs& msg)
    {
        bool bRet = BaseClass::MouseLeave(msg);
        //SDL_Log("MouseLeave");


        return bRet;
    }

    bool LchColorControl_Primary::ButtonUp(const EventArgs& msg)
    {
        m_bMouseHueCircle = false;
        m_bMouseColorPan = false;
        SetMouseCapture(false);
        return BaseClass::ButtonUp(msg);
    }

    bool LchColorControl_Primary::OnCaptureChanged(const ui::EventArgs& msg)
    {
        m_bMouseHueCircle = false;
        m_bMouseColorPan = false;
        return true;
    }

    void LchColorControl_Primary::SelectColor(const UiColor& selColor)
    {
        m_lastPt.x = -1;
        m_lastPt.y = -1;
        //UiRect rect = GetRect();

        UiRect rect = GetRect();
        UiPadding padding = GetPadding();
        rect.left += padding.left;
        rect.right -= padding.right;
        rect.top += padding.top;
        rect.bottom -= padding.bottom;

        const int32_t nHeight = rect.Height();
        const int32_t nWidth = rect.Width();
        if ((nHeight <= 0) || (nWidth <= 0)) {
            return;
        }
        if ((m_spBitmap != nullptr) && (selColor.GetARGB() != UiColors::Black)) {
            double hue = 0;
            double sat = 0;
            double value = 0;
            ColorConvert::RGB2HSV(selColor.GetR() / 255.0, selColor.GetG() / 255.0,
                selColor.GetB() / 255.0, &hue, &sat, &value);
            m_lastPt.x = static_cast<int32_t>((hue / 360) * nWidth);
            m_lastPt.y = static_cast<int32_t>((1.0 - sat) * nHeight);
            if (m_lastPt.x >= nWidth) {
                m_lastPt.x = nWidth - 1;
            }
            if (m_lastPt.y >= nHeight) {
                m_lastPt.y = nHeight - 1;
            }
            m_lastPt.x = rect.left + m_lastPt.x;
            m_lastPt.y = rect.top + m_lastPt.y;
        }
        Invalidate();
    }

    IBitmap* LchColorControl_Primary::GetColorBitmap(const UiRect& rect)
    {
        int32_t newWidth = std::min(rect.Height(), rect.Width());
        if ((newWidth <= 0) ) {
            return nullptr;
        }
        if (m_spBitmap != nullptr) {
            //m_spBitmap总是正方形（绘制的lch色轮是个圆盘）

            if (((int32_t)m_spBitmap->GetWidth() == newWidth) &&
                ((int32_t)m_spBitmap->GetHeight() == newWidth)) {
                //宽度和高度没有变化，不需要重新生成
                return m_spBitmap.get();
            }
            else {
                m_spBitmap.reset();
            }
        }

        IRenderFactory* pRenderFactory = GlobalManager::Instance().GetRenderFactory();
        ASSERT(pRenderFactory != nullptr);
        if (pRenderFactory != nullptr) {
            m_spBitmap.reset(pRenderFactory->CreateBitmap());
        }

        if (m_spBitmap != nullptr) {
            m_spBitmap->Init(newWidth, newWidth, nullptr, 1.0f, BitmapAlphaType::kUnpremul_SkAlphaType);
         
            _RePaintWheel();
        }
        return m_spBitmap.get();
    }
#if 0
    ui::IBitmap* LchColorControl_Primary::GetHueCircleBitmap()
    {
        if (m_spBitmap_HueCircle)
        {
            return m_spBitmap_HueCircle.get();
        }

        IRenderFactory* pRenderFactory = GlobalManager::Instance().GetRenderFactory();
        ASSERT(pRenderFactory != nullptr);
        if (pRenderFactory != nullptr) {
            m_spBitmap_HueCircle.reset(pRenderFactory->CreateBitmap());
        }





        if (m_spBitmap_HueCircle != nullptr) {
            m_spBitmap_HueCircle->Init(imageWidth, imageWidth, nullptr, 1.0f, BitmapAlphaType::kUnpremul_SkAlphaType);


            {
                void* pPixelBits = m_spBitmap_HueCircle->LockPixelBits();

                if (pPixelBits != nullptr) {



                    uint8_t* pData = (uint8_t*)pPixelBits;
                    for (int y = 0; y < (int)imageWidth; y++)
                        for (int x = 0; x < (int)imageWidth; x++)
                        {
                            //坐标转极坐标
                            float distance = ((imageWidth * 0.5f - y-0.5f) * (imageWidth * 0.5f - y - 0.5f) + (imageWidth * 0.5f - x - 0.5f) * (imageWidth * 0.5f - x - 0.5f));
                            distance = sqrtf(distance);
                            //distance = distance / (imageWidth * 0.5f);s

                            float invertDistance=imageWidth * 0.5f - distance;

                            if (invertDistance > hueCircleWide+2.f)
                            {
                                //BGRA
                                pData[0] = 0;
                                pData[1] = 0;
                                pData[2] = 0;
                                pData[3] = 0;
                            }
                            else
                            {

                                float degree = atan2f(y+0.5f - imageWidth * 0.5f, x+0.5f - imageWidth * 0.5f) * 180.f / 3.1415927f;
                                degree += 150.f;//与CSP色相环同步
                                if (degree < 0) {
                                    degree += 360.0f;
                                }
                                else if (degree >= 360.f)degree -= 360.0f;
                                float r;
                                float g;
                                float b;

                                CSPMOD_ColorConvert::Lch2RGB(hueCircleLight, 100.f, CSPMOD_ColorConvert::RGBH2LCHH(degree), &r, &g, &b);

                                pData[2] = static_cast<uint8_t>(SDL_clamp(r * 256.f, 0.f, 255.f));
                                pData[1] = static_cast<uint8_t>(SDL_clamp(g * 256.f, 0.f, 255.f));
                                pData[0] = static_cast<uint8_t>(SDL_clamp(b * 256.f, 0.f, 255.f));

                                float alphaFactor = 1.f;
                                if (invertDistance < 1.f)
                                {
                                    alphaFactor = invertDistance;
                                }
                                else if (invertDistance < hueCircleWide+1.f)
                                {
                                    alphaFactor = 1.f;
                                }
                                else
                                {
                                    alphaFactor =hueCircleWide + 2.f- invertDistance;
                                }
                                pData[3] = static_cast<uint8_t>(SDL_clamp(alphaFactor * 256.f, 0.f, 255.f));

                            }




                            pData += 4;
                        }

                    //for (int32_t nRow = 0; nRow < newWidth; ++nRow) {
                    //    ColorConvert::HSV_HUE(pData, newWidth, sat, 1.0);
                    //    pData += newWidth;
                    //    sat -= satStep;
                    //    if ((sat < 0.0) || (nRow == (newWidth - 2))) {
                    //        //最后一个数值是0.0
                    //        sat = 0.0;
                    //    }
                    //}



                    m_spBitmap_HueCircle->UnLockPixelBits();
                    needRepaint = false;

                }
            }
        }


        return m_spBitmap_HueCircle.get();
    }
#endif
    void LchColorControl_Primary::OnSelectPosChanged( const UiPoint& pt)
    {
        m_lastPt = pt;

        if (wheelType == WheelType_Rectangle)
        {
            //内环内接矩形
            rcColorPan;
            float mPosInRectX = m_lastPt.x - rcColorPan.left + 0.5f-0.5f* rcColorPan.Width();
            float mPosInRectY = m_lastPt.y - rcColorPan.top + 0.5f - 0.5f * rcColorPan.Width();
            float limit = 0.5f * rcColorPan.Width() / 1.4142135623731f;
            mPosInRectX=SDL_clamp(mPosInRectX, -limit, limit);
            mPosInRectY=SDL_clamp(mPosInRectY, -limit, limit);

            m_lastPt.x =static_cast<int> (rcColorPan.left+ 0.5f * rcColorPan.Width() + mPosInRectX);
            m_lastPt.y =static_cast<int> (rcColorPan.top+ 0.5f * rcColorPan.Width() + mPosInRectY);

            mPosInRectX += limit;
            mPosInRectY += limit;
            mPosInRectX /= 2 * limit;
            mPosInRectY /= 2 * limit;
            mPosInRectY = 1 - mPosInRectY;
            //此时mPosInRectXY为归一化的 原点在左下的 色盘坐标

            c_percent = mPosInRectX * 100.f;
            L = mPosInRectY * 100.f;

        }
        else if(wheelType==WheelType_Triangle)
        {

            float mPosInRectX = m_lastPt.x - rcColorPan.left + 0.5f;
            float mPosInRectY = m_lastPt.y - rcColorPan.top + 0.5f;
            float pt1x = rcColorPan.Width() * 0.25f;
            float pt1y = rcColorPan.Width() * 0.5f - rcColorPan.Width() * 0.25f * 1.7320508f;//根号3
            float pt2x = pt1x;
            float pt2y = rcColorPan.Width() * 0.5f + rcColorPan.Width() * 0.25f * 1.7320508f;//根号3;
            float pt3x = (float)rcColorPan.Width();
            float pt3y = 0.5f * rcColorPan.Width();

            auto cross = [](float px, float py, float x1, float y1, float x2, float y2)->float {
                return (px - x1) * (y2 - y1) - (py - y1) * (x2 - x1);
                };
            float d1 = cross(mPosInRectX, mPosInRectY, pt1x, pt1y, pt2x, pt2y);
            float d2 = cross(mPosInRectX, mPosInRectY, pt2x, pt2y, pt3x, pt3y);
            float d3 = cross(mPosInRectX, mPosInRectY, pt3x, pt3y, pt1x, pt1y);

            float resultPX;
            float resultPY;
            //if (1)
            if (d1>=0&&d2>=0&&d3>=0)
            {
                resultPX = mPosInRectX;
                resultPY = mPosInRectY;
            }
            else if (mPosInRectX < pt1x)
            {
                resultPX = pt1x;
                resultPY = mPosInRectY -(pt1x - mPosInRectX) * (1.f / 1.7320508f);
                resultPY=SDL_clamp(resultPY, pt1y, pt2y);
            }
            else if (d3<=0)
            {
                resultPX = pt1x;
                resultPY = mPosInRectY - (pt1x - mPosInRectX) * (1.f / 1.7320508f);
                resultPX= resultPX+(resultPY - pt1y) * 0.5f* 1.7320508f;
                resultPX=SDL_clamp(resultPX, pt1x, pt3x);
                resultPY = pt1y + (resultPX - pt1x) / 1.7320508f;
                //resultPY = SDL_clamp(resultPY, pt1y, pt2y);
            }
            else
            {
                resultPX = pt1x;
                resultPY = mPosInRectY - (mPosInRectX-pt1x ) * (1.f / 1.7320508f);

                resultPX = resultPX + (pt2y-resultPY) * 0.5f * 1.7320508f;
                resultPX = SDL_clamp(resultPX, pt1x, pt3x);
                resultPY = pt2y - (resultPX - pt1x) / 1.7320508f;
            }

            m_lastPt.x = static_cast<int> (resultPX+ rcColorPan.left);
            m_lastPt.y = static_cast<int> (resultPY+ rcColorPan.top);


            //计算三角内部坐标
            float Xstrength = resultPX - pt1x;
            if (resultPY > pt3y)
            {
                if (pt2y - resultPY < 1e-6f)
                {
                    Xstrength = 0;
                }
                else
                {

                    Xstrength = Xstrength / ((pt2y - resultPY) * 1.7320508f);
                }
            }
            else
            {
                if (resultPY - pt1y < 1e-6f)
                {
                    Xstrength = 0;
                }
                else
                {

                    Xstrength = Xstrength / ((resultPY - pt1y) * 1.7320508f);
                }

            }
            float Y = (pt2y - resultPY)/(pt2y- pt1y);

            c_percent = Xstrength * 100.f;
            L = Y * 100.f;
        }








        SendEvent(kEventSelectColor, {});
        Invalidate();
    }

    void LchColorControl_Primary::OnHuePosChanged(const ui::UiPoint& pt)
    {
        //当鼠标操作色相环的时候
        float mPosInRectX = pt.x - rcColorPan.left + 0.5f;
        float mPosInRectY = pt.y - rcColorPan.top + 0.5f;
        mPosInRectX -= rcColorPan.Width()*0.5f;
        mPosInRectY -= rcColorPan.Width() * 0.5f;

        float rad = atan2f(mPosInRectY,mPosInRectX );
        float deg = rad * 180.f / 3.1415927535f;
        deg += 150.f;
        if (deg < 0)deg += 360.f;
        else if (deg >= 360.f)deg -= 360.f;
        h_rgb = deg;

        SendEvent(kEventSelectColor, {});
        needRepaint=true;
        Invalidate();
    }

    void LchColorControl_Primary::SetMouseCapture(bool bCapture)
    {
        Window* pWindow = GetWindow();
        if (pWindow == nullptr) {
            return;
        }
        if (bCapture) {
            pWindow->SetCapture();
        }
        else {
            pWindow->ReleaseCapture();
        }
    }



    bool LchColorControl_Primary::OnSizeChanged(const ui::EventArgs& msg)
    {
        //根据Lch参数计算取色光标位置

        if (wheelType == WheelType_Rectangle)
        {
            float xstart = rcColorPan.Width() * 0.5F- rcColorPan.Width() * 0.5F / 1.414213562373f;//根号3
            float xend = rcColorPan.Width() * 0.5F + rcColorPan.Width() * 0.5F / 1.414213562373f;
            float ystart = xend;
            float yend = xstart;
            float xPosInRect = (xend - xstart)*c_percent/100.f+ xstart;
            float yPosInRect = (yend - ystart)*L/100.f+ ystart;
            m_lastPt.x = static_cast<int>(rcColorPan.left +xPosInRect);
            m_lastPt.y = static_cast<int>(rcColorPan.top + yPosInRect);

            needResetPos = false;
        }
        else if (wheelType == WheelType_Triangle)
        {
            float pt1x = rcColorPan.Width() * 0.25f;
            float pt1y = rcColorPan.Width() * 0.5f - rcColorPan.Width() * 0.25f * 1.7320508f;//根号3
            float pt2x = pt1x;
            float pt2y = rcColorPan.Width() * 0.5f + rcColorPan.Width() * 0.25f * 1.7320508f;//根号3;
            float pt3x = (float)rcColorPan.Width();
            float pt3y = 0.5f * rcColorPan.Width();

            float ystart = pt2y;
            float yend = pt1y;
            float yPosInRect = (yend - ystart) * L / 100.f + ystart;
            float xstart = pt1x;
            float xend = (L > 50.f) ? ((yPosInRect- pt1y)* 1.7320508f) : ((pt2y-yPosInRect ) * 1.7320508f);
            xend = xstart + xend;
            float xPosInRect = (xend - xstart) * c_percent / 100.f + xstart;
            
            m_lastPt.x = static_cast<int>(rcColorPan.left + xPosInRect);
            m_lastPt.y = static_cast<int>(rcColorPan.top + yPosInRect);

            needResetPos = false;
        }
        else
        {
            ASSERT(false);
        }


        return true;
    }

    void LchColorControl_Primary::_RePaintWheel()
    {

        if (m_spBitmap)
        {


            colWheelTex.Resize(m_spBitmap->GetWidth(), m_spBitmap->GetWidth());
            colWheelTex.Clear();

            if (wheelType == WheelType_Rectangle)
            {
                LchColorControlContext::GetIns().DrawLchColorControlPrimaryRectangle(&colWheelTex, CSPMOD_ColorConvert::RGBH2LCHH(h_rgb));
            }
            else
            {
                LchColorControlContext::GetIns().DrawLchColorControlPrimaryTriangle(&colWheelTex, CSPMOD_ColorConvert::RGBH2LCHH(h_rgb));
            }
            LchColorControlContext::GetIns().DrawLchColorControlPrimaryHueCircle(&colWheelTex);


            void* pPixelBits = m_spBitmap->LockPixelBits();

            if (pPixelBits != nullptr) {
            
                colWheelTex.CopyToMem((uint8_t*)pPixelBits,true, m_spBitmap->GetWidth()*4);
                m_spBitmap->UnLockPixelBits();
            }


            needRepaint = false;
        }
#if 0
        if (m_spBitmap)
        {



            colWheelTex.Resize(m_spBitmap->GetWidth(), m_spBitmap->GetWidth());


            void* pPixelBits = m_spBitmap->LockPixelBits();
            auto newWidth = m_spBitmap->GetWidth();

            if (pPixelBits != nullptr) {



                uint8_t* pData = (uint8_t*)pPixelBits;



                if (wheelType == WheelType_Rectangle)
                {
                    float left = newWidth - newWidth * 1.7320508f;
                    float right = newWidth - left;
                    float top = left;
                    float bottom = right;


                    for (int y = 0; y < (int)newWidth; y++)
                        for (int x = 0; x < (int)newWidth; x++)
                        {
                            float u = x + 0.5f;
                            float v = y + 0.5f;

                            float alphaFactorX = 0.f;
                            float alphaFactorY = 0.f;
                            {
                                if (u < left - 0.5f)
                                {
                                    alphaFactorX = 0.f;
                                }
                                else if (u < left + 0.5f)
                                {
                                    alphaFactorX = 1.f - (left + 0.5f - u);
                                }
                                else if (u < right - 0.5f)
                                {
                                    alphaFactorX = 1.f;
                                }
                                else if (u < right + 0.5f)
                                {
                                    alphaFactorX = right + 0.5f - u;
                                }
                                else
                                {
                                    alphaFactorX = 0.f;
                                }

                                if (v < left - 0.5f)
                                {
                                    alphaFactorY = 0.f;
                                }
                                else if (v < left + 0.5f)
                                {
                                    alphaFactorY = 1.f - (left + 0.5f - u);
                                }
                                else if (v < right - 0.5f)
                                {
                                    alphaFactorY = 1.f;
                                }
                                else if (v < right + 0.5f)
                                {
                                    alphaFactorY = right + 0.5f - u;
                                }
                                else
                                {
                                    alphaFactorY = 0.f;
                                }
                            }
                            float alphaFactor = alphaFactorX * alphaFactorY;
                            if (0.f == alphaFactor)
                            {
                                //BGRA
                                pData[0] = 0;
                                pData[1] = 0;
                                pData[2] = 0;
                                pData[3] = 0;
                            }
                            else
                            {
                                float curL = SDL_clamp((bottom - v) / (bottom - top), 0.f, 1.f) * 100.f;
                                float curC = SDL_clamp((u - left) / (right - left), 0.f, 1.f) * 100.f;

                                float r;
                                float g;
                                float b;
                                CSPMOD_ColorConvert::Lch2RGB(curL, curC, CSPMOD_ColorConvert::RGBH2LCHH(h_rgb), &r, &g, &b);

                                pData[2] = static_cast<uint8_t>(SDL_clamp(r * 256.f, 0.f, 255.f));
                                pData[1] = static_cast<uint8_t>(SDL_clamp(g * 256.f, 0.f, 255.f));
                                pData[0] = static_cast<uint8_t>(SDL_clamp(b * 256.f, 0.f, 255.f));
                                pData[3] = static_cast<uint8_t>(SDL_clamp(alphaFactor * 256.f, 0.f, 255.f));
                            }




                            pData += 4;
                        }
                }
                else if (wheelType == WheelType_Triangle)
                {
                    float pt1x = rcColorPan.Width() * 0.25f;
                    float pt1y = rcColorPan.Width() * 0.5f - rcColorPan.Width() * 0.25f * 1.7320508f;//根号3
                    float pt2x = pt1x;
                    float pt2y = rcColorPan.Width() * 0.5f + rcColorPan.Width() * 0.25f * 1.7320508f;//根号3;
                    float pt3x = (float)rcColorPan.Width();
                    float pt3y = 0.5f * rcColorPan.Width();

                    float ystart = pt2y;
                    float yend = pt1y;
                    float yPosInRect = (yend - ystart) * L / 100.f + ystart;
                    float xstart = pt1x;
                    float xend = (L > 50.f) ? ((yPosInRect - pt1y) * 1.7320508f) : ((pt2y - yPosInRect) * 1.7320508f);
                    float xPosInRect = (xend - xstart) * c_percent / 100.f + xstart;



                    geo::PointF T1{ pt1x,pt1y };//(0.5*1+sqrt2)*sqrt3
                    geo::PointF T2{ pt1x,pt2y };//(0.5*1+sqrt2)*sqrt3
                    geo::PointF T3{ pt3x ,pt3y };//(0.5*1+sqrt2)*sqrt3

                    geo::PointF outT1{ pt1x - 0.5f,pt1y - (2.09077f) };//(0.5*1+sqrt2)*sqrt3
                    geo::PointF outT2{ pt1x - 0.5f,pt2y + (2.09077f) };//(0.5*1+sqrt2)*sqrt3
                    geo::PointF outT3{ pt3x + 1.414213562373f,pt3y };//(0.5*1+sqrt2)*sqrt3

                    geo::PointF inT1{ pt1x + 0.5f,pt1y + (2.09077f) };//(0.5*1+sqrt2)*sqrt3
                    geo::PointF inT2{ pt1x + 0.5f,pt2y - (2.09077f) };//(0.5*1+sqrt2)*sqrt3
                    geo::PointF inT3{ pt3x - 1.414213562373f,pt3y };//(0.5*1+sqrt2)*sqrt3

                    //pData -= 4;
                    for (int y = 0; y < (int)newWidth; y++)
                    {
                        for (int x = 0; x < (int)newWidth; x++)
                        {
                            float u = x + 0.5f;
                            float v = y + 0.5f;

                            geo::PointF curPos{ u,v };
                            if (!geo::isPointInTriangle(curPos, outT1, outT2, outT3))
                            {
                                pData[0] = 0;
                                pData[1] = 0;
                                pData[2] = 0;
                                pData[3] = 0;
                            }
                            else
                            {
                                //pData[0] = 255;
                                //pData[1] = 0;
                                //pData[2] = 0;
                                //pData[3] = 255;
                                
                                //在两个三角形夹包的中间，进行抗锯齿计算
                                float alphaFactor = geo::isPointInTriangle(curPos, inT1, inT2, inT3) ? 1.f : geo::getPixelAlpha(T3, T2, T1, x, y);


                                float curL = 100.f * (pt2y - v) / (pt2y - pt1y);
                                float Xstrentch = curL > 50.f ? ((v - pt1y) * 1.7320508f) : ((pt2y - v) * 1.7320508f);
                                float curC = Xstrentch < 1e-6f ? 0 : ((u - pt1x) / Xstrentch);
                                float r;
                                float g;
                                float b;
                                CSPMOD_ColorConvert::Lch2RGB(SDL_clamp(curL, 0.f, 100.f), SDL_clamp(curC*100.f, 0.f, 100.f),
                                    CSPMOD_ColorConvert::RGBH2LCHH(h_rgb), &r, &g, &b);

                                pData[2] = static_cast<uint8_t>(SDL_clamp(r * 256.f, 0.f, 255.f));
                                pData[1] = static_cast<uint8_t>(SDL_clamp(g * 256.f, 0.f, 255.f));
                                pData[0] = static_cast<uint8_t>(SDL_clamp(b * 256.f, 0.f, 255.f));
                                pData[3] = static_cast<uint8_t>(SDL_clamp(alphaFactor * 256.f, 0.f, 255.f));
                            }

                            pData += 4;
                        }
                    }
                }
                else
                {
                    ASSERT(false);

                }

                m_spBitmap->UnLockPixelBits();


















                needRepaint = false;

            }
        }

#endif
    }