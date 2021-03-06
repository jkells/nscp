#include "StdAfx.h"

#include <map>
#include <list>

#include <boost/bind.hpp>
#include <boost/assign.hpp>

#include <parsers/where.hpp>
#include <parsers/filter/where_filter.hpp>
#include <parsers/filter/where_filter_impl.hpp>

#include <parsers/where/unary_fun.hpp>
#include <parsers/where/list_value.hpp>
#include <parsers/where/binary_op.hpp>
#include <parsers/where/unary_op.hpp>
#include <parsers/where/variable.hpp>

#include <simple_timer.hpp>
#include <strEx.h>
#include "filter.hpp"

#include "file_info.hpp"
#include "file_finder.hpp"

//#include <config.h>

using namespace boost::assign;
using namespace parsers::where;

file_filter::filter_obj_handler::filter_obj_handler() {
		insert(types)
			(_T("filename"), (type_string))
			(_T("name"), (type_string))
			(_T("version"), (type_string))
			(_T("path"), (type_string))
			(_T("line_count"), (type_int))
			(_T("size"), (type_size))
			(_T("access"), (type_date))
			(_T("creation"), (type_date))
			(_T("written"), (type_date));
	}

bool file_filter::filter_obj_handler::has_variable(std::wstring key) {
	return types.find(key) != types.end();
}
parsers::where::value_type file_filter::filter_obj_handler::get_type(std::wstring key) {
	types_type::const_iterator cit = types.find(key);
	if (cit == types.end())
		return parsers::where::type_invalid;
	return cit->second;
}
bool file_filter::filter_obj_handler::can_convert(parsers::where::value_type from, parsers::where::value_type to) {
	if ((from == parsers::where::type_string)&&(to == type_custom_severity))
		return true;
	if ((from == parsers::where::type_string)&&(to == type_custom_type))
		return true;
	return false;
}

file_filter::filter_obj_handler::base_handler::bound_string_type file_filter::filter_obj_handler::bind_simple_string(std::wstring key) {
	base_handler::bound_string_type ret;
	if (key == _T("filename"))
		ret = &filter_obj::get_filename;
	else if (key == _T("name"))
		ret = &filter_obj::get_filename;
	else if (key == _T("path"))
		ret = &filter_obj::get_path;
	else if (key == _T("version"))
		ret = boost::bind(&filter_obj::get_version, _1, this);
	else
		NSC_DEBUG_MSG_STD(_T("Failed to bind (string): ") + key);
	return ret;
}



file_filter::filter_obj_handler::base_handler::bound_int_type file_filter::filter_obj_handler::bind_simple_int(std::wstring key) {
	base_handler::bound_int_type ret;
	if (key == _T("size"))
		ret = &filter_obj::get_size;
	else if (key == _T("line_count"))
		ret = &filter_obj::get_line_count;
	else if (key == _T("access"))
		ret = &filter_obj::get_access;
	else if (key == _T("creation"))
		ret = &filter_obj::get_creation;
	else if (key == _T("written"))
		ret = &filter_obj::get_write;
	else
		NSC_DEBUG_MSG_STD(_T("Failed to bind (int): ") + key);
	return ret;
}

bool file_filter::filter_obj_handler::has_function(parsers::where::value_type to, std::wstring name, ast_expr_type *subject) {
	return false;
}
file_filter::filter_obj_handler::base_handler::bound_function_type file_filter::filter_obj_handler::bind_simple_function(parsers::where::value_type to, std::wstring name, ast_expr_type *subject) {
	base_handler::bound_function_type ret;
	return ret;
}

//////////////////////////////////////////////////////////////////////////

#ifdef WIN32
file_filter::filter_obj file_filter::filter_obj::get(unsigned long long now, const WIN32_FILE_ATTRIBUTE_DATA info, boost::filesystem::wpath path, std::wstring filename) {
	return file_filter::filter_obj(path, filename, now, 
		(info.ftCreationTime.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.ftCreationTime.dwLowDateTime,
		(info.ftLastAccessTime.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.ftLastAccessTime.dwLowDateTime,
		(info.ftLastWriteTime.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.ftLastWriteTime.dwLowDateTime,
		(info.nFileSizeHigh * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.nFileSizeLow
		);
};
file_filter::filter_obj file_filter::filter_obj::get(unsigned long long now, const BY_HANDLE_FILE_INFORMATION info, boost::filesystem::wpath path, std::wstring filename) {
	return file_filter::filter_obj(path, filename, now, 
		(info.ftCreationTime.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.ftCreationTime.dwLowDateTime,
		(info.ftLastAccessTime.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.ftLastAccessTime.dwLowDateTime,
		(info.ftLastWriteTime.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.ftLastWriteTime.dwLowDateTime,
		(info.nFileSizeHigh * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.nFileSizeLow
	);
};
boost::shared_ptr<file_filter::filter_obj> file_filter::filter_obj::get(unsigned long long now, const WIN32_FIND_DATA info, boost::filesystem::wpath path) {
	return boost::shared_ptr<file_filter::filter_obj>(new file_filter::filter_obj(path, info.cFileName, now, 
		(info.ftCreationTime.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.ftCreationTime.dwLowDateTime, 
		(info.ftLastAccessTime.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.ftLastAccessTime.dwLowDateTime,
		(info.ftLastWriteTime.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.ftLastWriteTime.dwLowDateTime,
		(info.nFileSizeHigh * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)info.nFileSizeLow
		));
	//attributes = info.dwFileAttributes;
};
#endif
file_filter::filter_obj file_filter::filter_obj::get(unsigned long long now, boost::filesystem::wpath path, std::wstring filename) {
 	WIN32_FILE_ATTRIBUTE_DATA data;
 	if (!GetFileAttributesEx((path.string() + _T("\\") + filename).c_str(), GetFileExInfoStandard, reinterpret_cast<LPVOID>(&data))) {
		throw new file_object_exception("Could not open file (2) " + utf8::cvt<std::string>(path.string()) + "\\" + utf8::cvt<std::string>(filename) + ": " + utf8::cvt<std::string>(error::lookup::last_error()));
	}
	return get(now, data, path, filename);
}
file_filter::filter_obj file_filter::filter_obj::get(std::wstring file) {
	FILETIME now;
	GetSystemTimeAsFileTime(&now);
	unsigned __int64 nowi64 = ((now.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)now.dwLowDateTime);
	return get(nowi64, file);
}
file_filter::filter_obj file_filter::filter_obj::get(unsigned long long now, std::wstring file) {

	BY_HANDLE_FILE_INFORMATION _info;

	HANDLE hFile = CreateFile(file.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
		0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		throw new file_object_exception("Could not open file (2) " + utf8::cvt<std::string>(file) + ": " + utf8::cvt<std::string>(error::lookup::last_error()));
	}
	GetFileInformationByHandle(hFile, &_info);
	CloseHandle(hFile);
	return get(now, _info, file_helpers::meta::get_path(file), file_helpers::meta::get_filename(file));
}



std::wstring file_filter::filter_obj::get_version(filter_obj_handler *handler) {
	if (cached_version)
		return *cached_version;
	std::wstring fullpath = (path / filename).string();

	DWORD dwDummy;
	DWORD dwFVISize = GetFileVersionInfoSize(fullpath.c_str(), &dwDummy);
	if (dwFVISize == 0)
		return _T("");
	LPBYTE lpVersionInfo = new BYTE[dwFVISize+1];
	if (!GetFileVersionInfo(fullpath.c_str(),0,dwFVISize,lpVersionInfo)) {
		delete [] lpVersionInfo;
		handler->error(_T("Failed to get version for ") + fullpath + _T(": ") + error::lookup::last_error());
		return _T("");
	}
	UINT uLen;
	VS_FIXEDFILEINFO *lpFfi;
	if (!VerQueryValue( lpVersionInfo , _T("\\") , (LPVOID *)&lpFfi , &uLen )) {
		delete [] lpVersionInfo;
		handler->error(_T("Failed to query version for ") + fullpath + _T(": ") + error::lookup::last_error());
		return _T("");
	}
	DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
	DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;
	delete [] lpVersionInfo;
	DWORD dwLeftMost = HIWORD(dwFileVersionMS);
	DWORD dwSecondLeft = LOWORD(dwFileVersionMS);
	DWORD dwSecondRight = HIWORD(dwFileVersionLS);
	DWORD dwRightMost = LOWORD(dwFileVersionLS);
	cached_version.reset(strEx::itos(dwLeftMost) + _T(".") +
		strEx::itos(dwSecondLeft) + _T(".") +
		strEx::itos(dwSecondRight) + _T(".") +
		strEx::itos(dwRightMost));
	return *cached_version;
}

unsigned long file_filter::filter_obj::get_line_count() {
	if (cached_count)
		return *cached_count;

	unsigned long count = 0;
	std::wstring fullpath = (path / filename).string();
	FILE * pFile = fopen(strEx::wstring_to_string(fullpath).c_str(),"r");;
	if (pFile==NULL) 
		return 0;
	char c;
	do {
		c = fgetc (pFile);
		if (c == '\r') {
			c = fgetc (pFile);
			count++;
		} else if (c == '\n') {
			c = fgetc (pFile);
			count++;
		}
	} while (c != EOF);
	fclose (pFile);
	cached_count.reset(count);
	return *cached_count;
}


std::wstring file_filter::filter_obj::render(std::wstring syntax, std::wstring datesyntax) {
	strEx::replace(syntax, _T("%path%"), path.string());
	strEx::replace(syntax, _T("%filename%"), filename);
	strEx::replace(syntax, _T("%creation%"), strEx::format_filetime(ullCreationTime, datesyntax));
	strEx::replace(syntax, _T("%access%"), strEx::format_filetime(ullLastAccessTime, datesyntax));
	strEx::replace(syntax, _T("%write%"), strEx::format_filetime(ullLastWriteTime, datesyntax));
	strEx::replace(syntax, _T("%creation-raw%"), strEx::itos(ullCreationTime));
	strEx::replace(syntax, _T("%access-raw%"), strEx::itos(ullLastAccessTime));
	strEx::replace(syntax, _T("%write-raw%"), strEx::itos(ullLastWriteTime));
	strEx::replace(syntax, _T("%now-raw%"), strEx::itos(ullNow));
/*
	strEx::replace(syntax, _T("%creation-d%"), strEx::format_filetime(ullCreationTime, DATE_FORMAT));
	strEx::replace(syntax, _T("%access-d%"), strEx::format_filetime(ullLastAccessTime, DATE_FORMAT));
	strEx::replace(syntax, _T("%write-d%"), strEx::format_filetime(ullLastWriteTime, DATE_FORMAT));
*/
	strEx::replace(syntax, _T("%size%"), strEx::itos_as_BKMG(ullSize));
	if (cached_version)
		strEx::replace(syntax, _T("%version%"), *cached_version);
	if (cached_count)
		strEx::replace(syntax, _T("%line_count%"), strEx::itos(*cached_count));
	return syntax;
}

//////////////////////////////////////////////////////////////////////////

struct where_mode_filter : public file_filter::filter_engine_type {
	typedef file_filter::filter_obj_handler handler_type;
	typedef file_filter::filter_obj object_type;

	typedef boost::shared_ptr<handler_type> handler_instance_type;

	file_filter::filter_argument data;
	parsers::where::parser ast_parser;
	handler_instance_type object_handler;

	where_mode_filter(file_filter::filter_argument data) : data(data), object_handler(handler_instance_type(new handler_type())) {}
	bool boot() { return true; }

	bool validate(std::wstring &message) {
		if (data->debug)
			data->error->report_debug(_T("Parsing: ") + data->filter);

		if (!ast_parser.parse(data->filter)) {
			data->error->report_error(_T("Parsing failed of '") + data->filter + _T("' at: ") + ast_parser.rest);
		message = _T("Parsing failed: ") + ast_parser.rest;
		return false;
		}
		if (data->debug)
			data->error->report_debug(_T("Parsing succeeded: ") + ast_parser.result_as_tree());

		if (!ast_parser.derive_types(object_handler) || object_handler->has_error()) {
			message = _T("Invalid types: ") + object_handler->get_error();
			return false;
		}
			if (data->debug)
				data->error->report_debug(_T("Type resolution succeeded: ") + ast_parser.result_as_tree());

		if (!ast_parser.bind(object_handler) || object_handler->has_error()) {
			message = _T("Variable and function binding failed: ") + object_handler->get_error();
			return false;
		}
			if (data->debug)
				data->error->report_debug(_T("Binding succeeded: ") + ast_parser.result_as_tree());

		if (!ast_parser.static_eval(object_handler) || object_handler->has_error()) {
			message = _T("Static evaluation failed: ") + object_handler->get_error();
			return false;
		}
			if (data->debug)
				data->error->report_debug(_T("Static evaluation succeeded: ") + ast_parser.result_as_tree());

		return true;
	}

	bool match(boost::shared_ptr<object_type> record) {
		object_handler->set_current_element(record);
		bool ret = ast_parser.evaluate(object_handler);
		if (object_handler->has_error()) {
			data->error->report_error(_T("Error: ") + object_handler->get_error());
		}
		return ret;
	}

	std::wstring get_name() {
		return _T("where");
	}
	std::wstring get_subject() { return data->filter; }
};

//////////////////////////////////////////////////////////////////////////

file_filter::file_finder_data_arguments::file_finder_data_arguments(std::wstring pattern, int max_depth, file_filter::filter_argument_type::error_type error, std::wstring syntax, std::wstring datesyntax, bool debug) 
		: debugThreshold(0), bFilterIn(true), bFilterAll(false), bShowDescriptions(false), max_level(max_depth), pattern(pattern) 
		, file_filter::file_finder_data_arguments::parent_type(error, syntax, datesyntax, debug)
{
	FILETIME ft_now;
	GetSystemTimeAsFileTime(&ft_now);
	now = ((ft_now.dwHighDateTime * ((unsigned long long)MAXDWORD+1)) + (unsigned long long)ft_now.dwLowDateTime);
}

struct size_file_engine : public file_filter::filesize_engine_interface_type {
	size_file_engine() : size(0) {}

	bool boot() { return true; }
	bool validate(std::wstring &message) {
		return true;
	}
	bool match(file_filter::filter_obj &record) {
		if (!file_helpers::checks::is_directory(record.attributes)) {
			size += record.get_size();
		}
		return true;
	}
	std::wstring get_name() { return _T("file-size"); }
	std::wstring get_subject() { return _T("TODO"); }
	unsigned long long get_size() {
		return size;
	}
private:  
	unsigned long long size;

};



//////////////////////////////////////////////////////////////////////////
file_filter::filter_engine file_filter::factories::create_engine(file_filter::filter_argument arg) {
	return filter_engine(new where_mode_filter(arg));
}
// file_filter::filesize_engine_interface file_filter::factories::create_size_engine() {
// 	return filesize_engine_interface(new size_file_engine());
// }
file_filter::filter_argument file_filter::factories::create_argument(std::wstring pattern, int max_depth, std::wstring syntax, std::wstring datesyntax) {
	return filter_argument(new file_filter::file_finder_data_arguments(pattern, max_depth, file_filter::filter_argument_type::error_type(new where_filter::nsc_error_handler()), syntax, datesyntax));
}

file_filter::filter_result file_filter::factories::create_result(file_filter::filter_argument arg) {
	return filter_result(new where_filter::simple_count_result<filter_obj>(arg));
}





