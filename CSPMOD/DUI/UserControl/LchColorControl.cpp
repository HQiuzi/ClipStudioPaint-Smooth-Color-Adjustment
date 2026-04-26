#include "LchColorControl.h"
#include "duilib/Control/ColorConvert.h"
#include "duilib/Core/Window.h"
#include "duilib/Core/GlobalManager.h"
#include"Graphic/CSPMOD_ColorConvert.h"
#include"SDL3/SDL.h"
#include"CspData.h"
#include "LchColorControlContext.h"

using namespace ui;
    LchColorControl::LchColorControl(Window* pWindow) :
        Control(pWindow)
    {
        m_lastPt.x = -1;
        m_lastPt.y = -1;
        m_bMouseDown = false;

        AttachSizeChanged(UiBind(&LchColorControl::OnSizeChanged, this, std::placeholders::_1));
    }

    DString LchColorControl::GetType() const { return L"LchColorControl"; }

    void LchColorControl::GetCurrentColor96(Color96* pColor)
    {
        float r, g, b;
       float degree = h_rgb;
        CSPMOD_ColorConvert::Lch2RGB(L,c_percent, CSPMOD_ColorConvert::RGBH2LCHH(degree),&r,&g,&b);
        double dR = r;
        double dG = g;
        double dB = b;
        pColor->R=static_cast<uint32_t>(dR* UINT32_MAX);
        pColor->G=static_cast<uint32_t>(dG* UINT32_MAX);
        pColor->B =static_cast<uint32_t>(dB* UINT32_MAX);

        //return Color96();
    }

    void LchColorControl::GetCurrentUIColor(ui::UiColor* pColor)
    {
        float r, g, b;
        float degree = h_rgb;
        CSPMOD_ColorConvert::Lch2RGB(L, c_percent, CSPMOD_ColorConvert::RGBH2LCHH(degree), &r, &g, &b);
        double dR = r;
        double dG = g;
        double dB = b;
        pColor->SetARGB(ui::UiColor::MakeARGB(
            255,
            static_cast<uint8_t>(std::round (dR * UINT8_MAX)),
            static_cast<uint8_t>(std::round (dG * UINT8_MAX)),
            static_cast<uint8_t>(std::round (dB * UINT8_MAX))
        ));

    }

    void LchColorControl::GetCurrentColor(float* r, float* g, float* b)
    {

        float degree = h_rgb;
        CSPMOD_ColorConvert::Lch2RGB(L, c_percent, CSPMOD_ColorConvert::RGBH2LCHH(degree), r, g, b);

    }

    void LchColorControl::SetLCHrgb(float LCH_L, float LCH_C, float LCH_H)
    {

        if (L != LCH_L || c_percent != LCH_C || h_rgb != LCH_H)
        {

            needRepaint = L != LCH_L;
            needResetPos = c_percent != LCH_C || h_rgb != LCH_H;

            L = LCH_L;
            c_percent = LCH_C;
            h_rgb = LCH_H;
            
            Invalidate();

        }
    }

    //void LchColorControl::SetL(float LCH_L)
    //{
    //    if (L != LCH_L)
    //    {
    //        L = LCH_L;
    //        needRepaint = true;
    //        Invalidate();
    //    }
    //}

    //void LchColorControl::SetC(float LCH_C)
    //{
    //    if (c_percent != LCH_C)
    //    {
    //        c_percent = LCH_C;
    //        
    //        needResetPos = true;
    //        Invalidate();
    //    }
    //}

    //void LchColorControl::SetH(float LCH_H)
    //{
    //    if (h_rgb != LCH_H)
    //    {
    //        h_rgb = LCH_H;
    //        needResetPos = true;
    //        Invalidate();
    //    }
    //}

    void LchColorControl::PaintBkImage(IRender* pRender)
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

        IBitmap* pBitmap = GetColorBitmap(rcDest);




        UiRect rcSource;
        rcSource.left = 0;
        rcSource.top = 0;
        rcSource.right = rcSource.left + rc.Width();
        rcSource.bottom = rcSource.top + rc.Height();
        uint8_t uFade = 255;
        IMatrix* pMatrix = nullptr;

        if (rcDest.Width() > rcDest.Height())
        {
            rcDest.left = rcDest.left+(rcDest.Width() - rcDest.Height()) / 2;
            rcDest.right = rcDest.left+ rcDest.Height();
        }
        else
        {
            rcDest.top = rcDest.top + (rcDest.Height() - rcDest.Width()) / 2;
            rcDest.bottom = rcDest.top + rcDest.Width();
        }
        

        //计算位置
        if (needResetPos)
        {
            OnSizeChanged({});
            needResetPos = false;
        }









        if (pBitmap != nullptr) {

            if (needRepaint)_RePaintWheel();
            //pRender->DrawImageRect(rcPaint, pBitmap, rcDest, rcSource, uFade, pMatrix);
            pRender->DrawImage(rcPaint, pBitmap, rcDest, {0,0, (int)pBitmap->GetWidth(),(int)pBitmap->GetHeight() }, uFade);

            //{
            //UiPoint centerPt = rcDest.Center();
            //int32_t radius = (pBitmap->GetWidth())/2;
            ////CSPColor_CaptionFocus
            ////UiColor penColor = UiColor(ui::GlobalManager::Instance().Color().ConvertToUiColor(L"#CSPColor_CaptionFocus"));
            //UiColor penColor = UiColor(ui::GlobalManager::Instance().Color().ConvertToUiColor(L"#CSPColor_PanelBG"));
            //int32_t nWidth = Dpi().GetScaleInt(2);
            //pRender->DrawCircle(centerPt, radius, penColor, nWidth);
            //}
        }

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
    }

    bool LchColorControl::ButtonDown(const EventArgs& msg)
    {
        
        bool bRet = BaseClass::ButtonDown(msg);
        if (msg.IsSenderExpired()) {
            return false;
        }

        //计算是否点击在位图上
        //且需要设置鼠标形状

        //稍微放大些有效范围能加强体验
        bool isInWheel=false;
        {
            UiRect rect = GetRect();
            auto pCenter = rect.Center();
            pCenter.x -= msg.ptMouse.x;
            pCenter.y -= msg.ptMouse.y;

            auto distance2 = pCenter.x * pCenter.x + pCenter.y * pCenter.y;
            int r = 8 + m_spBitmap->GetWidth() / 2;
            isInWheel=(distance2 <= int(r*r));
        }
        
        if (isInWheel) {
            m_bMouseDown = true;
            SetMouseCapture(true);
            UiRect rect = GetRect();
            UiPadding padding = GetPadding();
            rect.left += padding.left;
            rect.right -= padding.right;
            rect.top += padding.top;
            rect.bottom -= padding.bottom;
            OnSelectPosChanged(rect, msg.ptMouse);
        }
        return bRet;
    }

    bool LchColorControl::MouseMove(const EventArgs& msg)
    {
        bool bRet = BaseClass::MouseMove(msg);
        if (msg.IsSenderExpired()) {
            return false;
        }
        if (m_bMouseDown) {
            UiRect rect = GetRect();
            UiPadding padding = GetPadding();
            rect.left += padding.left;
            rect.right -= padding.right;
            rect.top += padding.top;
            rect.bottom -= padding.bottom;
            OnSelectPosChanged(rect, msg.ptMouse);
        }

        if (m_bMouseDown)
        {
            ui::GlobalManager::Instance().Cursor().SetImageCursor(GetWindow(), ui::FilePath(L"Eyedropper.cur"));
        }
        else
        {
            bool isInWheel = false;
            bool isInCurWindow = false;
            {
                UiRect rect = GetRect();
                auto pCenter = rect.Center();
                pCenter.x -= msg.ptMouse.x;
                pCenter.y -= msg.ptMouse.y;

                auto distance2 = pCenter.x * pCenter.x + pCenter.y * pCenter.y;
                int r = 4 + m_spBitmap->GetWidth() / 2;
                isInWheel = (distance2 <= int(r * r));
            }
            {
                POINT currenCursorPos = {};
                ::GetCursorPos(&currenCursorPos);
                isInCurWindow = GetWindow()->GetWindowHandle() == ::WindowFromPoint(currenCursorPos);

            }
            if (isInWheel && isInCurWindow) {
                ui::GlobalManager::Instance().Cursor().SetImageCursor(GetWindow(), ui::FilePath(L"Eyedropper.cur"));
                SetMouseCapture(true);
            }
            else
                SetMouseCapture(false);
        }
        return bRet;
    }

    bool LchColorControl::ButtonUp(const EventArgs& msg)
    {
        m_bMouseDown = false;
        SetMouseCapture(false);
        return BaseClass::ButtonUp(msg);
    }

    bool LchColorControl::OnCaptureChanged(const ui::EventArgs& msg)
    {
        m_bMouseDown = false;
        return true;
    }

    void LchColorControl::SelectColor(const UiColor& selColor)
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

    IBitmap* LchColorControl::GetColorBitmap(const UiRect& rect)
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
            m_spBitmap->Init(newWidth, newWidth, nullptr, 1.0f, BitmapAlphaType::kPremul_SkAlphaType);
         
            _RePaintWheel();
        }
        return m_spBitmap.get();
    }

    void LchColorControl::OnSelectPosChanged(const UiRect& rect, const UiPoint& pt)
    {
        m_lastPt = pt;
        auto pCenter=rect.Center();
        if (!isMouseInWheel(pt))
        {
            ui::UiPointF deltaP;
            deltaP.x = (float)pt.x-pCenter.x;
            deltaP.y = (float)pt.y- pCenter.y;




            float rad= atan2f(deltaP.y, deltaP.x);

            float distance = sqrtf(float(deltaP.x* deltaP.x+ deltaP.y* deltaP.y));
            deltaP.x = deltaP.x * m_spBitmap->GetWidth()*0.5f/ distance;
            deltaP.y = deltaP.y * m_spBitmap->GetWidth()*0.5f/ distance;
            m_lastPt.x = int(pCenter.x + deltaP.x);
            m_lastPt.y = int(pCenter.y + deltaP.y);




            c_percent = 100.f;
            float deg = rad * 180.f / 3.1415927535f;
            deg += 150.f;
            if (deg < 0)deg += 360.f;
            else if (deg >= 360.f)deg -= 360.f;
            h_rgb = deg;
        }
        else
        {
            ui::UiPointF deltaP;
            deltaP.x = (float)pt.x - pCenter.x;
            deltaP.y = (float)pt.y - pCenter.y;
            float rad = atan2f(deltaP.y, deltaP.x);
            float distance = sqrtf(float(deltaP.x * deltaP.x + deltaP.y * deltaP.y));
            c_percent = distance*100.f/(m_spBitmap->GetWidth() * 0.5f);
            float deg = rad * 180.f / 3.1415927535f;
            deg += 150.f;
            if (deg < 0)deg += 360.f;
            else if (deg >= 360.f)deg -= 360.f;
            h_rgb = deg;
        }


        //SendEvent(kEventSelectColor, newColor.GetARGB());
        SendEvent(kEventSelectColor, {});




        Invalidate();
    }

    void LchColorControl::SetMouseCapture(bool bCapture)
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

    bool LchColorControl::isMouseInWheel(const ui::UiPoint& cursorPos)
    {
        if (!m_spBitmap)return false;


        UiRect rect = GetRect();
        UiPadding padding = GetPadding();
        rect.left += padding.left;
        rect.right -= padding.right;
        rect.top += padding.top;
        rect.bottom -= padding.bottom;


        auto pCenter=rect.Center();
        pCenter.x-= cursorPos.x;
        pCenter.y-= cursorPos.y;
        
        auto distance2 = pCenter.x * pCenter.x + pCenter.y * pCenter.y;

        return (distance2 <=int (m_spBitmap->GetWidth() * m_spBitmap->GetWidth()/4));
    }

    bool LchColorControl::OnSizeChanged(const ui::EventArgs& msg)
    {
        //根据Lch参数计算

        UiRect rect = GetRect();
        UiPadding padding = GetPadding();
        rect.left += padding.left;
        rect.right -= padding.right;
        rect.top += padding.top;
        rect.bottom -= padding.bottom;
        auto pCenter=rect.Center();
        float r=c_percent* SDL_min( rect.Width(), rect.Height()) / 200.f;
        float rad = h_rgb;
        rad += 210.f;
        if (rad >= 360.f)rad -= 360.f;
        rad = rad *3.1415926535f / 180.f;
        m_lastPt.x= static_cast<int>( pCenter.x+r* SDL_cosf(rad));
        m_lastPt.y= static_cast<int>(pCenter.y+r* SDL_sinf(rad));
        needResetPos = false;
        return true;
    }

    void LchColorControl::_RePaintWheel()
    {
        if (m_spBitmap)
        {
            void* pPixelBits = m_spBitmap->LockPixelBits();
            auto newWidth = m_spBitmap->GetWidth();

            if (pPixelBits != nullptr)
            {
                colWheelTex.Resize(m_spBitmap->GetWidth(), m_spBitmap->GetWidth());
                colWheelTex.Clear();


                LchColorControlContext::GetIns().DrawLchColorControl(&colWheelTex,L);


                void* pPixelBits = m_spBitmap->LockPixelBits();

                if (pPixelBits != nullptr) {

                    colWheelTex.CopyToMem((uint8_t*)pPixelBits, true, m_spBitmap->GetWidth() * 4);
                    m_spBitmap->UnLockPixelBits();
                }


                needRepaint = false;
            }

#if 0
            {

                uint8_t* pData = (uint8_t*)pPixelBits;
                for (int y = 0; y < (int)newWidth; y++)
                    for (int x = 0; x < (int)newWidth; x++)
                    {
                        //坐标转极坐标
                        float distance = ((newWidth * 0.5f - y) * (newWidth * 0.5f - y) + (newWidth * 0.5f - x) * (newWidth * 0.5f - x));
                        distance = sqrtf(distance);
                        distance = distance / (newWidth * 0.5f);
                        if (distance > 1.f)
                        {
                            //BGRA
                            pData[0] = 0;
                            pData[1] = 0;
                            pData[2] = 0;
                            pData[3] = 0;
                        }
                        else
                        {

                            float degree = atan2f(y - newWidth * 0.5f, x - newWidth * 0.5f) * 180.f / 3.1415927f;
                            degree += 150.f;//与CSP色相环同步
                            if (degree < 0) {
                                degree += 360.0f;
                            }
                            else if (degree >= 360.f)degree -= 360.0f;
                            float r;
                            float g;
                            float b;
                            
                            CSPMOD_ColorConvert::Lch2RGB(L, distance * 100.f, CSPMOD_ColorConvert::RGBH2LCHH(degree), &r, &g, &b);
                            //CSPMOD_ColorConvert::Lch2RGB(L, distance * 100.f, degree, &r, &g, &b);
                            pData[2] = static_cast<uint8_t>(SDL_clamp(r * 256.f, 0.f, 255.f));
                            pData[1] = static_cast<uint8_t>(SDL_clamp(g * 256.f, 0.f, 255.f));
                            pData[0] = static_cast<uint8_t>(SDL_clamp(b * 256.f, 0.f, 255.f));
                            pData[3] = 255;

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



                m_spBitmap->UnLockPixelBits();
                needRepaint = false;

            }
#endif
        }
    }
