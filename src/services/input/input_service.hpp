#pragma once

namespace big
{
	using input_callback_t = std::function<void(std::string const&)>;

	class input_service
	{
		friend class view;
	public:
		explicit input_service() = default;
		~input_service() noexcept = default;

		void show(std::string title);
		void show(std::string const& title, input_callback_t callback);
		void hide();
		void toggle() { if (is_open()) hide(); else open(); }
		void callback(input_callback_t callback);
		void execute();
		bool is_open() { return g_settings.window.input; }

	private:
		void open();

		bool m_mouse_before = false;
		char m_input[64];
		char m_title[64] = "Input Here";
		std::stack<input_callback_t> m_callback;
		std::recursive_mutex m_mutex;
	};

	inline input_service g_input_service{};
}