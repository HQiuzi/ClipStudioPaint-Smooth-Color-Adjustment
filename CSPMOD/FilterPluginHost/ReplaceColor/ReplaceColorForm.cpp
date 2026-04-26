
#include "ReplaceColorForm.h"
#include"ReplaceColor.h"
#include"AppSettings.h"
#include "CspData.h"
#include<iostream>


#include"Graphic/HSVColorConvert.h"

#include"framework.h"
void ReplaceColorForm::ShowForm()
{
    //return;


    if (pthis)return;
    pthis = new ReplaceColorForm;
    pthis->CreateWnd(nullptr,ui::WindowCreateParam(L"CSP Filter Replace Color",true));

    if (pthis->IsWindow())
    {
        HWND child = pthis->NativeWnd()->GetHWND();
        //HWND parent =(HWND) CspData::GetNativeWindowHandle();
        //::SetWindowLongPtr(child, GWLP_HWNDPARENT, (LONG_PTR)parent);

        pthis->ShowWindow(ui::kSW_SHOW_NORMAL);


        SDL_Log("Createing ReplaceColor Window");
        //设置窗口位置
        SDL_Rect windowRect = AppSettings::GetReplaceColorWindowRect();
        std::string displayName= AppSettings::GetReplaceColorDisplayName();
        int64_t displayIndex=AppSettings::GetReplaceColorDisplayIndex();


        SDL_Log("Last Saved WindowRect:%d,%d,%d,%d", windowRect.x, windowRect.y, windowRect.w, windowRect.h);
        SDL_Log("Last Saved DisplayName:%s", displayName.c_str());
        SDL_Log("Last Saved DisplayIndex:%llu", displayIndex);


        int displayCount=0;
        SDL_DisplayID* displays= SDL_GetDisplays(&displayCount);
        SDL_DisplayID targetDisplayID = 0;
        //优先检查窗口名
        for (int i = 0; i < displayCount; i++)
        {

            std::string curDisplayName=SDL_GetDisplayName(displays[i]);
            if (curDisplayName == displayName)
            {

                SDL_Log("Name Matched DisplayIndex:%d", i);
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

        SDL_Log("Finale Display:%s", SDL_GetDisplayName(targetDisplayID));

        SDL_Rect targetDisplayRect = { 0,0,1920,1080 };
        SDL_GetDisplayBounds(targetDisplayID,&targetDisplayRect);


        SDL_Log("Target Display Bounds:%d,%d,%d,%d", targetDisplayRect.x, targetDisplayRect.y, targetDisplayRect.w, targetDisplayRect.h);


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


         SDL_Log("Fixed WindowRect:%d,%d,%d,%d", windowRect.x, windowRect.y, windowRect.w, windowRect.h);
        if (windowRect.w != 0)
        {
            ui::UiPadding shadowPadding= pthis->GetCurrentShadowCorner();
            pthis->MoveWindow(windowRect.x- shadowPadding.left, windowRect.y-shadowPadding.top, 
                windowRect.w+shadowPadding.right+shadowPadding.left,
                windowRect.h+shadowPadding.top+shadowPadding.bottom , 
                true);

        }











        //查找名为替换颜色的子窗口


        //ui::GlobalManager::Instance().Thread().PostDelayedTask(ui::kThreadUI, []() {


            HWND targetWindow = nullptr;




            WNDENUMPROC enumProc = [](HWND hwnd, LPARAM param)->BOOL
                {


                    //确保是同一个应用
                    wchar_t titleBuf1[256] = {};
                    wchar_t titleBuf2[256] = {};
                    UINT numCount1=::GetWindowModuleFileName(hwnd, titleBuf1, 256);

                    UINT numCount2 = ::GetWindowModuleFileName((HWND)CspData::GetNativeWindowHandle(), titleBuf2, 256);
                    
                    if (numCount1 == numCount2&&memcmp(titleBuf1, titleBuf2, numCount1*2)==0)
                    {
                        ::GetWindowText(hwnd, titleBuf1, 256);

                        if (ui::GlobalManager::Instance().Lang().GetStringViaID(L"STRID_FILTERPLUGIN_REPLACECOLOR") == titleBuf1)
                        {




                            *(HWND*)param = hwnd;
                            return FALSE;
                        }

                    }
                    else
                    {

                        return TRUE;
                    }




                    return TRUE;
                };

            ::EnumWindows(/*(HWND)CspData::GetNativeWindowHandle(),*/ enumProc, (LPARAM)&targetWindow);
            if (targetWindow)
            {
                auto vap = ::GetWindowLongPtr(targetWindow, GWLP_HWNDPARENT);


                ::ShowWindow(targetWindow, SW_HIDE);
                //::EnableWindow(targetWindow,false);




                ::SetWindowLongPtr(child, GWLP_HWNDPARENT, (LONG_PTR)targetWindow);
                pthis->originFilterDlg =(uintptr_t) targetWindow;



            }


            //},
            //0
            //);
        









    }
    
}

ReplaceColorForm::ReplaceColorForm()
{
    PostQuitMsgWhenClosed(false);



}

ReplaceColorForm::~ReplaceColorForm()
{

}

std::wstring ReplaceColorForm::GetSkinFolder()
{
	return L"cspHelper_default";
}

std::wstring ReplaceColorForm::GetSkinFile()
{
	return L"ReplaceColorForm.xml";
}




//颜色替换因为需要吸取图层原本的颜色，所以只能使用插件点，不能直接屏幕取色

void ReplaceColorForm::FocusCheck()
{
    if (pthis)
    {

        if (::GetKeyState(VK_LBUTTON) & 0x80)
        {

        }
        else if (::GetKeyState(VK_SPACE) & 0x80)
        {

            pthis->__ReleaseCapture();
        }
        else
        {
            if (pthis->IsMouseOnPaintWindow())
            {
                pthis->SetCapture();
                pthis->SetSipCursor();
            }
        }
        ui::GlobalManager::Instance().Thread().PostDelayedTask(ui::kThreadUI, FocusCheck, 17);//每秒轮询60次
    }
}

void ReplaceColorForm::RenderLoop()
{
    if (!pthis)return;


    if (pthis->needDrawCanvas)
    {
        ReplaceColor::GetIns().Render(pthis->H, pthis->S, pthis->V, pthis->strength, pthis->colorTolerance, pthis->bPreview);
        pthis->needDrawCanvas = false;
    }

    if (pthis->needDrawSelectArea&& pthis->renderSubwWindow.GetNativeWindow())
    {
        pthis->renderSubwWindow.Begin();
        float value=0.5f*sinf( SDL_GetTicks()/1000.f)+0.5f;
        pthis->renderSubwWindow.Clear(value, value, value, value);


        pthis->renderSubwWindow.BindBackBuffer();

        ReplaceColor::GetIns().RenderSeleceArea();


        pthis->renderSubwWindow.End();
        pthis->needDrawSelectArea = false;
    }
    ui::GlobalManager::Instance().Thread().PostDelayedTask(ui::kThreadUI, RenderLoop, 33);

}


void ReplaceColorForm::OnInitWindow()
{
	__super::OnInitWindow();
    //this->SetCapture();
    //ui::GlobalManager::Instance().Cursor().SetImageCursor(this, ui::FilePath(L"Eyedropper.cur"));
   




    



    //吸管按钮

   
    option_dropper_normal= static_cast<ui::Option*>(FindControl(L"eyedropper_normal"));
    option_dropper_add = static_cast<ui::Option*>(FindControl(L"eyedropper_add"));
    option_dropper_sub = static_cast<ui::Option*>(FindControl(L"eyedropper_sub"));
    
    option_dropper_normal->AttachSelect(UiBind(&ReplaceColorForm::OnEyedropperOptionSelect, this, std::placeholders::_1));
    option_dropper_add->AttachSelect(UiBind(&ReplaceColorForm::OnEyedropperOptionSelect, this, std::placeholders::_1));
    option_dropper_sub->AttachSelect(UiBind(&ReplaceColorForm::OnEyedropperOptionSelect, this, std::placeholders::_1));
    


    colorBlock_target = static_cast<ui::Control*>(FindControl(L"color_selected"));
    colorBlock_result = static_cast<ui::Control*>(FindControl(L"color_result"));










    //滑块&edit
    slider_tolerance = static_cast<ui::Slider*>(FindControl(L"slider_tolerance"));
    slider_hue = static_cast<ui::Slider*>(FindControl(L"slider_hue"));
    slider_saturation = static_cast<ui::Slider*>(FindControl(L"slider_saturation"));
    slider_value = static_cast<ui::Slider*>(FindControl(L"slider_value"));
    slider_strength = static_cast<ui::Slider*>(FindControl(L"slider_strength"));

    edit_tolerance = static_cast<ui::RichEdit*>(FindControl(L"edit_tolerance"));
    edit_hue = static_cast<ui::RichEdit*>(FindControl(L"edit_hue"));
    edit_saturation = static_cast<ui::RichEdit*>(FindControl(L"edit_saturation"));
    edit_value = static_cast<ui::RichEdit*>(FindControl(L"edit_value"));
    edit_strength = static_cast<ui::RichEdit*>(FindControl(L"edit_strength"));

    slider_tolerance->SetValue(static_cast<double>(AppSettings::GetReplaceColorTolerance()*100));
    edit_tolerance->SetText(std::to_wstring(AppSettings::GetReplaceColorTolerance()));

    slider_tolerance->AttachValueChanged(UiBind(&ReplaceColorForm::OnSliderValueChange, this, std::placeholders::_1));
    slider_hue->AttachValueChanged(UiBind(&ReplaceColorForm::OnSliderValueChange, this, std::placeholders::_1));
    slider_saturation->AttachValueChanged(UiBind(&ReplaceColorForm::OnSliderValueChange, this, std::placeholders::_1));
    slider_value->AttachValueChanged(UiBind(&ReplaceColorForm::OnSliderValueChange, this, std::placeholders::_1));
    slider_strength->AttachValueChanged(UiBind(&ReplaceColorForm::OnSliderValueChange, this, std::placeholders::_1));

    edit_tolerance->AttachTextChanged(UiBind(&ReplaceColorForm::OnSliderValueTextChange, this, std::placeholders::_1));
    edit_hue->AttachTextChanged(UiBind(&ReplaceColorForm::OnSliderValueTextChange, this, std::placeholders::_1));
    edit_saturation->AttachTextChanged(UiBind(&ReplaceColorForm::OnSliderValueTextChange, this, std::placeholders::_1));
    edit_value->AttachTextChanged(UiBind(&ReplaceColorForm::OnSliderValueTextChange, this, std::placeholders::_1));
    edit_strength->AttachTextChanged(UiBind(&ReplaceColorForm::OnSliderValueTextChange, this, std::placeholders::_1));



    //预览
    ui::CheckBox* checkBox_preview= static_cast<ui::CheckBox*>(FindControl(L"checkBox_preview"));
    checkBox_preview->AttachSelect(UiBind(&ReplaceColorForm::OnButtonClick, this, std::placeholders::_1));
    checkBox_preview->AttachUnSelect(UiBind(&ReplaceColorForm::OnButtonClick, this, std::placeholders::_1));


    ui::Button* pBtn_Close = static_cast<ui::Button*>(FindControl(L"btn_Close"));
    if (pBtn_Close)
    {
        pBtn_Close->AttachClick(UiBind(&ReplaceColorForm::OnButtonClick, this, std::placeholders::_1));
    }
    ui::Button* pBtn_OK = static_cast<ui::Button*>(FindControl(L"btn_OK"));
    if (pBtn_OK)
    {
        pBtn_OK->AttachClick(UiBind(&ReplaceColorForm::OnButtonClick, this, std::placeholders::_1));
    }
    ui::Button* pBtn_Cancel = static_cast<ui::Button*>(FindControl(L"btn_Cancel"));
    if (pBtn_Cancel)
    {
        pBtn_Cancel->AttachClick(UiBind(&ReplaceColorForm::OnButtonClick, this, std::placeholders::_1));
    }







    //色相饱和度明度文本对齐
    ui::Label* label_hue= static_cast<ui::Label*>(FindControl(L"label_hue"));
    ui::Label* label_saturation = static_cast<ui::Label*>(FindControl(L"label_saturation"));
    ui::Label* label_value = static_cast<ui::Label*>(FindControl(L"label_value"));
    ui::Label* label_strength = static_cast<ui::Label*>(FindControl(L"label_strength"));

    int labelSizeMax = max(max(max(
        label_hue->EstimateText({ 9999,9999 }).cx,
        label_saturation->EstimateText({ 9999,9999 }).cx),
        label_value->EstimateText({ 9999,9999 }).cx),
        label_strength->EstimateText({ 9999,9999 }).cx
        );
    label_hue->SetFixedWidth( ui::UiFixedInt(labelSizeMax), false, false);
    label_saturation->SetFixedWidth( ui::UiFixedInt(labelSizeMax), false, false);
    label_value->SetFixedWidth( ui::UiFixedInt(labelSizeMax), false, false);
    label_strength->SetFixedWidth( ui::UiFixedInt(labelSizeMax), true, false);







    //初始化颜色 只初始化一次
    if(lastPickColor.GetA()==0)
        lastPickColor = ui::UiColor(255,255,255,255);



    pickingColor = lastPickColor;
    SDL_Color col;
    col.r = pthis->pickingColor.GetR();
    col.g = pthis->pickingColor.GetG();
    col.b = pthis->pickingColor.GetB();
    col.a = 255;
    ReplaceColor::GetIns().SetPickingColor(&col);
    ReplaceColor::GetIns().PickSuccess();
    pthis->needDrawCanvas = true;
    pthis->needDrawSelectArea = true;




    colorBlock_target->SetBkColor(lastPickColor);
    colorBlock_result->SetBkColor(lastPickColor);





    //子窗口相关
    ui::Control* pRenderWindowHost= static_cast<ui::Control*>(FindControl(L"ChildWindow"));
    pRenderWindowHost->AttachPosChanged(UiBind(&ReplaceColorForm::OnChildWindowControlPosSizeChanged, this, std::placeholders::_1));
    pRenderWindowHost->AttachSizeChanged(UiBind(&ReplaceColorForm::OnChildWindowControlPosSizeChanged, this, std::placeholders::_1));























    FocusCheck();
    RenderLoop();
}


bool ReplaceColorForm::OnButtonClick(const ui::EventArgs& args)
{
    DString controlName = args.GetSender()->GetName();
    if (controlName == L"btn_Close" || controlName == L"btn_Cancel")
    {
        this->CloseWnd();
        return true;
    }
    if (controlName == L"btn_OK")
    {
        isCloseWithOK = true;
        this->CloseWnd();
        return true;
    }
    if(controlName==L"checkBox_preview")
    {
        bPreview = ((ui::CheckBox*)(args.GetSender()))->IsSelected();
        needDrawCanvas = true;
    }
    return true;
}

bool ReplaceColorForm::OnEyedropperOptionSelect(const ui::EventArgs& args)
{
    //基本的样式设置
    ui::Option* pSender = (ui::Option*)args.GetSender();
    if (pSender == option_dropper_normal)
    {
        currentCursorType = SipCursorType_Normal;
    }
    else if (pSender == option_dropper_add)
    {
        currentCursorType = SipCursorType_Add;
    }
    else if (pSender == option_dropper_sub)
    {
        currentCursorType = SipCursorType_Sub;
    }


    return true;
}


bool ReplaceColorForm::OnSliderValueChange(const ui::EventArgs& args)
{
    ui::Slider* pslider = (ui::Slider*)args.GetSender();



    //为了增加流畅性，调色滑块总刻度数量是100* n
    int realValue= static_cast<int>( pslider->GetValue()/100.f);

    bool valueUpdated = false;
    if (pslider == slider_tolerance)
    {
        if (realValue != colorTolerance)
        {
            colorTolerance = realValue;
            edit_tolerance->SetTextNoEvent(std::to_wstring(realValue));
            //edit_tolerance->SetText(L"");

            needDrawCanvas = true;
            needDrawSelectArea = true;

            //valueUpdated = true;
        }
    }
    else if (pslider == slider_hue)
    {
        //std::cout<<"slider value:"<<slider_hue->GetValue()<<std::endl;
        if (realValue != H)
        {
            edit_hue->SetTextNoEvent(std::to_wstring(realValue));
            H = realValue;
            needDrawCanvas = true;
            valueUpdated = true;
        }
    }
    else if (pslider == slider_saturation)
    {
        if (realValue != S)
        {
            edit_saturation->SetTextNoEvent(std::to_wstring(realValue));
            S = realValue;
            needDrawCanvas = true;
            valueUpdated = true;
        }
    }
    else if (pslider == slider_value)
    {
        if (realValue != V)
        {
            edit_value->SetTextNoEvent(std::to_wstring(realValue));
            V = realValue;
            needDrawCanvas = true;
            valueUpdated = true;
        }

    }
    else if (pslider == slider_strength)
    {

        if (realValue != strength)
        {
            edit_strength->SetTextNoEvent(std::to_wstring(realValue));
            strength = realValue;
            needDrawCanvas = true;
            valueUpdated = true;
        }
    }

    if (valueUpdated)
    {

        SDL_Color color;
        color.r = lastPickColor.GetR();
        color.g = lastPickColor.GetG();
        color.b = lastPickColor.GetB();


        colorUtil::HSVFilter(&color, H, S, V, strength);

        ui::UiColor dstColor(color.r, color.g, color.b);
        colorBlock_result->SetBkColor(dstColor);
    }

	return true;
}

bool ReplaceColorForm::OnSliderValueTextChange(const ui::EventArgs& args)
{
    ui::RichEdit* curEdit = (ui::RichEdit*)args.GetSender();
    int64_t sliderValue = curEdit->GetTextNumber()*100;
    
    ui::EventArgs event;
    if (curEdit == edit_tolerance)
    {
        slider_tolerance->SetValue(static_cast<double>(sliderValue));

        event.SetSender(slider_tolerance);
    }
    else if (curEdit == edit_hue)
    {
        slider_hue->SetValue(static_cast<double>(sliderValue));
        event.SetSender(slider_hue);
    }
    else if (curEdit == edit_saturation)
    {
        slider_saturation->SetValue(static_cast<double>(sliderValue));
        event.SetSender(slider_saturation);
    }
    else if (curEdit == edit_value)
    {
        slider_value->SetValue(static_cast<double>(sliderValue));
        event.SetSender(slider_value);
    }
    else if (curEdit == edit_strength)
    {
        slider_strength->SetValue(static_cast<double>(sliderValue));
        event.SetSender(slider_strength);
    }

    //Slider::SetValue无法触发回调，这里手动调用
    if (event.GetSender())
    {
        OnSliderValueChange(event);
    }


    return true;
}




























static void SendKey(HWND hwnd, UINT vkCode)
{
    // 1. 获取扫描码
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);

    // 2. 构造 lParam
    // KeyDown: 1 (count) | scanCode << 16
    INT_PTR lParamDown = (INT_PTR)((1) | (scanCode << 16));

    // KeyUp: 1 (count) | scanCode << 16 | 1<<30 (prev) | 1<<31 (transition)
    INT_PTR lParamUp = (INT_PTR)((1) | (scanCode << 16) | (1 << 30) | (1 << 31));

    // 3. 发送
    PostMessage(hwnd, WM_KEYDOWN, (INT_PTR)vkCode, lParamDown);
    PostMessage(hwnd, WM_KEYUP, (INT_PTR)vkCode, lParamUp);

    //// 4. 如果是回车，补充 WM_CHAR
    //if (vkCode == VK_RETURN)
    //{
    //    PostMessage(hwnd, WM_CHAR, (IntPtr)'\r', lParamDown);
    //}
}







void ReplaceColorForm::OnCloseWindow()
{
    //关闭窗口的同时关闭旧窗口
    if (originFilterDlg)
    {
        //::ShowWindow((HWND)originFilterDlg, SW_SHOW);
        if (isCloseWithOK)
        {
            SendKey((HWND)originFilterDlg, VK_RETURN);
            
            //应用了图像则保存容差参数
            AppSettings::SetReplaceColorTolerance(colorTolerance);
        }
        else
            SendKey((HWND)originFilterDlg, VK_ESCAPE);


        //保存窗口位置信息
        SDL_Log("Start Save Replace Color Window Pos....");
        ui::UiRect curWindowRect= this->GetWindowPos(false);

        int displayCount = 0;
        SDL_DisplayID* displayids= SDL_GetDisplays(&displayCount);

        SDL_Rect saveRect;
        saveRect.x = curWindowRect.left;
        saveRect.y = curWindowRect.top;
        saveRect.w = curWindowRect.Width();
        saveRect.h = curWindowRect.Height();


        SDL_Log("Current Save Replace Color Window Pos:%d,%d,%d,%d", saveRect.x, saveRect.y, saveRect.w, saveRect.h);

        SDL_DisplayID targetDisplay = SDL_GetDisplayForRect(&saveRect);
        int displayIndex = 0;
        for (; displayIndex < displayCount; displayIndex++)
        {
            if (targetDisplay == displayids[displayIndex])
            {
                break;
            }
        }
        SDL_Rect targetDisplayBounds = {0,0,1920,1080};
        SDL_GetDisplayBounds(targetDisplay ,&targetDisplayBounds);


        //计算屏幕相关坐标
        saveRect.x -= targetDisplayBounds.x;
        saveRect.y -= targetDisplayBounds.y;
        std::string displayName=  SDL_GetDisplayName(targetDisplay);


        SDL_Log("Target Display Bounds:%d,%d,%d,%d", targetDisplayBounds.x, targetDisplayBounds.y, targetDisplayBounds.w, targetDisplayBounds.h);
        SDL_Log("Target Display:%s", displayName.c_str());
        SDL_Log("Save Result Rect:%d,%d,%d,%d", saveRect.x, saveRect.y, saveRect.w, saveRect.h);


        AppSettings::SetReplaceColorDisplayName(displayName);
        AppSettings::SetReplaceColorDisplayIndex(displayIndex);
        AppSettings::SetReplaceColorWindowRect(saveRect);
        SDL_free(displayids);
    }

    pthis = nullptr;
    isDataFilled = false;
}

LRESULT ReplaceColorForm::OnKeyDownMsg(ui::VirtualKeyCode vkCode, uint32_t modifierKey, const ui::NativeMsg& nativeMsg, bool& bHandled)
{





    //if (ui::kVK_A == vkCode )
    //{
    //    bHandled = true;
    //    return true;
    //}






    //空格移动
    if (ui::kVK_SPACE == vkCode &&!(LButtonDown&&(GetKeyState(VK_LBUTTON) & 0x80)))
    {

        //如果在外部
        //ui::UiRect windowPos = this->GetWindowPos(false);
        //ui::UiPoint cursorPoint;
        //this->GetCursorPos(cursorPoint);
        //if (cursorPoint.x<windowPos.left || cursorPoint.y<windowPos.top || cursorPoint.x>windowPos.right || cursorPoint.y>windowPos.bottom)
        //{
        //    this->__ReleaseCapture();
        //}

        this->__ReleaseCapture();
    }



    if (ui::kVK_RETURN == vkCode )
    {
        isCloseWithOK = true;
        this->CloseWnd();
        bHandled = true;
        return true;
    }
    if (ui::kVK_ESCAPE == vkCode)
    {
        this->CloseWnd();
        bHandled = true;
        return true;
    }


    return Window::OnKeyDownMsg(vkCode, modifierKey, nativeMsg, bHandled);

}

LRESULT ReplaceColorForm::OnKeyUpMsg(ui::VirtualKeyCode vkCode, uint32_t modifierKey, const ui::NativeMsg& nativeMsg, bool& bHandled)
{

    //空格移动
    //if (ui::kVK_SPACE == vkCode)
    //{
    //    ui::UiRect windowPos = this->GetWindowPos(false);
    //    ui::UiPoint cursorPoint;
    //    this->GetCursorPos(cursorPoint);



    //    if (cursorPoint.x<windowPos.left || cursorPoint.y<windowPos.top || cursorPoint.x>windowPos.right || cursorPoint.y>windowPos.bottom)
    //    {
    //        //鼠标在客户区外
    //        this->SetCapture();
    //        //ui::GlobalManager::Instance().Cursor().SetCursor(ui::CursorType::kCursorCross);
    //        ui::GlobalManager::Instance().Cursor().SetImageCursor(this, ui::FilePath(L"Eyedropper.cur"));
    //    }
    //    else
    //    {
    //        //在客户区内
    //        this->__ReleaseCapture();
    //        ui::GlobalManager::Instance().Cursor().SetCursor(ui::CursorType::kCursorArrow);

    //    }

    //}

    return Window::OnKeyUpMsg(vkCode, modifierKey, nativeMsg, bHandled);
}

LRESULT ReplaceColorForm::OnCaptureChangedMsg(const ui::NativeMsg& nativeMsg, bool& bHandled)
{
    //丢失
    if (isColorPicking)
    {

        //取消此次选色
        colorBlock_target->SetBkColor(lastPickColor);
        ReplaceColor::GetIns().PickCancel();
        needDrawCanvas=true;
        needDrawSelectArea = true;


        SDL_Color color;
        color.r = lastPickColor.GetR();
        color.g = lastPickColor.GetG();
        color.b = lastPickColor.GetB();


        colorUtil::HSVFilter(&color, H, S, V, strength);

        ui::UiColor dstColor(color.r, color.g, color.b);
        colorBlock_result->SetBkColor(dstColor);


        isColorPicking = false; 

    }
    std::cout << "OnCaptureChangedMsg." << std::endl;


    return Window::OnCaptureChangedMsg(nativeMsg, bHandled);
}

LRESULT ReplaceColorForm::OnMouseMoveMsg(const ui::UiPoint& pt, uint32_t modifierKey, bool bFromNC, const ui::NativeMsg& nativeMsg, bool& bHandled)
{
    if(!isDataFilled)return Window::OnMouseMoveMsg(pt, modifierKey, bFromNC, nativeMsg, bHandled);


    if (isColorPicking)
    {
        if (GetKeyState(VK_LBUTTON) & 0x80)
        {
            DoPickColor();
            return Window::OnMouseMoveMsg(pt, modifierKey, bFromNC, nativeMsg, bHandled);
        }
        else
        {
            this->__ReleaseCapture();
        }
    }

    
    if (LButtonDown&& GetKeyState(VK_LBUTTON) & 0x80)
    {
        //拉动滑块的时候也不应该进行仍和其他动作
        return Window::OnMouseMoveMsg(pt, modifierKey, bFromNC, nativeMsg, bHandled);
    }


    //鼠标移动的时候，检查鼠标所指向的窗口
    if (!IsMouseOnPaintWindow())
    {
        this->__ReleaseCapture();
        return Window::OnMouseMoveMsg(pt, modifierKey, bFromNC, nativeMsg, bHandled);
    }


    //鼠标在画布上
    this->SetCapture();
    SetSipCursor();




    return Window::OnMouseMoveMsg(pt, modifierKey, bFromNC, nativeMsg, bHandled);
}

LRESULT ReplaceColorForm::OnMouseLButtonDownMsg(const ui::UiPoint& pt, uint32_t modifierKey, const ui::NativeMsg& nativeMsg, bool& bHandled)
{
    LButtonDown = true;
    if (IsMouseOnPaintWindow()&&isDataFilled)
    {
        std::cout << "Pick Color Start." << std::endl;
        isColorPicking = true;


        if (GetKeyState(VK_SHIFT) & 0x80)
        {
            workingCursorType = SipCursorType_Add;
        }
        else if (GetKeyState(VK_MENU) & 0x80)
        {
            workingCursorType = SipCursorType_Sub;
        }
        else
        {
            workingCursorType = currentCursorType;
        }
        ReplaceColor::GetIns().StartPick((ReplaceColor::PickType)workingCursorType);


        DoPickColor();



    }
    return Window::OnMouseLButtonDownMsg(pt, modifierKey,nativeMsg, bHandled);
}

LRESULT ReplaceColorForm::OnMouseLButtonUpMsg(const ui::UiPoint& pt, uint32_t modifierKey, const ui::NativeMsg& nativeMsg, bool& bHandled)
{
    LButtonDown = false;
    if (IsMouseOnPaintWindow()&& isColorPicking)
    {
        //取色
        DoPickColor();
        ReplaceColor::GetIns().PickSuccess();
        lastPickColor = pickingColor;
    }
    isColorPicking = false;

    return Window::OnMouseLButtonUpMsg(pt, modifierKey,nativeMsg, bHandled);
}

bool ReplaceColorForm::OnChildWindowControlPosSizeChanged(const ui::EventArgs& args)
{
    ui::Control* pChildHost = args.GetSender();





    
    //ui::UiRect windowPos= this->GetWindowPos(true);
    //应用位置

    ui::UiRect childRealRect = CalcChildWindowPos(pChildHost);


    if (renderSubwWindow.GetNativeWindow())
    {

        ::MoveWindow(renderSubwWindow.GetNativeWindow(),
            childRealRect.left, childRealRect.top,
            childRealRect.right - childRealRect.left, childRealRect.bottom - childRealRect.top, true);

        needDrawSelectArea = true;
    }
    else
    {
        if (renderSubwWindow.Create((HWND)this->GetWindowHandle(), childRealRect.left, childRealRect.top,
            childRealRect.right - childRealRect.left, childRealRect.bottom - childRealRect.top))
        {

            //auto canvasSize=ReplaceColor::GetIns().GetSelectAreaRect();
            //renderSubwWindow.InitDX(canvasSize.right-canvasSize.left, canvasSize.bottom-canvasSize.top);
            renderSubwWindow.InitDX();


            //needDrawSelectArea = true;




        }

    }

    return true;
}

ui::UiRect ReplaceColorForm::CalcChildWindowPos(ui::Control* pChildHost)
{
    auto& canvasRect = ReplaceColor::GetIns().GetSelectAreaRect();
    int canvasW = canvasRect.right - canvasRect.left;
    int canvasH = canvasRect.bottom - canvasRect.top;


    //pChildHost->GetPos
    //ui::UiRect childHostRect = pChildHost->GetRect();
    ui::UiRect childHostRect = pChildHost->GetPos();
    //计算实际位置
    ui::UiRect childRealRect;
    if (canvasW * childHostRect.Height() > childHostRect.Width() * canvasH)
    {

        //当画布比控件更扁平的情况下, 水平方向拉满
        int realW = childHostRect.Width();
        int realH = childHostRect.Width() * canvasH / canvasW;

        childRealRect.left = childHostRect.left;
        childRealRect.right = childHostRect.right;
        childRealRect.top = childHostRect.top + ((childHostRect.Height() - realH) / 2);
        childRealRect.bottom = childRealRect.top + realH;
    }
    else
    {
        //当画布比控件更瘦高的情况下, 垂直方向拉满
        int realW = childHostRect.Height() * canvasW / canvasH;
        int realH = childHostRect.Height();


        childRealRect.left = childHostRect.left + ((childHostRect.Width() - realW) / 2);
        childRealRect.right = childRealRect.left + realW;
        childRealRect.top = childHostRect.top;
        childRealRect.bottom = childHostRect.bottom;
    }

    return childRealRect;
}



void ReplaceColorForm::__SetCapture()
{

    if(isDataFilled)
		this->SetCapture();

}

void ReplaceColorForm::__ReleaseCapture()
{
    LButtonDown = false;
    this->ReleaseCapture();
}

void ReplaceColorForm::SetSipCursor()
{
    SipCursorType cursorType= SipCursorType_Normal;
    if (isColorPicking)
    {
        cursorType = workingCursorType;
    }
    else
    {
        if (GetKeyState(VK_SHIFT) & 0x80)
        {
            cursorType = SipCursorType_Add;
        }
        else if (GetKeyState(VK_MENU) & 0x80)
        {
            cursorType = SipCursorType_Sub;
        }
        else
        {
            cursorType = currentCursorType;
        }
    }


    switch (cursorType)
    {
    case ReplaceColorForm::SipCursorType_Normal:
        ui::GlobalManager::Instance().Cursor().SetImageCursor(this, ui::FilePath(L"Eyedropper.cur"));
        break;
    case ReplaceColorForm::SipCursorType_Add:
        ui::GlobalManager::Instance().Cursor().SetImageCursor(this, ui::FilePath(L"Eyedropper_Add.cur"));
        break;
    case ReplaceColorForm::SipCursorType_Sub:
        ui::GlobalManager::Instance().Cursor().SetImageCursor(this, ui::FilePath(L"Eyedropper_Sub.cur"));
        break;
    default:
        break;
    }

}

void ReplaceColorForm::DoPickColor()
{
    double x, y;
    GetCurrentPosInCanvas(&x, &y);
    SDL_Color pickedColor = ReplaceColor::GetIns().SetPickingColor(x, y);

    ui::UiColor srcColor(pickedColor.r, pickedColor.g, pickedColor.b);
    colorBlock_target->SetBkColor(srcColor);

    colorUtil::HSVFilter(&pickedColor,H,S,V, strength);
    ui::UiColor dstColor(pickedColor.r, pickedColor.g, pickedColor.b);
    colorBlock_result->SetBkColor(dstColor);

    pickingColor.SetARGB(ui::UiColor::MakeARGB(255,pickedColor.r, pickedColor.g, pickedColor.b));

    needDrawCanvas = true;
    needDrawSelectArea = true;
}




