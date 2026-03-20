#pragma once

namespace big
{
	class detour_base
	{
	private:

	protected:
		const std::string_view m_name;
		bool m_enabled;
	public:
		detour_base(const std::string_view name);
		virtual ~detour_base() = default;
		detour_base(const detour_base&) = delete;
		detour_base(detour_base&&) noexcept = delete;
		detour_base& operator=(const detour_base&) = delete;
		detour_base& operator=(detour_base&&) noexcept = delete;

		const std::string_view name() const
		{
			return m_name;
		}
		inline bool is_enabled() const
		{
			return m_enabled;
		}

		virtual bool enable() = 0;
		virtual bool disable() = 0;
		virtual void* get_original_ptr() = 0;

	public:
		template<auto detour_function>
		struct detour_helper
		{
			static inline detour_base* m_hook;
		};

		template<auto detour_function>
		static void add(detour_base* hook);

		template<auto detour_function, typename T>
		static T* get();

		template<auto detour_function>
		static auto get_original();

		static std::vector<detour_base*>& hooks();

		static bool enable_all();
		static bool disable_all();

	private:
		inline static std::vector<detour_base*> m_detour_bases;

	};

	template<auto detour_function>
	inline void detour_base::add(detour_base* hook)
	{
		detour_helper<detour_function>::m_hook = hook;
	}

	template<auto detour_function, typename T>
	inline T* detour_base::get()
	{
		return reinterpret_cast<T*>(detour_helper<detour_function>::m_hook);
	}

	template<auto detour_function>
	inline auto detour_base::get_original()
	{
		return static_cast<decltype(detour_function)>(detour_helper<detour_function>::m_hook->get_original_ptr());
	}
}