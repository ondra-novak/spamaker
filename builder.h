/*
 * builder.h
 *
 *  Created on: 18. 6. 2021
 *      Author: ondra
 */

#ifndef BUILDER_H_
#define BUILDER_H_

#include <set>
#include <vector>
#include <filesystem>

enum class BuildType {
	///build script only - ignore resources
	script_only,
	///build html only - don't generate script, styles and files
	html_only,
	///build html all together
	single_page_file,
	///generate standard 3 files (html, style, code)
	std_page,
	///generate html, but link resources to the page (developer mode)0
	develop_page,
	///develop page symlinked
	develop_page_symlink
};

class Builder {
public:

	Builder(const std::filesystem::path &cachePath);

	static const unsigned int cont_script = 0;
	static const unsigned int cont_html = 1;
	static const unsigned int cont_style = 2;
    static const unsigned int cont_image=3;
    static const unsigned int cont_file=4;
	static const unsigned int cont_pagehdr=5;
	static const unsigned int cont_htmltemplate=6;
	static const unsigned int cont_config=7;
	static const unsigned int cont_count=8;


	using Resource = std::filesystem::path;
	using ResourceList = std::vector<Resource>;
	using ContainerType = std::string;


	void parse(const std::filesystem::path &fname);
	void parse(const std::filesystem::path &fname, std::vector<std::filesystem::path> &&recurse);

	void build(const std::filesystem::path &out, BuildType bt);

    void create_dep_file(const std::filesystem::path &depfile, const std::filesystem::path &output);





protected:


	std::filesystem::path cachePath;

	ResourceList resources[cont_count];
	mutable std::string buffer;

	using NSSet = std::set<std::string>;
	using Modules = std::set<Resource>;
	NSSet nsset;
	Modules modules;
	std::string lang;


	std::filesystem::path prepare(const std::filesystem::path &dir, const std::string_view &fname);
	std::filesystem::path make_cache_file(const std::string_view &name_source) const;
	void download(const std::string &source, const std::filesystem::path &target);
	std::filesystem::path createNSSet(const std::filesystem::path &out_name) const;

	void checkFile(std::ostream &out, const std::filesystem::path &out_name);
	void copyNewer(const std::filesystem::path &from, const std::filesystem::path &to);
	void linkScript(std::ostream &out, const std::filesystem::path &rel, const std::filesystem::path &link);
	void linkStyle(std::ostream &out, const std::filesystem::path &rel, const std::filesystem::path &link);

	static std::string createRelativePath(const std::filesystem::path &rel, const std::filesystem::path &link);
	static void insertFile(std::ostream &out, const std::filesystem::path &rs);
	template<typename StyleFN, typename ScriptFN>
	void buildPage(std::ostream &out, StyleFN &&stylefn, ScriptFN &&scriptfn);
	void buildScript( const std::filesystem::path &nsf, std::ostream &out); ///<returns source map mapping
	void buildStyle(std::ostream &out);

	void insertScript(std::ostream &out, const std::filesystem::path &rs);
	void symlink_all_resources(const std::filesystem::path &pagefile);

};

#endif /* BUILDER_H_ */
