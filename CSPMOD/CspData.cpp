#include"CSPMOD.h"
#include "CspData.h"
#include"AddressTable.h"
#include"duilib/duilib.h"
#include"DUI/CatDuiThread.h"
#include"DUI/Dialog/ColWheelWnd.h"

#include"CSPHelper.h"


void CspData::Init()
{
	curCanvasSizeBaseAddr = AddressTable::GetAddress("CSPInfo_CanvasSize");
	nativeWindowBaseAddr = AddressTable::GetAddress("CSPInfo_NativeWindowHandle");
	 


	//onSetPanelsShowStates= (void*)0X000000141DF2630;
	onSetPanelsShowStates= AddressTable::GetAddress("CSPInfo_OnSetPanelShowStates_Func");

	navigatorRectPosConvertCodeAddr = AddressTable::GetAddress("CSPNavigator_GetRectPos_CodeAddr");


	if (onSetPanelsShowStates)
	{
		CSPMOD::Hook(onSetPanelsShowStates, Hook_OnSetPanelsShowStates, (void**)&orig_OnSetPanelsShowStates);
	}



	CspColorTable::Init();

}

bool CspData::GetCurrentProjCanvasSize(uint32_t* sizeW, uint32_t* sizeH)
{


	if (!curCanvasSizeBaseAddr)return false;
	uintptr_t pSize= *(uintptr_t*)(curCanvasSizeBaseAddr);
	if (!CSPMOD::IsPtrValid(pSize + 0x290))return false;
	pSize= *(uintptr_t*)(pSize + 0x290);
	if (!CSPMOD::IsPtrValid(pSize + 0x20))return false;
	pSize= *(uintptr_t*)(pSize + 0x20);
	if (!CSPMOD::IsPtrValid(pSize + 0x38))return false;
	pSize= *(uintptr_t*)(pSize + 0x38);

	pSize = pSize + 0x2B8;
	if (!CSPMOD::IsPtrValid(pSize))return false;

	*sizeW = *(uint32_t*)pSize;
	*sizeH = *(uint32_t*)(pSize+4);
	return true;
}

void CspData::SetStrData(const char* key, const char* value)
{
	strdata[key] = value;
}

const char* CspData::GetStrData(const char* key, const char* _default)
{
	auto it = strdata.find(key);
	if (it != strdata.end())
	{
		return it->second.c_str();
	}
	return _default;
}

uintptr_t CspData::GetNativeWindowHandle()
{
	if (CSPMOD::IsPtrValid(nativeWindowBaseAddr))
	{
		return *(uintptr_t*)nativeWindowBaseAddr;
	}
	return 0;
}

void CspData::SetHideNavigatorViewIndicatorEnabled(bool b)
{
	if (navigatorRectPosConvertCodeAddr)
	{
		static uint8_t origPatchData[7+ 5+10+5    +5+10+5    +5+10+4+   5];
		static bool  origPatchDataFilled = false;


		if (!origPatchDataFilled)
		{
			memcpy(origPatchData, navigatorRectPosConvertCodeAddr, sizeof(origPatchData));
			origPatchDataFilled = true;
		}


		if (b)
		{
			uint8_t newCode[sizeof(origPatchData)];
			memcpy(newCode, origPatchData,sizeof(origPatchData));
			//目标代码中含有几个函数调用，它们的返回值是导航器红框坐标点
			//将E8 xx xx xx xx（CALL *****）改为 mov rax,0xFFFFFFFFFFFFFFFF以使红框坐标均为(-1,-1)，这样就不会绘制红框了
			//见AddressGenerator_CSPData.cpp
			uint8_t movRaxF[7] = {0x48,0xC7,0xC0, 0xFF,0xFF,0xFF,0xFF };
			uint8_t replaceCall[5] = {0x90,0x90,0x90, 0x90,0x90};
			memcpy(newCode, movRaxF, sizeof(movRaxF));

			memcpy(newCode + 7, replaceCall, sizeof(replaceCall));
			memcpy(newCode + 7 +5+10+5, replaceCall, sizeof(replaceCall));
			memcpy(newCode + 7 +5+10+5 +5+10+5, replaceCall, sizeof(replaceCall));
			memcpy(newCode + 7 +5+10+5 +5+10+5 +5+10+4, replaceCall, sizeof(replaceCall));


			CSPMOD::CodePatch(navigatorRectPosConvertCodeAddr, newCode, sizeof(newCode));

		}
		else
		{
			CSPMOD::CodePatch(navigatorRectPosConvertCodeAddr, origPatchData, sizeof(origPatchData));



		}






	}
}

int64_t CspData::Hook_OnSetPanelsShowStates(const void* object, uintptr_t showStates)
{	
	//这个可以作为启动的标识？
	//草为什么回看代码这hook的目标函数只有1个入参？

	static bool mainWindowStart = false;
	if (!mainWindowStart)
	{
		ui::GlobalManager::Instance().Thread().PostTask(ui::kThreadUI, [] {CSPMOD::OnMainWindowStart(); });
		mainWindowStart = true;
	}
	bool showPanels = !bool(showStates & 0x2);
	if (showPanels)
	{
		if (ColWheelWnd::GetCurrentForm()&& ColWheelWnd::GetCurrentForm()->IsSyncVisibility())
			ColWheelWnd::GetCurrentForm()->ShowWindow(ui::ShowWindowCommands::kSW_SHOW_NA);
	}
	else
	{
		if (ColWheelWnd::GetCurrentForm() && ColWheelWnd::GetCurrentForm()->IsSyncVisibility())
			ColWheelWnd::GetCurrentForm()->ShowWindow(ui::ShowWindowCommands::kSW_HIDE);
	}
	return orig_OnSetPanelsShowStates(object, showStates);
}












static ui::UiColor(*orig_ColorManager_ConvertToUiColor)(const DString& colorName) = nullptr;
static ui::UiColor Hook_ColorManager_ConvertToUiColor(const DString& colorName)
{
	if ((colorName[0] == L'#'&& colorName[1]==L'C')|| colorName[0] == L'C')
	{
		std::wstring colorStr;
		if (colorName[0] == L'#')
		{
			colorStr = &colorName[1];
		}
		else
		{
			colorStr = colorName;
		}

		if (colorStr == L"CSPColor_CaptionNoFocus")
		{
			return ui::UiColor(CspColorTable::GetCaptionContentColorNoFocus());
		}
		if (colorStr == L"CSPColor_CaptionFocus")
		{
			return ui::UiColor(CspColorTable::GetColorCaptionFocus());
		}
		if (colorStr == L"CSPColor_PanelBG")
		{
			return ui::UiColor(CspColorTable::GetColorPanelBG());
		}
		if (colorStr == L"CSPColor_ControlBG")
		{
			return ui::UiColor(CspColorTable::GetColorControlBG());
		}
		if (colorStr == L"CSPColor_Content")
		{
			return ui::UiColor(CspColorTable::GetContentColor());
		}
		if (colorStr == L"CSPColor_Separator")
		{
			return ui::UiColor(CspColorTable::GetSeparatorColor());
		}
		if (colorStr == L"CSPColor_CaptionContent")
		{
			return ui::UiColor(CspColorTable::GetCaptionContentColor());
		}
		if (colorStr == L"CSPColor_CaptionContentNoFocus")
		{
			return ui::UiColor(CspColorTable::GetCaptionContentColorNoFocus());
		}


		if (colorStr == L"CSPColor_Selected")
		{
			return ui::UiColor(CspColorTable::GetSelectedColor());
		}
		if (colorStr == L"CSPColor_SelectedHot")
		{
			return ui::UiColor(CspColorTable::GetSelectedHotColor());
		}

		if (colorStr == L"CSPColor_BlueButton")
		{
			return ui::UiColor(CspColorTable::GetBlueButtonColor());
		}
		if (colorStr == L"CSPColor_BlueButtonHot")
		{
			return ui::UiColor(CspColorTable::GetBlueButtonHotColor());
		}
		if (colorStr == L"CSPColor_BlueButtonPush")
		{
			return ui::UiColor(CspColorTable::GetBlueButtonPushColor());
		}

		if (colorStr == L"CSPColor_ButtonPush")
		{
			return ui::UiColor(CspColorTable::GetButtonPushColor());
		}
	}
	//兼容SVG
	else if (colorName == L"currentColor")
		{
			return ui::UiColor(255, 0, 0, 0);
		}
	else if (colorName == L"null")
		{
			return ui::UiColor(0, 0, 0, 0);
		}
	else if (colorName == L"none")
		{
			return ui::UiColor(0, 0, 0, 0);
		}
	return orig_ColorManager_ConvertToUiColor(colorName);
}


void CspColorTable::Init()
{

	colorTableBaseAttr = AddressTable::GetAddress("CSPInfo_UIColorTable");
	setCurrentPainColorFuncAttr = AddressTable::GetAddress("CSPPaintColor_SetColor_Func");//这个用于设置颜色
	//setCurrentPainColorFuncAttr = (void*)0x141c83b50;//这个用于设置颜色


	getTargetColorTypeInfoFuncAttr= AddressTable::GetAddress("CSPPaintColor_GetColorHost_Func");
	setTargetColorTypeFuncAttr= AddressTable::GetAddress("CSPPaintColor_SetWorkingColor_Func");
	releaseTargetColorTypeInfoFuncAttr= AddressTable::GetAddress("CSPPaintColor_ReleaseColorHost_Func");

	setPaintColorFuncAttr = AddressTable::GetAddress("CSPPaintColor_OnColorChanged_Func");//这个用于监听颜色变化



	//无论如何进行一个内部hook
	CSPMOD::Hook((void*)(ui::ColorManager::ConvertToUiColor), Hook_ColorManager_ConvertToUiColor, (void**)&orig_ColorManager_ConvertToUiColor);
	
	
	
	//CSPMOD::Hook(setCurrentPainColorFuncAttr, Hook_SetCurrentPaintColor, (void**)&orig_SetCurrentPaintColor);
	if (CSPHelper::isProUser())
	{
		if (setCurrentPainColorFuncAttr)
			CSPMOD::Hook(setTargetColorTypeFuncAttr, Hook_SetTargetColorType, (void**)&orig_SetTargetColorType);

		if (setPaintColorFuncAttr)
		CSPMOD::Hook(setPaintColorFuncAttr, Hook_SetPaintColor, (void**)&orig_SetPaintColor);
	}

}







CspColorTable::Theme CspColorTable::GetTheme()
{
	auto color = GetContentColor()&0xFF;
	if (color > 0x88)
		return Theme::Dark;
	else
		return Theme::Light;
}





//void CspColorTable::SetCurrentPaintColor(const Color96& col)
//{
//	typedef void(*SetColorFunc)(const Color96* colAddr);
//	SetColorFunc  setColorFunc= (SetColorFunc)(0x141c83b50);
//	setColorFunc(&col);
//}

//测试60ms间隔设置
static bool needSetColor = false;
static Color96 targetCol;
static uint64_t lastSetTimeMS = 0;
static bool hasTaskRunning = false;
void CspColorTable::_Task_SetCurrentPaintColor()
{
	if (SDL_GetTicks() - lastSetTimeMS > 16)
	{
		lastSetTimeMS = SDL_GetTicks();

		typedef void(*SetColorFunc)(const Color96* colAddr);
		SetColorFunc  setColorFunc = (SetColorFunc)(setCurrentPainColorFuncAttr);
		setColorFunc(&targetCol);



		needSetColor = false;
		hasTaskRunning = false;
		return;
	}


	ui::GlobalManager::Instance().Thread().PostDelayedTask(ui::kThreadUI, _Task_SetCurrentPaintColor,16 );
}


void CspColorTable::SetCurrentPaintColor(const Color96& col)
{

	//if (SDL_GetTicks() - lastSetTimeMS > 16)
	{
		lastSetTimeMS = SDL_GetTicks();
		typedef void(*SetColorFunc)(const Color96* colAddr);
		SetColorFunc  setColorFunc = (SetColorFunc)(setCurrentPainColorFuncAttr);
		setColorFunc(&col);

		//CspColorTable::orig_SetCurrentPaintColor(&col);
		needSetColor = false;
		return;
	}
	

	targetCol = col;
	needSetColor = true;

	if (!hasTaskRunning)
	{
		hasTaskRunning = true;
		_Task_SetCurrentPaintColor();
	}


}

void CspColorTable::GetMainPaintColor(Color96* col)
{
	typedef uintptr_t(*FI2)(uintptr_t arg1, uintptr_t arg2);

	uint8_t infoBuffer[0X60];
	((FI2)getTargetColorTypeInfoFuncAttr)((uintptr_t)infoBuffer, Main);
	SDL_memcpy(col, infoBuffer+0x10,sizeof(Color96));
	((FI2)releaseTargetColorTypeInfoFuncAttr)((uintptr_t)infoBuffer, 0);
}

void CspColorTable::GetSubPaintColor(Color96* col)
{
	typedef uintptr_t(*FI2)(uintptr_t arg1, uintptr_t arg2);

	uint8_t infoBuffer[0X60];
	((FI2)getTargetColorTypeInfoFuncAttr)((uintptr_t)infoBuffer, Sub);
	SDL_memcpy(col, infoBuffer+0x10,sizeof(Color96));
	((FI2)releaseTargetColorTypeInfoFuncAttr)((uintptr_t)infoBuffer, 0);
}














void CspColorTable::SetPaintColorType(PaintColorType type)
{
	if (getTargetColorTypeInfoFuncAttr && orig_SetTargetColorType && releaseTargetColorTypeInfoFuncAttr)
	{

		if (type < PaintColorType::Transparent)
		{
			typedef uintptr_t(*FI2)(uintptr_t arg1, uintptr_t arg2);

			uint8_t infoBuffer[0X60];
			((FI2)getTargetColorTypeInfoFuncAttr)((uintptr_t)infoBuffer, type);
			//((FI2)setTargetColorTypeFuncAttr)(type,(uintptr_t)infoBuffer );
			//((FI2)orig_SetTargetColorType)(type,(uintptr_t)infoBuffer );
			orig_SetTargetColorType(type, infoBuffer);
			((FI2)releaseTargetColorTypeInfoFuncAttr)((uintptr_t)infoBuffer,0);

		}



	}


}

uintptr_t CspColorTable::GetColorTableAddr()
{
	//0x144f62860 + 8 +0

	if(!CSPMOD::IsPtrValid(colorTableBaseAttr))return 0;

	uintptr_t addr = *(uintptr_t*)(colorTableBaseAttr);
	if (!CSPMOD::IsPtrValid(addr + 0x8))return 0;
	addr = *(uintptr_t*)(addr + 0x8);
	if (!CSPMOD::IsPtrValid(addr))return 0;
	return addr;
}



//DUI int 颜色 int32 ARGB //字节序为BGRA
uint32_t CspColorTable::GetColorCaptionNoFocus()
{
	
	return _GetColorAt(0X288,0xFF474747);
}
uint32_t CspColorTable::GetColorCaptionFocus()
{
	return _GetColorAt(0X24,0XFF383838);
}

uint32_t CspColorTable::GetColorPanelBG()
{
	return _GetColorAt(0x48,0XFF4E4E4E);
}
uint32_t CspColorTable::GetColorControlBG()
{
	return _GetColorAt(0xF0, 0XFF676767);
}

uint32_t CspColorTable::GetContentColor()
{
	return _GetColorAt(0x120,0XFFDADADA);
}
uint32_t CspColorTable::GetSeparatorColor()
{
	return _GetColorAt(0x264,0XFF727272);
}
uint32_t CspColorTable::GetCaptionContentColor()
{
	return _GetColorAt(0x18,0XFFA1A1A1);
}

uint32_t CspColorTable::GetCaptionContentColorNoFocus()
{
	return _GetColorAt(0x280,0XFF626262);
}



uint32_t CspColorTable::GetSelectedColor()
{
	return 0x3073B2E6;//找不到，手动选个相近的颜色
}
uint32_t CspColorTable::GetSelectedHotColor()
{
	return 0xE0C7DFFA;
}

uint32_t CspColorTable::GetBlueButtonColor()
{
	return _GetColor96At(0x21C, 0xE073B2E6);
}
uint32_t CspColorTable::GetBlueButtonHotColor()
{

	return _GetColor96At(0x234, 0xE0A1C7E6);
}
uint32_t CspColorTable::GetBlueButtonPushColor()
{
	return _GetColor96At(0x228, 0x300036A2);
}
uint32_t CspColorTable::GetButtonPushColor()
{
	return _GetColor96At(0x174, 0x800036A2);
}

uint32_t CspColorTable::_GetColorAt(uintptr_t offset, uint32_t fullback)
{
	uint32_t resultColor = 0xFFFFFFFF;
	uintptr_t pColor = GetColorTableAddr();
	if (0 == pColor|| !CSPMOD::IsPtrValid(pColor+offset))
	{
		return fullback;
	}

	pColor += offset;
	memcpy(&resultColor, (void*)pColor, 3);
	return resultColor;
}

uint32_t CspColorTable::_GetColor96At(uintptr_t offset, uint32_t fullback)
{
	uint32_t resultColor = 0xFFFFFFFF;
	uintptr_t pColor = GetColorTableAddr();
	if (0 == pColor || !CSPMOD::IsPtrValid(pColor + offset))
	{
		return fullback;
	}

	pColor += offset;
	uint8_t* pResultByte = (uint8_t*) & resultColor;
	pResultByte[2] = *(uint8_t*)(pColor + 3);//一个通道就占用32位，转为8位通道差不多等效于直接取最高8位
	pResultByte[1] = *(uint8_t*)(pColor + 3+4);
	pResultByte[0] = *(uint8_t*)(pColor + 3+4+4);
	return resultColor;
}

//int64_t CspColorTable::Hook_SetCurrentPaintColor(const Color96* arg0)
//{
//	if (ColWheelWnd::GetCurrentForm())
//	{
//		ColWheelWnd::GetCurrentForm()->SetColor((Color96*)arg0);
//	}
//	return orig_SetCurrentPaintColor(arg0);
//}

int64_t CspColorTable::Hook_SetTargetColorType(uintptr_t type, const void* infoBuffer)
{

	auto retult = orig_SetTargetColorType(type, infoBuffer);
	if (ColWheelWnd::GetCurrentForm())
	{
		{
			//原版色盘上取色后这里得到的颜色为0 
			//Color96 col;
			//typedef uintptr_t(*FI2)(uintptr_t arg1, uintptr_t arg2);
			//uint8_t _infoBuffer[0X60];
			//((FI2)getTargetColorTypeInfoFuncAttr)((uintptr_t)_infoBuffer, type);
			//SDL_memcpy(&col, _infoBuffer + 0x10, sizeof(Color96));
			//((FI2)releaseTargetColorTypeInfoFuncAttr)((uintptr_t)_infoBuffer, 0);

			ColWheelWnd::GetCurrentForm()->SetColorType((PaintColorType)type);
		}

	}
	return retult;
}

int64_t CspColorTable::Hook_SetPaintColor(void* unknown, uintptr_t type, Color96* col)
{



	auto retult = orig_SetPaintColor(unknown, type, col);
	if (ColWheelWnd::GetCurrentForm())
	{
		ColWheelWnd::GetCurrentForm()->SetTypeColor((PaintColorType)type, col);
	}
	return retult;
}
