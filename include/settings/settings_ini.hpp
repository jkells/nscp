#pragma once

#include <string>
#include <map>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <settings/settings_core.hpp>
#include <settings/settings_core_impl.hpp>
//#define SI_CONVERT_ICU
#include <simpleini/simpleini.h>
#include <error.hpp>

namespace settings {
	class INISettings : public settings::SettingsInterfaceImpl {
	private:
		std::wstring filename_;
		bool is_loaded_;
		CSimpleIni ini;

	public:
		INISettings(settings::settings_core *core, std::wstring context) : ini(false, false, false), is_loaded_(false), settings::SettingsInterfaceImpl(core, context) {
			load_data();
		}
		//////////////////////////////////////////////////////////////////////////
		/// Create a new settings interface of "this kind"
		///
		/// @param context the context to use
		/// @return the newly created settings interface
		///
		/// @author mickem
		virtual SettingsInterfaceImpl* create_new_context(std::wstring context) {
			return new INISettings(get_core(), context);
		}
		//////////////////////////////////////////////////////////////////////////
		/// Get a string value if it does not exist exception will be thrown
		///
		/// @param path the path to look up
		/// @param key the key to lookup
		/// @return the string value
		///
		/// @author mickem
		virtual std::wstring get_real_string(settings_core::key_path_type key) {
			load_data();
			const wchar_t *val = ini.GetValue(key.first.c_str(), key.second.c_str(), NULL);
			if (val == NULL)
				throw KeyNotFoundException(key);
			return val;
		}
		//////////////////////////////////////////////////////////////////////////
		/// Get an integer value if it does not exist exception will be thrown
		///
		/// @param path the path to look up
		/// @param key the key to lookup
		/// @return the int value
		///
		/// @author mickem
		virtual int get_real_int(settings_core::key_path_type key) {
			std::wstring str = get_real_string(key);
			return strEx::stoi(str);
		}
		//////////////////////////////////////////////////////////////////////////
		/// Get a boolean value if it does not exist exception will be thrown
		///
		/// @param path the path to look up
		/// @param key the key to lookup
		/// @return the boolean value
		///
		/// @author mickem
		virtual bool get_real_bool(settings_core::key_path_type key) {
			std::wstring str = get_real_string(key);
			return SettingsInterfaceImpl::string_to_bool(str);
		}
		//////////////////////////////////////////////////////////////////////////
		/// Check if a key exists
		///
		/// @param path the path to look up
		/// @param key the key to lookup
		/// @return true/false if the key exists.
		///
		/// @author mickem
		virtual bool has_real_key(settings_core::key_path_type key) {
			return ini.GetValue(key.first.c_str(), key.second.c_str()) != NULL;
		}
		//////////////////////////////////////////////////////////////////////////
		/// Get the type this settings store represent.
		///
		/// @return the type of settings store
		///
		/// @author mickem
// 		virtual settings_core::settings_type get_type() {
// 			return settings_core::ini_file;
// 		}
		//////////////////////////////////////////////////////////////////////////
		/// Is this the active settings store
		///
		/// @return
		///
		/// @author mickem
// 		virtual bool is_active() {
// 			return true;
// 		}
		//////////////////////////////////////////////////////////////////////////
		/// Write a value to the resulting context.
		///
		/// @param key The key to write to
		/// @param value The value to write
		///
		/// @author mickem
		virtual void set_real_value(settings_core::key_path_type key, conainer value) {
			try {
				const settings_core::key_description desc = get_core()->get_registred_key(key.first, key.second);
				std::wstring comment = _T("; ");
				if (!desc.title.empty())
					comment += desc.title + _T(" - ");
				if (!desc.description.empty())
					comment += desc.description;
				strEx::replace(comment, _T("\n"), _T(" "));
				
				ini.Delete(key.first.c_str(), key.second.c_str());
				ini.SetValue(key.first.c_str(), key.second.c_str(), value.get_string().c_str(), comment.c_str());
			} catch (KeyNotFoundException e) {
				ini.SetValue(key.first.c_str(), key.second.c_str(), value.get_string().c_str(), _T("; Undocumented key"));
			} catch (settings_exception e) {
				nsclient::logging::logger::get_logger()->error(__FILE__, __LINE__, std::wstring(_T("Failed to write key: ") + e.getError()));
			} catch (...) {
				nsclient::logging::logger::get_logger()->error(__FILE__, __LINE__, std::wstring(_T("Unknown filure when writing key: ") + key.first + _T(".") + key.second));
			}
		}

		virtual void set_real_path(std::wstring path) {
			try {
				const settings_core::path_description desc = get_core()->get_registred_path(path);
				if (!desc.description.empty()) {
					std::wstring comment = _T("; ") + desc.description;
					ini.SetValue(path.c_str(), NULL, NULL, comment.c_str());
				}
			} catch (KeyNotFoundException e) {
				ini.SetValue(path.c_str(), NULL, NULL, _T("; Undocumented section"));
			} catch (settings_exception e) {
				nsclient::logging::logger::get_logger()->error(__FILE__, __LINE__, std::wstring(_T("Failed to write section: ") + e.getError()));
			} catch (...) {
				nsclient::logging::logger::get_logger()->error(__FILE__, __LINE__, std::wstring(_T("Unknown filure when writing section: ") + path));
			}
		}

		//////////////////////////////////////////////////////////////////////////
		/// Get all (sub) sections (given a path).
		/// If the path is empty all root sections will be returned
		///
		/// @param path The path to get sections from (if empty root sections will be returned)
		/// @param list The list to append nodes to
		/// @return a list of sections
		///
		/// @author mickem
		virtual void get_real_sections(std::wstring path, string_list &list) {
			CSimpleIni::TNamesDepend lst;
			std::wstring::size_type path_len = path.length();
			ini.GetAllSections(lst);
			if (path.empty()) {
				for (CSimpleIni::TNamesDepend::const_iterator cit = lst.begin(); cit != lst.end(); ++cit) {
					std::wstring mapped = (*cit).pItem;
					if (mapped.length() > 1) {
						std::wstring::size_type pos = mapped.find(L'/', 1);
						if (pos != std::wstring::npos)
							mapped = mapped.substr(0,pos);
					}
					list.push_back(mapped);
				}
			} else {
				BOOST_FOREACH(const CSimpleIni::Entry e, lst) {
					std::wstring key = e.pItem;
					if (key.length() > path_len+1 && key.substr(0,path_len) == path) {
						std::wstring::size_type pos = key.find(L'/', path_len+1);
						if (pos == std::wstring::npos)
							key = key.substr(path_len+1);
						else
							key = key.substr(path_len+1, pos-path_len-1);
						list.push_back(key);
					}
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////
		/// Get all keys given a path/section.
		/// If the path is empty all root sections will be returned
		///
		/// @param path The path to get sections from (if empty root sections will be returned)
		/// @param list The list to append nodes to
		/// @return a list of sections
		///
		/// @author mickem
		virtual void get_real_keys(std::wstring path, string_list &list) {
			load_data();
			CSimpleIni::TNamesDepend lst;
			ini.GetAllKeys(path.c_str(), lst);
			for (CSimpleIni::TNamesDepend::const_iterator cit = lst.begin(); cit != lst.end(); ++cit) {
				list.push_back((*cit).pItem);
			}
		}
		//////////////////////////////////////////////////////////////////////////
		/// Save the settings store
		///
		/// @author mickem
		virtual void save() {
			SettingsInterfaceImpl::save();
			SI_Error rc = ini.SaveFile(get_file_name().c_str());
			if (rc < 0)
				throw_SI_error(rc, _T("Failed to save file"));
		}
// 		virtual settings_core::key_type get_key_type(std::wstring path, std::wstring key) {
// 			return SettingsInterfaceImpl::get_key_type(path, key);
// 			return settings_core::key_string;
// 		}
	private:
		void load_data() {
			if (is_loaded_)
				return;
			if (boost::filesystem::is_directory(get_file_name())) {

				boost::filesystem::wdirectory_iterator it(get_file_name()), eod;

				BOOST_FOREACH(boost::filesystem::wpath const &p, std::make_pair(it, eod)) {
					add_child(_T("ini:///") + p.string());
				}
			}
			if (!file_exists()) {
				is_loaded_ = true;
				return;
			}
			std::wstring f = get_file_name();
			ini.SetUnicode();
			nsclient::logging::logger::get_logger()->debug(__FILE__, __LINE__, _T("Loading: ") + f + _T(" from ") + get_context());
			SI_Error rc = ini.LoadFile(f.c_str());
			if (rc < 0)
				throw_SI_error(rc, _T("Failed to load file"));

			CSimpleIni::TNamesDepend lst;
			ini.GetAllKeys(_T("/includes"), lst);
			for (CSimpleIni::TNamesDepend::const_iterator cit = lst.begin(); cit != lst.end(); ++cit) {
				add_child(ini.GetValue(_T("/includes"), (*cit).pItem));
			}
			is_loaded_ = true;
		}
		void throw_SI_error(SI_Error err, std::wstring msg) {
			std::wstring error_str = _T("unknown error");
			if (err == SI_NOMEM)
				error_str = _T("Out of memmory");
			if (err == SI_FAIL)
				error_str = _T("General failure");
			if (err == SI_FILE)
				error_str = _T("I/O error: ") + error::lookup::last_error();
			throw settings_exception(msg + _T(" '") + get_context() + _T("': ") + error_str);
		}
		std::wstring get_file_name() {
			if (filename_.empty()) {
				filename_ = get_file_from_context();
				if (filename_.size() > 0) {
					if (boost::filesystem::is_regular(filename_)) {
					} else if (boost::filesystem::is_regular(filename_.substr(1))) {
						filename_ = filename_.substr(1);
					} else if (boost::filesystem::is_directory(filename_)) {
					} else if (boost::filesystem::is_directory(filename_.substr(1))) {
						filename_ = filename_.substr(1);
					}
				}
				nsclient::logging::logger::get_logger()->debug(__FILE__, __LINE__, _T("Reading INI settings from: ") + filename_);
			}
			return filename_;
		}
		bool file_exists() {
			return boost::filesystem::is_regular(get_file_name());
		}
		virtual std::wstring get_info() {
			return _T("INI settings: (") + context_ + _T(", ") + get_file_name() + _T(")");
		}
		public:
		static bool context_exists(settings::settings_core *core, std::wstring key) {
			net::wurl url = net::parse(key);
			std::wstring file = url.host + url.path;
			std::wstring tmp = core->expand_path(file);
			if (tmp.size()>1 && tmp[0] == '/') {
				if (boost::filesystem::is_regular(tmp) || boost::filesystem::is_directory(tmp))
					return true;
				tmp = tmp.substr(1);
			}
			return boost::filesystem::is_regular(tmp) || boost::filesystem::is_directory(tmp);
		}

	};
}
