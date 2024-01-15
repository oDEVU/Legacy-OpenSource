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

#include "FirstPersonCamera.hpp"
#include <algorithm>

namespace Diligent
{

void FirstPersonCamera::Update(Key key, KeyState state, float ElapsedTime)
{
    float3 MoveDirection = float3(0, 0, 0);

    // Input repeated
    if (state == KeyState::Press || state == KeyState::Repeat)
    {
        switch (key)
        {
            case Key::W:
                MoveDirection.z += 1.0f;
                //cam.location.Translation();
                break;

            case Key::S:
                MoveDirection.z -= 1.0f;
                ////m_Player.PendingPos.y -= 1.0f;
                break;

            case Key::D:
                MoveDirection.x += 1.0f;
                //m_Player.PendingPos.x += 1.0f;
                break;

            case Key::A:
                MoveDirection.x -= 1.0f;
                //m_Player.PendingPos.x -= 1.0f;
                break;

            case Key::Space:
                MoveDirection.y += 1.0f;
                break;

            case Key::RightShift:
            case Key::LeftShift:
                MoveDirection.y -= 1.0f;

            default:
                break;
        }
    }

    // Normalize vector so if moving in 2 dirs (left & forward),
    // the camera doesn't move faster than if moving in 1 dir
    auto len = length(MoveDirection);
    if (len != 0.0)
        MoveDirection /= len;

    MoveDirection *= m_fMoveSpeed;

    m_fCurrentSpeed = length(MoveDirection);

    float3 PosDelta = MoveDirection * ElapsedTime;

    m_PosDelta += PosDelta;

    /*{
        const auto& mouseState = Controller.GetMouseState();

        float MouseDeltaX = 0;
        float MouseDeltaY = 0;
        if (m_LastMouseState.PosX >= 0 && m_LastMouseState.PosY >= 0 &&
            m_LastMouseState.ButtonFlags != MouseState::BUTTON_FLAG_NONE)
        {
            MouseDeltaX = mouseState.PosX - m_LastMouseState.PosX;
            MouseDeltaY = mouseState.PosY - m_LastMouseState.PosY;
        }
        m_LastMouseState = mouseState;

        float fYawDelta   = MouseDeltaX * m_fRotationSpeed;
        float fPitchDelta = MouseDeltaY * m_fRotationSpeed;
        if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_LEFT)
        {
            m_fYawAngle += fYawDelta * -m_fHandness;
            m_fPitchAngle += fPitchDelta * -m_fHandness;
            m_fPitchAngle = std::max(m_fPitchAngle, -PI_F / 2.f);
            m_fPitchAngle = std::min(m_fPitchAngle, +PI_F / 2.f);
        }
    }*/
}

void FirstPersonCamera::UpdateMouse(float2 pos){
        float MouseDeltaX = 0;
        float MouseDeltaY = 0;
        if (pos.x >= 0 && pos.y >= 0)
        {
            MouseDeltaX = pos.x - m_LastMouseState.x;
            MouseDeltaY = pos.y - m_LastMouseState.y;
        }
        m_LastMouseState = pos;

        float fYawDelta   = MouseDeltaX * m_fRotationSpeed;
        float fPitchDelta = MouseDeltaY * m_fRotationSpeed;

        m_fYawAngle += fYawDelta * -m_fHandness;
        m_fPitchAngle += fPitchDelta * -m_fHandness;
        m_fPitchAngle = std::max(m_fPitchAngle, -PI_F / 2.f);
        m_fPitchAngle = std::min(m_fPitchAngle, +PI_F / 2.f);
}

void FirstPersonCamera::UpdateMat(){

    float4x4 ReferenceRotation = GetReferenceRotiation();

    float4x4 CameraRotation = float4x4::RotationArbitrary(m_ReferenceUpAxis, m_fYawAngle) *
        float4x4::RotationArbitrary(m_ReferenceRightAxis, m_fPitchAngle) *
        ReferenceRotation;
    float4x4 WorldRotation = CameraRotation.Transpose();

    float3 PosDeltaWorld = m_PosDelta * WorldRotation;
    m_Pos += PosDeltaWorld;

    m_ViewMatrix  = float4x4::Translation(-m_Pos) * CameraRotation;
    m_WorldMatrix = WorldRotation * float4x4::Translation(m_Pos);

    m_PosDelta = float3(.0f, .0f, .0f);
}

float4x4 FirstPersonCamera::GetReferenceRotiation() const
{
    // clang-format off
    return float4x4
    {
        m_ReferenceRightAxis.x, m_ReferenceUpAxis.x, m_ReferenceAheadAxis.x, 0,
        m_ReferenceRightAxis.y, m_ReferenceUpAxis.y, m_ReferenceAheadAxis.y, 0,
        m_ReferenceRightAxis.z, m_ReferenceUpAxis.z, m_ReferenceAheadAxis.z, 0,
                             0,                   0,                      0, 1
    };
    // clang-format on
}

void FirstPersonCamera::SetReferenceAxes(const float3& ReferenceRightAxis, const float3& ReferenceUpAxis, bool IsRightHanded)
{
    m_ReferenceRightAxis    = normalize(ReferenceRightAxis);
    m_ReferenceUpAxis       = ReferenceUpAxis - dot(ReferenceUpAxis, m_ReferenceRightAxis) * m_ReferenceRightAxis;
    auto            UpLen   = length(m_ReferenceUpAxis);
    constexpr float Epsilon = 1e-5f;
    if (UpLen < Epsilon)
    {
        UpLen = Epsilon;
        LOG_WARNING_MESSAGE("Right and Up axes are collinear");
    }
    m_ReferenceUpAxis /= UpLen;

    m_fHandness          = IsRightHanded ? +1.f : -1.f;
    m_ReferenceAheadAxis = m_fHandness * cross(m_ReferenceRightAxis, m_ReferenceUpAxis);
    auto AheadLen        = length(m_ReferenceAheadAxis);
    if (AheadLen < Epsilon)
    {
        AheadLen = Epsilon;
        LOG_WARNING_MESSAGE("Ahead axis is not well defined");
    }
    m_ReferenceAheadAxis /= AheadLen;
}

void FirstPersonCamera::SetLookAt(const float3& LookAt)
{
    float3 ViewDir = LookAt - m_Pos;

    ViewDir = ViewDir * GetReferenceRotiation();

    m_fYawAngle = atan2f(ViewDir.x, ViewDir.z);

    float fXZLen  = sqrtf(ViewDir.z * ViewDir.z + ViewDir.x * ViewDir.x);
    m_fPitchAngle = -atan2f(ViewDir.y, fXZLen);
}

void FirstPersonCamera::SetRotation(float Yaw, float Pitch)
{
    m_fYawAngle   = Yaw;
    m_fPitchAngle = Pitch;
}

void FirstPersonCamera::SetProjAttribs(Float32           NearClipPlane,
                                       Float32           FarClipPlane,
                                       Float32           AspectRatio,
                                       Float32           FOV,
                                       SURFACE_TRANSFORM SrfPreTransform,
                                       bool              IsGL)
{
    m_ProjAttribs.NearClipPlane = NearClipPlane;
    m_ProjAttribs.FarClipPlane  = FarClipPlane;
    m_ProjAttribs.AspectRatio   = AspectRatio;
    m_ProjAttribs.FOV           = FOV;
    m_ProjAttribs.PreTransform  = SrfPreTransform;
    m_ProjAttribs.IsGL          = IsGL;

    float XScale, YScale;
    if (SrfPreTransform == SURFACE_TRANSFORM_ROTATE_90 ||
        SrfPreTransform == SURFACE_TRANSFORM_ROTATE_270 ||
        SrfPreTransform == SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90 ||
        SrfPreTransform == SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270)
    {
        // When the screen is rotated, vertical FOV becomes horizontal FOV
        XScale = 1.f / std::tan(FOV / 2.f);
        // Aspect ratio is width/height accounting for pretransform
        YScale = XScale / AspectRatio;
    }
    else
    {
        YScale = 1.f / std::tan(FOV / 2.f);
        XScale = YScale / AspectRatio;
    }

    float4x4 Proj;
    Proj._11 = XScale;
    Proj._22 = YScale;
    Proj.SetNearFarClipPlanes(NearClipPlane, FarClipPlane, IsGL);

    m_ProjMatrix = float4x4::Projection(m_ProjAttribs.FOV, m_ProjAttribs.AspectRatio, m_ProjAttribs.NearClipPlane, m_ProjAttribs.FarClipPlane, IsGL);
}

void FirstPersonCamera::SetSpeedUpScales(Float32 SpeedUpScale, Float32 SuperSpeedUpScale)
{
    m_fSpeedUpScale      = SpeedUpScale;
    m_fSuperSpeedUpScale = SuperSpeedUpScale;
}

} // namespace Diligent
