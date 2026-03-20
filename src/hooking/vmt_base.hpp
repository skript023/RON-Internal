#pragma once

namespace big
{
    class vmt_base
    {
    protected:
        const std::string_view m_name;
        bool m_enabled{ false };

    public:
        vmt_base(const std::string_view name)
            : m_name(name)
        {
            m_vmt_hooks.push_back(this);
        }

        virtual ~vmt_base() = default;

        vmt_base(const vmt_base&) = delete;
        vmt_base(vmt_base&&) noexcept = delete;
        vmt_base& operator=(const vmt_base&) = delete;
        vmt_base& operator=(vmt_base&&) noexcept = delete;

        const std::string_view name() const
        {
            return m_name;
        }

        bool is_enabled() const
        {
            return m_enabled;
        }

        virtual bool enable() = 0;
        virtual bool disable() = 0;
        virtual void* get_original_ptr() = 0;

    public:

        template<auto detour_function>
        struct helper
        {
            static inline vmt_base* m_hook{};
        };

        template<auto detour_function>
        static void add(vmt_base* hook)
        {
            helper<detour_function>::m_hook = hook;
        }

        template<auto detour_function, typename T>
        static T* get()
        {
            return reinterpret_cast<T*>(helper<detour_function>::m_hook);
        }

        template<auto detour_function>
        static auto get_original()
        {
            return static_cast<decltype(detour_function)>(
                helper<detour_function>::m_hook->get_original_ptr()
                );
        }

        static std::vector<vmt_base*>& hooks()
        {
            return m_vmt_hooks;
        }

        static bool enable_all()
        {
            for (auto* hook : m_vmt_hooks)
                hook->enable();
            return true;
        }

        static bool disable_all()
        {
            for (auto* hook : m_vmt_hooks)
                hook->disable();
            return true;
        }

    private:
        inline static std::vector<vmt_base*> m_vmt_hooks;
    };
}