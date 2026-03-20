#pragma once

namespace big
{
    enum class graphics_api
    {
        none,
        directx9,
        directx10,
        directx11,
        directx12,
        opengl,
        vulkan
    };

    enum class eInitializationStatus
    {
        SUCCESS,
        FAILED,
        MODULE_NOT_FOUND_ERROR,
        CREATE_DUMMY_DEVICE_ERROR
    };
}