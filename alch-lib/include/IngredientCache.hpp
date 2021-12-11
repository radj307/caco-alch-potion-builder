#pragma once
#include <set>
#include "using.h"
#include "Ingredient.hpp"
#include "Format.hpp"

namespace caco_alch {

	/**
	 * @struct IngredientCache
	 * @brief Contains an ingredient list in convenient format.
	 * @tparam Sort	- The sorting predicate function used by the set.
	 * @tparam Cont	- The type of set containing the list of ingredients.
	 */
	template<class Cont>
	struct IngredientCache {
		using Container = Cont;
		Container _ingr; ///< @brief This is the live cache

		explicit IngredientCache() : _ingr{} {}
		explicit IngredientCache(Container&& ingr_cont) : _ingr{ std::move(ingr_cont) } {}
		IngredientCache(IngrList&& ingr_cont) : _ingr{ sort(std::move(ingr_cont)).first } {}

		IngredientCache& operator=(IngredientCache&& cache)
		{
			_ingr = std::move(cache._ingr);
			return *this;
		}
		IngredientCache& operator=(const IngredientCache& cache)
		{
			_ingr = cache._ingr;
			return *this;
		}

		auto begin() const
		{
			return _ingr.begin();
		}
		auto end() const
		{
			return _ingr.end();
		}
		auto rbegin() const
		{
			return _ingr.rbegin();
		}
		auto rend() const
		{
			return _ingr.rend();
		}

		/**
		 * @function sort(IngrList&&)
		 * @brief Sort an IngrList into a Container.
		 * @param ingr_cont	- Ingredient List rvalue.
		 * @returns std::pair<Container, unsigned>
		 *			Container	- A sorted container with all of the ingredients from ingr_cont, except duplicates.
		 *			unsigned	- The number of duplicates found.
		 */
		static std::pair<Container, unsigned> sort(IngrList&& ingr_cont)
		{
			Container list{};
			unsigned count{ 0u };
			for (auto& it : ingr_cont)
				if (const auto [existing, state] { list.insert(it) }; !state)
					++count;
			return{ list, count };
		}

		/**
		 * @function get(const std::string&, const bool = false)
		 * @brief Retrieve an iterator to the first position in the cache matching a given name.
		 * @param name			- The name to search for
		 * @param off			- The position to begin searching at.
		 * @param only_effects	- When true, only returns ingredients with an effect matching the given search, else only returns ingredients with names matching the given search.
		 * @returns Container::iterator
		 */
		[[nodiscard]] typename Container::iterator get(const std::string& name, const typename Container::iterator& off, const bool only_effects = false)
		{
			return std::find_if(off, _ingr.end(), [&name, &only_effects](const Ingredient& i) -> bool {
				if (only_effects)
					return std::any_of(i._effects.begin(), i._effects.end(), [&name, this](const Effect& fx) { return pred(str::tolower(fx._name), name); });
				return i._name == name;
			});
		}

		/**
		 * @function clear()
		 * @brief Clears the internal cache by moving it to a Container that is returned from this function.
		 * @returns Container
		 */
		Container clear() const
		{
			auto tmp{ std::move(_ingr) };
			_ingr = Container{};
			return tmp;
		}
		/**
		 * @brief Copy the cache to an IngrList and return it.
		 * @returns IngrList
		 */
		[[nodiscard]] IngrList getList() const
		{
			IngrList vec;
			vec.reserve(_ingr.size());
			for (auto& it : _ingr)
				vec.emplace_back(it);
			vec.shrink_to_fit();
			return vec;
		}
		/**
		 * @brief Copy the cache to a SortedIngrList and return it.
		 * @returns SortedIngrList
		 */
		[[nodiscard]] SortedIngrList getSortedList() const
		{
			SortedIngrList vec;
			for (auto& it : _ingr)
				vec.insert(it);
			return vec;
		}
		/**
		 * @brief Check if the ingredient cache is empty.
		 * @returns bool
		 */
		[[nodiscard]] bool empty() const
		{
			return _ingr.empty();
		}
		/**
		 * @brief Set the ingredient cache by moving a given ingredient container.
		 * @param cont	- Container to move.
		 * @returns IngredientCache&
		 */
		IngredientCache& operator=(Container&& cont)
		{
			_ingr = std::move(cont);
			return *this;
		}
	};
	struct RegistryType : IngredientCache<std::set<Ingredient, _internal::less<Ingredient>>> {

		explicit RegistryType() : IngredientCache() {}
		explicit RegistryType(Container&& ingr_cont) : IngredientCache(std::move(ingr_cont)) {}
		RegistryType(IngrList&& ingr_cont) : IngredientCache(std::move(ingr_cont)) {}

		enum class FindType {
			BOTH,
			INGR,
			EFFECT
		};

		template<class PredT>
		Container find(std::string name, PredT&& pred, const FindType& search = FindType::BOTH) const
		{
			name = str::tolower(name);

			Container cont;

			switch (search) {
			case FindType::INGR:
				for (auto& it : _ingr) {
					if (pred(str::tolower(it._name), name))
						cont.insert(it);
				}
				break;
			case FindType::BOTH:
				for (auto& it : _ingr) {
					if (pred(str::tolower(it._name), name) || std::any_of(it._effects.begin(), it._effects.end(), [&pred, &name, this](auto&& fx) {
						return pred(str::tolower(fx._name), name);
						}))
						cont.insert(it);
				}
				break;
			case FindType::EFFECT:
				for (auto& it : _ingr)
					if (std::any_of(it._effects.begin(), it._effects.end(), [&pred, &name, this](auto&& fx) {
						return pred(str::tolower(fx._name), name);
						}))
						cont.insert(it);
			}
			return cont;
		}
		template<class PredT>
		RegistryType find_and_duplicate(std::string name, PredT&& pred, const FindType& search = FindType::BOTH) const
		{
			name = str::tolower(name);

			Container cont;

			switch (search) {
			case FindType::INGR:
				for (auto& it : _ingr)
					if (pred(str::tolower(it._name), name))
						cont.insert(it);
				break;
			case FindType::BOTH:
				for (auto& it : _ingr)
					if (pred(str::tolower(it._name), name) || std::any_of(it._effects.begin(), it._effects.end(), [&pred, &name, this](auto&& fx) { return pred(str::tolower(fx._name), name); }))
						cont.insert(it);
				break;
			case FindType::EFFECT:
				for (auto& it : _ingr)
					if (std::any_of(it._effects.begin(), it._effects.end(), [&pred, &name, this](auto&& fx) {
						return pred(str::tolower(fx._name), name);
						}))
						cont.insert(it);
			}
			return RegistryType{ std::move(cont) };
		}
		Container::const_iterator find_best_fit(std::string name, const FindType& search = FindType::INGR) const
		{
			std::vector<Container::const_iterator> vec;

			switch (search) {
			case FindType::INGR:
				for (auto it{ _ingr.begin() }; it != _ingr.end(); ++it) {
					const auto name_lc{ str::tolower(it->_name) };
					if (name_lc == name)
						return it;
					else if ((name_lc.find(name) < name_lc.size()))
						vec.emplace_back(it);
				}
				break;
			case FindType::BOTH:
				for (auto it{ _ingr.begin() }; it != _ingr.end(); ++it) {
					const auto name_lc{ str::tolower(it->_name) };
					if (name_lc == name)
						return it;
					else if ((name_lc.find(name) < name_lc.size()))
						vec.emplace_back(it);
					else for (auto fx{ it->_effects.begin() }; fx != it->_effects.end(); ++fx) {
						const auto lc{ str::tolower(fx->_name) };
						if (lc == name)
							return it;
						else if ((lc.find(name) < lc.size()))
							vec.emplace_back(it);
					}
				}
				break;
			case FindType::EFFECT:
				for (auto it{ _ingr.begin() }; it != _ingr.end(); ++it) {
					for (auto fx{ it->_effects.begin() }; fx != it->_effects.end(); ++fx) {
						const auto lc{ str::tolower(fx->_name) };
						if (lc == name)
							return it;
						else if ((lc.find(name) < lc.size()))
							vec.emplace_back(it);
					}
				}
				break;
			}
			if (vec.empty())
				return _ingr.end();
			return vec.front();
		}

		explicit operator Container() const { return _ingr; }
	};
}