#pragma once
#ifndef ColWheelWnd_H
#define ColWheelWnd_H


#include"Graphic/RenderSubWindow.h"
#include"DUI/DuiCommon.h"
#include"CspData.h"


//为了将LCH(基于LAB)在极坐标空间中完整无冗余和缺损地展现，这里魔改了LCH系统，与标准的Lab不对等
//视觉色轮窗口
class ColWheelWnd : public ui::WindowImplBase
{
	static inline ColWheelWnd* pthis = nullptr;
public:
	static void ShowForm(bool needInitColor = true);
	static void HideForm();
	static ColWheelWnd* GetCurrentForm() { return pthis; };



	ColWheelWnd();
	virtual ~ColWheelWnd();

	/**
	 * 一下三个接口是必须要覆写的接口，父类会调用这三个接口来构建窗口
	 * GetSkinFolder		接口设置你要绘制的窗口皮肤资源路径
	 * GetSkinFile			接口设置你要绘制的窗口的 xml 描述文件
	 * GetWindowClassName	接口设置窗口唯一的类名称
	 */
	virtual std::wstring GetSkinFolder() override;
	virtual std::wstring GetSkinFile() override;



	/**
	 * 收到 WM_CREATE 消息时该函数会被调用，通常做一些控件初始化的操作
	 */
	virtual void OnInitWindow() override;
	virtual void OnPreCloseWindow()override;
	void SavePos();

	virtual ui::Control* CreateControl(const DString& strClass) override;



	void SetColor(const struct Color96* col96);
	void SetColor(float r,float g,float b);
	void SetColorType(CspColorTable::PaintColorType type);
	void SetTypeColor(CspColorTable::PaintColorType type, const struct Color96* col96);

	bool IsSyncVisibility() { return syncVisibility; };


private:
	bool OnButtonClick(const ui::EventArgs& args);
	bool OnSliderValueChange(const ui::EventArgs& args);
	bool OnSliderValueTextChange(const ui::EventArgs& args);

	bool OnColorSelect(const ui::EventArgs& args);

	static void setWindowMinSize();
	static bool sizeChangeFunc(const ui::EventArgs& msg);




	//监听Capture丢失的信息
	virtual LRESULT OnCaptureChangedMsg(const ui::NativeMsg& nativeMsg, bool& bHandled) override;
	//监听大小位置改变
	virtual LRESULT OnSizeMsg(ui::WindowSizeType sizeType, const ui::UiSize& newWindowSize, const ui::NativeMsg& nativeMsg, bool& bHandled) override;
	virtual LRESULT OnMoveMsg(const ui::UiPoint& ptTopLeft, const ui::NativeMsg& /*nativeMsg*/, bool& bHandled)override;
	
	//监听窗口关闭的信息

	//virtual LRESULT OnWindowCloseMsg(uint32_t wParam, const ui::NativeMsg& nativeMsg, bool& bHandled)override;



	bool _needInitColor = true;;
	bool syncVisibility = true;
	bool isSetColSelf = false;//增加状态防止主动设置CSP颜色时又反馈进DUI里

	inline static SDL_Rect saveRect = {};
	//参数
	float LCH_L=50.F;
	float LCH_C=50.F;
	float LCH_H=0.F;//实际为RGB色相 用的时候需要转换
	ui::Slider* slider_vlch_L;
	ui::Slider* slider_vlch_C;
	ui::Slider* slider_vlch_H;
	ui::RichEdit* edit_vlch_L;
	ui::RichEdit* edit_vlch_C;
	ui::RichEdit* edit_vlch_H;
	class LchColorControl_Primary* colorControl_Primary;
	class LchColorControl* colorControl;
	class BrushColorBlock* colorBlock=nullptr;
	ui::Box* sliderContainer = nullptr;
	ui::Box* colorControlContainer = nullptr;
};





#endif
