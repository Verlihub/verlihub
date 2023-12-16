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

#include "cserverdc.h"
#include "casyncconn.h"
#include "creguserinfo.h"
#include "cdcconf.h"
#include <string>
#include <zlib.h>

using namespace std;

namespace nVerliHub {
	namespace nTables {

cDCConf::cDCConf(cServerDC &serv):
	mS(serv)
{}

cDCConf::~cDCConf()
{}

void cDCConf::AddVars()
{
	// hub info and basic settings
	Add("hub_name", hub_name, HUB_VERSION_NAME);
	Add("hub_desc",hub_desc, "");
	Add("hub_topic",hub_topic, "");
	Add("hub_category",hub_category, "");
	Add("hub_icon_url", hub_icon_url, "");
	Add("hub_logo_url", hub_logo_url, "");
	Add("hub_encoding", hub_encoding, DEFAULT_HUB_ENCODING);
	Add("hub_owner",hub_owner, "");
	Add("hub_version", hub_version, HUB_VERSION_VERS);
	Add("hub_version_special",hub_version_special, "");
	Add("hub_security", hub_security, HUB_VERSION_NAME);
	Add("hub_security_desc", hub_security_desc, "Hub security");
	Add("opchat_name", opchat_name, "OpChat");
	Add("opchat_desc", opchat_desc, "Operator chat");
	Add("opchat_class", opchat_class, int(eUC_OPERATOR));
	Add("hub_host", hub_host, "");
	Add("hub_failover_hosts", hub_failover_hosts, "");
	Add("listen_ip",mS.mAddr, "0.0.0.0");

	//#if !defined _WIN32
		Add("listen_port", mS.mPort, 4111);
	/*
	#else
		Add("listen_port", mS.mPort, 411);
	#endif
	*/

	Add("extra_listen_ports", extra_listen_ports, "");
	Add("tls_proxy_ip", mS.mTLSProxy, "");
	// end of section

	// begin hublist configuration
	Add("hublist_host", hublist_host, "hublist.te-home.net hublist.pwiam.com");
	Add("hublist_port", hublist_port, 2501);
	Add("hublist_send_minshare", hublist_send_minshare, true);
	Add("hublist_send_listhost", hublist_send_listhost, true);
	Add("timer_hublist_period", mS.mHublistTimer.mMinDelay.tv_sec, (__typeof__(mS.mHublistTimer.mMinDelay.tv_sec)) 6 * 3600);
	// end hublist configuration

	// begin update check configuration
	Add("update_check_git", update_check_git, false);
	Add("update_check_period", mS.mUpdateTimer.mMinDelay.tv_sec, (__typeof__(mS.mUpdateTimer.mMinDelay.tv_sec)) 12 * 3600);
	// end of section

	// Max users configuration
	Add("max_users",max_users_total,6000);
	Add("max_users_passive", max_users_passive, -1);
	Add("max_users_from_ip", max_users_from_ip, 0);
	Add("max_extra_pings", max_extra_pings, 10);
	Add("max_extra_regs",max_extra_regs,25);
	Add("max_extra_vips",max_extra_vips,50);
	Add("max_extra_ops",max_extra_ops,100);
	Add("max_extra_cheefs",max_extra_cheefs,100);
	Add("max_extra_admins",max_extra_admins,200);
	Add("max_users0",max_users[0],6000);
	Add("max_users1",max_users[1],1000);
	Add("max_users2",max_users[2],1000);
	Add("max_users3",max_users[3],1000);
	Add("max_users4",max_users[4],1000);
	Add("max_users5",max_users[5],1000);
	Add("max_users6",max_users[6],1000);
	Add("hubfull_message", hubfull_message, "");
	// End max users configuration

	// Share configuration
	Add("min_share",min_share,(unsigned long)0);
	Add("min_share_reg",min_share_reg,(unsigned long)0);
	Add("min_share_vip",min_share_vip,(unsigned long)0);
	Add("min_share_ops",min_share_ops,(unsigned long)0);
	Add("min_share_factor_passive", min_share_factor_passive, 1.0);
	Add("min_share_use_hub",min_share_use_hub,(unsigned long)0);
	Add("min_share_use_hub_reg",min_share_use_hub_reg,(unsigned long)0);
	Add("min_share_use_hub_vip",min_share_use_hub_vip,(unsigned long)0);
	Add("max_share",max_share,(unsigned long)30*1024*1024);
	Add("max_share_reg",max_share_reg,(unsigned long)30*1024*1024);
	Add("max_share_vip",max_share_vip,(unsigned long)30*1024*1024);
	Add("max_share_ops",max_share_ops,(unsigned long)30*1024*1024);
	// End share configuration

	// search configuration
	Add("use_search_filter", use_search_filter, true);
	Add("filter_lan_requests", filter_lan_requests, false);
	Add("search_number", search_number, 1);
	Add("int_search", int_search, 32);
	Add("int_search_pas", int_search_pas, 48);
	Add("int_search_reg", int_search_reg, 16);
	Add("int_search_reg_pas", int_search_reg_pas, 48);
	Add("int_search_vip", int_search_vip, 8);
	Add("int_search_op", int_search_op, 1);
	Add("min_search_chars", min_search_chars, 4);
	Add("max_passive_sr",max_passive_sr,25);
	Add("delayed_search", delayed_search, true);
	// End search configuration

	// Nicklist configuration
	Add("max_nick",max_nick,64u);
	Add("min_nick",min_nick,1u);
	Add("nick_chars",nick_chars, "");
	Add("nick_prefix", nick_prefix, "");
	Add("nick_prefix_nocase", nick_prefix_nocase, false);
	Add("nick_prefix_cc", nick_prefix_cc, false);
	Add("nick_prefix_autoreg",nick_prefix_autoreg, "");
	Add("autoreg_class", autoreg_class, 0);
	Add("autounreg_class", autounreg_class, 0);
	// End nicklist configuration

	// protocol commands length
	Add("max_len_supports", max_len_supports, 512);
	Add("max_len_version", max_len_version, 64);
	Add("max_len_myinfo", max_len_myinfo, 512);
	Add("max_len_in", max_len_in, 512);
	Add("max_len_extjson", max_len_extjson, 1024);
	Add("max_len_myhuburl", max_len_myhuburl, 128);
	Add("max_len_search", max_len_search, 256);
	Add("max_len_mynick", max_len_mynick, 128);
	Add("max_len_lock", max_len_lock, 256);
	// end of section

	// chat configuration
	Add("max_chat_msg", max_chat_msg, 256);
	Add("max_pm_msg", max_pm_msg, 1024);
	Add("max_mcto_msg", max_mcto_msg, 1024);
	Add("max_chat_lines", max_chat_lines, 5);
	Add("delayed_chat", delayed_chat, false);
	Add("int_chat_ms", int_chat_ms, 1000ul);
	Add("chat_default_on", chat_default_on, true);
	Add("mainchat_class", mainchat_class, int(eUC_NORMUSER));
	Add("private_class", private_class, int(eUC_NORMUSER));
	Add("notify_gag_chats", notify_gag_chats, false);
	Add("clean_gags_cleanbans", clean_gags_cleanbans, false);
	// end of section

	// same message flood
	Add("max_flood_counter_pm", max_flood_counter_pm, 5);
	Add("max_flood_counter_mcto", max_flood_counter_mcto, 5);
	Add("same_flood_ban_time", same_flood_ban_time, 900);
	// end of section

	/*
		protocol flood, period in seconds, limit is maximum count, any of two values to 0 means disabled
		actions: 0 = report only, 1 = skip, 2 = drop, 3 = ban (default)
	*/
	Add("max_class_proto_flood", max_class_proto_flood, int(eUC_VIPUSER));
	Add("proto_flood_report", proto_flood_report, true);
	Add("proto_flood_report_locked", proto_flood_report_locked, true);
	Add("proto_flood_tban_time", proto_flood_tban_time, 1800); // in seconds, 30 minutes
	Add("proto_flood_report_time", proto_flood_report_time, 600); // in seconds, 10 minutes

	Add("int_flood_chat_period", int_flood_chat_period, 10);
	Add("int_flood_chat_limit", int_flood_chat_limit, 5);
	Add("proto_flood_chat_action", proto_flood_chat_action, 3);

	Add("int_flood_mcto_period", int_flood_mcto_period, 10);
	Add("int_flood_mcto_limit", int_flood_mcto_limit, 5);
	Add("proto_flood_mcto_action", proto_flood_mcto_action, 3);

	Add("int_flood_to_period", int_flood_to_period, 10);
	Add("int_flood_to_limit", int_flood_to_limit, 5);
	Add("proto_flood_to_action", proto_flood_to_action, 3);

	Add("int_flood_myinfo_period", int_flood_myinfo_period, 60);
	Add("int_flood_myinfo_limit", int_flood_myinfo_limit, 20);
	Add("proto_flood_myinfo_action", proto_flood_myinfo_action, 3);

	Add("int_flood_in_period", int_flood_in_period, 60);
	Add("int_flood_in_limit", int_flood_in_limit, 20);
	Add("proto_flood_in_action", proto_flood_in_action, 3);

	Add("int_flood_extjson_period", int_flood_extjson_period, 60);
	Add("int_flood_extjson_limit", int_flood_extjson_limit, 20);
	Add("proto_flood_extjson_action", proto_flood_extjson_action, 3);

	Add("int_flood_search_period", int_flood_search_period, 60);
	Add("int_flood_search_limit", int_flood_search_limit, 30);
	Add("proto_flood_search_action", proto_flood_search_action, 3);

	Add("int_flood_sr_period", int_flood_sr_period, 30);
	Add("int_flood_sr_limit", int_flood_sr_limit, 500);
	Add("proto_flood_sr_action", proto_flood_sr_action, 3);

	Add("int_flood_ctm_period", int_flood_ctm_period, 10);
	Add("int_flood_ctm_limit", int_flood_ctm_limit, 300);
	Add("proto_flood_ctm_action", proto_flood_ctm_action, 3);

	Add("int_flood_rctm_period", int_flood_rctm_period, 10);
	Add("int_flood_rctm_limit", int_flood_rctm_limit, 300);
	Add("proto_flood_rctm_action", proto_flood_rctm_action, 3);

	Add("int_flood_nicklist_period", int_flood_nicklist_period, 60);
	Add("int_flood_nicklist_limit", int_flood_nicklist_limit, 3);
	Add("proto_flood_nicklist_action", proto_flood_nicklist_action, 3);

	Add("int_flood_getinfo_period", int_flood_getinfo_period, 10);
	Add("int_flood_getinfo_limit", int_flood_getinfo_limit, 200);
	Add("proto_flood_getinfo_action", proto_flood_getinfo_action, 3);

	Add("int_flood_ping_period", int_flood_ping_period, 60);
	Add("int_flood_ping_limit", int_flood_ping_limit, 5);
	Add("proto_flood_ping_action", proto_flood_ping_action, 3);

	Add("int_flood_unknown_period", int_flood_unknown_period, 30);
	Add("int_flood_unknown_limit", int_flood_unknown_limit, 10);
	Add("proto_flood_unknown_action", proto_flood_unknown_action, 3);

	// from all
	Add("int_flood_all_chat_period", int_flood_all_chat_period, 15);
	Add("int_flood_all_chat_limit", int_flood_all_chat_limit, 10);
	Add("int_flood_all_mcto_period", int_flood_all_mcto_period, 15);
	Add("int_flood_all_mcto_limit", int_flood_all_mcto_limit, 10);
	Add("int_flood_all_to_period", int_flood_all_to_period, 15);
	Add("int_flood_all_to_limit", int_flood_all_to_limit, 10);
	Add("int_flood_all_search_period", int_flood_all_search_period, 5); // todo: still not sure about these numbers
	Add("int_flood_all_search_limit", int_flood_all_search_limit, 400);
	Add("int_flood_all_rctm_period", int_flood_all_rctm_period, 10);
	Add("int_flood_all_rctm_limit", int_flood_all_rctm_limit, 200);
	// end of section

	// User control configuration
	Add("classdif_reg", classdif_reg, 2);
	Add("classdif_kick", classdif_kick, 0);
	Add("classdif_pm",classdif_pm,10);
	Add("classdif_mcto", classdif_mcto, 10);
	//Add("classdif_search",classdif_search,10);
	Add("classdif_download",classdif_download,10);
	Add("min_class_use_hub", min_class_use_hub, (int)eUC_NORMUSER);
	Add("min_class_use_hub_passive", min_class_use_hub_passive, (int)eUC_NORMUSER);
	Add("use_hub_msg_time", use_hub_msg_time, 0);
	Add("min_class_register" , min_class_register, (int)eUC_CHEEF);
	Add("min_class_redir", min_class_redir, (int)eUC_CHEEF);
	Add("min_class_lstredir", min_class_lstredir, (int)eUC_CHEEF);
	Add("min_class_bc", min_class_bc, (int)eUC_CHEEF);
	Add("min_class_bc_guests", min_class_bc_guests, (int)eUC_CHEEF);
	Add("min_class_bc_regs", min_class_bc_regs, (int)eUC_CHEEF);
	Add("min_class_bc_vips", min_class_bc_vips, (int)eUC_CHEEF);
	Add("bc_reply",mS.LastBCNick,mEmpty);
	Add("plugin_mod_class", plugin_mod_class, (int)eUC_ADMIN);
	Add("topic_mod_class", topic_mod_class, (int)eUC_CHEEF);
	Add("cmd_start_op", cmd_start_op, string(DEFAULT_COMMAND_TRIGS));
	Add("cmd_start_user", cmd_start_user, string(DEFAULT_COMMAND_TRIGS));
	Add("unknown_cmd_chat", unknown_cmd_chat, false);
	Add("dest_report_chat", dest_report_chat, false);
	Add("dest_regme_chat", dest_regme_chat, false);
	Add("dest_drop_chat", dest_drop_chat, false);
	Add("disable_me_cmd", disable_me_cmd, false);
	Add("disable_regme_cmd", disable_regme_cmd, false);
	Add("disable_usr_cmds", disable_usr_cmds, false);
	Add("disable_report_cmd", disable_report_cmd, false);
	Add("always_ask_password", always_ask_password, false);
	Add("default_password_encryption", default_password_encryption, (unsigned int)cRegUserInfo::eCRYPT_MD5); // 2
	Add("password_min_len", password_min_len, 6);
	Add("pwd_tmpban", pwd_tmpban, 60);
	Add("wrongpass_message", wrongpass_message, "");
	Add("wrongpassword_report", wrongpassword_report, true);
	Add("report_pass_reset", report_pass_reset, false);
	Add("wrongauthip_report", wrongauthip_report, 1);
	Add("wrongip_message", wrongip_message, false);
	Add("clone_detect_count", clone_detect_count, 0); // 0 means disabled
	Add("clone_detect_report", clone_detect_report, true);
	Add("nullchars_report", nullchars_report, true);
	Add("botinfo_report", botinfo_report, false);
	Add("send_user_ip", send_user_ip, false);
	Add("user_ip_class", user_ip_class, int(eUC_OPERATOR));
	Add("oplist_class", oplist_class, int(eUC_OPERATOR));
	Add("send_user_info", send_user_info, true);
	Add("send_pass_request", send_pass_request, true);
	Add("send_crash_report", send_crash_report, true);
	Add("ban_bypass_class", ban_bypass_class, int(eUC_MASTER));
	Add("chatonly_bypass_class", chatonly_bypass_class, int(eUC_OPERATOR));
	// end of section

	// Advanced hub configuration and tweaks
	Add("extended_welcome_message", extended_welcome_message, true);
	Add("host_header", host_header, 1);
	Add("int_myinfo", int_myinfo, 60);
	Add("int_nicklist", int_nicklist, 60);
	Add("int_login",int_login, 60);
	Add("max_class_int_login", max_class_int_login, int(eUC_OPERATOR));
	Add("max_class_check_clone", max_class_check_clone, 0);
	Add("max_class_self_repass", max_class_self_repass, 0); // 0 means disabled
	Add("clone_det_tban_time", clone_det_tban_time, 1800); // 30 minutes
	Add("clone_ip_tban_time", clone_ip_tban_time, 0); // 0 means disabled
	Add("tban_kick", tban_kick, 300);
	Add("tban_max", tban_max, 3600 * 24 * 30);
	Add("log_level",mS.msLogLevel, 0);
	Add("dns_lookup",mS.mUseDNS, 0);
	Add("report_dns_lookup", report_dns_lookup, false);
	Add("report_user_country", report_user_country, true);
	Add("hide_all_kicks", hide_all_kicks, true);
	Add("notify_kicks_to_all", notify_kicks_to_all, -1); // -1 means disabled, higher values are minimum class
	Add("allow_same_user", allow_same_user, true);
	Add("max_class_same_user", max_class_same_user, int(eUC_NORMUSER));

	/*
		hides following messages
			connect request to offline user
			connect request to bot
			connect request to self
			connect request to user with hidden share
			connect request to user in lan or wan
			passive connect request to passive user
	*/
	Add("hide_msg_badctm", hide_msg_badctm, false);
	Add("hide_msg_badtlsctm", hide_msg_badtlsctm, false); // hides tls compatibility messages

	Add("adv_timer_conn_period", mS.timer_conn_period, 4);
	Add("adv_timer_serv_period", mS.timer_serv_period, 1);
	Add("adv_min_frequency", min_frequency, 0.3);
	Add("adv_step_delay", mS.mStepDelay, 50); // note: this is milliseconds
	Add("adv_no_conn_delay", mS.mNoConnDelay, 50); // note: this is microseconds
	Add("adv_no_read_try", mS.mNoReadTry, 100);
	Add("adv_no_read_delay", mS.mNoReadDelay, 5); // note: this is microseconds
	Add("adv_conn_choose_timeout", mS.mChooseTimeOut, 10); // note: this is milliseconds
	Add("adv_conn_accept_num", mS.mAcceptNum, 100); // note: this also sets listen backlog
	Add("adv_conn_accept_try", mS.mAcceptTry, 10);
	Add("adv_max_upload_kbps", max_upload_kbps, 131072.);
	Add("adv_max_outbuf_size", max_outbuf_size, (unsigned long)MAX_SEND_SIZE);
	Add("adv_max_outfill_size", max_outfill_size, (unsigned long)MAX_SEND_FILL_SIZE);
	Add("adv_max_unblock_size", max_unblock_size, (unsigned long)MAX_SEND_UNBLOCK_SIZE);
	Add("adv_max_message_size", mS.mMaxLineLength, 10240ul);
	Add("timer_reloadcfg_period", mS.mReloadcfgTimer.mMinDelay.tv_sec, (__typeof__( mS.mReloadcfgTimer.mMinDelay.tv_sec))300); // 5 minutes
	Add("use_reglist_cache", use_reglist_cache, true);
	Add("use_penlist_cache", use_penlist_cache, true);
	Add("delayed_myinfo", delayed_myinfo, true);
	Add("drop_invalid_key", drop_invalid_key, false);
	Add("delayed_ping", delayed_ping, 60);
	Add("disable_zlib", disable_zlib, true);
	Add("zlib_compress_level", zlib_compress_level, int(Z_DEFAULT_COMPRESSION));
	Add("zlib_min_len", zlib_min_len, 100);
	Add("detect_ctmtohub", detect_ctmtohub, true); // ctm2hub
	Add("disable_extjson", disable_extjson, true); // extjson
	Add("myinfo_tls_filter", myinfo_tls_filter, false);
	Add("mmdb_names_lang", mmdb_names_lang, ""); // maxminddb names language, empty means english
	Add("mmdb_conv_depth", mmdb_conv_depth, 2); // conversion depth, 0 = do nothing, 1 = utf8 to hub_encoding conversion, 2 = transliteration
	Add("mmdb_cache", mmdb_cache, true);
	Add("mmdb_cache_mins", mmdb_cache_mins, 60); // 0 = never delete

	static const char *to_names[] = {
		"key", "nick", "login", "myinfo", "flush", "setpass"
	};

	double to_default[] = {
		60., 30., 600., 40., 30., 300.
	};

	string s_varname;

	for (int i = 0; i < 6; i++) {
		s_varname = "timeout_";
		s_varname += to_names[i];
		Add(s_varname, timeout_length[i], to_default[i]);
	}
	// end advanced hub configuration and tweaks

	 // Tag configuration
	Add("show_tags", show_tags, true);
	Add("tag_allow_none", tag_allow_none, true);
	Add("tag_allow_unknown", tag_allow_unknown, true);
	Add("tag_allow_passive", tag_allow_passive, true);
	Add("tag_allow_sock5", tag_allow_sock5, true);
	Add("tag_sum_hubs", tag_sum_hubs, 3);
	Add("tag_min_class_ignore", tag_min_class_ignore, (int)eUC_OPERATOR);
	Add("show_desc_len",show_desc_len,-1);
	Add("desc_insert_mode", desc_insert_mode, false);
	Add("desc_insert_vars", desc_insert_vars, ""); // %[CLASS], %[CLASSNAME], %[MODE], %[CC], %[CN], %[CITY]
	Add("show_email", show_email, true);
	Add("show_speed", show_speed, true);
	Add("tag_min_hs_ratio", tag_min_hs_ratio, 0.);
	Add("tag_max_hs_ratio", tag_max_hs_ratio, 0.);
	Add("tag_max_hubs",tag_max_hubs, 0);
	Add("tag_min_hubs", tag_min_hubs, 0);
	Add("tag_min_hubs_usr", tag_min_hubs_usr, 0);
	Add("tag_min_hubs_reg", tag_min_hubs_reg, 0);
	Add("tag_min_hubs_op", tag_min_hubs_op, 0);
	Add("tag_min_version",tag_min_version,-1);
	Add("tag_max_version",tag_max_version,-1);
	// End tag configuration

	// IP and zone configuration
	Add("cc_zone1",cc_zone[0], "");
	Add("cc_zone2",cc_zone[1], "");
	Add("cc_zone3",cc_zone[2], "");
	Add("ip_zone4_min",ip_zone4_min, "");
	Add("ip_zone4_max",ip_zone4_max, "");
	Add("ip_zone5_min",ip_zone5_min, "");
	Add("ip_zone5_max",ip_zone5_max, "");
	Add("ip_zone6_min",ip_zone6_min, "");
	Add("ip_zone6_max",ip_zone6_max, "");
	// End IP and zone configuration

	// custom messages
	Add("ban_extra_message", ban_extra_message, "");
	Add("msg_replace_ban", msg_replace_ban, "");
	Add("msg_welcome_guest", msg_welcome[int(eUC_NORMUSER)]);
	Add("msg_welcome_reg", msg_welcome[int(eUC_REGUSER)]);
	Add("msg_welcome_vip", msg_welcome[int(eUC_VIPUSER)]);
	Add("msg_welcome_op", msg_welcome[int(eUC_OPERATOR)]);
	Add("msg_welcome_cheef", msg_welcome[int(eUC_CHEEF)]);
	Add("msg_welcome_admin", msg_welcome[int(eUC_ADMIN)]);
	Add("msg_welcome_master", msg_welcome[int(eUC_MASTER)]);
	// end of section
}

int cDCConf::Load()
{
	mS.mSetupList.LoadFileTo(this, mS.mDBConf.config_name.c_str());
	return 0;
}

int cDCConf::Save()
{
	string val_new, val_old;
	mS.SetConfig(mS.mDBConf.config_name.c_str(), "hub_version", HUB_VERSION_VERS, val_new, val_old);
	mS.mSetupList.SaveFileTo(this, mS.mDBConf.config_name.c_str());
	return 0;
}

	}; // namepsace nTables
}; // namespace nVerliHub
