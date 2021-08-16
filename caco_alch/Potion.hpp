#pragma once
#include <unordered_set>
#include "using.h"
#include "Ingredient.hpp"
#include "GameSettings.hpp"

namespace caco_alch {

	/**
	 * @function get_common_effects(const std::vector<Ingredient>&)
	 * @brief Retrieve a list of common effects with the magnitude of the strongest effect of that type. (Base Magnitude)
	 * @param ingr	- List of ingredients
	 * @returns EffectList
	 */
	inline EffectList get_common_effects(const std::vector<Ingredient>& ingr)
	{
		if ( ingr.size() > 4 ) throw std::exception("Too many ingredients! (Max 4)"); else if ( ingr.size() < 2 ) throw std::exception("Not enough ingredients! (Min 2)");
		EffectList common, tmp;
		const auto is_duplicate{ [](EffectList& target, const Effect& fx) {
			for ( auto it{ target.begin() }; it != target.end(); ++it )
				if ( it->_name == fx._name ) // if effect names are the same, consider it a duplicate even though the magnitudes might be different
					return it;
			return target.end();
		} };

		for ( auto& i : ingr )
			for ( auto& it : i._effects ) {
				if ( auto dupl{ is_duplicate(tmp, it) }; dupl != tmp.end() ) { // if effect is a duplicate, push it to the common effects vector
					if ( auto current{ is_duplicate(common, it) }; current == common.end() ) {
						if ( it._magnitude < dupl->_magnitude )
							common.push_back(*dupl); // if effect is not in the common list yet, add it
						else
							common.push_back(it);
					}
					else {
						if ( it._magnitude > current->_magnitude)	// Set magnitude to largest (base_mag)
							( *current )._magnitude = it._magnitude;
						if ( it._duration > current->_duration )	// Set duration to largest (base_dur)
							( *current )._duration = it._duration;
					}
				}
				else
					tmp.emplace_back(it);
			}
		return common;
	}

	struct PotionBase : ObjectBase {
	protected:
		EffectList _base_fx; ///< @brief The base effects of a potion, before calculating the potion magnitude.

	public:
		PotionBase(const std::string& name) : ObjectBase(name), _base_fx{ } {}
		PotionBase(const std::string& name, const EffectList& effects) : ObjectBase(name), _base_fx{ effects } {}
		PotionBase(const std::string& name, std::vector<Ingredient>&& ingredients) : ObjectBase(name), _base_fx{ get_common_effects(std::move(ingredients)) } {}
		PotionBase(const std::string& name, const std::vector<Ingredient>& ingredients) : ObjectBase(name), _base_fx{ get_common_effects(ingredients) } {}
	};

	class Potion : public PotionBase {
		EffectList _fx; ///< @brief The final effects of a potion, these are applied when using it.

		/**
		 * @function calculate_stats(const EffectList, GameSettings&)
		 * @brief Calculate a potion's final stats, including the players level & perks, etc.
		 * @param base	- Base effects.
		 * @param gs	- Ref of a GameSettings instance.
		 * @returns EffectList
		 */
		static EffectList calculate_stats(const EffectList& base, GameSettings& gs)
		{
			EffectList vec;
			for ( auto& it : base ) {
				double magnitude{ gs.calculate_magnitude(it._magnitude) };
				unsigned int duration{ it._duration };
				vec.push_back(Effect{ it._name, magnitude, duration });
			}
			return vec;
		}

	public:
		Potion(const std::string& name, std::vector<Ingredient>&& ingredients, GameSettings gs) : PotionBase(name, std::forward<std::vector<Ingredient>>(ingredients)), _fx{ calculate_stats(_base_fx, gs) } {}
		Potion(const std::string& name, const std::vector<Ingredient>& ingredients, GameSettings gs) : PotionBase(name, ingredients), _fx{ calculate_stats(_base_fx, gs) } {}
		Potion(std::vector<Ingredient>&& ingredients, GameSettings gs) : PotionBase("Potion", std::forward<std::vector<Ingredient>>(ingredients)), _fx{ calculate_stats(_base_fx, gs) } {}
		Potion(const std::vector<Ingredient>& ingredients, GameSettings gs) : PotionBase("Potion", ingredients), _fx{ calculate_stats(_base_fx, gs) } {}

		std::string name() const { return _name; }
		EffectList effects() const { return _fx; }

		friend std::ostream& operator<<(std::ostream& os, const Potion& p)
		{
			os << '\t' << p._name;
			for ( auto& fx : p._fx )
				os << "\t\t" << fx._magnitude << '\t' << fx._duration << '\n';
			return os;

		}
	};
}