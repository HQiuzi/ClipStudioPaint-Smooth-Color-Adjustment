#ifndef LCH_COLOR_CONTROL_PRIMARY_H_
#define LCH_COLOR_CONTROL_PRIMARY_H_

#include "duilib/Core/Control.h"
#include "duilib/Render/IRender.h"


#include "Graphic/Texture.h"
    /** 自定义颜色控件
    */

    //struct Color96
    //{
    //    uint32_t R=0;
    //    uint32_t G=0;
    //    uint32_t B=0;
    //};


    class LchColorControl_Primary : public ui::Control
    {
        typedef  ui::Control BaseClass;
    public:
        enum WheelType
        {
            WheelType_Triangle,
            WheelType_Rectangle
        };



        explicit LchColorControl_Primary(ui::Window* pWindow);

        /** 获取控件类型
        */
        virtual DString GetType() const override;

        /** 选择颜色
        */
        void SelectColor(const  ui::UiColor& selColor);

        /** 监听选择颜色的事件
        * @param [in] callback 选择颜色变化时的回调函数
        * @param [in] callbackID 该回调函数对应的ID（用于删除回调函数）
        *            参数说明:
        *                   wParam: 当前新选择的颜色值，可以用UiColor((uint32_t)wParam)生成颜色
        *                   lParam: 原来旧选择的颜色值，可以用UiColor((uint32_t)lParam)生成颜色
        */
        void AttachSelectColor(const  ui::EventCallback& callback) { AttachEvent(ui::kEventSelectColor, callback); }


        void GetCurrentColor96(struct Color96* pColor);




        //void SetL(float LCH_L);
        //void SetC(float LCH_L);
        //void SetH(float LCH_L); 
        void SetLCHrgb(float LCH_L, float LCH_C, float LCH_H);
        void SetWheelType(WheelType _wheelType);

        float GetL() { return L; };
        float GetC() { return c_percent; };
        float GetH() { return h_rgb; };
        WheelType GetWheelType() { return wheelType; };

    protected:
        /** 绘制背景图片的入口函数
        * @param[in] pRender 指定绘制区域
        */
        virtual void PaintBkImage(ui::IRender* pRender) override;

        /** 鼠标左键按下
        */
        virtual bool ButtonDown(const  ui::EventArgs& msg) override;

        /** 鼠标移动
        */
        virtual bool MouseMove(const  ui::EventArgs& msg) override;
        virtual bool MouseLeave(const  ui::EventArgs& msg) override;

        /** 鼠标左键弹起
        */
        virtual bool ButtonUp(const  ui::EventArgs& msg) override;

        virtual bool OnCaptureChanged(const  ui::EventArgs& msg)override;
    private:
        /** 获取绘制的颜色位图接口
        * @param [in] rect 显示区域大小信息
        */
        ui::IBitmap* GetColorBitmap(const  ui::UiRect& rect);
        //ui::IBitmap* GetHueCircleBitmap();

        /** 选择位置发生变化
        */
        void OnSelectPosChanged(const  ui::UiPoint& pt);
        void OnHuePosChanged(const  ui::UiPoint& pt);

        /** 设置鼠标捕获
        */
        void SetMouseCapture(bool bCapture);


        bool isMouseInWheel(const ui::UiPoint& cursorPos);

        bool OnSizeChanged(const  ui::EventArgs& msg);

        void _RePaintWheel();
    private:
        /** 颜色位图
        */
        std::unique_ptr< ui::IBitmap> m_spBitmap;
        //std::unique_ptr< ui::IBitmap> m_spBitmap_HueCircle;//色相环
        bool needRepaint = true;
        bool needResetPos = true;
        float L=50.F;//0~100;
		float c_percent = 0.f;//0~100;
		float h_rgb = 0.f;//0~360;

        enum WheelType wheelType= WheelType_Rectangle;


        /** 鼠标点击位置
        */
        ui::UiPoint m_lastPt;
        ui::UiRect rcHueCircle;
        ui::UiRect rcColorPan;


        RenderTexture colWheelTex;

        /** 鼠标是否按下
        */
        //bool m_bMouseDown;
        bool m_bMouseHueCircle;
        bool m_bMouseColorPan;
    };


#endif //UI_CONTROL_COLOR_CONTROL_H_