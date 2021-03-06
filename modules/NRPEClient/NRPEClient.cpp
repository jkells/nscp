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
#include "stdafx.h"
#include "NRPEClient.h"

#include <time.h>
#include <strEx.h>

#include <strEx.h>
#include <nrpe/client/socket.hpp>

#include <settings/client/settings_client.hpp>
#include <nscapi/nscapi_protobuf_functions.hpp>

namespace sh = nscapi::settings_helper;

const std::wstring NRPEClient::command_prefix = _T("nrpe");
/**
 * Default c-tor
 * @return 
 */
NRPEClient::NRPEClient() {}

/**
 * Default d-tor
 * @return 
 */
NRPEClient::~NRPEClient() {}

/**
 * Load (initiate) module.
 * Start the background collector thread and let it run until unloadModule() is called.
 * @return true
 */
bool NRPEClient::loadModule() {
	return false;
}

bool NRPEClient::loadModuleEx(std::wstring alias, NSCAPI::moduleLoadMode mode) {

	try {

		sh::settings_registry settings(get_settings_proxy());
		settings.set_alias(_T("NRPE"), alias, _T("client"));

		target_path = settings.alias().get_settings_path(_T("targets"));

		settings.alias().add_path_to_settings()
			(_T("NRPE CLIENT SECTION"), _T("Section for NRPE active/passive check module."))

			(_T("handlers"), sh::fun_values_path(boost::bind(&NRPEClient::add_command, this, _1, _2)), 
			_T("CLIENT HANDLER SECTION"), _T(""))

			(_T("targets"), sh::fun_values_path(boost::bind(&NRPEClient::add_target, this, _1, _2)), 
			_T("REMOTE TARGET DEFINITIONS"), _T(""))
			;

		settings.alias().add_key_to_settings()
			(_T("channel"), sh::wstring_key(&channel_, _T("NRPE")),
			_T("CHANNEL"), _T("The channel to listen to."))

			;

		settings.register_all();
		settings.notify();

		targets.add_missing(get_settings_proxy(), target_path, _T("default"), _T(""), true);


		get_core()->registerSubmissionListener(get_id(), channel_);
		register_command(_T("nrpe_query"), _T("Check remote NRPE host"));
		register_command(_T("nrpe_submit"), _T("Submit (via query) remote NRPE host"));
		register_command(_T("nrpe_forward"), _T("Forward query to remote NRPE host"));
		register_command(_T("nrpe_exec"), _T("Execute (via query) remote NRPE host"));
		register_command(_T("nrpe_help"), _T("Help on using NRPE Client"));
	} catch (nscapi::nscapi_exception &e) {
		NSC_LOG_ERROR_STD(_T("NSClient API exception: ") + utf8::to_unicode(e.what()));
		return false;
	} catch (std::exception &e) {
		NSC_LOG_ERROR_STD(_T("Exception caught: ") + utf8::to_unicode(e.what()));
		return false;
	} catch (...) {
		NSC_LOG_ERROR_STD(_T("Exception caught: <UNKNOWN EXCEPTION>"));
		return false;
	}
	return true;
}
std::string get_command(std::string alias, std::string command = "") {
	if (!alias.empty())
		return alias; 
	if (!command.empty())
		return command; 
	return "_NRPE_CHECK";
}

//////////////////////////////////////////////////////////////////////////
// Settings helpers
//

void NRPEClient::add_target(std::wstring key, std::wstring arg) {
	try {
		targets.add(get_settings_proxy(), target_path , key, arg);
	} catch (const std::exception &e) {
		NSC_LOG_ERROR_STD(_T("Failed to add target: ") + key + _T(", ") + utf8::to_unicode(e.what()));
	} catch (...) {
		NSC_LOG_ERROR_STD(_T("Failed to add target: ") + key);
	}
}

void NRPEClient::add_command(std::wstring name, std::wstring args) {
	try {
		std::wstring key = commands.add_command(name, args);
		if (!key.empty())
			register_command(key.c_str(), _T("NRPE relay for: ") + name);
	} catch (boost::program_options::validation_error &e) {
		NSC_LOG_ERROR_STD(_T("Could not add command ") + name + _T(": ") + utf8::to_unicode(e.what()));
	} catch (...) {
		NSC_LOG_ERROR_STD(_T("Could not add command ") + name);
	}
}

/**
 * Unload (terminate) module.
 * Attempt to stop the background processing thread.
 * @return true if successfully, false if not (if not things might be bad)
 */
bool NRPEClient::unloadModule() {
	return true;
}

NSCAPI::nagiosReturn NRPEClient::handleRAWCommand(const wchar_t* char_command, const std::string &request, std::string &result) {
	std::wstring cmd = client::command_line_parser::parse_command(char_command, command_prefix);

	Plugin::QueryRequestMessage message;
	message.ParseFromString(request);

	client::configuration config;
	setup(config, message.header());

	return commands.process_query(cmd, config, message, result);
}

NSCAPI::nagiosReturn NRPEClient::commandRAWLineExec(const wchar_t* char_command, const std::string &request, std::string &result) {
	std::wstring cmd = client::command_line_parser::parse_command(char_command, command_prefix);

	Plugin::ExecuteRequestMessage message;
	message.ParseFromString(request);

	client::configuration config;
	setup(config, message.header());

	return commands.process_exec(cmd, config, message, result);
}

NSCAPI::nagiosReturn NRPEClient::handleRAWNotification(const wchar_t* channel, std::string request, std::string &result) {
	Plugin::SubmitRequestMessage message;
	message.ParseFromString(request);

	client::configuration config;
	setup(config, message.header());

	return client::command_line_parser::do_relay_submit(config, message, result);
}

//////////////////////////////////////////////////////////////////////////
// Parser setup/Helpers
//

void NRPEClient::add_local_options(po::options_description &desc, client::configuration::data_type data) {
 	desc.add_options()
		("certificate,c", po::value<std::string>()->notifier(boost::bind(&nscapi::functions::destination_container::set_string_data, &data->recipient, "certificate", _1)), 
			"Length of payload (has to be same as on the server)")

		("buffer-length,l", po::value<unsigned int>()->notifier(boost::bind(&nscapi::functions::destination_container::set_int_data, &data->recipient, "payload length", _1)), 
			"Length of payload (has to be same as on the server)")

 		("no-ssl,n", po::value<bool>()->zero_tokens()->default_value(false)->notifier(boost::bind(&nscapi::functions::destination_container::set_bool_data, &data->recipient, "no ssl", _1)), 
			"Do not initial an ssl handshake with the server, talk in plaintext.")
 		;
}

void NRPEClient::setup(client::configuration &config, const ::Plugin::Common_Header& header) {
	boost::shared_ptr<clp_handler_impl> handler = boost::shared_ptr<clp_handler_impl>(new clp_handler_impl(this));
	add_local_options(config.local, config.data);

	config.data->recipient.id = header.recipient_id();
	std::wstring recipient = utf8::cvt<std::wstring>(config.data->recipient.id);
	if (!targets.has_object(recipient)) {
		recipient = _T("default");
	}
	nscapi::targets::optional_target_object opt = targets.find_object(recipient);

	if (opt) {
		nscapi::targets::target_object t = *opt;
		nscapi::functions::destination_container def = t.to_destination_container();
		config.data->recipient.apply(def);
	}
	config.data->host_self.id = "self";
	//config.data->host_self.host = hostname_;

	config.target_lookup = handler;
	config.handler = handler;
}

NRPEClient::connection_data NRPEClient::parse_header(const ::Plugin::Common_Header &header, client::configuration::data_type data) {
	nscapi::functions::destination_container recipient;
	nscapi::functions::parse_destination(header, header.recipient_id(), recipient, true);
	return connection_data(recipient, data->recipient);
}

//////////////////////////////////////////////////////////////////////////
// Parser implementations
//

int NRPEClient::clp_handler_impl::query(client::configuration::data_type data, const Plugin::QueryRequestMessage &request_message, std::string &reply) {
	const ::Plugin::Common_Header& request_header = request_message.header();
	connection_data con = parse_header(request_header, data);

	Plugin::QueryResponseMessage response_message;
	nscapi::functions::make_return_header(response_message.mutable_header(), request_header);

	for (int i=0;i<request_message.payload_size();i++) {
		std::string command = get_command(request_message.payload(i).alias(), request_message.payload(i).command());
		std::string data = command;
		for (int a=0;a<request_message.payload(i).arguments_size();a++) {
			data += "!" + request_message.payload(i).arguments(a);
		}
		boost::tuple<int,std::wstring> ret = instance->send(con, data);
		std::pair<std::wstring,std::wstring> rdata = strEx::split(ret.get<1>(), std::wstring(_T("|")));
		nscapi::functions::append_simple_query_response_payload(response_message.add_payload(), utf8::cvt<std::wstring>(command), ret.get<0>(), rdata.first, rdata.second);
	}
	response_message.SerializeToString(&reply);
	return NSCAPI::isSuccess;
}

int NRPEClient::clp_handler_impl::submit(client::configuration::data_type data, const Plugin::SubmitRequestMessage &request_message, std::string &reply) {
	const ::Plugin::Common_Header& request_header = request_message.header();
	connection_data con = parse_header(request_header, data);
	std::wstring channel = utf8::cvt<std::wstring>(request_message.channel());
	
	Plugin::SubmitResponseMessage response_message;
	nscapi::functions::make_return_header(response_message.mutable_header(), request_header);

	for (int i=0;i<request_message.payload_size();++i) {
		std::string command = get_command(request_message.payload(i).alias(), request_message.payload(i).command());
		std::string data = command;
		for (int a=0;a<request_message.payload(i).arguments_size();a++) {
			data += "!" + request_message.payload(i).arguments(i);
		}
		boost::tuple<int,std::wstring> ret = instance->send(con, data);
		nscapi::functions::append_simple_submit_response_payload(response_message.add_payload(), command, ret.get<0>(), utf8::cvt<std::string>(ret.get<1>()));
	}
	response_message.SerializeToString(&reply);
	return NSCAPI::isSuccess;
}

int NRPEClient::clp_handler_impl::exec(client::configuration::data_type data, const Plugin::ExecuteRequestMessage &request_message, std::string &reply) {
	const ::Plugin::Common_Header& request_header = request_message.header();
	connection_data con = parse_header(request_header, data);

	Plugin::ExecuteResponseMessage response_message;
	nscapi::functions::make_return_header(response_message.mutable_header(), request_header);

	for (int i=0;i<request_message.payload_size();i++) {
		std::string command = get_command(request_message.payload(i).command());
		std::string data = command;
		for (int a=0;a<request_message.payload(i).arguments_size();a++)
			data += "!" + request_message.payload(i).arguments(a);
		boost::tuple<int,std::wstring> ret = instance->send(con, data);
		nscapi::functions::append_simple_exec_response_payload(response_message.add_payload(), command, ret.get<0>(), utf8::cvt<std::string>(ret.get<1>()));
	}
	response_message.SerializeToString(&reply);
	return NSCAPI::isSuccess;
}

//////////////////////////////////////////////////////////////////////////
// Protocol implementations
//

boost::tuple<int,std::wstring> NRPEClient::send(connection_data con, const std::string data) {
	try {
		NSC_DEBUG_MSG_STD(_T("NRPE Connection details: ") + con.to_wstring());
		NSC_DEBUG_MSG_STD(_T("NRPE data: ") + utf8::cvt<std::wstring>(data));
		nrpe::packet packet;
		if (con.use_ssl) {
#ifdef USE_SSL
			packet = send_ssl(con.cert, con.host, con.port, con.timeout, nrpe::packet::make_request(utf8::cvt<std::wstring>(data), con.buffer_length));
#else
			NSC_LOG_ERROR_STD(_T("SSL not avalible (compiled without USE_SSL)"));
			return boost::make_tuple(NSCAPI::returnUNKNOWN, _T("SSL support not available (compiled without USE_SSL)"));
#endif
		} else
			packet = send_nossl(con.host, con.port, con.timeout, nrpe::packet::make_request(utf8::cvt<std::wstring>(data), con.buffer_length));
		return boost::make_tuple(static_cast<int>(packet.getResult()), packet.getPayload());
	} catch (nrpe::nrpe_packet_exception &e) {
		return boost::make_tuple(NSCAPI::returnUNKNOWN, _T("NRPE Packet errro: ") + e.wwhat());
	} catch (std::runtime_error &e) {
		NSC_LOG_ERROR_STD(_T("Socket error: ") + utf8::to_unicode(e.what()));
		return boost::make_tuple(NSCAPI::returnUNKNOWN, _T("Socket error: ") + utf8::to_unicode(e.what()));
	} catch (std::exception &e) {
		NSC_LOG_ERROR_STD(_T("Error: ") + utf8::to_unicode(e.what()));
		return boost::make_tuple(NSCAPI::returnUNKNOWN, _T("Error: ") + utf8::to_unicode(e.what()));
	} catch (...) {
		return boost::make_tuple(NSCAPI::returnUNKNOWN, _T("Unknown error -- REPORT THIS!"));
	}
}


#ifdef USE_SSL
nrpe::packet NRPEClient::send_ssl(std::string cert, std::string host, std::string port, int timeout, nrpe::packet packet) {
	boost::asio::io_service io_service;
	boost::asio::ssl::context ctx(io_service, boost::asio::ssl::context::sslv23);
	SSL_CTX_set_cipher_list(ctx.impl(), "ADH");
	ctx.use_tmp_dh_file(to_string(cert));
	ctx.set_verify_mode(boost::asio::ssl::context::verify_none);
	nrpe::client::ssl_socket socket(io_service, ctx, host, port);
	socket.send(packet, boost::posix_time::seconds(timeout));
	return socket.recv(packet, boost::posix_time::seconds(timeout));
}
#endif

nrpe::packet NRPEClient::send_nossl(std::string host, std::string port, int timeout, nrpe::packet packet) {
	boost::asio::io_service io_service;
	nrpe::client::socket socket(io_service, host, port);
	socket.send(packet, boost::posix_time::seconds(timeout));
	return socket.recv(packet, boost::posix_time::seconds(timeout));
}

NSC_WRAP_DLL();
NSC_WRAPPERS_MAIN_DEF(NRPEClient);
NSC_WRAPPERS_IGNORE_MSG_DEF();
NSC_WRAPPERS_HANDLE_CMD_DEF();
NSC_WRAPPERS_CLI_DEF();
NSC_WRAPPERS_HANDLE_NOTIFICATION_DEF();

