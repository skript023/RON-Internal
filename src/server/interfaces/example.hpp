std::vector<std::pair<std::string, std::string>> LoadFromJson(std::string) {};
namespace big
{
	using cookies_id = uint32_t;

	enum class eCookiesStatus
	{
		VALID,
		REUSE,
		BLACKLISTED,
		USAGE_TIMEOUT,
		BANNED
	};

	struct Cookies
	{
		Cookies() = default;
		Cookies(cookies_id id, std::string const& name, std::string const& token, bool permanent, eCookiesStatus status):
			m_id(id),
			m_name(name),
			m_po_token(token),
			m_usage_time(std::chrono::high_resolution_clock::now() + 10min),
			m_is_permanent_ban(permanent),
			m_status(status),
			reuse_count(0)
		{}
		cookies_id m_id;
		std::string m_name;
		std::string m_po_token;
		std::chrono::high_resolution_clock::time_point m_usage_time;
		std::chrono::high_resolution_clock::time_point m_duration;
		bool m_is_permanent_ban;
		eCookiesStatus m_status;
		uint32_t reuse_count;

		bool empty()
		{
			return m_name.empty() || m_po_token.empty();
		}
	};

	class CookiesManager
	{
		std::map<cookies_id, Cookies> m_cookies{};
	public:
		explicit CookiesManager()
		{
			int i = 0;
			auto file = LoadFromJson("./cookies/cookies.json");

			for (auto& cookie : file)
			{
				this->insert(i, Cookies(i, cookie.first, cookie.second, false, eCookiesStatus::VALID));

				i++;
			}
		};
		~CookiesManager() noexcept = default;

		CookiesManager(CookiesManager const&) = delete;
		CookiesManager(CookiesManager&&) = delete;

		CookiesManager& operator=(CookiesManager const&) = delete;
		CookiesManager& operator=(CookiesManager&&) = delete;

		void insert(cookies_id id, Cookies blacklist)
		{
			if (m_cookies.insert({ id, blacklist }).second)
			{
				LOG(INFO) << "Cookies already in blacklisted";
			}
		}

		bool find(cookies_id id, Cookies* blacklist)
		{
			if (auto it = m_cookies.find(id); it != m_cookies.end())
			{
				*blacklist = it->second;

				LOG(INFO) << "Customer data found";

				return true;
			}

			LOG(INFO) << "Customer data not found";

			return false;
		}

		bool update(cookies_id id, Cookies customer)
		{
			if (auto it = m_cookies.find(id); it != m_cookies.end())
			{
				it->second = customer;

				return true;
			}

			return false;
		}

		bool remove(cookies_id id)
		{
			if (auto it = m_cookies.find(id); it != m_cookies.end())
			{
				auto result = m_cookies.erase(it);

				LOG(INFO) << "Cookies has been cleared at index " << result->first;

				return true;
			}

			LOG(INFO) << "Failed clear blacklist";

			return false;
		}

		bool is_blacklisted(cookies_id id)
		{
			Cookies blacklisted;
			if (this->find(id, &blacklisted))
			{
				if (blacklisted.m_is_permanent_ban || blacklisted.m_duration > std::chrono::high_resolution_clock::now())
					return true;

				blacklisted.m_usage_time = std::chrono::high_resolution_clock::now() + 10min;
				blacklisted.m_status = eCookiesStatus::REUSE;
				blacklisted.reuse_count++;

				this->update(id, blacklisted);
			}

			return false;
		}

		bool is_timeout(cookies_id id)
		{
			Cookies cookies;
			if (this->find(id, &cookies))
			{
				if (cookies.m_usage_time < std::chrono::high_resolution_clock::now())
				{
					cookies.m_duration = std::chrono::high_resolution_clock::now() + 15min;
					cookies.m_status = eCookiesStatus::USAGE_TIMEOUT;

					this->update(id, cookies);

					return true;
				}
			}

			return false;
		}

		Cookies use_cookies_ordered()
		{
			for (auto& [id, cookies] : m_cookies)
			{
				if (is_timeout(id)) continue;

				if (is_blacklisted(id)) continue;

				return cookies;
			}

			return {};
		}

		void remove_blacklisted()
		{
			for (auto& [id, cookies] : m_cookies)
			{
				cookies.m_is_permanent_ban = false;
				cookies.m_usage_time = std::chrono::high_resolution_clock::now() + 10min;
				cookies.m_duration = std::chrono::high_resolution_clock::now();
				cookies.m_status = eCookiesStatus::VALID;
				cookies.reuse_count = 0;

				if (this->update(id, cookies))
				{
					LOG(INFO) << "Reset cookies index : " << id;
				}
			}
		}

		void reset()
		{
			m_cookies.clear();
		}
	};
}