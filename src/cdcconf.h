/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	Verlihub is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifndef CDCCONF_H
#define CDCCONF_H
#include "cconfigbase.h"
#include "cmessagedc.h"
#include "cdctag.h"

using std::string;

namespace nVerliHub {
	namespace nProtocol {
		class cDCProto;
	};

	using nProtocol::cDCProto;
	class cDCBanList;
	class cDCConsole;

	namespace nSocket {
		class cConnDC;
		class cServerDC;
	};

namespace nTables {
	class cDCBanList;

/**dc configuration
  *@author Daniel Muller
  * This class contains almost all verlihub's configuration parameters
  */

class cDCConf : public nConfig::cConfigBase //<sBasicItemCreator>
{
public:
	cDCConf(nSocket::cServerDC &);
	~cDCConf();
	virtual int Load();
	virtual int Save();
	void AddVars(); // todo: there is no DelVars to free memory

	friend class nVerliHub::nSocket::cServerDC;
	friend class nProtocol::cDCProto;
	friend class nTables::cDCBanList;
	friend class nVerliHub::cDCConsole;
	friend class nVerliHub::nSocket::cConnDC;
public:
	unsigned int max_users_total;
	int max_users_passive; // -1 means disabled, 0 means what it is
	unsigned int max_users_from_ip;
	unsigned int max_users[7];
	string hubfull_message;
	unsigned int max_extra_pings;
	unsigned int max_extra_regs;
	unsigned int max_extra_vips;
	unsigned int max_extra_ops;
	unsigned int max_extra_cheefs;
	unsigned int max_extra_admins;
	double max_upload_kbps;
	unsigned long min_share;
	unsigned long min_share_reg;
	unsigned long min_share_vip;
	unsigned long min_share_ops;
	double min_share_factor_passive;
	unsigned long max_share;
	unsigned long max_share_reg;
	unsigned long max_share_vip;
	unsigned long max_share_ops;
	unsigned long min_share_use_hub;
	unsigned long min_share_use_hub_reg;
	unsigned long min_share_use_hub_vip;
	int min_class_use_hub;
	int min_class_use_hub_passive;
	unsigned long use_hub_msg_time;
	unsigned int max_passive_sr;
	unsigned int tban_kick;
	unsigned int tban_max;
	unsigned int max_nick;
	unsigned int min_nick;
	string nick_chars;
	unsigned long max_outbuf_size;
	unsigned long max_outfill_size;
	unsigned long max_unblock_size;
	unsigned int max_len_supports;
	unsigned int max_len_version;
	unsigned int max_len_myinfo;
	unsigned int max_len_in;
	unsigned int max_len_extjson;
	unsigned int max_len_myhuburl;
	unsigned int max_len_search;
	unsigned int max_len_mynick;
	unsigned int max_len_lock;
	unsigned int max_chat_msg;
	unsigned int max_pm_msg;
	unsigned int max_mcto_msg;
	unsigned int max_chat_lines;
	unsigned int max_flood_counter_pm;
	unsigned int max_flood_counter_mcto;
	unsigned int same_flood_ban_time;
	bool delayed_search;
	bool delayed_myinfo; // implies also delayed quit
	bool drop_invalid_key;
	bool delayed_chat;
	unsigned int delayed_ping; // is number of seconds, not bool
	double min_frequency;
	string nick_prefix;
	bool nick_prefix_nocase;
	string nick_prefix_autoreg;
	bool nick_prefix_cc;
	bool extended_welcome_message;
	unsigned int host_header;
	string hub_security;
	string hub_category;
	string hub_icon_url;
	string hub_logo_url;
	string hub_encoding;
	string hub_security_desc;
	string opchat_name;
	string opchat_desc;
	int opchat_class;
	string cmd_start_op;
	string cmd_start_user;
	bool unknown_cmd_chat;
	bool dest_report_chat;
	bool dest_regme_chat;
	bool dest_drop_chat;
	bool report_dns_lookup;
	bool report_user_country;
	string extra_listen_ports;
	string hub_version_special;
	string hub_name;
	string hub_version;
	string hub_topic;
	string hub_desc;
	string hub_host;
	string hub_failover_hosts;
	string hub_owner;
	string hublist_host;
	unsigned int hublist_port;
	bool hublist_send_minshare;
	bool hublist_send_listhost;
	bool update_check_git;
	// checking preferences
	unsigned int classdif_reg;
	//unsigned int classdif_search;
	unsigned int classdif_download;
	unsigned int classdif_pm;
	unsigned int classdif_mcto;
	unsigned int classdif_kick;
	int min_class_register;
	int min_class_bc;
	int min_class_bc_guests;
	int min_class_bc_regs;
	int min_class_bc_vips;
	int min_class_redir;
	int min_class_lstredir;
	int max_class_int_login;
	unsigned int max_class_check_clone;
	unsigned int max_class_self_repass;
	bool allow_same_user;
	int max_class_same_user;
	bool hide_all_kicks;
	int notify_kicks_to_all;
	bool use_search_filter;
	bool filter_lan_requests;
	bool hide_msg_badctm;
	bool hide_msg_badtlsctm;
	unsigned int search_number;
	unsigned int min_search_chars;
	unsigned int int_search;
	unsigned int int_search_pas;
	unsigned int int_search_reg_pas;
	unsigned int int_search_reg;
	unsigned int int_search_vip;
	unsigned int int_search_op;
	unsigned int int_login;

	// protocol flood
	int max_class_proto_flood;
	bool proto_flood_report;
	bool proto_flood_report_locked;
	unsigned long proto_flood_tban_time;
	unsigned long proto_flood_report_time;

	unsigned long int_flood_chat_period;
	unsigned int int_flood_chat_limit;
	unsigned int proto_flood_chat_action;

	unsigned long int_flood_mcto_period;
	unsigned int int_flood_mcto_limit;
	unsigned int proto_flood_mcto_action;

	unsigned long int_flood_to_period;
	unsigned int int_flood_to_limit;
	unsigned int proto_flood_to_action;

	unsigned long int_flood_myinfo_period;
	unsigned int int_flood_myinfo_limit;
	unsigned int proto_flood_myinfo_action;

	unsigned long int_flood_in_period;
	unsigned int int_flood_in_limit;
	unsigned int proto_flood_in_action;

	unsigned long int_flood_extjson_period;
	unsigned int int_flood_extjson_limit;
	unsigned int proto_flood_extjson_action;

	unsigned long int_flood_search_period;
	unsigned int int_flood_search_limit;
	unsigned int proto_flood_search_action;

	unsigned long int_flood_sr_period;
	unsigned int int_flood_sr_limit;
	unsigned int proto_flood_sr_action;

	unsigned long int_flood_ctm_period;
	unsigned int int_flood_ctm_limit;
	unsigned int proto_flood_ctm_action;

	unsigned long int_flood_rctm_period;
	unsigned int int_flood_rctm_limit;
	unsigned int proto_flood_rctm_action;

	unsigned long int_flood_nicklist_period;
	unsigned int int_flood_nicklist_limit;
	unsigned int proto_flood_nicklist_action;

	unsigned long int_flood_getinfo_period;
	unsigned int int_flood_getinfo_limit;
	unsigned int proto_flood_getinfo_action;

	unsigned long int_flood_ping_period;
	unsigned int int_flood_ping_limit;
	unsigned int proto_flood_ping_action;

	unsigned long int_flood_unknown_period;
	unsigned int int_flood_unknown_limit;
	unsigned int proto_flood_unknown_action;

	// flood from all
	unsigned long int_flood_all_chat_period;
	unsigned int int_flood_all_chat_limit;
	unsigned long int_flood_all_mcto_period;
	unsigned int int_flood_all_mcto_limit;
	unsigned long int_flood_all_to_period;
	unsigned int int_flood_all_to_limit;
	unsigned long int_flood_all_search_period;
	unsigned int int_flood_all_search_limit;
	unsigned long int_flood_all_rctm_period;
	unsigned int int_flood_all_rctm_limit;
	// end of section

	unsigned long int_chat_ms;
	unsigned int int_nicklist;
	unsigned int int_myinfo;
	bool disable_me_cmd;
	bool disable_regme_cmd;
	bool disable_usr_cmds;
	bool disable_report_cmd;
	bool disable_zlib;
	int zlib_compress_level;
	unsigned int zlib_min_len;
	bool detect_ctmtohub; // ctm2hub
	bool disable_extjson; // extjson
	bool myinfo_tls_filter;
	string mmdb_names_lang; // mmdb
	unsigned int mmdb_conv_depth;
	bool mmdb_cache;
	unsigned int mmdb_cache_mins;
	int plugin_mod_class;
	int topic_mod_class;
	int mainchat_class;
	int private_class;
	string ip_zone4_min;
	string ip_zone4_max;
	string ip_zone5_min;
	string ip_zone5_max;
	string ip_zone6_min;
	string ip_zone6_max;

	int tag_max_hubs;
	int tag_min_hubs;
	int tag_min_hubs_usr;
	int tag_min_hubs_reg;
	int tag_min_hubs_op;
	double tag_min_hs_ratio;
	double tag_max_hs_ratio;
	bool tag_allow_unknown;
	bool tag_allow_none;
	bool tag_allow_sock5;
	bool tag_allow_passive;
	int tag_min_class_ignore;
	unsigned int tag_sum_hubs;
	double tag_min_version;
	double tag_max_version;
	string cc_zone[3];
	bool show_tags;
	int show_desc_len; // cut first x bytes of description, -1 means disabled
	int autoreg_class; // todo: change to unsigned
	int autounreg_class;
	bool show_email;
	bool show_speed;
	bool send_user_ip;
	int user_ip_class;
	int oplist_class;
	bool send_user_info;
	bool send_pass_request;
	bool send_crash_report;
	int ban_bypass_class;
	int chatonly_bypass_class;
	bool use_reglist_cache;
	bool use_penlist_cache;
	bool chat_default_on;
	bool notify_gag_chats;
	bool clean_gags_cleanbans;
	bool always_ask_password;
	unsigned int default_password_encryption;
	unsigned int password_min_len;
	unsigned int pwd_tmpban;
	string wrongpass_message;
	bool wrongpassword_report;
	bool report_pass_reset;
	int wrongauthip_report; // note: is class number, not bool
	bool wrongip_message;
	bool clone_detect_report;
	unsigned int clone_detect_count;
	unsigned long clone_det_tban_time;
	unsigned long clone_ip_tban_time;
	bool nullchars_report;
	bool botinfo_report;
	double timeout_length[6];

	string ban_extra_message;
	string msg_replace_ban;
	string msg_welcome[int(eUC_MASTER) + 1];
	bool desc_insert_mode;
	string desc_insert_vars;
public: // Public attributes
	nSocket::cServerDC &mS;
};

	}; // nmaespace nTables
}; // namespace nVerliHub

#endif
