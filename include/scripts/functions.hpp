#pragma once

#include <boost/filesystem.hpp>

struct script_container {
	typedef std::list<script_container> list_type;

	script_container(std::wstring alias, boost::filesystem::wpath script) : alias(alias), script(script) {}
	script_container(boost::filesystem::wpath script) : script(script) {}
	script_container(const script_container &other) : alias(other.alias), script(other.script) {}
	script_container& operator=(const script_container &other) {
		alias = other.alias;
		script = other.script;
	}

	bool validate(std::wstring &error) const {
		if (script.empty()) {
			error = _T("No script given on command line!");
			return false;
		}
		if (!boost::filesystem::exists(script)) {
			error = _T("Script not found: ") + script.string();
			return false;
		}
		if (!boost::filesystem::is_regular(script)) {
			error = _T("Script is not a file: ") + script.string();
			return false;
		}
		return true;
	}

	static void push(list_type &list, std::wstring alias, boost::filesystem::wpath script) {
		list.push_back(script_container(alias, script));
	}
	static void push(list_type &list, boost::filesystem::wpath script) {
		list.push_back(script_container(script));
	}
	boost::filesystem::wpath script;
	std::wstring alias;

	std::wstring to_wstring() {
		return script.string() + _T(" as ") + alias;
	}
};
