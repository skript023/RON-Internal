#pragma once

#include "graphic_enums.hpp"
#include "detour_hook.hpp"

namespace big
{
    using create_d3d11_device_and_swapchain_t = long(__stdcall*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

    class graphic_manager
    {
        graphic_manager() : m_render_type(graphics_api::none) {}

        // Detect which graphics API is loaded
        graphics_api detect_graphics_api();

        // Forbid copying the singleton
        graphic_manager(const graphic_manager&) = delete;
        graphic_manager& operator=(const graphic_manager&) = delete;

        eInitializationStatus create_dummy_device_impl(graphics_api renderType);

        eInitializationStatus get_swapchain_impl(graphics_api render_type)
        {
            return create_dummy_device(render_type);
        }
        void* get_method_table_impl(int index);

        graphics_api m_render_type;
        uint64_t* m_swapchain_methods = NULL;
        static graphic_manager& get_instance();
    public:

        static graphics_api get_render_type() { return graphic_manager::get_instance().m_render_type; }

        static std::string get_render_type_name();
        static void* get_method_table(int index) { return get_instance().get_method_table_impl(index); };
		static eInitializationStatus get_swapchain(graphics_api render_type) { return get_instance().get_swapchain_impl(render_type); }
        static eInitializationStatus create_dummy_device(graphics_api renderType) { return get_instance().create_dummy_device_impl(renderType); };
    };
}