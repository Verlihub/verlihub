/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2015 Verlihub Team, info at verlihub dot net

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
namespace  nVerliHub {

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
	void AddVars();

	friend class nVerliHub::nSocket::cServerDC;
	friend class nProtocol::cDCProto;
	friend class nTables::cDCBanList;
	friend class cDCConsole;
	friend class nVerliHub::nSocket::cConnDC;
public:
	unsigned int max_users_total;
	int max_users_passive;
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
	unsigned int max_passive_sr;
	unsigned tban_kick;
	unsigned tban_max;
	//unsigned int max_length[nEnums::eDC_UNKNOWN+1];
	unsigned max_nick;
	unsigned min_nick;
	string nick_chars;
	unsigned int max_chat_msg;
	unsigned int max_chat_lines;
	int max_flood_counter_pm;
	int max_flood_counter_mcto;
	int nicklist_on_login;
	bool delayed_login;
	int delayed_search;
	int delayed_myinfo; // implies also delayed quit
	bool drop_invalid_key;
	bool delayed_chat;
	int delayed_ping;
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
	string cmd_start_op;
	string cmd_start_user;
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
	int hublist_port;
	bool hublist_send_minshare;
	/** checking prefferences */
	unsigned int classdif_reg;
	unsigned int classdif_search;
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
	int max_class_int_login;
	int max_class_check_clone;
	int hide_all_kicks;
	int optimize_userlist;
	bool filter_lan_requests;
	bool hide_msg_badctm; // hide following messages: connecting to offline user, connecting to bot
	int search_number;
	int int_search;
	int int_search_pas;
	int int_search_reg_pas;
	int int_search_reg;
	int int_search_vip;
	int int_search_op;
	int int_login;

	// protocol flood
	int max_class_proto_flood;
	bool proto_flood_report;
	unsigned long proto_flood_tban_time;
	unsigned long int_flood_chat_period;
	unsigned int int_flood_chat_limit;
	unsigned long int_flood_mcto_period;
	unsigned int int_flood_mcto_limit;
	unsigned long int_flood_to_period;
	unsigned int int_flood_to_limit;
	unsigned long int_flood_myinfo_period;
	unsigned int int_flood_myinfo_limit;
	unsigned long int_flood_in_period;
	unsigned int int_flood_in_limit;
	unsigned long int_flood_search_period;
	unsigned int int_flood_search_limit;
	unsigned long int_flood_sr_period;
	unsigned int int_flood_sr_limit;
	unsigned long int_flood_ctm_period;
	unsigned int int_flood_ctm_limit;
	unsigned long int_flood_rctm_period;
	unsigned int int_flood_rctm_limit;
	unsigned long int_flood_nicklist_period;
	unsigned int int_flood_nicklist_limit;
	unsigned long int_flood_getinfo_period;
	unsigned int int_flood_getinfo_limit;
	unsigned long int_flood_ping_period;
	unsigned int int_flood_ping_limit;
	unsigned long int_flood_unknown_period;
	unsigned int int_flood_unknown_limit;
	// from all
	unsigned long int_flood_all_chat_period;
	unsigned int int_flood_all_chat_limit;
	unsigned long int_flood_all_mcto_period;
	unsigned int int_flood_all_mcto_limit;
	unsigned long int_flood_all_to_period;
	unsigned int int_flood_all_to_limit;

	int int_chat_ms;
	int int_nicklist;
	int int_myinfo;
	bool disable_me_cmd;
	bool disable_regme_cmd;
	bool disable_usr_cmds;
	bool disable_report_cmd;
	bool disable_zlib;
	unsigned int zlib_min_len;
	bool detect_ctmtohub; // ctm2hub
	int plugin_mod_class;
	int topic_mod_class;
	int mainchat_class;
	int private_class;
	int ul_portion;
	unsigned int min_search_chars;
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
	int tag_sum_hubs;
	double tag_min_version;
	double tag_max_version;
	string cc_zone[3];
	int show_tags; // 0 - no at all, 1 - only to ops, 2 - show to everyone
	int show_desc_len; // cut first n bytes of description
	int autoreg_class;
	int show_email;
	int show_speed;
	bool send_user_ip;
	int user_ip_class;
	bool send_user_info;
	bool send_pass_request;
	int ban_bypass_class;
	bool use_reglist_cache;
	bool use_penlist_cache;
	bool chat_default_on;
	bool always_ask_password;
	int default_password_encryption;
	int password_min_len;
	int pwd_tmpban;
	string wrongpass_message;
	bool wrongpassword_report;
	int wrongauthip_report;
	bool clone_detect_report;
	unsigned long clone_det_tban_time;
	bool nullchars_report;
	int botinfo_report;
	double timeout_length[6];

	string ban_extra_message;
	string msg_replace_ban;
	string msg_welcome[11];
	bool desc_insert_mode;
	string desc_insert_vars;
public: // Public attributes
	nSocket::cServerDC & mS;
};
	}; // nmaespace nTables
}; // namespace nVerliHub

#endif
