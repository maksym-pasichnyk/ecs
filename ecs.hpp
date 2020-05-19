#pragma once

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <limits>
#include <vector>
#include <map>

namespace ecs {
	enum class entity : uint32_t {
		null = std::numeric_limits<uint32_t>::max()
	};

	template <typename entity, typename component>
	struct storage_entry {
		std::map<entity, component> components{};
	};

	template <typename entity, typename... components>
	struct storage : storage_entry<entity, components>... {
		template <typename component>
		inline storage_entry<entity, component>& get() {
			return *static_cast<storage_entry<entity, component>*>(this);
		}
	};

	template<typename... components>
	struct registry {
		inline registry() noexcept {
			entities.reserve(1000000);
		}

		registry(const registry&) = delete;
		registry& operator=(const registry&) = delete;
		registry(registry&&) = delete;
		registry& operator=(registry&&) = delete;

		inline entity create() noexcept {
			if (destroyed != entity::null) {
				entity e = destroyed;
				destroyed = entities[size_t(e)];
				return e;
			}

			return entities.emplace_back(entity(entities.size()));
		}

		inline void destroy(entity e) noexcept {
			assert(valid(e));

			entities[size_t(e)] = destroyed;
			destroyed = e;

			(pools.template get<components>().components.erase(e), ...);
		}

		inline bool valid(entity e) {
			return entities[size_t(e)] == e;
		}

		template <typename Compoenent, typename... Args>
		inline void emplace(entity e, Args&&... args) noexcept {
			assert(valid(e));

			pools.template get<Compoenent>().components.try_emplace(e, std::forward<Args>(args)...);
		}

		template <typename... Components, typename Execution, typename Fn>
		inline void for_each(Execution&& execution, Fn&& fn) noexcept {
			std::for_each(std::forward<Execution>(execution), std::rbegin(entities), std::rend(entities),
				[this, fn = std::forward<Fn>(fn)](entity e) {
					if (valid(e)) {
						if constexpr (sizeof...(Components) != 0) {
							apply<Components...>(e, fn, pools.template get<Components>().components.find(e)...);
						} else {
							fn(e);
						}
					}
				}
			);
		}
	private:
		template <typename... Components, typename Fn, typename... Its>
		inline void apply(entity e, Fn&& fn, Its&&... its) noexcept {
			if (((its != pools.template get<Components>().components.end()) && ...)) {
				fn(e, its->second...);
			}
		}

		storage<entity, components...> pools;
		std::vector<entity> entities{};
		entity destroyed = entity::null;
	};

	template <typename... components>
	struct iterator {
		template<typename Execution, typename Registry, typename Fn>
		inline static void for_each(Execution&& execution, Registry& ctx, Fn&& fn) {
			ctx.template for_each<components...>(std::forward<Execution>(execution), std::forward<Fn>(fn));
		}
	};
}
