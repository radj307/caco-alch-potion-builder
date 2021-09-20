#pragma once
#include <set>
#include <iomanip>
#include <strconv.hpp>

#include <using.h>
#include <ColorAPI.hpp>
#include <Potion.hpp>
#include <Ingredient.hpp>
#include <ColorConfigLoader.hpp>

namespace caco_alch {
	/**
	 * @struct OutputFormat
	 * @brief Contains all of the variables used within the Format struct.
	 */
	struct OutputFormat {
		bool
			_flag_quiet,
			_flag_verbose,
			_flag_exact,
			_flag_all,
			_flag_export,
			_flag_reverse,
			_flag_color,
			_flag_smart;
		unsigned
			_indent,
			_precision;
		ColorAPI _colors;

		explicit OutputFormat(const bool quiet, const bool verbose, const bool exact, const bool all, const bool fileExport, const bool reverse, const bool color, const bool smart, const size_t indent, const size_t precision, ColorAPI colors) :
			_flag_quiet{ quiet },
			_flag_verbose{ verbose },
			_flag_exact{ exact },
			_flag_all{ all },
			_flag_export{ fileExport },
			_flag_reverse{ reverse },
			_flag_color{ color },
			_flag_smart{ smart },
			_indent{ indent },
			_precision{ precision },
			_colors{ std::move(colors) }
		{}
		virtual ~OutputFormat() = default;

		[[nodiscard]] ColorAPI& colorizer() { return _colors; }
		[[nodiscard]] ColorAPI colorizer() const { return _colors; }

		[[nodiscard]] bool match(const std::string& objName, const std::string& searchName) const
		{
			return _flag_exact ? (objName == searchName) : (objName == searchName || objName.find(searchName) < objName.size());
		}

		[[nodiscard]] bool quiet() const { return _flag_quiet; }
		[[nodiscard]] bool verbose() const { return _flag_verbose; }
		[[nodiscard]] bool all() const { return _flag_all; }
		[[nodiscard]] bool file_export() const { return _flag_export; }
		[[nodiscard]] bool reverse_output() const { return _flag_reverse; }
		[[nodiscard]] bool doLocalCaching() const { return _flag_smart; }
		[[nodiscard]] size_t indent() const { return _indent; }
		[[nodiscard]] size_t precision() const { return _precision; }
		[[nodiscard]] bool color() const { return _flag_color; }
	};

	struct Formatter : OutputFormat {
		explicit Formatter(const file::ini::INI& ini, const opt::Params& args, const unsigned indent) : OutputFormat(
			args.check_flag('q'),
			args.check_flag('v'),
			args.check_flag('e'),
			args.check_flag('a'),
			args.check_flag('E'),
			args.check_flag('R'),
			args.check_flag('c'),
			args.check_flag('S'),
			indent,
			[&args]() { const auto val{ args.getv("precision") }; return val.has_value() && !val.value().empty() ? str::stoui(val.value()) : 2u; }(),
			std::move(loadColorConfig(ini))
		) {}

		/**
		 * @brief Split a given name by searched subwords.
		 * @param name				- The ingredient/effect name to split.
		 * @param search_strings	- The searched-for words to highlight.
		 * @returns std::tuple<std::string, std::string, std::string>
		 *\n		0	- This contains the characters that appear before the searched for string if found, otherwise contains the whole name.
		 *\n		1	- This contains the search string if found, otherwise is empty.
		 *\n		2	- This contains the characters that appear after the searched for string if found, otherwise is empty.
		 */
		[[nodiscard]] std::tuple<std::string, std::string, std::string> split_name(const std::string& name, const std::vector<std::string>& search_strings) const
		{
			if (!name.empty())
				for (auto& search_str : search_strings)
					if (const auto pos{ str::tolower(name).find(str::tolower(search_str)) }; str::pos_valid(pos))
						return { name.substr(0u, pos), name.substr(pos, search_str.size()), name.substr(pos + search_str.size()) };
			return{ name, {}, {} };
		}
		/**
		 * @brief Split a given name by a single searched subword.
		 * @param name			- The ingredient/effect name to split.
		 * @param search_str	- The searched-for word to highlight.
		 * @returns std::tuple<std::string, std::string, std::string>
		 *\n		0	- This contains the characters that appear before the searched for string if found, otherwise contains the whole name.
		 *\n		1	- This contains the search string if found, otherwise is empty.
		 *\n		2	- This contains the characters that appear after the searched for string if found, otherwise is empty.
		 */
		[[nodiscard]] std::tuple<std::string, std::string, std::string> split_name(const std::string& name, const std::string& search_str) const
		{
			return split_name(name, { search_str });
		}
		[[nodiscard]] std::tuple<std::string, std::string, std::string> split_name(const std::string& name, const std::optional<std::vector<std::string>>& search_strings) const
		{
			return search_strings.has_value() ? split_name(name, search_strings.value()) : std::make_tuple(name, std::string{}, std::string{});
		}
		/**
		 * @brief Convert an Ingredient's Effect array into a vector of Effects, and applies the _flag_quiet logic to omit any non-matching effect names.
		 * @param effects			- An Ingredient's Effect array.
		 * @param search_strings	- Searched-for subwords.
		 * @returns std::vector<Effect>
		 */
		[[nodiscard]] std::vector<Effect> vectorize_effects(const std::array<Effect, 4>& effects, const std::vector<std::string>& search_strings) const
		{
			std::vector<Effect> vec;
			vec.reserve(effects.size());
			for (auto fx{ effects.begin() }; fx != effects.end(); ++fx) {
				if (!_flag_quiet)
					vec.emplace_back(*fx);
				else if (const auto name{ str::tolower(fx->_name) }; std::any_of(search_strings.begin(), search_strings.end(), [this, &name](const std::string& search_str) {
					return match(name, search_str);
				}))
					vec.emplace_back(*fx);
			}
			vec.shrink_to_fit();
			return vec;
		}
		[[nodiscard]] std::vector<Effect> vectorize_effects(const std::array<Effect, 4>& effects) const
		{
			std::vector<Effect> vec;
			vec.reserve(effects.size());
			for (auto& fx : effects)
				vec.emplace_back(fx);
			vec.shrink_to_fit();
			return vec;
		}

		/**
		 * @brief Get a ColorSetter instance associated with the given effect's keywords, or if _flag_color is false, the default effect color.
		 * @param effect	- An Effect instance.
		 * @returns ColorAPI::ColorSetter
		 */
		[[nodiscard]] ColorAPI::ColorSetter getEffectColorizer(const Effect& effect) const
		{
			if (_flag_color) {
				if (!effect._keywords.empty()) {
					if (hasNegative(effect))
						return _colors.set(UIElement::EFFECT_NAME_NEGATIVE);
					if (hasPositive(effect))
						return _colors.set(UIElement::EFFECT_NAME_POSITIVE);
					if (!effect.hasKeyword(Keywords::KYWD_MagicInfluence))
						return _colors.set(UIElement::EFFECT_NAME_NEUTRAL);
				}
				// use fallback instead. (possible return values: 0,1,2)
				switch (hasKeywordTypeFallback(str::tolower(effect._name))) {
				case 0: // neutral
					return _colors.set(UIElement::EFFECT_NAME_NEUTRAL);
				case 1: // negative
					return _colors.set(UIElement::EFFECT_NAME_NEGATIVE);
				case 2: // positive
					return _colors.set(UIElement::EFFECT_NAME_POSITIVE);
				default:break;
				}
			}
			return _colors.set(UIElement::EFFECT_NAME_DEFAULT); // colors are disabled
		}

		/**
		 * @struct Indentation
		 * @brief Used to correctly increment & insert indentation strings in output streams.
		 */
		struct Indentation {
			char ch;
			size_t rep;
			std::string indent;

			Indentation() : ch{ ' ' }, rep{ 0u }, indent{ std::string(rep, ch) } {}
			Indentation(size_t rep, char ch = '\t') : ch{std::move(ch)}, rep{std::move(rep)}, indent{std::string(rep, ch)} {}

			Indentation getNext(const size_t increase_rep_by = 1u) const
			{
				return{ rep + increase_rep_by, ch };
			}

			Indentation operator()() const { return getNext(); }

			friend std::ostream& operator<<(std::ostream& os, const Indentation& obj)
			{
				os << obj.indent;
				return os;
			}
		};

		template<class T> struct ToStream {
			Formatter* fmt;
			T* obj;
			Indentation indent;
			std::optional<std::vector<std::string>> searched;
			std::streamsize suffix_indent_width;
			ColorAPI* colors;

			ToStream(const Formatter& format, const T& object, Indentation indentation, std::optional<std::vector<std::string>> search_strings = std::nullopt, std::streamsize suffixIndent = 25) : fmt{ &format }, obj{ &object }, indent{ std::move(indentation) }, searched{ std::move(search_strings) }, suffix_indent_width{ std::move(suffixIndent) }, colors{ &format._colors } {}

			struct PrintSplitName {
				std::string pre, highlight, post;
				const ColorAPI::ColorSetter& color, hlColor;
				PrintSplitName(std::tuple<std::string, std::string, std::string> splitName, const ColorAPI::ColorSetter& color, const ColorAPI::ColorSetter& highlight_color) : pre{ std::move(std::get<0>(splitName)) }, highlight{ std::move(std::get<1>(splitName)) }, post{ std::move(std::get<2>(splitName)) }, color{ color }, hlColor{ highlight_color } {}

				friend std::ostream& operator<<(std::ostream& os, const PrintSplitName& obj)
				{
					os << obj.color << obj.pre << color::reset << obj.hlColor << obj.highlight << color::reset << obj.color << obj.post << color::reset;
					return os;
				}
			};

			PrintSplitName split_name(const std::string& name, const UIElement& colorT) const
			{
				return PrintSplitName{ fmt->split_name(name, searched), colors->set(colorT), colors->set(UIElement::SEARCH_HIGHLIGHT) };
			}
			PrintSplitName split_name(const std::string& name, const ColorAPI::ColorSetter& colorT) const
			{
				return PrintSplitName{ fmt->split_name(name, searched), colorT, colors->set(UIElement::SEARCH_HIGHLIGHT) };
			}

			// KEYWORD OUTPUT
			friend std::enable_if_t<std::is_same_v<T, Keyword>, std::ostream&> operator<<(std::ostream& os, const ToStream& s)
			{
				if (s.fmt->_flag_export)
					os << "\t\t" << s.obj << '\n';
				else
					os << s.indent << colors->set(UIElement::KEYWORD) << obj->_name << color::reset << '\n';
				return os;
			}
			// EFFECT OUTPUT
			friend std::enable_if_t<std::is_same_v<T, Effect>, std::ostream&> operator<<(std::ostream& os, const ToStream& s)
			{
				if (s.fmt->_flag_export)
					os << '\t' << s.obj._name << "\n\t{\n\t\tmagnitude = " << s.obj._magnitude << "\n\t\tduration = " << s.obj._duration << '\n' << s.obj._keywords << "\t}\n";
				else {
					const Effect& fx{ s.obj };
					os << split_name(fx._name, fmt->getEffectColorizer(fx));

					// print split effect name
					const auto insert_num{ [&os, &s](const std::string& num, const ColorAPI::ColorSetter color, const std::streamsize indent) {
						if (indent > s.suffix_indent_width)
							os << std::setw(s.suffix_indent_width + 2u);
						else
							os << std::setw(s.suffix_indent_width - indent);
						os << ' ' << color << num;
						return num.size();
					} };

					auto size_factor{ s.obj._name.size() };

					if (fx._magnitude > 0.0 || s.fmt->_flag_all)
						size_factor = insert_num(str::to_string(fx._magnitude, s.fmt->_precision), colors->set(UIElement::EFFECT_MAGNITUDE), size_factor) + 10u;
					if (fx._duration > 0u || s.fmt->_flag_all) {
						insert_num(str::to_string(fx._duration, s.fmt->_precision), colors->set(UIElement::EFFECT_DURATION), size_factor);
						os << 's';
					}
					os << color::reset << '\n';
					if (s.fmt->_flag_verbose || s.fmt->_flag_all)
						for (auto& KYWD : fx._keywords)
							os << '\n' << ToStream(*s.fmt, KYWD, s.indent.getNext(), s.searched, s.suffix_indent_width);
				}
				return os;
			}
			// EFFECT VECTOR OUTPUT
			friend std::enable_if_t<std::is_same_v<T, std::vector<Effect>>, std::ostream&> operator<<(std::ostream& os, const ToStream& s)
			{
				for (auto& fx : s.obj)
					os << ToStream(*s.fmt, fx, s.indent, s.searched, s.suffix_indent_width) << '\n';
				return os;
			}
			// INGREDIENT OUTPUT
			friend std::enable_if_t<std::is_same_v<T, Ingredient>, std::ostream&> operator<<(std::ostream& os, const ToStream& s)
			{
				if (s.fmt->_flag_export) {
					os << s.obj._name << "\n{\n";
					for (auto& fx : s.obj._effects)
						os << ToStream(*s.fmt, fx, s.indent.getNext(), s.searched, s.suffix_indent_width) << '\n';
				}
				else {
					os << split_name(s.obj._name, UIElement::INGREDIENT_NAME) << '\n' << ToStream(*s.fmt, fmt->vectorize_effects(s.obj._effects));
				}
				return os;
			}
			// INGREDIENT LIST OUTPUT
			friend std::enable_if_t<std::is_same_v<T, IngrList>, std::ostream&> operator<<(std::ostream& os, const ToStream& s)
			{
				for (auto& it : s.obj)
					os << ToStream(*s.fmt, it, s.indent.getNext(), s.searched, s.suffix_indent_width) << '\n';
				return os;
			}
		};
	};

	/**
	 * @struct Format
	 * @brief Provides formatting information for some output stream methods in the Alchemy class.
	 */
	struct Format : OutputFormat {
		/**
		 * @constructor Format(const bool = false, const bool = true, const bool = false, const bool = false, const size_t = 3u, const size_t = 2u, const unsigned short = color::_f_white)
		 * @brief Default Constructor
		 * @param quiet				- When true, only includes effects that match part of the search string in results.
		 * @param verbose			- When true, includes additional information about an effect's magnitude and duration.
		 * @param exact				- When true, only includes exact matches in results.
		 * @param all				- When true, includes all additional information in results.
		 * @param file_export		- When true, output is formatted in the registry format to allow piping output to a file, then back in again.
		 * @param reverse_output	- When true, prints output in the opposite order as it would normally be printed. (Alphabetical by default)
		 * @param allow_color_fx	- When true, prints effect colors where possible.
		 * @param use_local_cache	- When true, the alchemy instance will cache the list before parsing additional arguments.
		 * @param indent			- How many space characters to include before ingredient names. This is multiplied by 2 for effect names.
		 * @param precision			- How many decimal points of precision to use when outputting floating points.
		 * @param color				- General color override, changes the color of Ingredient names for search, list, and build.
		 */
		explicit Format(const file::ini::INI& ini, const bool quiet = false, const bool verbose = true, const bool exact = false, const bool all = false, const bool file_export = false, const bool reverse_output = false, const bool allow_color_fx = true, const bool use_local_cache = false, const size_t cli_precision = 2u) : OutputFormat(
			quiet,
			verbose,
			exact,
			all,
			file_export,
			reverse_output,
			allow_color_fx,
			use_local_cache,
			str::stoui(ini.getv("format", "indent")),
			(ini.check("format", "precision") ? str::stoui(ini.getv("format", "precision")) : cli_precision),
			loadColorConfig(ini)
		) {}
		explicit Format(const bool quiet = false, const bool verbose = true, const bool exact = false, const bool all = false, const bool file_export = false, const bool reverse_output = false, const bool allow_color_fx = true, const bool use_local_cache = false, const size_t cli_precision = 2u) : OutputFormat(
			quiet,
			verbose,
			exact,
			all,
			file_export,
			reverse_output,
			allow_color_fx,
			use_local_cache,
			3u,
			cli_precision,
			ColorAPI{ DefaultObjects._default_colors }
		) {}

#pragma region SPECIAL_GETTERS
		/**
		 * @function get_tuple(const std::string&, const std::string&) const
		 * @brief Uses a string to split of a line into a tuple of strings where { <preceeding text>, <delimiter text>, <trailing text> }
		 * @param str			The line to split.
		 * @param highlight		Substr of str to separate. Must be converted to lowercase beforehand or match will not be successful.
		 * @returns std::tuple<std::string, std::string, std::string>
		 *\n		0	- Text that preceeds highlight
		 *\n		1	- highlight
		 *\n		2	- Text that supersedes highlight
		 */
		[[nodiscard]] std::tuple<std::string, std::string, std::string> get_tuple(const std::string& str, const std::string& highlight) const
		{
			if ( !str.empty() )
				if ( const auto dPos{ str::tolower(str).find(highlight) }; dPos != std::string::npos )
					return { str.substr(0, dPos), str.substr(dPos, highlight.size()), str.substr(dPos + highlight.size()) };
			return{ str, { }, { } }; // by default, return blanks for unused tuple slots. This is used to hide the text color when nothing was found.
		}
		/**
		 * @function get_tuple(const std::string&, const std::string&) const
		 * @brief Uses a string to split of a line into a tuple of strings where { <preceeding text>, <delimiter text>, <trailing text> }
		 * @param str			The line to split.
		 * @param highlights	Substrings of str to separate. Must be converted to lowercase beforehand or match will not be successful.
		 * @returns std::tuple<std::string, std::string, std::string>
		 *\n		0	- Text that preceeds highlight
		 *\n		1	- highlight
		 *\n		2	- Text that supersedes highlight
		 */
		[[nodiscard]] std::tuple<std::string, std::string, std::string> get_tuple(const std::string& str, const std::vector<std::string>& highlights) const
		{
			if ( !str.empty() )
				for ( auto& highlight : highlights )
					if ( const auto dPos{ str::tolower(str).find(highlight) }; dPos != std::string::npos )
						return { str.substr(0, dPos), str.substr(dPos, highlight.size()), str.substr(dPos + highlight.size()) };
			return{ str, { }, { } }; // by default, return blanks for unused tuple slots. This is used to hide the text color when nothing was found.
		}

		/**
		 * @function get_fx(std::array<Effect, 4>&, const std::vector<std::string>&) const
		 * @brief Retrieve a vector of Effects from an Ingredient Effect array.
		 * @param arr				Effect array ref.
		 * @param names_lowercase	The names to search for, must be lowercase.
		 * @returns std::vector<Effect*>
		 */
		[[nodiscard]] std::vector<Effect> get_fx(const std::array<Effect, 4>& arr, const std::vector<std::string>& names_lowercase) const
		{
			std::vector<Effect> vec;
			vec.reserve(4llu);
			for ( auto it{ arr.begin() }; it != arr.end(); ++it ) {
				if ( !_flag_quiet )
					vec.push_back(*it);
				else if (const auto lc{str::tolower(it->_name)}; std::any_of(names_lowercase.begin(), names_lowercase.end(), [this, &lc](const std::string& name) -> bool { return match(lc, name); })) {
					vec.push_back(*it);
					if ( _flag_exact )
						break;
				}
			}
			return vec;
		}
#pragma endregion SPECIAL_GETTERS

		/**
		 * @brief Retrieve a color to use when outputting a given effect.
		 * @param effect	- The effect to check.
		 * @returns ColorAPI::ColorSetter
		 */
		ColorAPI::ColorSetter resolveEffectColor(const Effect& effect) const
		{
			// check if color override is enabled
			if (_flag_color)
				return _colors.set(UIElement::EFFECT_NAME_NEUTRAL);
			// check keywords
			if (!effect._keywords.empty()) {
				if (hasNegative(effect))
					return _colors.set(UIElement::EFFECT_NAME_NEGATIVE);
				if (hasPositive(effect))
					return _colors.set(UIElement::EFFECT_NAME_POSITIVE);
				if (!effect.hasKeyword(Keywords::KYWD_MagicInfluence))
					return _colors.set(UIElement::EFFECT_NAME_NEUTRAL);
			}
			return _colors.set(UIElement::EFFECT_NAME_DEFAULT); // else return white
		}
	#pragma region BASE
		/**
		 * @function to_fstream(std::ostream&, const Ingredient&) const
		 * @brief Insert a registry-formatted ingredient into an output stream.
		 * @param os	- Target output stream.
		 * @param ingr	- Target ingredient.
		 * @returns std::ostream&
		 */
		static std::ostream& to_fstream(std::ostream& os, const Ingredient& ingr)
		{
			os << ingr._name << "\n{\n";
			for ( auto& fx : ingr._effects )
				os << '\t' << fx._name << "\n\t{\n\t\tmagnitude = " << fx._magnitude << "\n\t\tduration = " << fx._duration << "\n" << fx._keywords << "}\n";
			os << "}\n";
			return os;
		}

		/**
		 * @function to_stream(std::ostream&, const Keyword&, const std::string&, const unsigned = 3u)
		 * @brief Insert a Keyword into an output stream in human-readable format.
		 * @param os				- Target output stream.
		 * @param kywd				- Target Keyword.
		 * @param indentation		- String to use as indentation before each line.
		 * @param repeatIndentation	- Repeats the indentation string this many times before the effect name.
		 * @returns std::ostream&
		 */
		std::ostream& to_stream(std::ostream& os, const Keyword& kywd, const std::string& indentation, const unsigned repeatIndentation = 3u) const
		{
			for (auto i{ 0u }; i < repeatIndentation; ++i)
				os << indentation;
			os << color::f::gray << kywd._name << '\n';
			return os;
		}
		/**
		 * @function to_stream(std::ostream&, const Effect&, const std::string&, const std::string&)
		 * @brief Insert an Effect into an output stream in human-readable format.
		 * @param os				- Target output stream.
		 * @param fx				- Target Effect.
		 * @param search_str		- Search string, used to highlight searched-for strings in the output.
		 * @param indentation		- String to use as indentation before each line.
		 * @param repeatIndentation	- Repeats the indentation string this many times before the effect name.
		 * @param ind_fac			- Subtract the number of used chars from this value to get final indentation when printing magnitude & duration.
		 * @returns std::ostream&
		 */
		std::ostream& to_stream(std::ostream& os, const Effect& fx, const std::string& search_str, const std::string& indentation, const unsigned repeatIndentation = 2u, const size_t ind_fac = 25u) const
		{
			const auto [pre, highlight, post] { get_tuple(fx._name, search_str) };
			const auto fx_color{ resolveEffectColor(fx) };
			for (auto i{ 0u }; i < repeatIndentation; ++i)
				os << indentation;

			os << fx_color << pre << color::reset << _colors.set(UIElement::SEARCH_HIGHLIGHT) << highlight << fx_color << post << color::bold;

			const auto insert_num{ [&os, &ind_fac](const std::string& num, const ColorAPI::ColorSetter color, const unsigned indent) -> unsigned {  // NOLINT(clang-diagnostic-c++20-extensions)
				if (indent > ind_fac)
					os << std::setw(static_cast<std::streamsize>(indent) + 2u) << ' ';
				else
					os << std::setw(ind_fac - static_cast<std::streamsize>(indent)) << ' ';
				os << color << num;
				return num.size();
			} };

			auto size_factor{ fx._name.size() };

			if (fx._magnitude > 0.0 || _flag_all)
				size_factor = insert_num(str::to_string(fx._magnitude, _precision), _colors.set(UIElement::EFFECT_MAGNITUDE), size_factor) + 10u;
			if (fx._duration > 0u || _flag_all) {
				insert_num(str::to_string(fx._duration, _precision), _colors.set(UIElement::EFFECT_DURATION), size_factor);
				os << color::reset_bold << 's';
			}
			os << color::reset << '\n';
			if (_flag_verbose || _flag_all)
				for (auto& KYWD : fx._keywords)
					to_stream(os, KYWD, indentation);
			return os;
		}
		/**
		 * @function to_stream(std::ostream&, const Effect&, const std::string&, const std::string&)
		 * @brief Insert an Effect into an output stream in human-readable format.
		 * @param os				- Target output stream.
		 * @param fx				- Target Effect.
		 * @param search_strings	- Search strings, used to highlight searched-for strings in the output.
		 * @param indentation		- String to use as indentation before each line.
		 * @param repeatIndentation	- Repeats the indentation string this many times before the effect name.
		 * @param ind_fac			- Subtract the number of used chars from this value to get final indentation when printing magnitude & duration.
		 * @returns std::ostream&
		 */
		std::ostream& to_stream(std::ostream& os, const Effect& fx, const std::vector<std::string>& search_strings, const std::string& indentation, const unsigned repeatIndentation = 2u, const std::streamsize ind_fac = 25) const
		{
			const auto [pre, highlight, post] { get_tuple(fx._name, search_strings) };
			const auto fx_color{ resolveEffectColor(fx) };
			for (auto i{ 0u }; i < repeatIndentation; ++i)
				os << indentation;
			os << fx_color << pre << color::reset << _colors.set(UIElement::SEARCH_HIGHLIGHT) << highlight << fx_color << post << color::reset;
			const auto insert_num{ [&os, &ind_fac](const std::string& num, const ColorAPI::ColorSetter color, const std::streamsize indent) -> unsigned {  // NOLINT(clang-diagnostic-c++20-extensions)
				if (indent > ind_fac)
					os << std::setw(indent + 2) << ' ';
				else
					os << std::setw(ind_fac - indent) << ' ';
				os << color << num;
				return num.size();
			} };
			auto size_factor{ fx._name.size() };
			if (fx._magnitude > 0.0 || _flag_all)
				size_factor = insert_num(str::to_string(fx._magnitude, _precision), _colors.set(UIElement::EFFECT_MAGNITUDE), size_factor) + 10u;
			if (fx._duration > 0u || _flag_all) {
				insert_num(str::to_string(fx._duration, _precision), _colors.set(UIElement::EFFECT_DURATION), size_factor);
				os << 's';
			}
			os << color::reset << '\n';
			return os;
		}

		/**
		 * @function to_stream(std::ostream&, Ingredient&, const std::string& = "")
		 * @brief Insert an ingredient into an output stream in human-readable format.
		 * @param os			- Target output stream.
		 * @param ingr			- Target Ingredient Ref.
		 * @param search_str	- Search string, used to highlight searched-for strings in the output.
		 * @returns std::ostream&
		 */
		std::ostream& to_stream(std::ostream& os, const Ingredient& ingr, const std::string& search_str = "") const
		{
			const auto indentation{ std::string(_indent, ' ') }; // get indentation string
			const auto [pre, highlight, post] { get_tuple(ingr._name, search_str) };
			os << indentation << _colors.set(UIElement::INGREDIENT_NAME) << pre << color::reset << _colors.set(UIElement::SEARCH_HIGHLIGHT) << color::bold << highlight << color::reset << _colors.set(UIElement::INGREDIENT_NAME) << post << color::reset << '\n';
			for (auto& fx : get_fx(ingr._effects, { search_str })) // iterate through this ingredient's effects, and insert them as well.
				to_stream(os, fx, search_str, indentation, 2u, 25u);
			return os;
		}
	#pragma endregion BASE
	#pragma region FSTREAM
		/**
		 * @function to_fstream(std::ostream&, const SortedIngrList&) const
		 * @brief Insert a registry-formatted list of ingredients into an output stream.
		 * @param os	- Target output stream.
		 * @param ingr	- Target ingredient list.
		 * @returns std::ostream&
		 */
		std::ostream& to_fstream(std::ostream& os, const SortedIngrList& ingr) const
		{
			if ( _flag_reverse )
				for ( auto it{ ingr.rbegin() }; it != ingr.rend(); ++it )
					to_fstream(os, *it);
			else
				for ( auto it{ ingr.begin() }; it != ingr.end(); ++it )
					to_fstream(os, *it);
			return os;
		}

		/**
		 * @function to_fstream(std::ostream&, const SortedIngrList&) const
		 * @brief Insert a registry-formatted list of ingredients into an output stream.
		 * @param os	- Target output stream.
		 * @param ingr	- Target ingredient list.
		 * @returns std::ostream&
		 */
		std::ostream& to_fstream(std::ostream& os, const IngrList& ingr) const
		{
			if ( _flag_reverse )
				for ( auto it{ ingr.rbegin() }; it != ingr.rend(); ++it )
					to_fstream(os, *it);
			else
				for ( auto it{ ingr.begin() }; it != ingr.end(); ++it )
					to_fstream(os, *it);
			return os;
		}

		[[nodiscard]] std::stringstream to_fstream(const SortedIngrList& ingr) const
		{
			std::stringstream ss;
			to_fstream(ss, ingr);
			return ss;
		}
#pragma endregion FSTREAM

#pragma region STREAM

		/**
		 * @function to_fstream(std::ostream&, const SortedIngrList&, const std::vector<std::string>&) const
		 * @brief Insert a list of ingredients into an output stream. Used for the list output mode.
		 * @param os				- Target output stream.
		 * @param ingr				- Target ingredient list.
		 * @param search_strings	- Search strings, used to highlight searched-for strings in the output.
		 * @returns std::ostream&
		 */
		std::ostream& to_stream(std::ostream& os, const SortedIngrList& ingr, const std::vector<std::string>& search_strings) const
		{
			const auto indentation{ std::string(_indent, ' ') };
			const auto to_stream{ [this, &os, &search_strings, &indentation](const SortedIngrList::iterator it) {
				os << indentation << _colors.set(UIElement::INGREDIENT_NAME) << it->_name << color::reset << '\n';
				for ( auto& fx : it->_effects )
					this->to_stream(os, fx, search_strings, indentation);
			} };
			if ( _flag_reverse )
				for ( auto it{ ingr.rbegin() }; it != ingr.rend(); ++it )
					to_stream(it.base());
			else
				for ( auto it{ ingr.begin() }; it != ingr.end(); ++it )
					to_stream(it);
			return os;
		}

		/**
		 * @function to_stream(std::ostream&, Potion&, const std::string& = "")
		 * @brief Insert a Potion into an output stream in human-readable format.
		 * @param os			- Target output stream.
		 * @param potion		- Target Potion Ref.
		 * @param indentation	- String to use as indentation before each line.
		 * @returns std::ostream&
		 */
		std::ostream& to_stream(std::ostream& os, const Potion& potion, const std::string& indentation) const
		{
			const auto [pre, highlight, post]{ get_tuple(potion.name(), "") };
			os << indentation << _colors.set(UIElement::INGREDIENT_NAME) << pre << color::reset << _colors.set(UIElement::SEARCH_HIGHLIGHT) << color::bold << highlight << color::reset << _colors.set(UIElement::INGREDIENT_NAME) << post << color::reset << '\n';
			for ( auto& fx : potion.effects() ) // iterate through this ingredient's effects, and insert them as well.
				to_stream(os, fx, "", indentation, 2u, 25u);
			return os;
		}

		/**
		 * @function to_stream_build(std::ostream&, Ingredient&, const Potion&)
		 * @brief Insert a potion ingredient into an output stream in human-readable format.
		 * @param os			- Target output stream.
		 * @param ingr			- Target Ingredient Ref.
		 * @param potion		- Resulting potion.
		 * @returns std::ostream&
		 */
		std::ostream& to_stream_build(std::ostream& os, const Ingredient& ingr, const Potion& potion) const
		{
			const auto indentation{ std::string(_indent, ' ') }; // get indentation string
			const auto names_lc{ [&potion]() -> std::vector<std::string> {
				std::vector<std::string> vec;
				for ( const auto& it : potion.effects() )
					vec.push_back(str::tolower(it._name));
				return vec;
			}() };
			os << indentation << _colors.set(UIElement::INGREDIENT_NAME) << ingr._name << color::reset << '\n';
			for ( auto& fx : get_fx(ingr._effects, names_lc) ) // iterate through this ingredient's effects, and insert them as well.
				to_stream(os, fx, "", indentation);
			return os;
		}

		std::ostream& list_to_stream(std::ostream& os, const SortedIngrList& list) const
		{
			const auto indentation{ std::string(_indent, ' ') };
			for ( auto& it : list )
				to_stream(os, it);
			return os;
		}
#pragma endregion STREAM

		/**
		 * @operator()()
		 * @brief Retrieve a reference to this Format instance.
		 * @returns Format&
		 */
		Format& operator()() { return *this; }
	};
}
