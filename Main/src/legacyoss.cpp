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

#include <random>
#include <vector>

#include "legacyoss.hpp"
#include "Graphics/GraphicsTools/interface/ShaderMacroHelper.hpp"
#include "Common/interface/CallbackWrapper.hpp"

#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include "Graphics/GraphicsTools/interface/GraphicsUtilities.h"
#include "TextureLoader/interface/TextureUtilities.h"

namespace Diligent
{

inline float fract(float x)
{
    return x - floor(x);
}

BaseEngine* CreateGLFWApp()
{
    return new Game{};
}

bool Game::Initialize()
{
    try
    {
        GetEngineFactory()->CreateDefaultShaderSourceStreamFactory(nullptr, &m_pShaderSourceFactory);
        CHECK_THROW(m_pShaderSourceFactory);

        RefCntAutoPtr<IRenderStateNotationParser> pRSNParser;
        {
            CreateRenderStateNotationParser({}, &pRSNParser);
            CHECK_THROW(pRSNParser);
            pRSNParser->ParseFile("assets/RenderStates.json", m_pShaderSourceFactory);
        }
        {
            RenderStateNotationLoaderCreateInfo RSNLoaderCI{};
            RSNLoaderCI.pDevice        = GetDevice();
            RSNLoaderCI.pStreamFactory = m_pShaderSourceFactory;
            RSNLoaderCI.pParser        = pRSNParser;
            CreateRenderStateNotationLoader(RSNLoaderCI, &m_pRSNLoader);
            CHECK_THROW(m_pRSNLoader);
        }

        CreatePipelineState();
        CreateVertexBuffer();
        CreateIndexBuffer();
        LoadTexture();

        m_Camera.SetPos(float3(0, 0, -10));
        m_Camera.SetRotation(0, 0);
        m_Camera.SetRotationSpeed(0.005f);
        m_Camera.SetMoveSpeed(5.f);
        m_Camera.SetSpeedUpScales(5.f, 10.f);

        SetInputModeGame();

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void Game::UpdateUI(float dt)
{
    // Game ui here
}

void Game::UpdateUIDebug(float dt)
{
    // Debug ui here

    ImGui::Begin("Debug panel");
    ImGui::Text("FPS: %f", 1/dt);
    ImGui::Text("Delta: %f", dt);
    ImGui::Text("Time: %f", CurrTime);
    ImGui::Text("Rot: %f, %f", m_Camera.GetRot().x, m_Camera.GetRot().y);
    ImGui::Checkbox("Vsync", GetVsync());
    ImGui::Checkbox("Disable Buffer Clearing", &u_NoClear); // i just like the effect, there is no need for this
    ImGui::End();
}

void Game::Update(float dt)
{
    m_Camera.UpdateMat();

    LastTime = CurrTime;
    CurrTime += dt;
    //dt = std::min(dt, Constants.MaxDT);
    // Apply rotation
    //float4x4 CubeModelTransform = float4x4::Translation(0.0f, 0.0f, 0.0f);

    // Camera is at (0, 0, -5) looking along the Z axis
    //const auto  CameraView     = m_Camera.GetViewMatrix() * SrfPreTransform;
    //const auto& CameraWorld    = m_Camera.GetWorldMatrix();
    //float3      CameraWorldPos = float3::MakeVector(CameraWorld[3]);
    //const auto& Proj           = m_Camera.GetProjMatrix();

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});
    
    float4x4 View = m_Camera.GetViewMatrix() * SrfPreTransform;

    // Get projection matrix adjusted to the current screen orientation
    auto Proj = GetAdjustedProjectionMatrix(m_Camera.GetProjAttribs().FOV, m_Camera.GetProjAttribs().NearClipPlane, m_Camera.GetProjAttribs().FarClipPlane);

    // Compute world-view-projection matrix
    m_WorldViewProjMatrix = View * Proj;

    UpdateUI(dt);
    if(u_ShowDebug){
        UpdateUIDebug(dt);
    }
}

void Game::Draw()
{
    auto* pRTV = GetSwapChain()->GetCurrentBackBufferRTV();
    auto* pDSV = GetSwapChain()->GetDepthBufferDSV();

    GetContext()->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Clear the back buffer
    const float ClearColor[] = {0.001f, 0.001f, 0.001f, 1.0f};
    if(!u_NoClear){
        GetContext()->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    GetContext()->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    
    {
        // Map the buffer and write current world-view-projection matrix
        MapHelper<float4x4> CBConstants(GetContext(), m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBConstants = m_WorldViewProjMatrix.Transpose();
    }

    // Bind vertex and index buffers
    const Uint64 offset   = 0;
    IBuffer*     pBuffs[] = {m_CubeVertexBuffer};
    GetContext()->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    GetContext()->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the pipeline state
    GetContext()->SetPipelineState(pPSO);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    GetContext()->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs DrawAttrs;     // This is an indexed draw call
    DrawAttrs.IndexType  = VT_UINT32; // Index type
    DrawAttrs.NumIndices = 36;
    // Verify the state of vertex and index buffers
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    GetContext()->DrawIndexed(DrawAttrs);
}

void Game::KeyEvent(Key key, KeyState state)
{
    m_Camera.Update(key, state, CurrTime-LastTime);

    // Input repeated
    if (state == KeyState::Press || state == KeyState::Repeat)
    {
        switch (key)
        {
            case Key::W:
            case Key::Up:
            case Key::NP_Up:
                //cam.location.Translation();
                break;

            case Key::S:
            case Key::Down:
            case Key::NP_Down:
                ////m_Player.PendingPos.y -= 1.0f;
                break;

            case Key::D:
            case Key::Right:
            case Key::NP_Right:
                //m_Player.PendingPos.x += 1.0f;
                break;

            case Key::A:
            case Key::Left:
            case Key::NP_Left:
                //m_Player.PendingPos.x -= 1.0f;
                break;

            case Key::Space:
                //m_Player.PendingPos = //m_Player.FlashLightDir;
                break;

            case Key::Esc:
                Quit();
                break;

            default:
                break;
        }
    }

    //Input action
    if (state == KeyState::Press)
    {
        switch (key)
        {
            case Key::F3:
                u_ShowDebug = !u_ShowDebug;
                if(u_ShowDebug){
                    SetInputModeUI();
                }else{
                    SetInputModeGame();
                }
                break;
        }
    }

    //if (key == Key::MB_Left)
        //m_Player.LMBPressed = (state != KeyState::Release);

    // generate new map
    //if (state == KeyState::Release && key == Key::Tab)
        //LoadNewMap();
    
}

void Game::MouseEvent(float2 pos)
{
    //m_Player.MousePos = pos;
    m_Camera.UpdateMouse(pos);
}

void Game::CreatePipelineState()
{
    // Pipeline state object encompasses configuration of all GPU stages

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSOCreateInfo.PSODesc.Name = "Cube PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = GetSwapChain()->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = GetSwapChain()->GetDesc().DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Cull back faces
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    // Enable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    // clang-format on

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.Desc.UseCombinedTextureSamplers = true;

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    GetEngineFactory()->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube VS";
        ShaderCI.FilePath        = "assets/cube.vsh";
        GetDevice()->CreateShader(ShaderCI, &pVS);
        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
        CreateUniformBuffer(GetDevice(), sizeof(float4x4), "VS constants CB", &m_VSConstants);
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube PS";
        ShaderCI.FilePath        = "assets/cube.psh";
        GetDevice()->CreateShader(ShaderCI, &pPS);
    }

    // clang-format off
    // Define vertex shader input layout
    LayoutElement LayoutElems[] =
    {
        // Attribute 0 - vertex position
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1 - texture coordinates
        LayoutElement{1, 0, 2, VT_FLOAT32, False}
    };
    // clang-format on

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    // clang-format off
    // Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SamLinearClampDesc}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    GetDevice()->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);

    // Since we did not explcitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables
    // never change and are bound directly through the pipeline state object.
    pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
    pPSO->CreateShaderResourceBinding(&m_SRB, true);
}

void Game::CreateVertexBuffer()
{
    // Layout of this structure matches the one we defined in the pipeline state
    struct Vertex
    {
        float3 pos;
        float2 uv;
    };

    // Cube vertices

    //      (-1,+1,+1)________________(+1,+1,+1)
    //               /|              /|
    //              / |             / |
    //             /  |            /  |
    //            /   |           /   |
    //(-1,-1,+1) /____|__________/(+1,-1,+1)
    //           |    |__________|____|
    //           |   /(-1,+1,-1) |    /(+1,+1,-1)
    //           |  /            |   /
    //           | /             |  /
    //           |/              | /
    //           /_______________|/
    //        (-1,-1,-1)       (+1,-1,-1)
    //

    // clang-format off
    // This time we have to duplicate verices because texture coordinates cannot
    // be shared
    Vertex CubeVerts[] =
    {
        {float3(-1,-1,-1), float2(0,1)},
        {float3(-1,+1,-1), float2(0,0)},
        {float3(+1,+1,-1), float2(1,0)},
        {float3(+1,-1,-1), float2(1,1)},

        {float3(-1,-1,-1), float2(0,1)},
        {float3(-1,-1,+1), float2(0,0)},
        {float3(+1,-1,+1), float2(1,0)},
        {float3(+1,-1,-1), float2(1,1)},

        {float3(+1,-1,-1), float2(0,1)},
        {float3(+1,-1,+1), float2(1,1)},
        {float3(+1,+1,+1), float2(1,0)},
        {float3(+1,+1,-1), float2(0,0)},

        {float3(+1,+1,-1), float2(0,1)},
        {float3(+1,+1,+1), float2(0,0)},
        {float3(-1,+1,+1), float2(1,0)},
        {float3(-1,+1,-1), float2(1,1)},

        {float3(-1,+1,-1), float2(1,0)},
        {float3(-1,+1,+1), float2(0,0)},
        {float3(-1,-1,+1), float2(0,1)},
        {float3(-1,-1,-1), float2(1,1)},

        {float3(-1,-1,+1), float2(1,1)},
        {float3(+1,-1,+1), float2(0,1)},
        {float3(+1,+1,+1), float2(0,0)},
        {float3(-1,+1,+1), float2(1,0)}
    };
    // clang-format on

    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Cube vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = sizeof(CubeVerts);
    BufferData VBData;
    VBData.pData    = CubeVerts;
    VBData.DataSize = sizeof(CubeVerts);
    GetDevice()->CreateBuffer(VertBuffDesc, &VBData, &m_CubeVertexBuffer);
}

void Game::CreateIndexBuffer()
{
    // clang-format off
    Uint32 Indices[] =
    {
        2,0,1,    2,3,0,
        4,6,5,    4,7,6,
        8,10,9,   8,11,10,
        12,14,13, 12,15,14,
        16,18,17, 16,19,18,
        20,21,22, 20,22,23
    };
    // clang-format on

    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Cube index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = sizeof(Indices);
    BufferData IBData;
    IBData.pData    = Indices;
    IBData.DataSize = sizeof(Indices);
    GetDevice()->CreateBuffer(IndBuffDesc, &IBData, &m_CubeIndexBuffer);
}

void Game::LoadTexture()
{
    TextureLoadInfo loadInfo;
    loadInfo.IsSRGB = true;
    RefCntAutoPtr<ITexture> Tex;
    CreateTextureFromFile("assets/base_txt.png", loadInfo, GetDevice(), &Tex);
    // Get shader resource view from the texture
    m_TextureSRV = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    // Set texture SRV in the SRB
    m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV);
}

} // namespace Diligent
