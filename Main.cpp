#include <iostream>
#include <vector>
#include <random>
#include <map>
#include <unordered_map>
#include <functional>
#include <bitset>
#include <set>
#include <numeric>
#include <fstream>
#include <iterator>
#include <string>
#include <filesystem>

#include "ExplorTypes.h"
#include "parsing/ExplorParser.h"
#include "ExplorLang.h"


namespace fs = std::filesystem;

int main(int argc, char** argv) {
	
	std::ifstream example;

	if(argc == 1){
		std::cout << "Input file missing";
		exit(0);
	}

	//Check if file exists
	auto path = fs::path(argv[1]);
	if (!fs::exists(path)) {
		std::cout << "Invalid path specified or File does not exist";
		exit(0);
	}

	
	example.open(path.filename(), std::ifstream::in);

	using ctxFileIter = ContextAwareIterator<std::istreambuf_iterator<char>,16,GenericContext<std::string>>;

	
	std::istreambuf_iterator<char> eof;
	ctxFileIter s = std::istreambuf_iterator<char>(example);
	ctxFileIter e = eof;
	
	 auto result = new decltype(pattern)::return_type;



	bool hasParsed = pattern(s, e, result);
	
	if (s != e) {
		std::cout << "Failed to parse file";
		std::cout << "Ended @ " << s.line() << '#' << s.column() << '\n';
		std::cout << "Read " << result->size() << " Lines" << std::endl;
		exit(1);
	}
	if (hasParsed) {
		

		auto program = new EXPLOR<320,240>;

		for(auto & line: *result) {
			std::apply([&program](std::string label,Commands command) {
				program->addLine(label, command);
			}, line);
		}
		try {
			program->execute();
		}
		catch (std::exception & e) {
			//Print Error
			std::cout << "\nEncountered Error -> " << e.what();
		}
		
		
		std::ofstream example_out;
		auto out_path = std::string("./");
		out_path.append(path.stem().generic_string().append(".pbm"));

		std::cout << "Output 1 Frame to: " << out_path;
		if (program->frames.size() != 0) {
			example_out.open(out_path, std::ofstream::out);

			auto& fframe = program->frames[0];
			example_out << "P1 320 240 1\n";

			for (size_t i = 0; i < fframe.size(); i++)
			{
				for (size_t j = 0; j < fframe[i].size(); j++)
				{
					example_out << fframe.at(i).at(j) << " ";

				}
			}
			example_out.close();
		}
		else {
			std::cout << "No frames generated\n";
		}
	} 

	example.close();

};