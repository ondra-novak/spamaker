/*
 * builder.cpp
 *
 *  Created on: 18. 6. 2021
 *      Author: ondra
 */

#include <algorithm>
#include <fstream>
#include <iostream>
#include "linux_spawn.h"
#include "builder.h"

void Builder::parse(const std::filesystem::path &fname) {


	parse(std::move(fname), std::vector<std::filesystem::path>());

}

static std::string_view trim(std::string_view str) {
	while (!str.empty() && isspace(str[0])) str = str.substr(1);
	while (!str.empty() && isspace(str[str.length()-1])) str = str.substr(0, str.length()-1);
	return str;
}

void Builder::parse(const std::filesystem::path &fname,
		std::vector<std::filesystem::path> &&recurse) {

	if (std::find(recurse.begin(), recurse.end(), fname) != recurse.end()) return;
	std::ifstream f(fname, std::ios::in);
	auto dirname = fname.parent_path();
	recurse.push_back(fname);

	if (!f) throw std::runtime_error("Can't open file: "+fname.string());


	while (!f.eof()) {
		std::getline(f, buffer);
		std::string_view ln(buffer);
		if (ln.length()>3 && ln.substr(0,3) == "//@") {
			ln = trim(ln.substr(3));
			auto np = ln.find(' ');
			std::string_view cmd;
			std::string_view args;
			if (np == ln.npos) {
				cmd = ln;
			} else {
				cmd = ln.substr(0,np);
				args = trim(ln.substr(np+1));
			}
			if (cmd == "require") {
				auto s = prepare(dirname , args);
				parse(std::move(s), std::move(recurse));
			} else if (cmd == "html") {
				resources[cont_html].push_back(prepare(dirname , args));
			} else if (cmd == "template") {
				resources[cont_htmltemplate].push_back(prepare(dirname , args));
			} else if (cmd == "style") {
				resources[cont_style].push_back(prepare(dirname , args));
			} else if (cmd == "image") {
				resources[cont_image].push_back(prepare(dirname , args));
			} else if (cmd == "file") {
				resources[cont_file].push_back(prepare(dirname , args));
			} else if (cmd == "config") {
				resources[cont_config].push_back(prepare(dirname , args));
			} else if (cmd == "head") {
				resources[cont_pagehdr].push_back(prepare(dirname , args));
			} else if (cmd == "lang") {
				lang = args;
			} else if (cmd == "namespace") {
				modules.insert(fname);
				auto sp = args.find('.');
				while (sp != args.npos) {
					nsset.insert(std::string(args.substr(0,sp)));
					sp = args.find('.', sp+1);
				}
				nsset.insert(std::string(args));
			}
		}
	}
	resources[cont_script].push_back(fname);
}

static void to_hex(std::size_t h, std::string &b, int cnt) {
	static const char symb[] = "0123456789ABCDEF";
	if (cnt) {
		to_hex(h/16, b, cnt-1);
		b.push_back(symb[h & 0xF]);
	}
}

static void to_hex(std::size_t h, std::string &b) {
	b.clear();
	to_hex(h, b, 16);
}

std::filesystem::path Builder::make_cache_file(const std::string_view &fname) const {
	std::hash<std::string_view> h;
	to_hex(h(fname), buffer);
	auto np = fname.rfind('.');
	auto sp = fname.rfind('/');
	if (np != fname.npos && np > sp) buffer.append(fname.substr(np));
	auto cpath = cachePath / buffer ;
	return cpath;
}

std::filesystem::path Builder::prepare(const std::filesystem::path &dir, const std::string_view &fname) {
	if (fname.empty()) throw std::runtime_error("empty reference");
	if (fname[0] == '/') return fname;
	if (fname.substr(0,7)=="http://" || fname.substr(0,8)=="https://") {
		std::string tmp ( fname);
		auto cpath = make_cache_file(tmp);
		if (!std::filesystem::exists(cpath)) {
			download(tmp, cpath);
		}
		return cpath;
	}
	return dir / fname;

}

void Builder::download(const std::string &source, const std::filesystem::path &target) {
	auto proc = ondra_shared::ExternalProcess::spawn(
			cachePath.string(), "curl", {"-o",target.string(),source});
	std::cout << "Downloading: " << source << std::endl;
	int i = proc.join();
	if (i) throw std::runtime_error("Failed to download: "+source+" error:"+ std::to_string(i));
}

void Builder::build(const std::filesystem::path &out, BuildType bt) {
	auto parent = out.parent_path();

	auto nsset_file = createNSSet(out);

	std::filesystem::path pagefile = out;
	pagefile.replace_extension(".html");
	std::filesystem::path scriptfile = out;
	scriptfile.replace_extension(".js");
	std::filesystem::path stylefile = out;
	stylefile.replace_extension(".css");
	std::filesystem::path imgdir = out.parent_path() / "img";
	std::filesystem::path filedir = out.parent_path() / "files";
	std::filesystem::path confdir = out.parent_path() / "conf";

	switch (bt) {
	case BuildType::script_only: {
			std::ofstream fout(scriptfile, std::ios::out| std::ios::trunc);
			buildScript(nsset_file, fout);
			checkFile(fout, scriptfile);
		} break;
	case BuildType::html_only: {
			std::ofstream fout(pagefile, std::ios::out| std::ios::trunc);
			buildPage(fout, [&]{linkStyle(fout,out,stylefile);}, [&]{linkScript(fout, out, scriptfile);});
			checkFile(fout, pagefile);
		} break;
	case BuildType::single_page_file: {
			std::ofstream fout(pagefile, std::ios::out| std::ios::trunc);
			buildPage(fout, [&]{
				fout << "<style type=\"text/css\">" << std::endl;
				buildStyle(fout);
				fout << "</style>";
			} , [&]{
				fout << "<script type=\"text/javascript\">" << std::endl;
				buildScript(nsset_file, fout);
				fout << "</script>";
			});
			checkFile(fout, pagefile);
		} break;
	case BuildType::std_page: {
			std::ofstream fout(pagefile, std::ios::out| std::ios::trunc);
			buildPage(fout, [&]{linkStyle(fout,out,stylefile);}, [&]{linkScript(fout, out,scriptfile);});
			checkFile(fout, pagefile);
		}{
			std::filesystem::path srcmap = scriptfile;
			srcmap.replace_extension(".map");
			std::ofstream fout(scriptfile, std::ios::out| std::ios::trunc);
			std::string mapping = buildScript(nsset_file, fout);
			fout << "//# sourceMappingURL=" << srcmap.filename().string() << std::endl;
			checkFile(fout, scriptfile);
			std::ofstream fmap(srcmap, std::ios::out| std::ios::trunc);
			buildSourceMap(fmap, scriptfile, nsset_file, mapping);
			checkFile(fmap, srcmap);
		}{
			std::ofstream fout(stylefile, std::ios::out| std::ios::trunc);
			buildStyle(fout);
			checkFile(fout, stylefile);
		}
		break;

	case BuildType::develop_page: {
			std::ofstream fout(pagefile, std::ios::out| std::ios::trunc);
			buildPage(fout, [&]{
				for (const Resource &res: resources[cont_style]) {
					linkStyle(fout, out, res);
				}
			} , [&]{
				linkScript(fout, out, nsset_file);
				for (const Resource &res: resources[cont_script]) {
					linkScript(fout, out, res);
				}
			});
			checkFile(fout, pagefile);
			}
		break;
	}

	for (const Resource &res: resources[cont_image]) {
		copyNewer(res, imgdir/res.filename());
	}
	for (const Resource &res: resources[cont_file]) {
		copyNewer(res, filedir/res.filename());
	}
	for (const Resource &res: resources[cont_config]) {
		copyNewer(res, confdir/res.filename());
	}

}

std::filesystem::path Builder::createNSSet(const std::filesystem::path &out_name) const {
	auto p = make_cache_file(out_name.string());
	p.replace_extension(".js");
	std::filesystem::create_directories(p.parent_path());
	std::ofstream out(p, std::ios::out| std::ios::trunc);
	out << "\"use strict\";" << std::endl;
	for (const auto &x : nsset) {
		if (x.find('.') == x.npos) out << "var " << x << "={};" << std::endl;
		else out << x << "={};" << std::endl;
	}
	if (!resources[cont_htmltemplate].empty()) {
		out <<
R"js(function loadTemplate(name){
	var nd = document.getElementById(name);
	var el = document.importNode(nd.content, true);
	if (el.firstElementChild && !el.firstElementChild.nextElementSibling) return el.firstElementChild;
	else return nd;
})js";
	}
	return p;
}


void Builder::checkFile(std::ostream &out, const std::filesystem::path &out_name) {
	if (!out) throw std::runtime_error(out_name.string() + ": failed to write");
}

void Builder::copyNewer(const std::filesystem::path &from, 	const std::filesystem::path &to) {
	std::filesystem::create_directories(to.parent_path());
	std::error_code ec;
	std::filesystem::file_time_type srcTime = std::filesystem::last_write_time(from);
	std::filesystem::file_time_type trgTime = std::filesystem::last_write_time(to, ec);
	if (srcTime > trgTime) {
		std::filesystem::copy_file(from,to, std::filesystem::copy_options::overwrite_existing);
	}
}

void Builder::linkScript(std::ostream &out, const std::filesystem::path &rel, const std::filesystem::path &link) {
	out << "<script type=\"text/javascript\" src=\"" << createRelativePath(rel, link) << "\"></script>";
}
void Builder::linkStyle(std::ostream &out, const std::filesystem::path &rel, const std::filesystem::path &link) {
	out << "<link rel=\"stylesheet\" href=\"" << createRelativePath(rel, link) << "\" />";
}

std::string Builder::createRelativePath(const std::filesystem::path &rel, const std::filesystem::path &link) {
	std::string res;
	auto ir = rel.begin();
	auto il = link.begin();
	while (ir != rel.end() && il != link.end()) {
		if (*ir == *il) {
			++ir;
			++il;
		} else {
			auto n = res.size();
			while (ir != rel.end()) {
				n = res.size();
				res.append("../");
				++ir;
			}
			res.resize(n);
			break;
		}
	}
	if (il != link.end()) {
		while (il != link.end()) {
			res.append(*il);
			res.push_back('/');
			++il;
		}
		res.pop_back();
	} else {
		res.append(link.filename());
	}

	return res;
}


template<typename StyleFN, typename ScriptFN>
void Builder::buildPage(std::ostream &out, StyleFN &&stylefn, ScriptFN &&scriptfn) {

	out << "<!DOCTYPE html>";
	if (lang.empty()) out <<"<HTML>";
	else out << "<HTML lang=\"" << lang <<"\">";
	out << "<HEAD><META charset=\"UTF-8\" />" ;
	for (const Resource &rs: resources[cont_pagehdr]) {
		insertFile(out, rs);
	}

	stylefn();
	out << "</HEAD><BODY>";
	for (const Resource &rs: resources[cont_html]) {
		insertFile(out, rs);
	}
	for (const Resource &rs: resources[cont_htmltemplate]) {
		out << "<TEMPLATE id=" << rs.stem() << ">";
		insertFile(out, rs);
		out << "</TEMPLATE>";
	}
	scriptfn();
	out << "</BODY></HTML>";

}

void Builder::insertFile(std::ostream &out, const std::filesystem::path &rs) {
	std::ifstream in(rs, std::ios::in);
	if (!(!in)) {
		char buff[4096];
		in.read(buff, 4096);
		auto sz = in.gcount();
		while (sz) {
			out.write(buff, sz);
			in.read(buff, 4096);
			sz = in.gcount();
		}
	}
	if (in.bad()) throw std::runtime_error(rs.string() + ": failed to read file");
}

std::string Builder::buildScript(const std::filesystem::path &nsf, std::ostream &out) {
	VLQ_state st;
	int scridx = 0;

	insertScript(out, nsf, scridx++, st);
	for (const Resource &rs: resources[cont_script]) {
		if (modules.find(rs) != modules.end()) {
			out << "(function(){" << std::endl;
			st.add_line();
			insertScript(out, rs, scridx++, st);
			out << "})();";

		} else {
			insertScript(out, rs, scridx++, st);
		}
	}
	return st.res;
}

Builder::Builder(const std::filesystem::path &cachePath):cachePath(cachePath) {
}

void Builder::buildStyle(std::ostream &out) {
	VLQ_state st;
	for (const Resource &rs: resources[cont_style]) {
		insertScript(out, rs, 0, st);
		out << std::endl;
	}
}

static const char base64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void vlq_append(std::string &mapping, int x) {
	if (x < 0) {
		x = (std::abs(x) << 1) | 1;
	} else {
		x = (x << 1);
	}
	while (x >= 32) {
		mapping.push_back(base64[(x & 0x1F) | 0x20]);
		x = x >> 5;
	}
	mapping.push_back(base64[(x & 0x1F) ]);
}


void Builder::VLQ_state::add_line() {
	res.push_back(';');
	last_out_col = 0;
	sep = false;
}

void Builder::VLQ_state::map(int out_col, int in_line,int in_col , int file_index) {
	if (sep) res.push_back(',');
	sep = true;
	vlq_append(res, out_col - last_out_col);
	vlq_append(res, file_index - last_file_index);
	vlq_append(res, in_line - last_in_line);
	vlq_append(res, in_col - last_in_col);

	last_out_col = out_col;
	last_in_line = in_line;
	last_in_col = in_col;
	last_file_index = file_index;
}


void Builder::insertScript(std::ostream &out, const std::filesystem::path &rs, int script_index, VLQ_state &st) {
	std::ifstream in(rs, std::ios::in);
	int out_col = 0;
	int in_line = 0;
	int i = in.get();
	st.map(out_col, in_line, 0, script_index);
	int string = 0;
	bool sp = false;
	while (i != EOF) {
		if (!string) {
			if (i == '\n') {
				out.put(' ');
				out_col++;
				in_line++;
				st.map(out_col, in_line, 0, script_index);
				sp = true;
			} else if (i == '\\') {
				out.put(i);
				out_col++;
				i = in.get();
				if (i == '\n') {
					st.add_line();
					in_line++;
					out_col = 0;
				}
				out.put(i);
				out_col++;
				sp = false;
			} else if (i == '/') {
				int j = in.get();
				if (j == '/') {
					while (j != '\n' && j != -1) j = in.get();
				} else if (j == '*') {
					j = in.get();
					while (true) {
//						std::cout.put(j);
						if (j == '\n') {
							in_line++;
							j = in.get();
						} else if (j == '*') {
							j = in.get();
							if (j == '/') break;
						} else if (j == -1) {
							break;
						} else {
							j = in.get();
						}
					}
					i = in.get();
//					std::cout << std::endl;
					continue;
				} else {
					out.put(i);
					out_col++;
				}
				in.putback(j);
				sp = true;
			} else if (i>=0 && i <= 32) {
				if (!sp) {
					out.put(' ');
					out_col++;
					sp = true;
				}
			} else {
				if (i == '"' || i == '\'') {
					string = i;
//					std::cout.put(i);
				}
				out.put(i);
				out_col++;
				sp = false;
			}
		} else {
//			std::cout.put(i);
			if (i == '\\') {
				out.put(i);
				out_col++;
				i = in.get();
//				std::cout.put(i);
			} else if (i == string) {
				string = 0;
//				std::cout << std::endl;
			}
			out.put(i);
			out_col++;
			sp = false;
		}
		i = in.get();

	}
	out << std::endl;
	st.add_line();
}

void Builder::buildSourceMap(std::ostream &out,
		const std::filesystem::path &outfile, const std::filesystem::path &nss,
		const std::string &mapping) {
	out << "{\"version\": 3, \"file\": " << outfile.filename() << ",\"sourceRoot\":\"\",\"sources\":[";
	out << "\"" << createRelativePath(outfile, nss) <<"\"";
	for (const Resource &rs: resources[cont_script]) {
		out << ",\"" << createRelativePath(outfile, rs) << "\"";
	}
	out << "],\"names\":[],\"mappings\":\"" << mapping << "\"}";
}
