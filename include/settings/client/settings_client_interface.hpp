#pragma once

#include <map>
#include <list>

namespace nscapi {
	namespace settings_helper {

		class settings_impl_interface {
		public:
			typedef std::list<std::wstring> string_list;

			//////////////////////////////////////////////////////////////////////////
			/// Register a path with the settings module.
			/// A registered key or path will be nicely documented in some of the settings files when converted.
			///
			/// @param path The path to register
			/// @param title The title to use
			/// @param description the description to use
			/// @param advanced advanced options will only be included if they are changed
			///
			/// @author mickem
			virtual void register_path(std::wstring path, std::wstring title, std::wstring description, bool advanced) = 0;

			//////////////////////////////////////////////////////////////////////////
			/// Register a key with the settings module.
			/// A registered key or path will be nicely documented in some of the settings files when converted.
			///
			/// @param path The path to register
			/// @param key The key to register
			/// @param type The type of value
			/// @param title The title to use
			/// @param description the description to use
			/// @param defValue the default value
			/// @param advanced advanced options will only be included if they are changed
			///
			/// @author mickem
			virtual void register_key(std::wstring path, std::wstring key, int type, std::wstring title, std::wstring description, std::wstring defValue, bool advanced) = 0;

			//////////////////////////////////////////////////////////////////////////
			/// Get a string value if it does not exist the default value will be returned
			/// 
			/// @param path the path to look up
			/// @param key the key to lookup
			/// @param def the default value to use when no value is found
			/// @return the string value
			///
			/// @author mickem
			virtual std::wstring get_string(std::wstring path, std::wstring key, std::wstring def) = 0;
			//////////////////////////////////////////////////////////////////////////
			/// Set or update a string value
			///
			/// @param path the path to look up
			/// @param key the key to lookup
			/// @param value the value to set
			///
			/// @author mickem
			virtual void set_string(std::wstring path, std::wstring key, std::wstring value) = 0;

			//////////////////////////////////////////////////////////////////////////
			/// Get an integer value if it does not exist the default value will be returned
			/// 
			/// @param path the path to look up
			/// @param key the key to lookup
			/// @param def the default value to use when no value is found
			/// @return the string value
			///
			/// @author mickem
			virtual int get_int(std::wstring path, std::wstring key, int def) = 0;
			//////////////////////////////////////////////////////////////////////////
			/// Set or update an integer value
			///
			/// @param path the path to look up
			/// @param key the key to lookup
			/// @param value the value to set
			///
			/// @author mickem
			virtual void set_int(std::wstring path, std::wstring key, int value) = 0;

			//////////////////////////////////////////////////////////////////////////
			/// Get a boolean value if it does not exist the default value will be returned
			/// 
			/// @param path the path to look up
			/// @param key the key to lookup
			/// @param def the default value to use when no value is found
			/// @return the string value
			///
			/// @author mickem
			virtual bool get_bool(std::wstring path, std::wstring key, bool def) = 0;
			//////////////////////////////////////////////////////////////////////////
			/// Set or update a boolean value
			///
			/// @param path the path to look up
			/// @param key the key to lookup
			/// @param value the value to set
			///
			/// @author mickem
			virtual void set_bool(std::wstring path, std::wstring key, bool value) = 0;

			// Meta Functions
			//////////////////////////////////////////////////////////////////////////
			/// Get all (sub) sections (given a path).
			/// If the path is empty all root sections will be returned
			///
			/// @param path The path to get sections from (if empty root sections will be returned)
			/// @return a list of sections
			///
			/// @author mickem
			virtual string_list get_sections(std::wstring path) = 0;
			//////////////////////////////////////////////////////////////////////////
			/// Get all keys for a path.
			///
			/// @param path The path to get keys under
			/// @return a list of keys
			///
			/// @author mickem
			virtual string_list get_keys(std::wstring path) = 0;


			virtual std::wstring expand_path(std::wstring key) = 0;

			//////////////////////////////////////////////////////////////////////////
			/// Log an ERROR message.
			///
			/// @param file the file where the event happened
			/// @param line the line where the event happened
			/// @param message the message to log
			///
			/// @author mickem
			virtual void err(const char* file, int line, std::wstring message) = 0;
			//////////////////////////////////////////////////////////////////////////
			/// Log an WARNING message.
			///
			/// @param file the file where the event happened
			/// @param line the line where the event happened
			/// @param message the message to log
			///
			/// @author mickem
			virtual void warn(const char* file, int line, std::wstring message) = 0;
			//////////////////////////////////////////////////////////////////////////
			/// Log an INFO message.
			///
			/// @param file the file where the event happened
			/// @param line the line where the event happened
			/// @param message the message to log
			///
			/// @author mickem
			virtual void info(const char* file, int line, std::wstring message) = 0;
			//////////////////////////////////////////////////////////////////////////
			/// Log an DEBUG message.
			///
			/// @param file the file where the event happened
			/// @param line the line where the event happened
			/// @param message the message to log
			///
			/// @author mickem
			virtual void debug(const char* file, int line, std::wstring message) = 0;
		};
	}
}