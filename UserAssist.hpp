/**
 * @file UserAssist.hpp
 * @author radj307
 * @brief Contains methods related to user-interaction; such as the Help namespace, and the opt::Matcher instance used to specify valid commandline options.
 */
#pragma once
#include <map>
#include <string>
#include <iostream>
#include <iomanip>
#include <optlib.hpp>

namespace caco_alch {
	/**
	 * @function handle_arguments(opt::Param&&)
	 * @brief Handles program execution.
	 * @param args	- rvalue reference of an opt::Param instance.
	 * @returns int - see main() documentation
	 */
	inline int handle_arguments(opt::Param&& args, Alchemy&& alch, GameSettings&& gs)
	{
		const Alchemy::Format _format{ args.getFlag('q'), args.getFlag('v'), args.getFlag('e'), args.getFlag('a'), 3u, [&args]() { const auto v{ str::stoui(args.getv("precision")) }; if ( v != 0.0 ) return v; return 2u; }( ), [&args]() -> unsigned short { const auto v{ str::stous(args.getv("color")) }; if ( v != 0 ) return v; return Color::_f_white; }( ) };

		if ( args.getFlag('l') ) // List mode
			alch.print_list_to(std::cout, _format);
		if ( args.getFlag('b') ) { // Build mode
			alch.print_build_to(std::cout, args._param, std::forward<GameSettings>(gs), _format);
		}
		else if ( args.getFlag('s') ) // Search mode
			for ( auto& it : args._param )
				alch.print_search_to(std::cout, it, _format);
		return 0;
	}

	inline int handle_arguments(std::tuple<opt::Param, Alchemy, GameSettings>&& pr) { return handle_arguments(std::forward<opt::Param>(std::get<0>(pr)), std::forward<Alchemy>(std::get<1>(pr)), std::forward<GameSettings>(std::get<2>(pr))); }

	// @brief Contains the list of valid commandline arguments for the alch program.
	inline const opt::Matcher _matcher{ { 'l', 's', 'a', 'h', 'q', 'v', 'b', 'e', 'C' }, { { "load", true }, { "validate", false }, { "color", true }, { "precision", true }, { "name", true }, { "ini", true }, { "ini-modav-alchemy", true }, { "ini-default-duration", true }, { "ini-reset", false } } };

	/**
	 * @namespace Help
	 * @brief Contains methods related to the inline terminal help display.
	 */
	namespace Help {
		/**
		 * @struct Helper
		 * @brief Provides a convenient extensible help display.
		 */
		struct Helper {
			using Cont = std::map<std::string, std::string>;
			std::string _usage;
			Cont _doc;
			/**
			 * @constructor Helper(const std::string&, std::map<std::string, std::string>&&)
			 * @brief Default constructor, takes a string containing usage instructions, and a map of all commandline parameters and a short description of them.
			 * @param usage_str	- Brief string showing the commandline syntax for this program.
			 * @param doc		- A map where the key represents the commandline option, and the value is the documentation for that option.
			 */
			Helper(const std::string& usage_str, Cont&& doc) : _usage{ usage_str }, _doc{ std::move(doc) } { validate(); }

			/**
			 * @function validate()
			 * @brief Called at construction time, iterates through _doc and corrects any abnormalities.
			 */
			void validate()
			{
				for ( auto it{ _doc.begin() }; it != _doc.end(); ++it ) {
					auto& key{ it->first };
					auto& doc{ it->second };
					if ( key.empty() ) // If key is empty, delete entry
						_doc.erase(it);
					else { // key isn't empty
						const auto fst_char{ key.at(0) };
						if ( fst_char != '-' ) { // if key does not have a dash prefix
							const auto tmp = *it;
							_doc.erase(it);
							std::string mod{ '-' };
							if ( key.size() > 1 ) // is longopt
								mod += '-';
							_doc[mod + tmp.first] = tmp.second;
						}
					}
				}
			}

			/**
			 * @function operator<<(std::ostream&, const Helper&)
			 * @brief Stream insertion operator, returns usage string and documentation for all options.
			 * @param os	- (implicit) Output stream instance.
			 * @param h		- (implicit) Helper instance.
			 * @returns std::ostream&
			 */
			friend std::ostream& operator<<(std::ostream& os, const Helper& h)
			{
				os << "Usage:\n  " << h._usage << "\nOptions:\n";
				size_t indent{ 0 };
				for ( auto& it : h._doc )
					if ( const auto size{ it.first.size() }; size > indent )
						indent = size;
				for ( auto& [key, doc] : h._doc )
					os << "  " << std::left << std::setw(static_cast<std::streamsize>( indent ) + 2ll) << key << doc << '\n';
				os.flush();
				return os;
			}
		};

		const Helper _default_doc("caco-alch <[options] [target]>", {
			{ "-h", "Shows this help display." },
			{ "-l", "List all ingredients." },
			{ "-a", "Lists all ingredients and a list of all known effects." },
			{ "-s", "Searches the ingredient & effect lists for all additional parameters, and prints a result to STDOUT" },
			{ "-e", "Exact mode, does not allow partial search matches." },
			{ "-q", "Quiet output, only shows effects that match the search string in search results." },
			{ "-v", "Verbose output, shows magnitude for ingredient effects." },
			{ "-b", "(Incompatible with -s) Build mode, accepts up to 4 ingredient names and shows the result of combining them." },
			{ "-R", "(Not Implemented) Reverse order." }, // TODO
			{ "--load <file>", "Allows specifying an alternative ingredient registry file." },
			{ "--validate", "Checks if the target file can be loaded successfully, and contains valid data. Specifying this option will cause all other options to be ignored." },
			{ "--color <string_color>", "Change the color of ingredient names. String colors must include either an 'f' (foreground) or 'b' (background), then the name of the desired color." },
			{ "--precision <uint>", "Set the floating-point precision value when printing numbers. (Default: 2)" },
			{ "--ini-modav-alchemy <uint>", "(Experimental) Set the alchemy skill level." },
			{ "--ini-default-duration <uint>", "(Experimental) Set the default duration to a value in seconds." },
			{ "--ini <file>", "(Experimental) Load a specific INI file." },
			{ "--ini-reset", "(Experimental) Reset / Write a new INI config file." },
			}); ///< @brief Help documentation used by default.

		/**
		 * @function print_help(const Helper& = _default_doc)
		 * @brief Display help information.
		 * @param documentation	- Documentation to display
		 */
		void print(const Helper& documentation = _default_doc)
		{
			std::cout << documentation << std::endl;
		}
	}
}