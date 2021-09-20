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
#include <Params.hpp>
#include <utility>

#include <Help.hpp>

#include "Alchemy.hpp"
#include "reloader.hpp"

namespace caco_alch {
	/**
	 * @function handle_arguments(opt::Param&&, Alchemy&&, GameConfig&&)
	 * @brief Handles primary program execution. Arguments can be retrieved from the init() function.
	 * @param args	- rvalue reference of an opt::Param instance.
	 * @param alch	- Alchemy instance rvalue.
	 * @param gs	- Gamesettings instance rvalue.
	 * @returns int - see main() documentation
	 */
	inline int handle_arguments(opt::Params&& args, Alchemy&& alch)
	{
		if ( args.check_flag('i') ) { // check for receive STDIN flag
			std::stringstream buffer;
			buffer << std::cin.rdbuf();
			alch.print_build_to(std::cout, parseFileContent(buffer)).flush();
			return 0;
		}

		else if (args.check_flag('l')) { // check for list flag
			alch.print_list_to(std::cout).flush();
			return 0;
		}

		else {
			const auto params{ args.getAllWithType<opt::Parameter>() };

			if (args.check_flag('b')) // Build mode
				alch.print_build_to(std::cout, params).flush();

			else if ( const auto smart{ args.check_flag('S') }; args.check_flag('s') || smart ) { // Search mode
				if ( smart )
					alch.print_smart_search_to(std::cout, params).flush();
				else
					for (auto it{ params.begin() }; it != params.end(); ++it)
						alch.print_search_to(std::cout, *it).flush();
			}
			return 0;
		}
		return 1;
	}
	// handle_arguments wrapper for TEST project
	inline int handle_arguments(std::tuple<opt::Params, Alchemy>&& pr) { return handle_arguments(std::forward<opt::Params>(std::get<0>(pr)), std::forward<Alchemy>(std::get<1>(pr))); }
}
