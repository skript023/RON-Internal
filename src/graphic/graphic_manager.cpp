#include "graphic_manager.hpp"
#include "hooking.hpp"

namespace big
{
    graphic_manager& graphic_manager::get_instance()
    {
        static graphic_manager instance;
        return instance;
    }
    std::string graphic_manager::get_render_type_name()
    {
        auto hash = graphic_manager::get_instance().m_render_type;

        switch (hash)
        {
        case graphics_api::none:
            return "No Render Type";
        case graphics_api::directx9:
            return "DirectX 9";
        case graphics_api::directx11:
            return "DirectX 11";
        case graphics_api::directx12:
            return "DirectX 12";
        }

        return "Unknown Type";
    }
    void* graphic_manager::get_method_table_impl(int index)
    {
        if (m_swapchain_methods)
        {
            auto address = (void*)m_swapchain_methods[index];

            LOG(INFO) << "Address of index : " << index << " is " << address;
            return address;
        }

        return nullptr;
    }
    graphics_api graphic_manager::detect_graphics_api()
    {
        if (GetModuleHandleA("d3d9.dll"))
        {
            return graphics_api::directx9;
        }
        else if (GetModuleHandleA("d3d10.dll"))
        {
            return graphics_api::directx10;
        }
        else if (GetModuleHandleA("d3d11.dll"))
        {
            return graphics_api::directx11;
        }
        else if (GetModuleHandleA("d3d12.dll"))
        {
            return graphics_api::directx12;
        }
        else if (GetModuleHandleA("opengl32.dll"))
        {
            return graphics_api::opengl;
        }
        else if (GetModuleHandleA("vulkan-1.dll"))
        {
            return graphics_api::vulkan;
        }
        return graphics_api::none;
    }
    eInitializationStatus graphic_manager::create_dummy_device_impl(graphics_api renderType)
	{
        WNDCLASSEX window_class;
        window_class.cbSize = sizeof(WNDCLASSEX);
        window_class.style = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc = DefWindowProc;
        window_class.cbClsExtra = 0;
        window_class.cbWndExtra = 0;
        window_class.hInstance = GetModuleHandle(NULL);
        window_class.hIcon = NULL;
        window_class.hCursor = NULL;
        window_class.hbrBackground = NULL;
        window_class.lpszMenuName = NULL;
        window_class.lpszClassName = "Ellohim";
        window_class.hIconSm = NULL;

        ::RegisterClassEx(&window_class);

        HWND window = ::CreateWindow(window_class.lpszClassName, "Ellohim Dummy Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, window_class.hInstance, NULL);

        if (window == NULL)
        {
            return eInitializationStatus::FAILED;
        }

        if (renderType == graphics_api::directx9)
        {
            HMODULE d3d9_module = ::GetModuleHandle("d3d9.dll");
            if (d3d9_module == NULL)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::MODULE_NOT_FOUND_ERROR;
            }

            void* Direct3DCreate9 = ::GetProcAddress(d3d9_module, "Direct3DCreate9");
            if (Direct3DCreate9 == NULL)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            auto create_d3d9 = ((LPDIRECT3D9(__stdcall*)(uint32_t))(Direct3DCreate9))(D3D_SDK_VERSION);
            if (create_d3d9 == NULL)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            D3DDISPLAYMODE DisplayMode;
            if (FAILED(create_d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &DisplayMode)))
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            D3DPRESENT_PARAMETERS d3d9_params;
            d3d9_params.BackBufferWidth = 0;
            d3d9_params.BackBufferHeight = 0;
            d3d9_params.BackBufferFormat = DisplayMode.Format;
            d3d9_params.BackBufferCount = 0;
            d3d9_params.MultiSampleType = D3DMULTISAMPLE_NONE;
            d3d9_params.MultiSampleQuality = NULL;
            d3d9_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
            d3d9_params.hDeviceWindow = window;
            d3d9_params.Windowed = 1;
            d3d9_params.EnableAutoDepthStencil = 0;
            d3d9_params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
            d3d9_params.Flags = NULL;
            d3d9_params.FullScreen_RefreshRateInHz = 0;
            d3d9_params.PresentationInterval = 0;

            LPDIRECT3DDEVICE9 d3d9_device;
            if (create_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &d3d9_params, &d3d9_device) < 0)
            {
                create_d3d9->Release();
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::CREATE_DUMMY_DEVICE_ERROR;
            }

            m_swapchain_methods = (uint64_t*)calloc(120, sizeof(uint64_t));
            memcpy(m_swapchain_methods, *(uint64_t**)d3d9_device, sizeof(uint64_t));

            create_d3d9->Release();
            create_d3d9 = nullptr;
            d3d9_device->Release();
            d3d9_device = nullptr;

            ::DestroyWindow(window);
            ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

            m_render_type = graphics_api::directx9;

            return eInitializationStatus::SUCCESS;
        }
        else if (renderType == graphics_api::directx11)
        {
            HMODULE d3d11 = ::GetModuleHandle("d3d11.dll");
            if (d3d11 == NULL)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::MODULE_NOT_FOUND_ERROR;
            }

            void* create_device_and_swapchain = ::GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain");
            if (create_device_and_swapchain == NULL)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            D3D_FEATURE_LEVEL featureLevel;
            const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1, };

            DXGI_RATIONAL refreshRate;
            refreshRate.Numerator = 60;
            refreshRate.Denominator = 1;

            DXGI_MODE_DESC bufferDesc;
            bufferDesc.Width = 100;
            bufferDesc.Height = 100;
            bufferDesc.RefreshRate = refreshRate;
            bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

            DXGI_SAMPLE_DESC sampleDesc;
            sampleDesc.Count = 1;
            sampleDesc.Quality = 0;

            DXGI_SWAP_CHAIN_DESC swapChainDesc;
            swapChainDesc.BufferDesc = bufferDesc;
            swapChainDesc.SampleDesc = sampleDesc;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = 1;
            swapChainDesc.OutputWindow = window;
            swapChainDesc.Windowed = 1;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            IDXGISwapChain* d3d11_swapchain = nullptr;
            ID3D11Device* d3d11_device = nullptr;
            ID3D11DeviceContext* d3d11_context = nullptr;

            HRESULT hr = ((create_d3d11_device_and_swapchain_t)(create_device_and_swapchain))(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &swapChainDesc, &d3d11_swapchain, &d3d11_device, &featureLevel, &d3d11_context);

            if (FAILED(hr))
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::CREATE_DUMMY_DEVICE_ERROR;
            }

            m_swapchain_methods = (uint64_t*)calloc(205, sizeof(uint64_t));
            memcpy(this->m_swapchain_methods, *(uint64_t**)d3d11_swapchain, 18 * sizeof(uint64_t));
            memcpy(this->m_swapchain_methods + 18, *(uint64_t**)d3d11_device, 43 * sizeof(uint64_t));
            memcpy(this->m_swapchain_methods + 18 + 43, *(uint64_t**)d3d11_context, 144 * sizeof(uint64_t));

            d3d11_swapchain->Release();
            d3d11_swapchain = nullptr;

            d3d11_device->Release();
            d3d11_device = nullptr;

            d3d11_context->Release();
            d3d11_context = nullptr;

            ::DestroyWindow(window);
            ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

            m_render_type = graphics_api::directx11;

            return eInitializationStatus::SUCCESS;
        }
        else if (renderType == graphics_api::directx12)
        {
            HMODULE dxgi_module;
            HMODULE d3d12_module;
            if ((dxgi_module = ::GetModuleHandle("dxgi.dll")) == NULL || (d3d12_module = ::GetModuleHandle("d3d12.dll")) == NULL)
            {
                DestroyWindow(window);
                UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                LOG(INFO) << "Failed get dx12 & dxgi module";

                return eInitializationStatus::MODULE_NOT_FOUND_ERROR;
            }

            void* CreateDXGIFactory;
            if ((CreateDXGIFactory = GetProcAddress(dxgi_module, "CreateDXGIFactory")) == NULL)
            {
                DestroyWindow(window);
                UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            IDXGIFactory* factory;
            if (((long(__stdcall*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&factory) < 0)
            {
                DestroyWindow(window);
                UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            IDXGIAdapter* adapter;
            if (factory->EnumAdapters(0, &adapter) == DXGI_ERROR_NOT_FOUND)
            {
                DestroyWindow(window);
                UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            void* D3D12CreateDevice;
            if ((D3D12CreateDevice = ::GetProcAddress(d3d12_module, "D3D12CreateDevice")) == NULL)
            {
                DestroyWindow(window);
                UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            ID3D12Device* device;
            if (((long(__stdcall*)(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**))(D3D12CreateDevice))(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device) < 0)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            D3D12_COMMAND_QUEUE_DESC queueDesc;
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Priority = 0;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 0;

            ID3D12CommandQueue* commandQueue;
            if (device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void**)&commandQueue) < 0)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            ID3D12CommandAllocator* commandAllocator;
            if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&commandAllocator) < 0)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            ID3D12GraphicsCommandList* commandList;
            if (device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&commandList) < 0)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            DXGI_RATIONAL refreshRate;
            refreshRate.Numerator = 60;
            refreshRate.Denominator = 1;

            DXGI_MODE_DESC bufferDesc;
            bufferDesc.Width = 100;
            bufferDesc.Height = 100;
            bufferDesc.RefreshRate = refreshRate;
            bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

            DXGI_SAMPLE_DESC sampleDesc;
            sampleDesc.Count = 1;
            sampleDesc.Quality = 0;

            DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
            swapChainDesc.BufferDesc = bufferDesc;
            swapChainDesc.SampleDesc = sampleDesc;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = 2;
            swapChainDesc.OutputWindow = window;
            swapChainDesc.Windowed = 1;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            IDXGISwapChain* swapChain;
            if (factory->CreateSwapChain(commandQueue, &swapChainDesc, &swapChain) < 0)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::CREATE_DUMMY_DEVICE_ERROR;
            }

            m_swapchain_methods = (uint64_t*)calloc(150, sizeof(uint64_t));
            ::memcpy(m_swapchain_methods, *(uint64_t**)device, 44 * sizeof(uint64_t));
            ::memcpy(m_swapchain_methods + 44, *(uint64_t**)commandQueue, 19 * sizeof(uint64_t));
            ::memcpy(m_swapchain_methods + 44 + 19, *(uint64_t**)commandAllocator, 9 * sizeof(uint64_t));
            ::memcpy(m_swapchain_methods + 44 + 19 + 9, *(uint64_t**)commandList, 60 * sizeof(uint64_t));
            ::memcpy(m_swapchain_methods + 44 + 19 + 9 + 60, *(uint64_t**)swapChain, 18 * sizeof(uint64_t));

            device->Release();
            device = NULL;

            commandQueue->Release();
            commandQueue = NULL;

            commandAllocator->Release();
            commandAllocator = NULL;

            commandList->Release();
            commandList = NULL;

            swapChain->Release();
            swapChain = NULL;

            ::DestroyWindow(window);
            ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

            m_render_type = graphics_api::directx12;

            return eInitializationStatus::SUCCESS;
        }

        return eInitializationStatus::FAILED;
	}
}