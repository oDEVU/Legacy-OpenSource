/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

#include "ImGuiImplGLFW.hpp"

#include "ThirdParty/imgui/imgui.h"
#include "./ThirdParty/imgui_impl_glfw.h"

//#include "DebugUtilities.hpp"

namespace Diligent
{

int p_backend = 0; 

std::unique_ptr<ImGuiImplGLFW> ImGuiImplGLFW::Create(const ImGuiDiligentCreateInfo& CI, GLFWwindow *window, int backend)
{
    return std::make_unique<ImGuiImplGLFW>(CI, window, backend);
}

ImGuiImplGLFW::ImGuiImplGLFW(const ImGuiDiligentCreateInfo& CI, GLFWwindow *window, int backend) : ImGuiImplDiligent{CI} // 1 vk / 2 ogl / 3 other
{
    if(backend == 1){
        ImGui_ImplGlfw_InitForVulkan(window, true);
    }else if(backend == 2){
        ImGui_ImplGlfw_InitForOpenGL(window, true);
    }else{
        ImGui_ImplGlfw_InitForOther(window, true);
    }

    p_backend = backend;
}

ImGuiImplGLFW::~ImGuiImplGLFW()
{
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiImplGLFW::NewFrame(Uint32 RenderSurfaceWidth, Uint32 RenderSurfaceHeight, SURFACE_TRANSFORM SurfacePreTransform)
{
    //VERIFY(SurfacePreTransform == SURFACE_TRANSFORM_IDENTITY, "Unexpected surface pre-transform");

    ImGui_ImplGlfw_NewFrame();

    ImGuiImplDiligent::NewFrame(RenderSurfaceWidth, RenderSurfaceHeight, SurfacePreTransform);

#ifdef DILIGENT_DEBUG
    {
        ImGuiIO& io = ImGui::GetIO();
        VERIFY(io.DisplaySize.x == 0 || io.DisplaySize.x == static_cast<float>(RenderSurfaceWidth),
               "Render surface width (", RenderSurfaceWidth, ") does not match io.DisplaySize.x (", io.DisplaySize.x, ")");
        VERIFY(io.DisplaySize.y == 0 || io.DisplaySize.y == static_cast<float>(RenderSurfaceHeight),
               "Render surface height (", RenderSurfaceHeight, ") does not match io.DisplaySize.y (", io.DisplaySize.y, ")");
    }
#endif
}

} // namespace Diligent
