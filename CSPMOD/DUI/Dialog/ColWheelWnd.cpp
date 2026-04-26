
#include "ColWheelWnd.h"
#include"AppSettings.h"
#include "CspData.h"
#include<iostream>


#include"Graphic/HSVColorConvert.h"
#include"DUI/UserControl/LchColorControl.h"
#include"DUI/UserControl/LchColorControl_Primary.h"
#include"DUI/UserControl/BrushColorBlock.h"
#include"Graphic/CSPMOD_ColorConvert.h"
#include"framework.h"

bool ColWheelWnd::sizeChangeFunc (const ui::EventArgs& msg)
{
    ui::Control* psender = msg.GetSender();
    if (psender == pthis->colorControl || psender == pthis->colorControl_Primary)
    {
        if (psender->IsVisible() && pthis->colorBlock)
        {
           //ui::UiPadding senderPadding= psender->GetPadding();

#if 0
            int32_t sideLen = SDL_min(psender->GetRect().Width(), psender->GetRect().Height());

            float fBlockLenPercent = 0.01f * BrushColorBlock::blockLenPercent;
            //L=0.5*sideLen - BrushColorBlockMargin;
            //(L-X)^2 + (L-fBlockLenPercent*X)^2 = (R)^2
            //float A = 1 + fBlockLenPercent * fBlockLenPercent;
            //float B = -sideLen * (1 + fBlockLenPercent);
            //float C = 0.5F * sideLen * 0.5F * sideLen;
            //A*x*x+B*x+C=0
            float L = 0.5f * sideLen - pthis->colorBlock->GetMargin().left;
            float R = 0.5f * sideLen - psender->GetPadding().left;
            float A = 1 + fBlockLenPercent * fBlockLenPercent;
            float B = -2.f*L-2* fBlockLenPercent*L;
            float C = 2*L*L-R*R;
            float resultX = (-B - sqrtf(B * B - 4 * A * C)) / (2 * A);
#endif
            float sideLenW = 0.5f*psender->GetRect().Width()- pthis->colorBlock->GetMargin().left;
            float sideLenH = 0.5f* psender->GetRect().Height()- pthis->colorBlock->GetMargin().bottom;

            float R = 0.5f * SDL_min(psender->GetRect().Width(), psender->GetRect().Height()) - psender->GetPadding().left;

            if (sideLenW < sideLenH)
            {
                float tem = sideLenH;
                sideLenH = sideLenW;
                sideLenW = tem;
            }


            float fBlockLenPercent = 0.01f * BrushColorBlock::blockLenPercent;
            //(W-X)2+(H-aX)2=R2
            float A= 1 + fBlockLenPercent * fBlockLenPercent;
            float B = -2.f * sideLenW - 2 * fBlockLenPercent * sideLenH;
            float C = sideLenW * sideLenW + sideLenH * sideLenH - R * R;
            float resultX = (-B - sqrtf(B * B - 4 * A * C)) / (2 * A);
            resultX = SDL_min(resultX,0.65F* sideLenH);
            if (resultX > 0)
            {
                pthis->colorBlock->SetFixedHeight(ui::UiFixedInt((int)resultX), true, false);
                pthis->colorBlock->SetFixedWidth(ui::UiFixedInt((int)resultX), true, false);
            }
            //pthis->colorBlock->SetFixedHeight(ui::UiFixedInt((int)100), true, false);
            //pthis->colorBlock->SetFixedWidth(ui::UiFixedInt((int)100), true, false);
            return true;
        }
    }
    else if (psender == pthis->sliderContainer)
    {
        if (pthis->sliderContainer->GetItemAt(0) == pthis->colorBlock)
        {
            ui::UiRect sliderConatinerRect = pthis->sliderContainer->GetRect();
            if (sliderConatinerRect.Width() > 300)
            {
                pthis->colorBlock->SetVisible(true);
            }
            else
            {
                    pthis->colorBlock->SetVisible(false);

            }

        }
    }
    return true;
 }
LRESULT ColWheelWnd::OnCaptureChangedMsg(const ui::NativeMsg& nativeMsg, bool& bHandled)
{
    //if(moused)
    return Window::OnCaptureChangedMsg(nativeMsg, bHandled);
}
LRESULT ColWheelWnd::OnSizeMsg(ui::WindowSizeType sizeType, const ui::UiSize& newWindowSize, const ui::NativeMsg& nativeMsg, bool& bHandled)
{
    
    //保存窗口位置信息

    ui::UiRect curWindowRect = this->GetWindowPos(false);


    saveRect.x = curWindowRect.left;
    saveRect.y = curWindowRect.top;
    saveRect.w = curWindowRect.Width();
    saveRect.h = curWindowRect.Height();



    return Window::OnSizeMsg(sizeType, newWindowSize, nativeMsg, bHandled);
}

LRESULT ColWheelWnd::OnMoveMsg(const ui::UiPoint& ptTopLeft, const ui::NativeMsg& msg, bool& bHandled)
{
    ui::UiRect curWindowRect = this->GetWindowPos(false);


    saveRect.x = curWindowRect.left;
    saveRect.y = curWindowRect.top;
    saveRect.w = curWindowRect.Width();
    saveRect.h = curWindowRect.Height();
    return Window::OnMoveMsg(ptTopLeft, msg,bHandled);
}



void  ColWheelWnd::setWindowMinSize ()
{
    int minheight = 18;
    int minWidth = 200;
    if (pthis->colorControl_Primary->IsVisible())minheight += 200;
    if (pthis->colorControl->IsVisible())minheight += 200;
    if (pthis->sliderContainer->IsVisible())minheight += pthis->sliderContainer->GetRect().Height();

    ui::UiRect windowRectWithShadow = pthis->GetWindowPos(true);
    ui::UiRect windowRect_NoShadow = pthis->GetWindowPos(false);
    pthis->SetWindowMinimumSize(
        {
            windowRectWithShadow.Width() - windowRect_NoShadow.Width() + minWidth,
        windowRectWithShadow.Height() - windowRect_NoShadow.Height() + minheight
        },
        true);

    //设置颜色控件的位置

    if (!pthis->colorBlock)pthis->colorBlock = new BrushColorBlock(pthis);

    pthis->colorControlContainer->SetVisible(pthis->colorControl_Primary->IsVisible() || pthis->colorControl->IsVisible());
    if (pthis->colorControl_Primary->IsVisible() || pthis->colorControl->IsVisible())
    {
        if (pthis->sliderContainer->GetItemIndex(pthis->colorBlock)!=ui::Box::InvalidIndex)
        {
            pthis->sliderContainer->SetAutoDestroyChild(false);
            pthis->sliderContainer->RemoveItem(pthis->colorBlock);
            pthis->sliderContainer->SetAutoDestroyChild(true);
        }

        ui::Control* prc = (pthis->colorControl_Primary->IsVisible() ? (ui::Control*)pthis->colorControl_Primary : (ui::Control*)pthis->colorControl);
        //prc->GetRect().Width(), prc->GetRect().Height();
        pthis->colorBlock->SetFloat(true);
        pthis->colorBlock->SetHorAlignType(ui::HorAlignType::kAlignLeft);
        pthis->colorBlock->SetVerAlignType(ui::VerAlignType::kAlignBottom);
        pthis->colorControlContainer->AddItem(pthis->colorBlock);

        ui::EventArgs arg;
        arg.SetSender(pthis->colorControl);
        sizeChangeFunc(arg);
        arg.SetSender(pthis->colorControl_Primary);
        sizeChangeFunc(arg);
    }
    else if (pthis->sliderContainer->IsVisible())
    {
        //颜色控件不可见、滑块可见
        //if (pthis->sliderContainer->GetItemIndex(pthis->colorBlock) != ui::Box::InvalidIndex)
        //{
        //    pthis->sliderContainer->SetAutoDestroyChild(false);
        //    pthis->sliderContainer->RemoveItem(pthis->colorBlock);
        //    pthis->sliderContainer->SetAutoDestroyChild(true);
        //}
        if (pthis->colorControlContainer->GetItemIndex(pthis->colorBlock) != ui::Box::InvalidIndex)
        {
            pthis->colorControlContainer->SetAutoDestroyChild(false);
            pthis->colorControlContainer->RemoveItem(pthis->colorBlock);
            pthis->colorControlContainer->SetAutoDestroyChild(true);
        }


        ui::UiRect sliderConatinerRect = pthis->sliderContainer->GetRect();
        //if (sliderConatinerRect.Width() > 300)
        {
            int32_t blockControlWidth = SDL_min(sliderConatinerRect.Width(), sliderConatinerRect.Height());
            pthis->colorBlock->SetFloat(false);
            pthis->colorBlock->SetHorAlignType(ui::HorAlignType::kAlignLeft);
            pthis->colorBlock->SetVerAlignType(ui::VerAlignType::kAlignTop);

            //pthis->colorBlock->SetFixedHeight(ui::UiFixedInt((int)blockControlWidth), true, false);
            //pthis->colorBlock->SetFixedWidth(ui::UiFixedInt((int)blockControlWidth), true, false);

            pthis->colorBlock->SetFixedHeight(ui::UiFixedInt((int)100), true, false);
            pthis->colorBlock->SetFixedWidth(ui::UiFixedInt((int)100), true, false);
            pthis->sliderContainer->AddItemAt(pthis->colorBlock, 0);
        }
    }
    else
    {
        //均不可见
        if (pthis->sliderContainer->GetItemIndex(pthis->colorBlock) != ui::Box::InvalidIndex)
        {
            pthis->sliderContainer->SetAutoDestroyChild(false);
            pthis->sliderContainer->RemoveItem(pthis->colorBlock);
            pthis->sliderContainer->SetAutoDestroyChild(true);
        }
        if (pthis->colorControlContainer->GetItemIndex(pthis->colorBlock) != ui::Box::InvalidIndex)
        {
            pthis->colorControlContainer->SetAutoDestroyChild(false);
            pthis->colorControlContainer->RemoveItem(pthis->colorBlock);
            pthis->colorControlContainer->SetAutoDestroyChild(true);
        }

    }


    };


void ColWheelWnd::ShowForm(bool needInitColor)
{
    if (pthis)
    {
        pthis->ShowWindow(ui::kSW_SHOW_NORMAL);
        return;
    }


    //HWND parent = (HWND)CspData::GetNativeWindowHandle();
    ////等待主窗口创建
    //if (!parent||!::IsWindow(parent))
    //{
    //    ui::GlobalManager::Instance().Thread().PostDelayedTask(ui::kThreadUI, [] { ColWheelWnd::ShowForm(); }, 100);

    //    return;
    //}


    pthis = new ColWheelWnd;
    pthis->_needInitColor = needInitColor;
    pthis->CreateWnd(nullptr,ui::WindowCreateParam(L"CSPMOD Color Picker",true));

    if (pthis->IsWindow())
    {


        pthis->ShowWindow(ui::kSW_SHOW_NORMAL);



        //设置窗口位置
        SDL_Rect windowRect = AppSettings::GetColorWheelWindowRect();
        std::string displayName= AppSettings::GetColorWheelDisplayName();
        int64_t displayIndex=AppSettings::GetColorWheelDisplayIndex();
        int displayCount=0;
        SDL_DisplayID* displays= SDL_GetDisplays(&displayCount);
        SDL_DisplayID targetDisplayID = 0;
        //优先检查窗口名
        for (int i = 0; i < displayCount; i++)
        {
            std::string curDisplayName=SDL_GetDisplayName(displays[i]);
            if (curDisplayName == displayName)
            {
                targetDisplayID = displays[i];
                if (i == displayIndex)break;
            }
        }
        if (!targetDisplayID)
        {
           //未按窗口名找到  直接使用索引
            if (displayIndex < displayCount)
            {
                targetDisplayID = displays[displayIndex];
            }
        }
        if (!targetDisplayID)
        {
            targetDisplayID = displays[0];
        }
        SDL_Rect targetDisplayRect = { 0,0,1920,1080 };
        SDL_GetDisplayBounds(targetDisplayID,&targetDisplayRect);

        if (windowRect.w < 50)windowRect.w = 50;
        if (windowRect.h < 50)windowRect.h = 50;
        windowRect.x += targetDisplayRect.x;
        windowRect.y += targetDisplayRect.y;
        SDL_GetDisplayUsableBounds(targetDisplayID,&targetDisplayRect);
        //调整windowRect 以使其完全在targetDisplayRect内部
        int xLimitL = targetDisplayRect.x;
        int xLimitR = targetDisplayRect.x + targetDisplayRect.w - windowRect.w;
        int yLimitT = targetDisplayRect.y;
        int yLimitB = targetDisplayRect.y+ targetDisplayRect.h- windowRect.h;

        windowRect.x = SDL_clamp(windowRect.x, xLimitL, xLimitR);
        windowRect.y=SDL_clamp(windowRect.y, yLimitT, yLimitB);
         SDL_free(displays);


        if (windowRect.w != 0)
        {
            ui::UiPadding shadowPadding= pthis->GetCurrentShadowCorner();
            bool result=
            pthis->MoveWindow(windowRect.x- shadowPadding.left, windowRect.y-shadowPadding.top, 
                windowRect.w+shadowPadding.right+shadowPadding.left,
                windowRect.h+shadowPadding.top+shadowPadding.bottom , 
                true);

        }


        //先移动再设置？
        HWND child = pthis->NativeWnd()->GetHWND();
        //HWND parent =(HWND) CspData::GetNativeWindowHandle();
        HWND parent = (HWND)CspData::GetNativeWindowHandle();
        ::SetWindowLongPtr(child, GWLP_HWNDPARENT, (LONG_PTR)parent);















    }
    
}

ColWheelWnd::ColWheelWnd()
{
    PostQuitMsgWhenClosed(false);



}

ColWheelWnd::~ColWheelWnd()
{
    //这个会调用，但不会调用其他东西... 因为收到的消息直接是finalmessage了
    SDL_Log("~ColWheelWnd()");
}

std::wstring ColWheelWnd::GetSkinFolder()
{
	return L"cspHelper_default";
}

std::wstring ColWheelWnd::GetSkinFile()
{
	return L"ColWheelWnd.xml";
}




//颜色替换因为需要吸取图层原本的颜色，所以只能使用插件点，不能直接屏幕取色


void ColWheelWnd::OnInitWindow()
{
	__super::OnInitWindow();
    //this->SetCapture();
    //ui::GlobalManager::Instance().Cursor().SetImageCursor(this, ui::FilePath(L"Eyedropper.cur"));
   





    
    
    sliderContainer= static_cast<ui::Box*>(FindControl(L"sliderContainer"));
    colorControlContainer = static_cast<ui::Box*>(FindControl(L"colorControlContainer"));

   colorControl= static_cast<LchColorControl*>(FindControl(L"visualLchColorWheel"));
   colorControl->AttachSelectColor(UiBind(&ColWheelWnd::OnColorSelect, this, std::placeholders::_1));
   colorControl_Primary = static_cast<LchColorControl_Primary*>(FindControl(L"visualLchColorWheel_Primary"));
   colorControl_Primary->AttachSelectColor(UiBind(&ColWheelWnd::OnColorSelect, this, std::placeholders::_1));



   colorControl->AttachSizeChanged(sizeChangeFunc);
   colorControl_Primary->AttachSizeChanged(sizeChangeFunc);
   sliderContainer->AttachSizeChanged(sizeChangeFunc);



    ui::Button* pBtn_Close = static_cast<ui::Button*>(FindControl(L"btn_Close"));
    if (pBtn_Close)
    {
        pBtn_Close->AttachClick(UiBind(&ColWheelWnd::OnButtonClick, this, std::placeholders::_1));
    }

    static_cast<ui::Button*>(FindControl(L"windowMenu"))
        ->AttachClick(UiBind(&ColWheelWnd::OnButtonClick, this, std::placeholders::_1));






    //色相饱和度明度文本对齐
    //ui::Label* label_hue= static_cast<ui::Label*>(FindControl(L"label_hue"));
    //ui::Label* label_saturation = static_cast<ui::Label*>(FindControl(L"label_saturation"));
    //ui::Label* label_value = static_cast<ui::Label*>(FindControl(L"label_value"));
    //ui::Label* label_strength = static_cast<ui::Label*>(FindControl(L"label_strength"));

    //int labelSizeMax = max(max(max(
    //    label_hue->EstimateText({ 9999,9999 }).cx,
    //    label_saturation->EstimateText({ 9999,9999 }).cx),
    //    label_value->EstimateText({ 9999,9999 }).cx),
    //    label_strength->EstimateText({ 9999,9999 }).cx
    //    );
    //label_hue->SetFixedWidth( ui::UiFixedInt(labelSizeMax), false, false);
    //label_saturation->SetFixedWidth( ui::UiFixedInt(labelSizeMax), false, false);
    //label_value->SetFixedWidth( ui::UiFixedInt(labelSizeMax), false, false);
    //label_strength->SetFixedWidth( ui::UiFixedInt(labelSizeMax), true, false);



    slider_vlch_L = static_cast<ui::Slider*>(FindControl(L"slider_vlch_L"));
    slider_vlch_C = static_cast<ui::Slider*>(FindControl(L"slider_vlch_C"));
    slider_vlch_H = static_cast<ui::Slider*>(FindControl(L"slider_vlch_H"));

    edit_vlch_L = static_cast<ui::RichEdit*>(FindControl(L"edit_vlch_L"));
    edit_vlch_C = static_cast<ui::RichEdit*>(FindControl(L"edit_vlch_C"));
    edit_vlch_H = static_cast<ui::RichEdit*>(FindControl(L"edit_vlch_H"));

    slider_vlch_L->AttachValueChanged(UiBind(&ColWheelWnd::OnSliderValueChange, this, std::placeholders::_1));
    slider_vlch_C->AttachValueChanged(UiBind(&ColWheelWnd::OnSliderValueChange, this, std::placeholders::_1));
    slider_vlch_H->AttachValueChanged(UiBind(&ColWheelWnd::OnSliderValueChange, this, std::placeholders::_1));

    edit_vlch_L->AttachTextChanged(UiBind(&ColWheelWnd::OnSliderValueTextChange, this, std::placeholders::_1));
    edit_vlch_C->AttachTextChanged(UiBind(&ColWheelWnd::OnSliderValueTextChange, this, std::placeholders::_1));
    edit_vlch_H->AttachTextChanged(UiBind(&ColWheelWnd::OnSliderValueTextChange, this, std::placeholders::_1));


    //根据系统设置设置显示隐藏

    int64_t itemShowState = AppSettings::GetColorWheelItemShow();
    if (itemShowState & 0x1|| itemShowState&(1<<1))
    {
        colorControl_Primary->SetVisible(true);
        if (itemShowState & 0x1)
            colorControl_Primary->SetWheelType(LchColorControl_Primary::WheelType_Rectangle);
        else
            colorControl_Primary->SetWheelType(LchColorControl_Primary::WheelType_Triangle);
    }
    else
    {
        colorControl_Primary->SetVisible(false);
    }

     colorControl->SetVisible(bool(itemShowState & (1 << 2)));
     sliderContainer->SetVisible(bool(itemShowState & (1 << 3)));
    setWindowMinSize();



    if (_needInitColor)
    {
        //颜色初始化
        Color96 mainPaintColor;
        CspColorTable::GetMainPaintColor(&mainPaintColor);
        float r = static_cast<float>(mainPaintColor.R / double(UINT32_MAX));
        float g = static_cast<float>(mainPaintColor.G / double(UINT32_MAX));
        float b = static_cast<float>(mainPaintColor.B / double(UINT32_MAX));
        float mainL, mainC, mainH;
        CSPMOD_ColorConvert::RGB2Lch(r, g, b, &mainL, &mainC, &mainH);
        mainH = CSPMOD_ColorConvert::LCHH2UIH(mainH);
        colorBlock->SetMainColor(r, g, b);

        Color96 subPaintColor;
        CspColorTable::GetSubPaintColor(&subPaintColor);
        r = static_cast<float>(subPaintColor.R / double(UINT32_MAX));
        g = static_cast<float>(subPaintColor.G / double(UINT32_MAX));
        b = static_cast<float>(subPaintColor.B / double(UINT32_MAX));
        float subL, subC, subH;
        CSPMOD_ColorConvert::RGB2Lch(r, g, b, &subL, &subC, &subH);
        subH = CSPMOD_ColorConvert::LCHH2UIH(subH);
        colorBlock->SetSubColor(r, g, b);
        LCH_L = mainL;
        LCH_C = mainC;
        LCH_H = mainH;
        {

            colorControl->SetLCHrgb(LCH_L, LCH_C, LCH_H);
            colorControl_Primary->SetLCHrgb(LCH_L, LCH_C, LCH_H);


            edit_vlch_L->SetTextNoEvent(std::to_wstring((int)SDL_round(LCH_L)));
            edit_vlch_C->SetTextNoEvent(std::to_wstring((int)SDL_round(LCH_C)));
            edit_vlch_H->SetTextNoEvent(std::to_wstring((int)SDL_round(LCH_H)));
        }
    }
}


void ColWheelWnd::SetColor(const Color96* col96)
{
    
    float r = CSPMOD_ColorConvert::UInt32ToFloat(col96->R);
    float g = CSPMOD_ColorConvert::UInt32ToFloat(col96->G);
    float b = CSPMOD_ColorConvert::UInt32ToFloat(col96->B);


    SetColor(r,g,b);
}

void ColWheelWnd::SetColor(float r, float g, float b)
{
    float lchH;
    CSPMOD_ColorConvert::RGB2Lch(r, g, b, &LCH_L, &LCH_C, &lchH);
    LCH_H = CSPMOD_ColorConvert::LCHH2UIH(lchH);

    {
        colorControl->SetLCHrgb(LCH_L, LCH_C, LCH_H);
        colorControl_Primary->SetLCHrgb(LCH_L, LCH_C, LCH_H);


        edit_vlch_L->SetTextNoEvent(std::to_wstring((int)SDL_round(LCH_L)));
        edit_vlch_C->SetTextNoEvent(std::to_wstring((int)SDL_round(LCH_C)));
        edit_vlch_H->SetTextNoEvent(std::to_wstring((int)SDL_round(LCH_H)));
    }

    if (colorBlock)
    {
        colorControl->GetCurrentColor(&r,&g,&b);
        colorBlock->SetCurrentColor(r,g,b);
    }

}

void ColWheelWnd::SetColorType(CspColorTable::PaintColorType type)
{
    if (colorBlock && colorControl && colorControl_Primary)
    {
        if (colorBlock->SetWorkingColorType(type))
        {

            float r, g, b;
            colorBlock->GetCurrentColor(&r, &g, &b);
            SetColor(r, g, b);
        }
    }
}

void ColWheelWnd::SetTypeColor(CspColorTable::PaintColorType type, const Color96* col96)
{
    if (isSetColSelf)return;//防止主动设置CSP颜色时这里也收到回馈

    if (colorBlock && colorControl && colorControl_Primary)
    {
        float r = CSPMOD_ColorConvert::UInt32ToFloat(col96->R);
        float g = CSPMOD_ColorConvert::UInt32ToFloat(col96->G);
        float b = CSPMOD_ColorConvert::UInt32ToFloat(col96->B);
        if (type == CspColorTable::PaintColorType::Main)
        {
            if (colorBlock->SetMainColor(r,g,b))
            {
                SetColor(r,g,b);
            }

        }
        else
        {
            if (colorBlock->SetSubColor(r, g, b))
            {
                SetColor(r, g, b);
            }
        }

    }
}

bool ColWheelWnd::OnButtonClick(const ui::EventArgs& args)
{
    DString controlName = args.GetSender()->GetName();
    if (controlName == L"btn_Close")
    {
        this->CloseWnd();
        return true;
    }
    if (controlName == L"windowMenu")
    {
        //菜单


        //ui::UiPoint p = args.ptMouse;
        ui::UiRect senderRect = args.GetSender()->GetRect();

        ui::Menu* menu = new ui::Menu(this);
        menu->SetSkinFolder(L"cspHelper_default");
        DString xml(L"ColWheelWndMenu.xml");

        ui::UiRect windowRect = this->GetWindowPos(true);
        //p.x += windowRect.left;
        //p.y += windowRect.top;
        menu->ShowMenu(xml, { windowRect.left + senderRect.left + 4,windowRect.top + senderRect.bottom });
        //menu->ShowMenu(xml, { p.x - 10,p.y + 4 });


        ui::MenuItem* colWheelMenu_Rectangle = dynamic_cast<ui::MenuItem*>(menu->FindControl(L"colWheelMenu_Rectangle"));
        ui::MenuItem* colWheelMenu_Triangle = dynamic_cast<ui::MenuItem*>(menu->FindControl(L"colWheelMenu_Triangle"));

        ui::MenuItem* colWheelMenu_Pan = dynamic_cast<ui::MenuItem*>(menu->FindControl(L"colWheelMenu_Pan"));
        ui::MenuItem* colWheelMenu_ColorSlider = dynamic_cast<ui::MenuItem*>(menu->FindControl(L"colWheelMenu_ColorSlider"));

        ui::MenuItem* colWheelMenu_SyncVisibility = dynamic_cast<ui::MenuItem*>(menu->FindControl(L"colWheelMenu_SyncVisibility"));



        //设置选中图标的显隐
        colWheelMenu_Rectangle->GetItemAt(0)->SetVisible(
            colorControl_Primary->IsVisible()&& colorControl_Primary->GetWheelType()==LchColorControl_Primary::WheelType_Rectangle);
        colWheelMenu_Triangle->GetItemAt(0)->SetVisible(
            colorControl_Primary->IsVisible()&& colorControl_Primary->GetWheelType()==LchColorControl_Primary::WheelType_Triangle);
        colWheelMenu_Pan->GetItemAt(0)->SetVisible(colorControl->IsVisible());
        colWheelMenu_ColorSlider->GetItemAt(0)->SetVisible(sliderContainer->IsVisible());
        colWheelMenu_SyncVisibility->GetItemAt(0)->SetVisible(syncVisibility);


        ui::UiSize sizeMax(9999, 9999);
        int32_t maxW = 0;

        {
            int32_t curW = colWheelMenu_Rectangle->EstimateSize(sizeMax).cx.GetInt32();
            if (curW > maxW)maxW = curW;
            //if (!ScriptAction_ApplyEffects::IsEnabled())
            //    helperMenu_applyEffects->SetEnabled(false);
        }
        {
            int32_t curW = colWheelMenu_Triangle->EstimateSize(sizeMax).cx.GetInt32();
            if (curW > maxW)maxW = curW;
        }
        {
            int32_t curW = colWheelMenu_Pan->EstimateSize(sizeMax).cx.GetInt32();
            if (curW > maxW)maxW = curW;
        }
        {
            int32_t curW = colWheelMenu_ColorSlider->EstimateSize(sizeMax).cx.GetInt32();
            if (curW > maxW)maxW = curW;
        }
        {
            int32_t curW = colWheelMenu_SyncVisibility->EstimateSize(sizeMax).cx.GetInt32();
            if (curW > maxW)maxW = curW;
        }

        colWheelMenu_Rectangle->SetFixedWidth(ui::UiFixedInt(maxW), true, false);
        colWheelMenu_Triangle->SetFixedWidth(ui::UiFixedInt(maxW), true, false);
        colWheelMenu_Pan->SetFixedWidth(ui::UiFixedInt(maxW), true, false);
        colWheelMenu_ColorSlider->SetFixedWidth(ui::UiFixedInt(maxW), true, false);
        colWheelMenu_SyncVisibility->SetFixedWidth(ui::UiFixedInt(maxW), true, false);






        //添加按钮功能
        colWheelMenu_Rectangle->AttachClick(
            [](const ui::EventArgs&)->bool {
                if (pthis->colorControl_Primary->IsVisible()
                    && pthis->colorControl_Primary->GetWheelType() == LchColorControl_Primary::WheelType::WheelType_Rectangle)
                {


                    //减小窗口高度
                    int fixHeight = pthis->colorControl_Primary->GetRect().Height();
                    ui::UiRect windowRect = pthis->GetWindowPos(true);


                    pthis->colorControl_Primary->SetVisible(false);

                    setWindowMinSize();
                    pthis->Resize(windowRect.Width(),
                        windowRect.Height() - fixHeight, true, true);
                }
                else
                {

                    pthis->colorControl_Primary->SetWheelType(LchColorControl_Primary::WheelType::WheelType_Rectangle);
                    if (!pthis->colorControl_Primary->IsVisible())
                    {
                        //扩大窗口高度
                        if (pthis->colorControl->IsVisible())
                        {
                            ui::UiRect windowRect = pthis->GetWindowPos(true);
                            pthis->Resize(windowRect.Width(),
                                windowRect.Height() + pthis->colorControl->GetRect().Height(), true, true);
                        }
                        else
                        {

                            ui::UiRect windowRect = pthis->GetWindowPos(true);
                            ui::UiRect windowRect_noshadow = pthis->GetWindowPos(false);
                            pthis->Resize(windowRect.Width(),
                                windowRect.Height() + windowRect_noshadow.Width(), true, true);
                        }
                        pthis->colorControl_Primary->SetVisible(true);

                        setWindowMinSize();
                    }
                }
                return true;
            });
        colWheelMenu_Triangle->AttachClick(
            [](const ui::EventArgs&)->bool {
                if (pthis->colorControl_Primary->IsVisible()
                    && pthis->colorControl_Primary->GetWheelType() == LchColorControl_Primary::WheelType::WheelType_Triangle)
                {
                    //减小窗口高度
                    int fixHeight = pthis->colorControl_Primary->GetRect().Height();
                    ui::UiRect windowRect = pthis->GetWindowPos(true);


                    pthis->colorControl_Primary->SetVisible(false);
                    setWindowMinSize();
                    pthis->Resize(windowRect.Width(),
                        windowRect.Height() - fixHeight, true, true);
                }
                else
                {
                    pthis->colorControl_Primary->SetWheelType(LchColorControl_Primary::WheelType::WheelType_Triangle);
                    if (!pthis->colorControl_Primary->IsVisible())
                    {
                        //扩大窗口高度
                        if (pthis->colorControl->IsVisible())
                        {
                            ui::UiRect windowRect = pthis->GetWindowPos(true);
                            pthis->Resize(windowRect.Width(),
                                windowRect.Height() + pthis->colorControl->GetRect().Height(), true, true);
                        }
                        else
                        {

                            ui::UiRect windowRect = pthis->GetWindowPos(true);
                            ui::UiRect windowRect_noshadow = pthis->GetWindowPos(false);
                            pthis->Resize(windowRect.Width(),
                                windowRect.Height() + windowRect_noshadow.Width(), true, true);
                        }
                        pthis->colorControl_Primary->SetVisible(true);
                        setWindowMinSize();
                    }
                }
                return true;
            });
        colWheelMenu_Pan->AttachClick(
            [](const ui::EventArgs&)->bool {
                if (pthis->colorControl->IsVisible())
                {
                    //减小窗口高度
                    int fixHeight = pthis->colorControl->GetRect().Height();
                    ui::UiRect windowRect = pthis->GetWindowPos(true);

                    pthis->colorControl->SetVisible(false);
                    setWindowMinSize();
                    pthis->Resize(windowRect.Width(),
                        windowRect.Height() - fixHeight, true, true);
                }
                else
                {
                    //扩大窗口高度
                    if (pthis->colorControl_Primary->IsVisible())
                    {
                        ui::UiRect windowRect = pthis->GetWindowPos(true);
                        pthis->Resize(windowRect.Width(),
                            windowRect.Height() + pthis->colorControl_Primary->GetRect().Height(), true, true);
                    }
                    else
                    {
                        ui::UiRect windowRect = pthis->GetWindowPos(true);
                        ui::UiRect windowRect_noshadow = pthis->GetWindowPos(false);
                        pthis->Resize(windowRect.Width(),
                            windowRect.Height() + windowRect_noshadow.Width(), true, true);
                    }

                    pthis->colorControl->SetVisible(true);
                    setWindowMinSize();
                }
                return true;
            });
        colWheelMenu_ColorSlider->AttachClick(
            [](const ui::EventArgs&)->bool {
                auto control = pthis->FindControl(L"sliderContainer");
                if (control->IsVisible())
                {
                    ui::UiRect windowRect = pthis->GetWindowPos(true);
                    control->SetVisible(false);
                    setWindowMinSize();


                    pthis->Resize(windowRect.Width(),
                        windowRect.Height() - control->GetRect().Height(), true, true);


                }
                else
                {
                    ui::UiRect windowRect = pthis->GetWindowPos(true);
                    control->SetVisible(true);
                    setWindowMinSize();
                    pthis->Resize(windowRect.Width(),
                        windowRect.Height() + control->GetRect().Height(), true, true);

                }
                return true;
            });
        colWheelMenu_SyncVisibility->AttachClick(
            [](const ui::EventArgs&)->bool {
                pthis->syncVisibility=!pthis->syncVisibility;
                return true;
            });

    }
    return true;
}

bool ColWheelWnd::OnSliderValueChange(const ui::EventArgs& args)
{
    ui::Slider* pslider = (ui::Slider*)args.GetSender();



    //为了增加流畅性，调色滑块总刻度数量是100* n
    float realValue= static_cast<float>( pslider->GetValue()/100.f);

    bool valueUpdated = false;
    if (pslider == slider_vlch_L)
    {
        if (realValue != LCH_L)
        {
            LCH_L = realValue;

            edit_vlch_L->SetTextNoEvent(std::to_wstring((int)SDL_round(realValue)));
            colorControl->SetLCHrgb(LCH_L, LCH_C, LCH_H);
            colorControl_Primary->SetLCHrgb(LCH_L, LCH_C, LCH_H);
            valueUpdated = true;
        }
    }
    else if (pslider == slider_vlch_C)
    {
        if (realValue != LCH_C)
        {
            LCH_C = realValue;
            edit_vlch_C->SetTextNoEvent(std::to_wstring((int)SDL_round(realValue)));
            colorControl->SetLCHrgb(LCH_L, LCH_C, LCH_H);
            colorControl_Primary->SetLCHrgb(LCH_L, LCH_C, LCH_H);
            valueUpdated = true;
        }
    }
    else if (pslider == slider_vlch_H)
    {
        if (realValue != LCH_H)
        {
            LCH_H = realValue;
            edit_vlch_H->SetTextNoEvent(std::to_wstring((int)SDL_round(realValue)));
            colorControl->SetLCHrgb(LCH_L, LCH_C, LCH_H);
            colorControl_Primary->SetLCHrgb(LCH_L, LCH_C, LCH_H);
            valueUpdated = true;
        }
    }

    if (valueUpdated)
    {

        Color96 col;
        colorControl->GetCurrentColor96(&col);
        colorControl_Primary->GetCurrentColor96(&col);
        isSetColSelf = true;
        CspColorTable::SetCurrentPaintColor(col);
        isSetColSelf = false;

        if (colorBlock)
        {
            float r, g, b;
            colorControl->GetCurrentColor(&r,&g,&b);
            colorBlock->SetCurrentColor(r, g, b);
        }
    }

	return true;
}

bool ColWheelWnd::OnSliderValueTextChange(const ui::EventArgs& args)
{
    ui::RichEdit* curEdit = (ui::RichEdit*)args.GetSender();
    int64_t sliderValue = curEdit->GetTextNumber()*100;
    
    ui::EventArgs event;
    if (curEdit == edit_vlch_L)
    {
        slider_vlch_L->SetValue(static_cast<double>(sliderValue));
        event.SetSender(slider_vlch_L);
    }
    else if (curEdit == edit_vlch_C)
    {
        slider_vlch_C->SetValue(static_cast<double>(sliderValue));
        event.SetSender(slider_vlch_C);
    }
    else if (curEdit == edit_vlch_H)
    {
        slider_vlch_H->SetValue(static_cast<double>(sliderValue));
        event.SetSender(slider_vlch_H);
    }


    //Slider::SetValue无法触发回调，这里手动调用
    if (event.GetSender())
    {
        OnSliderValueChange(event);
    }


    return true;
}

bool ColWheelWnd::OnColorSelect(const ui::EventArgs& args)
{
    Color96 col;
    if (args.GetSender() == colorControl)
    {
        colorControl->GetCurrentColor96(&col);
        isSetColSelf = true;
        CspColorTable::SetCurrentPaintColor(col);
        isSetColSelf = false;
        LCH_L = colorControl->GetL();
        LCH_C = colorControl->GetC();
        LCH_H = colorControl->GetH();
        edit_vlch_L->SetTextNoEvent(std::to_wstring((int)SDL_roundf(LCH_L)));
        edit_vlch_C->SetTextNoEvent(std::to_wstring((int)SDL_roundf(LCH_C)));
        edit_vlch_H->SetTextNoEvent(std::to_wstring((int)SDL_roundf(LCH_H)));
        slider_vlch_L->SetValue(LCH_L * 100.f);
        slider_vlch_C->SetValue(LCH_C * 100.f);
        slider_vlch_H->SetValue(LCH_H * 100.f);

        colorControl_Primary->SetLCHrgb(LCH_L, LCH_C, LCH_H);
    }
    else
    {
        colorControl_Primary->GetCurrentColor96(&col);
        isSetColSelf = true;
        CspColorTable::SetCurrentPaintColor(col);
        isSetColSelf = false;

        LCH_L = colorControl_Primary->GetL();
        LCH_C = colorControl_Primary->GetC();
        LCH_H = colorControl_Primary->GetH();
        edit_vlch_L->SetTextNoEvent(std::to_wstring((int)SDL_roundf(LCH_L)));
        edit_vlch_C->SetTextNoEvent(std::to_wstring((int)SDL_roundf(LCH_C)));
        edit_vlch_H->SetTextNoEvent(std::to_wstring((int)SDL_roundf(LCH_H)));
        slider_vlch_L->SetValue(LCH_L * 100.f);
        slider_vlch_C->SetValue(LCH_C * 100.f);
        slider_vlch_H->SetValue(LCH_H * 100.f);


        colorControl->SetLCHrgb(LCH_L, LCH_C, LCH_H);
    }
    if (colorBlock)
    {
        float r, g, b;
        colorControl->GetCurrentColor(&r, &g, &b);
        colorBlock->SetCurrentColor(r, g, b);
    }
    return true;
}

























void ColWheelWnd::OnPreCloseWindow()
{
    if (pthis->sliderContainer->GetItemIndex(pthis->colorBlock) != ui::Box::InvalidIndex)
    {
        pthis->sliderContainer->SetAutoDestroyChild(false);
        pthis->sliderContainer->RemoveItem(pthis->colorBlock);
        pthis->sliderContainer->SetAutoDestroyChild(true);
    }
    if (pthis->colorControlContainer->GetItemIndex(pthis->colorBlock) != ui::Box::InvalidIndex)
    {
        pthis->colorControlContainer->SetAutoDestroyChild(false);
        pthis->colorControlContainer->RemoveItem(pthis->colorBlock);
        pthis->colorControlContainer->SetAutoDestroyChild(true);
    }
    if (colorBlock)delete colorBlock;
    colorBlock = nullptr;

    {
        AppSettings::SetColorWheelOpenWhenStart(false);
        SavePos();
    }



    pthis = nullptr;
    __super::OnPreCloseWindow();
    //pthis = nullptr;
}

void ColWheelWnd::SavePos()
{
    //保存窗口位置信息

    
    int displayCount = 0;
    SDL_DisplayID* displayids = SDL_GetDisplays(&displayCount);

    auto _saveRect = saveRect;
    SDL_DisplayID targetDisplay = SDL_GetDisplayForRect(&_saveRect);
    int displayIndex = 0;
    for (; displayIndex < displayCount; displayIndex++)
    {
        if (targetDisplay == displayids[displayIndex])
        {
            break;
        }
    }
    SDL_Rect targetDisplayBounds = { 0,0,1920,1080 };
    SDL_GetDisplayBounds(targetDisplay, &targetDisplayBounds);
    //计算屏幕相关坐标
    _saveRect.x -= targetDisplayBounds.x;
    _saveRect.y -= targetDisplayBounds.y;
    std::string displayName = SDL_GetDisplayName(targetDisplay);

    AppSettings::SetColorWheelDisplayName(displayName);
    AppSettings::SetColorWheelDisplayIndex(displayIndex);
    AppSettings::SetColorWheelWindowRect(_saveRect);
    SDL_free(displayids);

}

ui::Control* ColWheelWnd::CreateControl(const DString& strClass)
{
    if (strClass == _T("LchColorControl")) {
        return new LchColorControl(this);
    }
    else if (strClass == _T("LchColorControl_Primary")) {
        return new LchColorControl_Primary(this);
    }
    else if (strClass == _T("BrushColorBlock")) {
        return new BrushColorBlock(this);
    }
    return nullptr;
}




