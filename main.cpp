#include <iostream>
#include <filesystem>

#include "builder.h"

static std::filesystem::path extend_filename(const std::string &fname, const std::filesystem::path &cwd) {
	if (fname.empty()) return cwd;
	if (fname[0] == '/') return fname;
	auto c = cwd / fname;
	auto d = std::filesystem::weakly_canonical(c);
	return d;
}


int main(int argc, char **argv) {

	if (argc < 4) {
		std::cerr << "Needs arguments: " << argv[0] << "<type> <input> <output>";
		std::cerr << std::endl;
		std::cerr << "type=script    build script only, no other files are created" << std::endl
				  << "type=html      build only html, no other files are created" << std::endl
				  << "type=packed    pack everything into signle page" << std::endl
				  << "type=page      build standard page" << std::endl
				  << "type=devel     create page suitable for develping" << std::endl;
		return 1;
	}

	BuildType bt;
	std::string type = argv[1];
	if (type == "script") bt = BuildType::script_only;
	else if (type == "html") bt = BuildType::html_only;
	else if (type == "packed") bt = BuildType::single_page_file;
	else if (type == "page") bt = BuildType::std_page;
	else if (type == "devel") bt = BuildType::develop_page;
	else {
		std::cerr << "Unknown type: " << type << std::endl;
		return 1;
	}
	std::string input = argv[2];
	std::string output = argv[3];
	auto cwd = std::filesystem::current_path();
	auto infile = extend_filename(input, cwd);
	auto outfile = extend_filename(output, cwd);
	auto cache = extend_filename(".cache", cwd);


	try {
		Builder bld(cache);
		bld.parse(infile);
		bld.build(outfile, bt);
		std::cout << "Built: " << outfile.string() << std::endl;
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 2;
	}


	return 0;
}


