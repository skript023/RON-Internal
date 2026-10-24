#pragma once
#include <SDK/Basic.hpp>
#include <SDK/CoreUObject_classes.hpp>

namespace big
{
	// A UObject pointer kept across frames, such as a weapon whose stats were changed or the target an inspector shows.
	// get() reads null once the engine frees the object or gives its slot to another one, so a level change never
	// leaves a write going through a stale pointer.
	template <typename T>
	class weak_object
	{
	public:
		weak_object() = default;
		weak_object(T* object) :
		    m_object(object),
		    m_index(object ? object->Index : -1)
		{
		}

		T* get() const
		{
			if (!m_object || m_index < 0)
				return nullptr;
			if (SDK::UObject::GObjects->GetByIndex(m_index) != m_object)
				return nullptr;
			return m_object;
		}

		explicit operator bool() const
		{
			return get() != nullptr;
		}

		// For comparing and as a map key only; never dereference it.
		const void* address() const
		{
			return m_object;
		}

		bool operator==(const weak_object& other) const
		{
			return m_object == other.m_object && m_index == other.m_index;
		}

	private:
		T* m_object{};
		std::int32_t m_index = -1;
	};
}
