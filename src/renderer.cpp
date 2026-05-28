#include "gui.hpp"
#include "logger.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "file_manager.hpp"
#include "fonts/font_list.hpp"
#include "fonts/icon_list.hpp"

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>

#include "ui/canvas.hpp"
#include <graphic/graphic_manager.hpp>
#include "utility/unreal_engine_utility.hpp"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace big
{
#ifdef UNIVERSAL
	renderer::renderer()
	{
		if (graphic_manager::get_swapchain(graphics_api::directx11) == eInitializationStatus::SUCCESS)
		{
			g_gui.init();
		}

		g_renderer = this;
	}
#else
	renderer::renderer() : m_dxgi_swapchain(*g_pointers->m_swapchain)
	{
		g_gui.init();
		if (!init())
		{
			throw std::runtime_error("Failed init renderer");
		}


		g_renderer = this;
	}
#endif // UNIVERSAL
	renderer::~renderer()
	{
		ImGui_ImplWin32_Shutdown();
		ImGui_ImplDX11_Shutdown();
		ImGui::DestroyContext();

		m_d3d_device->Release();
		m_d3d_context->Release();
		m_d3d_render_target->Release();

		g_renderer = nullptr;
	}

	void renderer::on_present()
	{
		if (g_settings.window.mouse_active)
		{
			ImGui::GetIO().MouseDrawCursor = true;
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		}
		else
		{
			ImGui::GetIO().MouseDrawCursor = false;
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		for (const auto& cb : g_gui.m_dx_callbacks | std::views::values)
			cb();

		ImGui::Render();
		m_d3d_context->OMSetRenderTargets(1, &m_d3d_render_target, NULL);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
#ifdef UNIVERSAL
	bool renderer::init(IDXGISwapChain* swapchain)
	{
		if (!m_init)
		{
			m_dxgi_swapchain = swapchain;

			void* d3d_device{};
			if (SUCCEEDED(m_dxgi_swapchain->GetDevice(__uuidof(ID3D11Device), &d3d_device)))
			{
				m_d3d_device.Attach(static_cast<ID3D11Device*>(d3d_device));

				ID3D11Texture2D* m_back_buffer;
				m_dxgi_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_back_buffer);
				if (m_back_buffer && m_d3d_device)
				{
					m_d3d_device->CreateRenderTargetView(m_back_buffer, NULL, &m_d3d_render_target);
				}
				else
				{
					return false;
				}

				m_back_buffer->Release();
			}
			else
			{
				return false;
			}

			m_d3d_device->GetImmediateContext(m_d3d_context.GetAddressOf());

			this->imgui_init();

			this->m_init = true;
			g_gui.script_init();
			LOG(INFO) << "Swapchain initialized.";
		}

		return true;
	}
#else
	bool renderer::init()
	{
		if (!m_init)
		{
			void* d3d_device{};
			if (SUCCEEDED(m_dxgi_swapchain->GetDevice(__uuidof(ID3D11Device), &d3d_device)))
			{
				m_d3d_device.Attach(static_cast<ID3D11Device*>(d3d_device));

				ID3D11Texture2D* m_back_buffer;
				m_dxgi_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_back_buffer);
				if (m_back_buffer && m_d3d_device)
				{
					m_d3d_device->CreateRenderTargetView(m_back_buffer, NULL, &m_d3d_render_target);
				}
				else
				{
					return false;
				}

				m_back_buffer->Release();
			}
			else
			{
				return false;
			}

			m_d3d_device->GetImmediateContext(m_d3d_context.GetAddressOf());

			this->imgui_init();

			this->m_init = true;
			g_gui.script_init();
			LOG(INFO) << "Swapchain initialized.";
		}

		return true;
	}
#endif // UNIVERSAL

	void renderer::imgui_init()
	{
		auto file_path = g_file_manager.get_project_file("./imgui.ini").get_path();

		ImGuiContext* ctx = ImGui::CreateContext();

		static std::string path = file_path.make_preferred().string();
		ctx->IO.IniFilename = path.c_str();

		ImGui_ImplWin32_Init(g_pointers->m_hwnd);
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
		if (m_d3d_render_target)
		{
			m_d3d_context->OMSetRenderTargets(0, 0, 0);
			m_d3d_render_target->Release();
		}

		ImGui_ImplDX11_InvalidateDeviceObjects();
	}

	void renderer::post_reset()
	{
		ImGui_ImplDX11_CreateDeviceObjects();

		ID3D11Texture2D* m_back_buffer;
		m_dxgi_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_back_buffer);

		if (m_back_buffer)
		{
			m_d3d_device->CreateRenderTargetView(m_back_buffer, NULL, &m_d3d_render_target);
		}

		m_back_buffer->Release();
		m_d3d_context->OMSetRenderTargets(1, &m_d3d_render_target, NULL);
	}

	void renderer::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg == WM_KEYUP && wparam == VK_INSERT)
		{
			//Persist and restore the cursor position between menu instances.
			static POINT cursor_coords{};
			if (canvas::is_opened())
			{
				GetCursorPos(&cursor_coords);
			}
			else if (cursor_coords.x + cursor_coords.y != 0)
			{
				SetCursorPos(cursor_coords.x, cursor_coords.y);
			}
		}
		if (msg == WM_KEYUP && wparam == VK_END)
		{
			g_running = false;
		}

		canvas::check_for_input();
		canvas::handle_input();

		if (canvas::is_opened())
		{
			ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
		}
	}

	void renderer::merge_icon_with_latest_font(float font_size, bool FontDataOwnedByAtlas)
	{
		static const ImWchar icons_ranges[3] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.FontDataOwnedByAtlas = FontDataOwnedByAtlas;

		g_settings.window.font_icon = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)font_icons, sizeof(font_icons), font_size, &icons_config, icons_ranges);
	}

	void renderer::load_texture(ID3D11Device* device)
	{
		for (auto& callback : g_gui.m_texture_callbacks)
		{
			callback(device);
		}
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
