#include "gui.hpp"
#include "logger/logger.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "file_manager.hpp"
#include "fonts/font_list.hpp"
#include "fonts/icon_list.hpp"

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>

#include "astra/host/canvas.hpp"
#include <graphic/graphic_manager.hpp>
#include "utility/unreal_engine_utility.hpp"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace big
{
    namespace
    {
        // No API returns the queue a DX12 swapchain was created on, but DXGI keeps it inside the swapchain object,
        // while the game's other direct queues are not there.
        comptr<ID3D12CommandQueue> find_present_queue(IDXGISwapChain* swapchain, const std::vector<comptr<ID3D12CommandQueue>>& candidates)
        {
            MEMORY_BASIC_INFORMATION region{};
            if (!VirtualQuery(swapchain, &region, sizeof(region)) || region.State != MEM_COMMIT || (region.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
                return {};
            const auto begin = reinterpret_cast<std::uintptr_t>(swapchain);
            const auto end = (std::min)(begin + 0x1000, reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize);

            comptr<ID3D12CommandQueue> found;
            for (const auto& queue : candidates)
            {
                for (auto slot = begin; slot + sizeof(void*) <= end; slot += sizeof(void*))
                {
                    if (*reinterpret_cast<void* const*>(slot) != queue.Get())
                        continue;
                    if (found)
                        return {};
                    found = queue;
                    break;
                }
            }
            return found;
        }
    }

    renderer::renderer()
    {
        g_gui.init();
        g_renderer = this;
    }

    renderer::~renderer()
    {
        std::lock_guard lock(m_mutex);
        release_targets();
        if (m_init)
        {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
        m_textures.clear();
        // Again, so D3D11On12 flushes the ImGui and banner resources before the GPU is waited on.
        release_targets();
        g_renderer = nullptr;
    }

    void renderer::capture_queue(ID3D12CommandQueue* queue)
    {
        if (queue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT)
            return;
        std::lock_guard lock(m_queue_mutex);
        for (const auto& existing : m_queues)
            if (existing.Get() == queue)
                return;
        m_queues.emplace_back(queue);
    }

	void renderer::on_present()
	{
        if (m_targets.empty() && !create_targets())
            return;
        const UINT index = m_on12 ? m_swapchain3->GetCurrentBackBufferIndex() : 0;
        if (index >= m_targets.size())
            return;
		canvas::prepare_imgui_mouse();
		canvas::apply_panel_theme();

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		canvas::keep_windows_off_menu();

		for (const auto& cb : g_gui.m_dx_callbacks | std::views::values)
			cb();

		ImGui::Render();
        ID3D11Resource* wrapped = m_on12 ? m_wrapped_buffers[index].Get() : nullptr;
        if (m_on12)
            m_on12->AcquireWrappedResources(&wrapped, 1);
        // Preserve the game's output merger state on the native DX11 path.
        ID3D11RenderTargetView* previous[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
        ID3D11DepthStencilView* depth = nullptr;
        m_d3d_context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, previous, &depth);
        auto target = m_targets[index].Get();
        m_d3d_context->OMSetRenderTargets(1, &target, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        m_d3d_context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, previous, depth);
        for (auto view : previous)
            if (view) view->Release();
        if (depth) depth->Release();
        if (m_on12)
        {
            m_on12->ReleaseWrappedResources(&wrapped, 1);
            m_d3d_context->Flush();
        }
        g_present_count.fetch_add(1, std::memory_order_relaxed);
	}
    bool renderer::init(IDXGISwapChain* swapchain)
    {
        if (m_init)
            return owns_swapchain(swapchain);
        DXGI_SWAP_CHAIN_DESC desc{};
        if (FAILED(swapchain->GetDesc(&desc)) || desc.OutputWindow != g_pointers->m_hwnd)
            return false;

        comptr<ID3D11Device> device11;
        comptr<ID3D12Device> device12;
        if (SUCCEEDED(swapchain->GetDevice(IID_PPV_ARGS(device11.GetAddressOf()))))
        {
            m_on12.Reset();
            m_swapchain3.Reset();
            m_queue.Reset();
            m_d3d_device = device11;
            m_d3d_device->GetImmediateContext(m_d3d_context.ReleaseAndGetAddressOf());
        }
        else if (SUCCEEDED(swapchain->GetDevice(IID_PPV_ARGS(device12.GetAddressOf()))))
        {
            // The overlay must submit on the queue the swapchain presents from. Compute/copy queues and queues
            // from other adapters cannot present; of this device's direct queues, the one the swapchain holds
            // is taken, or the only one there is.
            std::vector<comptr<ID3D12CommandQueue>> candidates;
            {
                std::lock_guard lock(m_queue_mutex);
                for (const auto& queue : m_queues)
                {
                    comptr<ID3D12Device> owner;
                    if (SUCCEEDED(queue->GetDevice(IID_PPV_ARGS(owner.GetAddressOf()))) && owner.Get() == device12.Get())
                        candidates.push_back(queue);
                }
            }
            auto candidate = find_present_queue(swapchain, candidates);
            if (!candidate && candidates.size() == 1)
                candidate = candidates.front();
            if (!candidate)
            {
                if (candidates.size() > 1)
                {
                    static bool warned = false;
                    if (!warned)
                    {
                        LOG(WARNING) << "DX12 initialization deferred: none of the " << candidates.size() << " direct queues on the game device is the swapchain's.";
                        warned = true;
                    }
                }
                return false;
            }
            if (FAILED(swapchain->QueryInterface(IID_PPV_ARGS(m_swapchain3.ReleaseAndGetAddressOf()))))
                return false;
            LOG(INFO) << "DX12 present queue chosen from " << candidates.size() << " direct queue(s) on the game device.";
            IUnknown* queues[] = { candidate.Get() };
            if (FAILED(D3D11On12CreateDevice(device12.Get(), D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                nullptr, 0, queues, 1, 0, m_d3d_device.ReleaseAndGetAddressOf(),
                m_d3d_context.ReleaseAndGetAddressOf(), nullptr)))
                return false;
            if (FAILED(m_d3d_device.As(&m_on12)))
                return false;
            m_queue = candidate;
        }
        else return false;

        m_dxgi_swapchain = swapchain;
        if (!create_targets())
            return false;
        imgui_init();
        m_init = true;
        g_gui.script_init();
        LOG(INFO) << "Swapchain initialized: " << (m_on12 ? "DirectX 12 (D3D11On12)" : "DirectX 11");
        return true;
    }

    bool renderer::create_targets()
    {
        release_targets();
        if (!m_dxgi_swapchain || !m_d3d_device)
            return false;
        DXGI_SWAP_CHAIN_DESC desc{};
        if (FAILED(m_dxgi_swapchain->GetDesc(&desc)))
            return false;
        const UINT count = m_on12 ? desc.BufferCount : 1;
        for (UINT i = 0; i < count; ++i)
        {
            comptr<ID3D11Resource> resource;
            if (m_on12)
            {
                comptr<ID3D12Resource> buffer;
                D3D11_RESOURCE_FLAGS flags{};
                flags.BindFlags = D3D11_BIND_RENDER_TARGET;
                if (FAILED(m_dxgi_swapchain->GetBuffer(i, IID_PPV_ARGS(buffer.GetAddressOf()))) ||
                    FAILED(m_on12->CreateWrappedResource(buffer.Get(), &flags,
                        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT,
                        IID_PPV_ARGS(resource.GetAddressOf()))))
                {
                    release_targets();
                    return false;
                }
            }
            else if (FAILED(m_dxgi_swapchain->GetBuffer(i, IID_PPV_ARGS(resource.GetAddressOf()))))
                return false;
            comptr<ID3D11RenderTargetView> target;
            if (FAILED(m_d3d_device->CreateRenderTargetView(resource.Get(), nullptr, target.GetAddressOf())))
            {
                release_targets();
                return false;
            }
            m_targets.push_back(target);
            if (m_on12) m_wrapped_buffers.push_back(resource);
        }
        return !m_targets.empty();
    }

    void renderer::release_targets()
    {
        if (m_d3d_context)
            m_d3d_context->OMSetRenderTargets(0, nullptr, nullptr);
        m_targets.clear();
        m_wrapped_buffers.clear();
        // Flush deferred destruction before ResizeBuffers releases the DX12 buffers.
        if (m_d3d_context) m_d3d_context->Flush();
        if (m_queue)
        {
            comptr<ID3D12Device> device;
            comptr<ID3D12Fence> fence;
            if (SUCCEEDED(m_queue->GetDevice(IID_PPV_ARGS(device.GetAddressOf()))) &&
                SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()))))
            {
                HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
                if (event)
                {
                    if (SUCCEEDED(m_queue->Signal(fence.Get(), 1)) &&
                        SUCCEEDED(fence->SetEventOnCompletion(1, event)))
                        WaitForSingleObject(event, INFINITE);
                    CloseHandle(event);
                }
            }
        }
    }

	void renderer::imgui_init()
	{
		auto file_path = g_file_manager.get_project_file("./imgui.ini").get_path();

		ImGuiContext* ctx = ImGui::CreateContext();

		static std::string path = file_path.make_preferred().string();
		ctx->IO.IniFilename = path.c_str();

		if (!ImGui_ImplWin32_Init(g_pointers->m_hwnd))
			throw std::runtime_error("Cannot initialize ImGui Win32 backend");
		ImGui_ImplDX11_Init(m_d3d_device.Get(), m_d3d_context.Get());

		ImFontConfig font_cfg{};
		font_cfg.FontDataOwnedByAtlas = false;
		std::strcpy(font_cfg.Name, "Rubik");

		m_font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(font_rubik), sizeof(font_rubik), 17.f, &font_cfg);

		ImFontConfig chinese_cfg{};
		chinese_cfg.MergeMode = true;
		chinese_cfg.PixelSnapH = true;

		ImGui::GetIO().Fonts->AddFontFromFileTTF(
			"C:\\Windows\\Fonts\\msyh.ttc",
			17.f,
			&chinese_cfg,
			ImGui::GetIO().Fonts->GetGlyphRangesChineseFull()
		);

		merge_icon_with_latest_font(14.f, false);

		m_monospace_font = ImGui::GetIO().Fonts->AddFontDefault();

		ImGui::GetIO().Fonts->Build();

		load_texture(m_d3d_device.Get());

		g_gui.dx_init();
	}

    void renderer::pre_reset()
    {
        if (!m_init) return;
        release_targets();
        ImGui_ImplDX11_InvalidateDeviceObjects();
    }

    void renderer::post_reset()
    {
        if (!m_init) return;
        ImGui_ImplDX11_CreateDeviceObjects();
        create_targets();
    }

	void renderer::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
        std::lock_guard lock(m_mutex);
		if (msg == WM_KEYUP && wparam == VK_END)
		{
			g_running = false;
		}

		if (m_init && ImGui::GetCurrentContext())
		{
			ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
		}

		g_gui.wndproc(hwnd, msg, wparam, lparam);
	}

	void renderer::merge_icon_with_latest_font(float font_size, bool FontDataOwnedByAtlas)
	{
		static const ImWchar icons_ranges[3] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.FontDataOwnedByAtlas = FontDataOwnedByAtlas;

		ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)font_icons, sizeof(font_icons), font_size, &icons_config, icons_ranges);
	}

	void renderer::load_texture(ID3D11Device*)
	{
		g_gui.load_textures();
	}

	ImTextureID renderer::upload_rgba(const unsigned char* pixels, int width, int height)
	{
		if (!m_d3d_device || !pixels || width <= 0 || height <= 0)
			return 0;
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = static_cast<UINT>(width);
		desc.Height = static_cast<UINT>(height);
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		D3D11_SUBRESOURCE_DATA data{pixels, static_cast<UINT>(width * 4), 0};
		comptr<ID3D11Texture2D> texture;
		comptr<ID3D11ShaderResourceView> view;
		if (FAILED(m_d3d_device->CreateTexture2D(&desc, &data, texture.GetAddressOf())) ||
			FAILED(m_d3d_device->CreateShaderResourceView(texture.Get(), nullptr, view.GetAddressOf())))
			return 0;
		auto* raw_view = view.Get();
		m_textures.push_back(std::move(view));
		return (ImTextureID)(intptr_t)raw_view;
	}
	void renderer::process_hwnd()
	{
		bool window_focus = false;
		while (window_focus == false)
		{
			DWORD foreground_window_process_id;
			GetWindowThreadProcessId(GetForegroundWindow(), &foreground_window_process_id);

			if (GetCurrentProcessId() == foreground_window_process_id)
			{

				auto id = GetCurrentProcessId();
				auto handle = GetCurrentProcess();
				m_window = GetForegroundWindow();

				RECT TempRect;
				GetWindowRect(m_window, &TempRect);
				auto width = TempRect.right - TempRect.left;
				auto height = TempRect.bottom - TempRect.top;

				char TempTitle[MAX_PATH];
				GetWindowText(m_window, TempTitle, sizeof(TempTitle));
				auto title = TempTitle;

				char TempClassName[MAX_PATH];
				GetClassName(m_window, TempClassName, sizeof(TempClassName));
				auto class_name = TempClassName;

				char TempPath[MAX_PATH];
				GetModuleFileNameEx(handle, NULL, TempPath, sizeof(TempPath));
				auto path = TempPath;

				window_focus = true;
			}
		}
	}
}
