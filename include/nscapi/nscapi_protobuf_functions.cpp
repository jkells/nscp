/**************************************************************************
*   Copyright (C) 2004-2007 by Michael Medin <michael@medin.name>         *
*                                                                         *
*   This code is part of NSClient++ - http://trac.nakednuns.org/nscp      *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#include <nscapi/nscapi_protobuf_functions.hpp>

namespace nscapi {
	namespace protobuf {

		namespace traits {

			template<class T>
			struct perf_data_consts {
				static const T get_valid_perf_numbers();
				static const T get_replace_perf_coma_src();
				static const T get_replace_perf_coma_tgt();
			};

			template<>
			struct perf_data_consts<std::wstring> {
				static const std::wstring get_valid_perf_numbers() {
					return _T("0123456789,.");
				}
				static const std::wstring get_replace_perf_coma_src() {
					return _T(",");
				}
				static const std::wstring get_replace_perf_coma_tgt() {
					return _T(".");
				}
			};
			template<>
			struct perf_data_consts<std::string> {
				static const std::string get_valid_perf_numbers() {
					return "0123456789,.";
				}
				static const std::string get_replace_perf_coma_src() {
					return ",";
				}
				static const std::string get_replace_perf_coma_tgt() {
					return ".";
				}
			};
		}

		template<class T>
		double trim_to_double(T s) {
			typename T::size_type pend = s.find_first_not_of(traits::perf_data_consts<T>::get_valid_perf_numbers());
			if (pend != T::npos)
				s = s.substr(0,pend);
			strEx::replace(s, traits::perf_data_consts<T>::get_replace_perf_coma_src(), traits::perf_data_consts<T>::get_replace_perf_coma_tgt());
			return strEx::stod(s);
		}



		void functions::create_simple_header(Plugin::Common::Header* hdr)  {
			hdr->set_version(Plugin::Common_Version_VERSION_1);
			hdr->set_max_supported_version(Plugin::Common_Version_VERSION_1);
			// @todo add additional fields here!
		}

		//////////////////////////////////////////////////////////////////////////

		void functions::add_host(Plugin::Common::Header* hdr, const destination_container &dst)  {
			::Plugin::Common::Host *host = hdr->add_hosts();
			if (!dst.id.empty())
				host->set_id(dst.id);
			if (!dst.address.host.empty())
				host->set_address(dst.address.to_string());
			if (!dst.has_protocol())
				host->set_protocol(dst.get_protocol());
			if (!dst.comment.empty())
				host->set_comment(dst.comment);
			BOOST_FOREACH(const std::string &t, dst.tags) {
				host->add_tags(t);
			}
			BOOST_FOREACH(const destination_container::data_map::value_type &kvp, dst.data) {
				::Plugin::Common_KeyValue* x = host->add_metadata();
				x->set_key(kvp.first);
				x->set_value(kvp.second);
			}
		}

		bool functions::parse_destination(const ::Plugin::Common_Header &header, const std::string tag, destination_container &data, const bool expand_meta) {
			for (int i=0;i<header.hosts_size();++i) {
				const ::Plugin::Common::Host &host = header.hosts(i);
				if (host.id() == tag) {
					data.id = tag;
					if (!host.address().empty())
						data.address.import(net::parse(host.address()));
					if (!host.comment().empty())
						data.comment = host.comment();
					if (expand_meta) {
						for(int j=0;j<host.tags_size(); ++j) {
							data.tags.insert(host.tags(j));
						}
						for(int j=0;j<host.metadata_size(); ++j) {
							data.data[host.metadata(j).key()] = host.metadata(j).value();
						}
					}
					return true;
				}
			}
			return false;
		}

		//////////////////////////////////////////////////////////////////////////

		void functions::make_submit_from_query(std::string &message, const std::wstring channel, const std::wstring alias, const std::wstring target) {
			Plugin::QueryResponseMessage response;
			response.ParseFromString(message);
			Plugin::SubmitRequestMessage request;
			request.mutable_header()->CopyFrom(response.header());
			request.mutable_header()->set_source_id(request.mutable_header()->recipient_id());
			request.set_channel(utf8::cvt<std::string>(channel));
			if (!target.empty()) {
				request.mutable_header()->set_recipient_id(utf8::cvt<std::string>(target));
			}
			for (int i=0;i<response.payload_size();++i) {
				request.add_payload()->CopyFrom(response.payload(i));
				if (!alias.empty())
					request.mutable_payload(i)->set_alias(utf8::cvt<std::string>(alias));
			}
			message = request.SerializeAsString();
		}

		void functions::make_query_from_exec(std::string &data) {
			Plugin::ExecuteResponseMessage exec_response_message;
			exec_response_message.ParseFromString(data);
			Plugin::QueryResponseMessage query_response_message;
			query_response_message.mutable_header()->CopyFrom(exec_response_message.header());
			for (int i=0;i<exec_response_message.payload_size();++i) {
				Plugin::ExecuteResponseMessage::Response p = exec_response_message.payload(i);
				append_simple_query_response_payload(query_response_message.add_payload(), p.command(), p.result(), p.message());
			}
			data = query_response_message.SerializeAsString();
		}
		void functions::make_query_from_submit(std::string &data) {
			Plugin::SubmitResponseMessage submit_response_message;
			submit_response_message.ParseFromString(data);
			Plugin::QueryResponseMessage query_response_message;
			query_response_message.mutable_header()->CopyFrom(submit_response_message.header());
			for (int i=0;i<submit_response_message.payload_size();++i) {
				Plugin::SubmitResponseMessage::Response p = submit_response_message.payload(i);
				append_simple_query_response_payload(query_response_message.add_payload(), p.command(), gbp_status_to_gbp_nagios(p.status().status()), p.status().message(), "");
			}
			data = query_response_message.SerializeAsString();
		}

		void functions::make_exec_from_submit(std::string &data) {
			Plugin::SubmitResponseMessage submit_response_message;
			submit_response_message.ParseFromString(data);
			Plugin::ExecuteResponseMessage exec_response_message;
			exec_response_message.mutable_header()->CopyFrom(submit_response_message.header());
			for (int i=0;i<submit_response_message.payload_size();++i) {
				Plugin::SubmitResponseMessage::Response p = submit_response_message.payload(i);
				append_simple_exec_response_payload(exec_response_message.add_payload(), p.command(), gbp_status_to_gbp_nagios(p.status().status()), p.status().message());
			}
			data = exec_response_message.SerializeAsString();
		}
		void functions::make_exec_from_query(std::string &data) {
			Plugin::QueryResponseMessage query_response_message;
			query_response_message.ParseFromString(data);
			Plugin::ExecuteResponseMessage exec_response_message;
			exec_response_message.mutable_header()->CopyFrom(query_response_message.header());
			for (int i=0;i<query_response_message.payload_size();++i) {
				Plugin::QueryResponseMessage::Response p = query_response_message.payload(i);
				std::string s = build_performance_data(p);
				if (!s.empty())
					s = p.message() + "|" + s;
				else
					s = p.message();
				append_simple_exec_response_payload(exec_response_message.add_payload(), p.command(), p.result(), s);
			}
			data = exec_response_message.SerializeAsString();
		}


		void functions::make_return_header(::Plugin::Common_Header *target, const ::Plugin::Common_Header &source) {
			target->CopyFrom(source);
			target->set_source_id(target->recipient_id());
		}

		void functions::create_simple_query_request(std::wstring command, std::vector<std::wstring> arguments, std::string &buffer) {
			Plugin::QueryRequestMessage message;
			create_simple_header(message.mutable_header());

			Plugin::QueryRequestMessage::Request *payload = message.add_payload();
			payload->set_command(utf8::cvt<std::string>(command));

			BOOST_FOREACH(std::wstring s, arguments)
				payload->add_arguments(utf8::cvt<std::string>(s));

			message.SerializeToString(&buffer);
		}

		void functions::create_simple_submit_request(std::wstring channel, std::wstring command, NSCAPI::nagiosReturn ret, std::wstring msg, std::wstring perf, std::string &buffer) {
			Plugin::SubmitRequestMessage message;
			create_simple_header(message.mutable_header());
			message.set_channel(utf8::cvt<std::string>(channel));

			Plugin::QueryResponseMessage::Response *payload = message.add_payload();
			payload->set_command(utf8::cvt<std::string>(command));
			payload->set_message(utf8::cvt<std::string>(msg));
			payload->set_result(nagios_status_to_gpb(ret));
			if (!perf.empty())
				parse_performance_data(payload, perf);

			message.SerializeToString(&buffer);
		}
		void functions::create_simple_submit_response(std::wstring channel, std::wstring command, NSCAPI::nagiosReturn ret, std::wstring msg, std::string &buffer) {
			Plugin::SubmitResponseMessage message;
			create_simple_header(message.mutable_header());
			//message.set_channel(to_string(channel));

			Plugin::SubmitResponseMessage::Response *payload = message.add_payload();
			payload->set_command(utf8::cvt<std::string>(command));
			payload->mutable_status()->set_message(utf8::cvt<std::string>(msg));
			payload->mutable_status()->set_status(status_to_gpb(ret));
			message.SerializeToString(&buffer);
		}
		NSCAPI::errorReturn functions::parse_simple_submit_request(const std::string &request, std::wstring &source, std::wstring &command, std::wstring &msg, std::wstring &perf) {
			Plugin::SubmitRequestMessage message;
			message.ParseFromString(request);

			if (message.payload_size() != 1) {
				throw nscapi_exception("Whoops, invalid payload size (for now)");
			}
			Plugin::QueryResponseMessage::Response payload = message.payload().Get(0);
			source = utf8::cvt<std::wstring>(payload.source());
			command = utf8::cvt<std::wstring>(payload.command());
			msg = utf8::cvt<std::wstring>(payload.message());
			perf = utf8::cvt<std::wstring>(build_performance_data(payload));
			return gbp_to_nagios_status(payload.result());
		}
		int functions::parse_simple_submit_request_payload(const Plugin::QueryResponseMessage::Response &payload, std::wstring &alias, std::wstring &message, std::wstring &perf) {
			alias = utf8::cvt<std::wstring>(payload.alias());
			if (alias.empty())
				alias = utf8::cvt<std::wstring>(payload.command());
			message = utf8::cvt<std::wstring>(payload.message());
			perf = utf8::cvt<std::wstring>(build_performance_data(payload));
			return gbp_to_nagios_status(payload.result());
		}
		int functions::parse_simple_submit_request_payload(const Plugin::QueryResponseMessage::Response &payload, std::wstring &alias, std::wstring &message) {
			alias = utf8::cvt<std::wstring>(payload.alias());
			if (alias.empty())
				alias = utf8::cvt<std::wstring>(payload.command());
			message = utf8::cvt<std::wstring>(payload.message());
			return gbp_to_nagios_status(payload.result());
		}
		void functions::parse_simple_query_request_payload(const Plugin::QueryRequestMessage::Request &payload, std::wstring &alias, std::wstring &command) {
			alias = utf8::cvt<std::wstring>(payload.alias());
			command = utf8::cvt<std::wstring>(payload.command());
		}
		NSCAPI::errorReturn functions::parse_simple_submit_response(const std::string &request, std::wstring &response) {
			Plugin::SubmitResponseMessage message;
			message.ParseFromString(request);

			if (message.payload_size() != 1) {
				throw nscapi_exception("Whoops, invalid payload size (for now)");
			}
			::Plugin::SubmitResponseMessage::Response payload = message.payload().Get(0);
			response = utf8::cvt<std::wstring>(payload.mutable_status()->message());
			return gbp_to_status(payload.mutable_status()->status());
		}
		NSCAPI::errorReturn functions::parse_simple_submit_response(const std::string &request, std::string response) {
			Plugin::SubmitResponseMessage message;
			message.ParseFromString(request);

			if (message.payload_size() != 1) {
				throw nscapi_exception("Whoops, invalid payload size (for now)");
			}
			::Plugin::SubmitResponseMessage::Response payload = message.payload().Get(0);
			response = payload.mutable_status()->message();
			return gbp_to_status(payload.mutable_status()->status());
		}

		void functions::create_simple_query_request(std::wstring command, std::list<std::wstring> arguments, std::string &buffer) {
			Plugin::QueryRequestMessage message;
			create_simple_header(message.mutable_header());

			Plugin::QueryRequestMessage::Request *payload = message.add_payload();
			payload->set_command(utf8::cvt<std::string>(command));

			BOOST_FOREACH(std::wstring s, arguments)
				payload->add_arguments(utf8::cvt<std::string>(s));

			message.SerializeToString(&buffer);
		}
		NSCAPI::nagiosReturn functions::create_simple_query_response_unknown(std::wstring command, std::wstring msg, std::wstring perf, std::string &buffer) {
			create_simple_query_response(command, NSCAPI::returnUNKNOWN, msg, perf, buffer);
			return NSCAPI::returnUNKNOWN;
		}
		NSCAPI::nagiosReturn functions::create_simple_query_response_unknown(std::wstring command, std::wstring msg, std::string &buffer) {
			create_simple_query_response(command, NSCAPI::returnUNKNOWN, msg, _T(""), buffer);
			return NSCAPI::returnUNKNOWN;
		}

		void functions::create_simple_query_response(std::wstring command, NSCAPI::nagiosReturn ret, std::wstring msg, std::wstring perf, std::string &buffer) {
			Plugin::QueryResponseMessage message;
			create_simple_header(message.mutable_header());

			append_simple_query_response_payload(message.add_payload(), command, ret, msg, perf);

			message.SerializeToString(&buffer);
		}


		void functions::append_simple_submit_request_payload(Plugin::QueryResponseMessage::Response *payload, std::wstring command, NSCAPI::nagiosReturn ret, std::wstring msg, std::wstring perf) {
			payload->set_command(utf8::cvt<std::string>(command));
			payload->set_message(utf8::cvt<std::string>(msg));
			payload->set_result(nagios_status_to_gpb(ret));
			if (!perf.empty())
				parse_performance_data(payload, perf);
		}

		void functions::append_simple_query_response_payload(Plugin::QueryResponseMessage::Response *payload, std::wstring command, NSCAPI::nagiosReturn ret, std::wstring msg, std::wstring perf) {
			payload->set_command(utf8::cvt<std::string>(command));
			payload->set_message(utf8::cvt<std::string>(msg));
			payload->set_result(nagios_status_to_gpb(ret));
			if (!perf.empty())
				parse_performance_data(payload, perf);
		}

		void functions::append_simple_query_response_payload(Plugin::QueryResponseMessage::Response *payload, std::string command, NSCAPI::nagiosReturn ret, std::string msg, std::string perf) {
			payload->set_command(utf8::cvt<std::string>(command));
			payload->set_message(utf8::cvt<std::string>(msg));
			payload->set_result(nagios_status_to_gpb(ret));
			if (!perf.empty())
				parse_performance_data(payload, perf);
		}
		void functions::append_simple_exec_response_payload(Plugin::ExecuteResponseMessage::Response *payload, std::string command, int ret, std::string msg) {
			payload->set_command(command);
			payload->set_message(msg);
			payload->set_result(nagios_status_to_gpb(ret));
		}
		void functions::append_simple_submit_response_payload(Plugin::SubmitResponseMessage::Response *payload, std::string command, int ret, std::string msg) {
			payload->set_command(command);
			payload->mutable_status()->set_status(status_to_gpb(ret));
			payload->mutable_status()->set_message(msg);
		}


		void functions::append_simple_query_request_payload(Plugin::QueryRequestMessage::Request *payload, std::wstring command, std::vector<std::wstring> arguments) {
			payload->set_command(utf8::cvt<std::string>(command));
			BOOST_FOREACH(const std::wstring &s, arguments) {
				payload->add_arguments(utf8::cvt<std::string>(s));
			}
		}

		void functions::append_simple_exec_request_payload(Plugin::ExecuteRequestMessage::Request *payload, std::wstring command, std::vector<std::wstring> arguments) {
			payload->set_command(utf8::cvt<std::string>(command));
			BOOST_FOREACH(const std::wstring &s, arguments) {
				payload->add_arguments(utf8::cvt<std::string>(s));
			}
		}


		functions::decoded_simple_command_data functions::parse_simple_query_request(const wchar_t* char_command, const std::string &request) {
			decoded_simple_command_data data;

			data.command = char_command;
			Plugin::QueryRequestMessage message;
			message.ParseFromString(request);

			if (message.payload_size() != 1) {
				throw nscapi_exception("Whoops, invalid payload size (for now)");
			}
			::Plugin::QueryRequestMessage::Request payload = message.payload().Get(0);
			for (int i=0;i<payload.arguments_size();i++) {
				data.args.push_back(utf8::cvt<std::wstring>(payload.arguments(i)));
			}
			return data;
		}
		functions::decoded_simple_command_data functions::parse_simple_query_request(const ::Plugin::QueryRequestMessage::Request &payload) {
			decoded_simple_command_data data;
			data.command = utf8::cvt<std::wstring>(payload.command());
			for (int i=0;i<payload.arguments_size();i++) {
				data.args.push_back(utf8::cvt<std::wstring>(payload.arguments(i)));
			}
			return data;
		}

		int functions::parse_simple_query_response(const std::string &response, std::wstring &msg, std::wstring &perf) {
			Plugin::QueryResponseMessage message;
			message.ParseFromString(response);


			if (message.payload_size() == 0) {
				return NSCAPI::returnUNKNOWN;
			} else if (message.payload_size() > 1) {
				throw nscapi_exception("Whoops, invalid payload size (for now)");
			}

			Plugin::QueryResponseMessage::Response payload = message.payload().Get(0);
			msg = utf8::cvt<std::wstring>(payload.message());
			perf = utf8::cvt<std::wstring>(build_performance_data(payload));
			return gbp_to_nagios_status(payload.result());
		}


		//////////////////////////////////////////////////////////////////////////

		void functions::create_simple_exec_request(const std::wstring &command, const std::list<std::wstring> & args, std::string &request) {

			Plugin::ExecuteRequestMessage message;
			create_simple_header(message.mutable_header());

			Plugin::ExecuteRequestMessage::Request *payload = message.add_payload();
			payload->set_command(utf8::cvt<std::string>(command));

			BOOST_FOREACH(std::wstring s, args)
				payload->add_arguments(utf8::cvt<std::string>(s));

			message.SerializeToString(&request);
		}
		void functions::create_simple_exec_request(const std::wstring &command, const std::vector<std::wstring> & args, std::string &request) {

			Plugin::ExecuteRequestMessage message;
			create_simple_header(message.mutable_header());

			Plugin::ExecuteRequestMessage::Request *payload = message.add_payload();
			payload->set_command(utf8::cvt<std::string>(command));

			BOOST_FOREACH(std::wstring s, args)
				payload->add_arguments(utf8::cvt<std::string>(s));

			message.SerializeToString(&request);
		}
		int functions::parse_simple_exec_result(const std::string &response, std::list<std::wstring> &result) {
			int ret = 0;
			Plugin::ExecuteResponseMessage message;
			message.ParseFromString(response);

			for (int i=0;i<message.payload_size(); i++) {
				result.push_back(utf8::cvt<std::wstring>(message.payload(i).message()));
				int r=gbp_to_nagios_status(message.payload(i).result());
				if (r > ret)
					ret = r;
			}
			return ret;
		}
		void functions::parse_simple_exec_result(const std::string &response, std::wstring &result) {
			Plugin::ExecuteResponseMessage message;
			message.ParseFromString(response);

			for (int i=0;i<message.payload_size(); i++) {
				result += utf8::cvt<std::wstring>(message.payload(i).message());
			}
		}

		functions::decoded_simple_command_data functions::parse_simple_exec_request(const wchar_t* char_command, const std::string &request) {
			Plugin::ExecuteRequestMessage message;
			message.ParseFromString(request);

			return parse_simple_exec_request(char_command, message);
		}
		functions::decoded_simple_command_data functions::parse_simple_exec_request(const std::wstring cmd, const Plugin::ExecuteRequestMessage &message) {
			decoded_simple_command_data data;
			data.command = cmd;
			if (message.has_header())
				data.target = utf8::cvt<std::wstring>(message.header().recipient_id());

			if (message.payload_size() != 1) {
				throw nscapi_exception("Whoops, invalid payload size (for now)");
			}
			Plugin::ExecuteRequestMessage::Request payload = message.payload().Get(0);
			for (int i=0;i<payload.arguments_size();i++) {
				data.args.push_back(utf8::cvt<std::wstring>(payload.arguments(i)));
			}
			return data;
		}
		functions::decoded_simple_command_data functions::parse_simple_exec_request_payload(const Plugin::ExecuteRequestMessage::Request &payload) {
			decoded_simple_command_data data;
			data.command = utf8::cvt<std::wstring>(payload.command());
			for (int i=0;i<payload.arguments_size();i++) {
				data.args.push_back(utf8::cvt<std::wstring>(payload.arguments(i)));
			}
			return data;
		}


		//////////////////////////////////////////////////////////////////////////

		template<class T>
		struct tokenizer_data {
			T perf_lable_enclosure;						// '
			T perf_separator;							// ' '
			T perf_item_splitter;						// ; 
			T perf_equal_sign;							// =
			T perf_valid_number;						// 0123456789.,

		};

		template<class T>
		void parse_performance_data_(Plugin::QueryResponseMessage::Response *payload, T &perf, tokenizer_data<T> tokenizer_data) {
			while (true) {
				if (perf.size() == 0)
					return;
				typename T::size_type p = 0;
				if (perf[0] == tokenizer_data.perf_lable_enclosure[0]) {
					p = perf.find(tokenizer_data.perf_lable_enclosure[0], 1)+1;
					if (p == T::npos)
						return;
				}
				p = perf.find(tokenizer_data.perf_separator, p);
				if (p == 0)
					return;
				T chunk;
				if (p == T::npos) {
					chunk = perf;
					perf = T();
				} else {
					chunk = perf.substr(0, p);
					p = perf.find_first_not_of(tokenizer_data.perf_separator, p);
					perf = perf.substr(p);
				}
				std::vector<T> items = strEx::splitV(chunk, tokenizer_data.perf_item_splitter);
				if (items.size() < 1) {
					Plugin::Common::PerformanceData* perfData = payload->add_perf();
					perfData->set_type(Plugin::Common_DataType_STRING);
					std::pair<T,T> fitem = strEx::split(T(), tokenizer_data.perf_equal_sign);
					perfData->set_alias("invalid");
					Plugin::Common_PerformanceData_StringValue* stringPerfData = perfData->mutable_string_value();
					stringPerfData->set_value("invalid performance data");
					break;
				}

				std::pair<T,T> fitem = strEx::split(items[0], tokenizer_data.perf_equal_sign);
				T alias = fitem.first;
				if (alias.size() > 0 && alias[0] == tokenizer_data.perf_lable_enclosure[0] && alias[alias.size()-1] == tokenizer_data.perf_lable_enclosure[0])
					alias = alias.substr(1, alias.size()-2);

				if (alias.empty())
					continue;
				Plugin::Common::PerformanceData* perfData = payload->add_perf();
				perfData->set_type(Plugin::Common_DataType_FLOAT);
				perfData->set_alias(utf8::cvt<std::string>(alias));
				Plugin::Common_PerformanceData_FloatValue* floatPerfData = perfData->mutable_float_value();

				typename T::size_type pstart = fitem.second.find_first_of(tokenizer_data.perf_valid_number);
				if (pstart == T::npos) {
					floatPerfData->set_value(0);
					continue;
				}
				if (pstart != 0)
					fitem.second = fitem.second.substr(pstart);
				typename T::size_type pend = fitem.second.find_first_not_of(tokenizer_data.perf_valid_number);
				if (pend == T::npos) {
					floatPerfData->set_value(trim_to_double(fitem.second));
				} else {
					floatPerfData->set_value(trim_to_double(fitem.second.substr(0,pend)));
					floatPerfData->set_unit(utf8::cvt<std::string>(fitem.second.substr(pend)));
				}
				if (items.size() > 2) {
					floatPerfData->set_warning(trim_to_double(items[1]));
					floatPerfData->set_critical(trim_to_double(items[2]));
				}
				if (items.size() >= 5) {
					floatPerfData->set_minimum(trim_to_double(items[3]));
					floatPerfData->set_maximum(trim_to_double(items[4]));
				}
			}
		}
		void functions::parse_performance_data(Plugin::QueryResponseMessage::Response *payload, std::wstring &perf) {
			tokenizer_data<std::wstring> data;
			data.perf_separator = _T(" ");
			data.perf_lable_enclosure = _T("'");
			data.perf_equal_sign = _T("=");
			data.perf_item_splitter = _T(";");
			data.perf_valid_number = _T("0123456789,.");
			parse_performance_data_<std::wstring>(payload, perf, data);
		}
		void functions::parse_performance_data(Plugin::QueryResponseMessage::Response *payload, std::string &perf) {
			tokenizer_data<std::string> data;
			data.perf_separator = " ";
			data.perf_lable_enclosure = "'";
			data.perf_equal_sign = "=";
			data.perf_item_splitter = ";";
			data.perf_valid_number = "0123456789,.";
			parse_performance_data_<std::string>(payload, perf, data);
		}

		std::string functions::build_performance_data(Plugin::QueryResponseMessage::Response const &payload) {
			std::stringstream ss;
			ss.precision(5);

			bool first = true;
			for (int i=0;i<payload.perf_size();i++) {
				Plugin::Common::PerformanceData perfData = payload.perf(i);
				if (!first)
					ss << " ";
				first = false;
				ss << '\'' << perfData.alias() << "'=";
				if (perfData.has_float_value()) {
					Plugin::Common_PerformanceData_FloatValue fval = perfData.float_value();

					ss << strEx::s::itos_non_sci(fval.value());
					if (fval.has_unit())
						ss << fval.unit();
					if (!fval.has_warning())
						continue;
					ss << ";" << fval.warning();
					if (!fval.has_critical())	
						continue;
					ss << ";" << fval.critical();
					if (!fval.has_minimum())
						continue;
					ss << ";" << fval.minimum();
					if (!fval.has_maximum())
						continue;
					ss << ";" << fval.maximum();
				}
			}
			return ss.str();
		}
	}
}