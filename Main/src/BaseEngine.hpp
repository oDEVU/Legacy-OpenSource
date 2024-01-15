/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

#include <chrono>
#include <vector>

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "RenderStateNotation/interface/RenderStateNotationLoader.h"
#include "Common/interface/BasicMath.hpp"

#include "ThirdParty/imgui/imgui.h"
#include "Imgui/interface/ImGuiImplDiligent.hpp"
#include "Imgui/interface/ImGuiUtils.hpp"

#include "ImGuiImplGLFW.hpp"

#include "GLFW/glfw3.h"

namespace Diligent
{


    enum class Key
    {
        Esc             = GLFW_KEY_ESCAPE,
        Space           = GLFW_KEY_SPACE,
        Tab             = GLFW_KEY_TAB,
        RightShift      = GLFW_KEY_RIGHT_SHIFT,
        LeftShift       = GLFW_KEY_LEFT_SHIFT,
        F3              = GLFW_KEY_F3,

        W       = GLFW_KEY_W,
        A       = GLFW_KEY_A,
        S       = GLFW_KEY_S,
        D       = GLFW_KEY_D,

        // arrows
        Left    = GLFW_KEY_LEFT,
        Right   = GLFW_KEY_RIGHT,
        Up      = GLFW_KEY_UP,
        Down    = GLFW_KEY_DOWN,

        // numpad arrows
        NP_Left  = GLFW_KEY_KP_4,
        NP_Right = GLFW_KEY_KP_6,
        NP_Up    = GLFW_KEY_KP_8,
        NP_Down  = GLFW_KEY_KP_2,

        // mouse buttons
        MB_Left   = GLFW_MOUSE_BUTTON_LEFT,
        MB_Right  = GLFW_MOUSE_BUTTON_RIGHT,
        MB_Middle = GLFW_MOUSE_BUTTON_MIDDLE,
    };
    enum class KeyState
    {
        Release = GLFW_RELEASE,
        Press   = GLFW_PRESS,
        Repeat  = GLFW_REPEAT,
    };

class BaseEngine
{
public:
    BaseEngine();
    virtual ~BaseEngine();

    //
    // Public API
    //

    IEngineFactory* GetEngineFactory() { return m_pDevice->GetEngineFactory(); }
    IRenderDevice*  GetDevice() { return m_pDevice; }
    IDeviceContext* GetContext() { return m_pImmediateContext; }
    ISwapChain*     GetSwapChain() { return m_pSwapChain; }
    bool*           GetVsync() {return &p_vsync;}

    void            SetInputModeGame();
    void            SetInputModeUI();

    // Returns projection matrix adjusted to the current screen orientation
    float4x4 GetAdjustedProjectionMatrix(float FOV, float NearPlane, float FarPlane) const;

    // Returns pretransform matrix that matches the current screen rotation
    float4x4 GetSurfacePretransformMatrix(const float3& f3CameraViewAxis) const;


    void Quit();

    //
    // Interface
    //

    virtual bool Initialize() = 0;

    virtual void Update(float dt) = 0;
    virtual void Draw()           = 0;

    virtual void KeyEvent(Key key, KeyState state) = 0;

    virtual void MouseEvent(float2 pos) = 0;

    struct Camera{
        float4x4 location = float4x4::Translation(0.0f, 0.0f, 5.0f);
        float4x4 rotation = float4x4::Translation(0.0f, 0.0f, 0.0f);
        float fov = 0.9;
    };

private:
    bool CreateWindow(const char* Title, int Width, int Height, int GlfwApiHint);
    bool InitEngine(RENDER_DEVICE_TYPE DevType);
    bool ProcessCommandLine(int argc, const char* const* argv, RENDER_DEVICE_TYPE& DevType);
    void Loop();
    void OnKeyEvent(Key key, KeyState state);

    static void GLFW_ResizeCallback(GLFWwindow* wnd, int w, int h);
    static void GLFW_KeyCallback(GLFWwindow* wnd, int key, int, int state, int);
    static void GLFW_MouseButtonCallback(GLFWwindow* wnd, int button, int state, int);
    static void GLFW_CursorPosCallback(GLFWwindow* wnd, double xpos, double ypos);
    static void GLFW_MouseWheelCallback(GLFWwindow* wnd, double dx, double dy);

    friend int BaseEngineMain(int argc, const char* const* argv);

private:
    RefCntAutoPtr<IRenderDevice>  m_pDevice;
    RefCntAutoPtr<IDeviceContext> m_pImmediateContext;
    RefCntAutoPtr<ISwapChain>     m_pSwapChain;
    GLFWwindow*                   m_Window = nullptr;

    std::unique_ptr<ImGuiImplDiligent> m_pImGui;

    struct ActiveKey
    {
        Key      key;
        KeyState state;
    };
    std::vector<ActiveKey> m_ActiveKeys;

    using TClock   = std::chrono::high_resolution_clock;
    using TSeconds = std::chrono::duration<float>;

    TClock::time_point m_LastUpdate = {};

    bool p_vsync = true;
    bool p_GameInput = false;
};

BaseEngine* CreateGLFWApp();

} // namespace Diligent
