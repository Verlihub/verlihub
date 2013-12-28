/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

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

#include <config.h>
#include <verlihub/cserverdc.h>
#include "cpiperl.h"

cpiPerl::cpiPerl()
{
	mName = "PerlScript";
	mVersion= "1.2.0";
}

cpiPerl::~cpiPerl()
{}

bool cpiPerl::RegisterAll()
{
	RegisterCallBack("VH_OnNewConn");
	RegisterCallBack("VH_OnCloseConn");
	RegisterCallBack("VH_OnParsedMsgChat");
	RegisterCallBack("VH_OnParsedMsgPM");
	RegisterCallBack("VH_OnParsedMsgSearch");
	RegisterCallBack("VH_OnParsedMsgSR");
	RegisterCallBack("VH_OnParsedMsgMyINFO");
	RegisterCallBack("VH_OnParsedMsgValidateNick");
	RegisterCallBack("VH_OnParsedMsgAny");
	RegisterCallBack("VH_OnParsedMsgSupport");
	RegisterCallBack("VH_OnParsedMsgMyPass");
	RegisterCallBack("VH_OnUnknownMsg");
	RegisterCallBack("VH_OnOperatorCommand");
	RegisterCallBack("VH_OnOperatorKicks");
	RegisterCallBack("VH_OnOperatorDrops");
	RegisterCallBack("VH_OnValidateTag");
	RegisterCallBack("VH_OnUserCommand");
	RegisterCallBack("VH_OnUserLogin");
	RegisterCallBack("VH_OnUserLogout");
	RegisterCallBack("VH_OnTimer");
	RegisterCallBack("VH_OnNewReg");
	RegisterCallBack("VH_OnNewBan");
	return true;
}

void cpiPerl::OnLoad(cServerDC* server)
{
	mServer = server;
	string script(mServer->mConfigBaseDir);
	script.append(DEFAULT_PERL_SCRIPT);
	char *argv[] = { "", (char*)script.c_str(), NULL };
	mPerl.Parse(2, argv);//) Suicide();
}

bool cpiPerl::OnNewConn(cConnDC * conn)
{
	char *args[] =  { (char *)conn->AddrIP().c_str(), NULL };
	mPerl.CallArgv("VH_OnNewConn",G_EVAL|G_DISCARD, args);
	return true;
}

bool cpiPerl::OnCloseConn(cConnDC * conn)
{
	char *args[] =  { (char *)conn->AddrIP().c_str(), NULL };
	mPerl.CallArgv("VH_OnCloseConn",G_EVAL|G_DISCARD, args);
	return true;
}

bool cpiPerl::OnParsedMsgAny(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)conn->AddrIP().c_str(),
				(char *)msg->mStr.c_str(),
				NULL };
	mPerl.CallArgv("VH_OnParsedMsgAny",G_EVAL|G_DISCARD, args);
	return true;
}

bool cpiPerl::OnParsedMsgSupport(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)conn->AddrIP().c_str(),
				(char *)msg->mStr.c_str(),
				NULL };
	mPerl.CallArgv("VH_OnParsedMsgSupport",G_EVAL|G_DISCARD, args);
	return true;
}

bool cpiPerl::OnParsedMsgValidateNick(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)conn->AddrIP().c_str(),
				(char *)msg->ChunkString(eCH_1_ALL).c_str(),
				NULL }; // eCH_1_ALL, eCH_1_PARAM
	mPerl.CallArgv("VH_OnParsedMsgValidateNick",G_EVAL|G_DISCARD, args);
	return true;
}

bool cpiPerl::OnParsedMsgMyPass(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)conn->AddrIP().c_str(),
				(char *)msg->ChunkString(eCH_1_ALL).c_str(),
				NULL }; // eCH_1_ALL, eCH_1_PARAM
	mPerl.CallArgv("VH_OnParsedMsgMyPass",G_EVAL|G_DISCARD, args);
	return true;
}

bool cpiPerl::OnParsedMsgMyINFO(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)conn->AddrIP().c_str(),
				(char *)msg->ChunkString(eCH_MI_ALL).c_str(),
				NULL }; // eCH_MI_ALL, eCH_MI_DEST, eCH_MI_NICK, eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED, eCH_MI_MAIL, eCH_MI_SIZE;
	mPerl.CallArgv("VH_OnParsedMsgMyINFO",G_EVAL|G_DISCARD, args);
	return true;
}

bool cpiPerl::OnParsedMsgSearch(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)conn->AddrIP().c_str(),
				(char *)msg->mStr.c_str(),
				NULL }; // active: eCH_AS_ALL, eCH_AS_ADDR, eCH_AS_IP, eCH_AS_PORT, eCH_AS_QUERY; passive: eCH_PS_ALL, eCH_PS_NICK, eCH_PS_QUERY;
	mPerl.CallArgv("VH_OnParsedMsgSearch",G_EVAL|G_DISCARD, args);
	return true;
}

bool cpiPerl::OnParsedMsgSR(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)conn->AddrIP().c_str(),
				(char *)msg->ChunkString(eCH_SR_ALL).c_str(),
				NULL }; // eCH_SR_ALL, eCH_SR_FROM, eCH_SR_PATH, eCH_SR_SIZE, eCH_SR_SLOTS, eCH_SR_SL_FR, eCH_SR_SL_TO, eCH_SR_HUBINFO, eCH_SR_TO;
	mPerl.CallArgv("VH_OnParsedMsgSR",G_EVAL|G_DISCARD, args);
	return true;
}

bool cpiPerl::OnParsedMsgChat(cConnDC *conn , cMessageDC *msg)
{
	char *args[]= {
		(char *)conn->mpUser->mNick.c_str(), 
		(char *) msg->ChunkString(eCH_CH_MSG).c_str(), 
		NULL}; // eCH_CH_ALL, eCH_CH_NICK, eCH_CH_MSG;
	mPerl.CallArgv("VH_OnParsedMsgChat",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnParsedMsgPM(cConnDC *conn , cMessageDC *msg)
{
	char *args[]= {
		(char *)conn->mpUser->mNick.c_str(), 
		(char *) msg->ChunkString(eCH_PM_MSG).c_str(), 
		(char *) msg->ChunkString(eCH_PM_TO).c_str(),
		NULL}; // eCH_PM_ALL, eCH_PM_TO, eCH_PM_FROM, eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG;
	mPerl.CallArgv("VH_OnParsedMsgPM",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnValidateTag(cConnDC *conn , cDCTag *tag)
{
	char *args[]= {	(char *)conn->mpUser->mNick.c_str(),
				(char *) tag->mTag.c_str(),
				NULL};
	mPerl.CallArgv("VH_OnValidateTag",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnOperatorCommand(cConnDC *conn , std::string *str)
{
	char *args[]= {	(char *)conn->mpUser->mNick.c_str(),
				(char *) str->c_str(),
				NULL};
	mPerl.CallArgv("VH_OnOperatorCommand",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnOperatorKicks(cUser *op , cUser *user, std::string *reason)
{
	char *args[]= {	(char *)op->mNick.c_str(),
				(char *)user->mNick.c_str(),
				(char *)reason->c_str(),
				NULL };
	mPerl.CallArgv("VH_OnOperatorKicks",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnOperatorDrops(cUser *op , cUser *user)
{
	char *args[]= {	(char *)op->mNick.c_str(),
				(char *)user->mNick.c_str(),
				NULL };
	mPerl.CallArgv("VH_OnOperatorDrops",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnUserCommand(cConnDC *conn , std::string *str)
{
	char *args[]= {	(char *)conn->mpUser->mNick.c_str(),
				(char *) str->c_str(),
				NULL};
	mPerl.CallArgv("VH_OnUserCommand",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnUserLogin(cUser *user)
{
	char *args[]= { (char *)user->mNick.c_str(), NULL };
	mPerl.CallArgv("VH_OnUserLogin",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnUserLogout(cUser *user)
{
	char *args[]= { (char *)user->mNick.c_str(), NULL };
	mPerl.CallArgv("VH_OnUserLogout",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnTimer()
{
	char *args[]= { NULL };
	mPerl.CallArgv("VH_OnTimer",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnNewReg(cRegUserInfo *reginfo)
{
	char *args[]= { NULL };
	mPerl.CallArgv("VH_OnNewReg",G_EVAL|G_DISCARD,args);
	return true;
}

bool cpiPerl::OnNewBan(cBan *ban)
{
	char *args[]= { NULL };
	mPerl.CallArgv("VH_OnNewReg",G_EVAL|G_DISCARD,args);
	return true;
}

REGISTER_PLUGIN(cpiPerl);
