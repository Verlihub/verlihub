#!/usr/bin/python
#
# VH Event Logger - an example script for the Verlihub Python plugin
# Copyright (C) 2016-2017 Verlihub Team, info at verlihub dot net
# Created in 2007 by Frog (frogged), the_frog at wp dot pl
# License: GNU General Public License v.3, see http://www.gnu.org/licenses/
# Last update: 2016-03-03
#
# This script prints all received hub events to stdout
# To turn on logging, use the !vh_event_log 1 admin command, and to turn off, !vh_event_log 0.
# You can also turn it on from another script using the vh_event_log script query.

import vh
import sys

vh.name_and_version("VH Event Logger", "1.1.0")

my_command = "vh_event_log"

messages = {
    "is_on": "Event logging is now active. ",
    "is_off": "Event logging is now disabled. ",
    "already_on": "Event logging was already active. ",
    "already_off": "Event logging was not active. ",
    "error": "Bad command argument. Should be 1 (to turn logging on), 0 (to turn it off), or nothing (to get status). "
}

_log = 0

def log():
    global _log
    if _log: return 1
    return 0

def process_command(arg):
    global _log
    ret = "error"

    if arg == "1":
        ret = "already_on" if _log else "is_on"
        if not _log:
            vh.SetConfig("pi_python", "callback_log", "1")
        _log = 1
    elif arg == "0":
        ret = "is_off" if _log else "already_off"
        _log = 0
        if _log:
            vh.SetConfig("pi_python", "callback_log", "0")
    elif arg == "":
        ret = "is_on" if _log else "is_off"
    print "%s: %s" % (vh.name, messages[ret])
    sys.stdout.flush()
    return ret



def OnScriptCommand(cmd, data, plug, script):
    if log(): print "%-45s" % "OnHubCommand (cmd, data, plug, script)", (cmd, data, plug, script); sys.stdout.flush()
    if cmd == my_command:
        process_command(data)

def OnScriptQuery(cmd, data, recipient, sender):
    if log(): print "%-45s" % "OnHubCommand (cmd, data, recipient, sender)", (cmd, data, recipient, sender); sys.stdout.flush()
    if cmd == my_command:
        return process_command(data)

def OnHubCommand(nick, clean_cmd, uclass, in_pm, cmd_prefix):
    if log(): print "%-45s" % "OnHubCommand (nick, clean_cmd, uclass, in_pm, cmd_prefix)", (nick, clean_cmd, uclass, in_pm, cmd_prefix); sys.stdout.flush()

    if uclass >= 5 :
        parts = clean_cmd.split(" ", 1)
        if parts[0] == my_command:
            arg = parts[1].strip() if len(parts) == 2 else ""
            vh.usermc(messages[process_command(arg)], nick)
            return 0

def OnOperatorCommand(nick, data):
    if log(): print "%-45s" % "OnOperatorCommand (nick, data)", (nick, data); sys.stdout.flush()

def OnUserCommand(nick, data):
    if log(): print "%-45s" % "OnUserCommand (nick, data)", (nick, data); sys.stdout.flush()

def OnNewConn(ip):
    if log(): print "%-45s" % "OnNewConn (ip)", (ip); sys.stdout.flush()

def OnCloseConn(ip):
    if log(): print "%-45s" % "OnCloseConn (ip)", (ip); sys.stdout.flush()

def OnCloseConnEx(ip, reason, nick):
    if log(): print "%-45s" % "OnCloseConnEx (ip, reason, nick)", (ip, reason, nick); sys.stdout.flush()

def OnUserLogin(nick):
    if log(): print "%-45s" % "OnUserLogin (nick)", (nick); sys.stdout.flush()

def OnUserLogout(nick):
    if log(): print "%-45s" % "OnUserLogout (nick)", (nick); sys.stdout.flush()

def OnParsedMsgChat(nick, data):
    if log(): print "%-45s" % "OnParsedMsgChat (nick, data)", (nick, data); sys.stdout.flush()

def OnParsedMsgSupports(ip, msg, back):
    if log(): print "%-45s" % "OnParsedMsgSupports (ip, msg, back)", (ip, msg, back); sys.stdout.flush()

def OnParsedMsgMyHubURL(nick, data):
    if log(): print "%-45s" % "OnParsedMsgMyHubURL (nick, data)", (nick, data); sys.stdout.flush()

def OnParsedMsgExtJSON(nick, data):
    if log(): print "%-45s" % "OnParsedMsgExtJSON (nick, data)", (nick, data); sys.stdout.flush()

def OnParsedMsgBotINFO(nick, data):
    if log(): print "%-45s" % "OnParsedMsgBotINFO (nick, data)", (nick, data); sys.stdout.flush()

def OnParsedMsgVersion(nick, data):
    if log(): print "%-45s" % "OnParsedMsgVersion (nick, data)", (nick, data); sys.stdout.flush()

def OnParsedMsgMyPass(nick, data):
    if log(): print "%-45s" % "OmParsedMsgMyPass (nick, data)", (nick, data); sys.stdout.flush()

def OnParsedMsgSearch(nick, data):
    if log(): print "%-45s" % "OnParsedMsgSearch (nick, data)", (nick, data); sys.stdout.flush()

def OnParsedMsgSR(nick, data):
    if log(): print "%-45s" % "OnParsedMsgSR (nick, data)", (nick, data); sys.stdout.flush()

def OnParsedMsgMyINFO(nick, desc, tag, speed, mail, size):
    if log(): print "%-45s" % "OnParsedMsgMyINFO (nick, desc, tag, speed, mail, size)", (nick, desc, speed, mail, size); sys.stdout.flush()

def OnFirstMyINFO(nick, desc, tag, speed, mail, size):
    if log(): print "%-45s" % "OnFirstMyINFO (nick, desc, tag, speed, mail, size)", (nick, desc, speed, mail, size); sys.stdout.flush()

def OnParsedMsgAny(nick, data):
    if log(): print "%-45s" % "OnParsedMsgAny (nick, data)", (nick, data); sys.stdout.flush()

def OnParsedMsgAnyEx(ip, data):
    if log(): print "%-45s" % "OnParsedMsgAnyEx (ip, data)", (ip, data); sys.stdout.flush()

def OnParsedMsgRevConnectToMe(nick, receiver):
    if log(): print "%-45s" % "OnParsedMsgRevConnectToMe (nick, receiver)", (nick, receiver); sys.stdout.flush()

def OnOpChatMessage(nick, data):
    if log(): print "%-45s" % "OnOpChatMessage (nick, data)", (nick, data); sys.stdout.flush()

def OnPublicBotMessage(nick, data, minclass, maxclass):
    if log(): print "%-45s" % "OnPublicBotMessage (nick, data, minclass, maxclass)", (nick, data, minclass, maxclass); sys.stdout.flush()

def OnUnknownMsg(nick, data):
    if log(): print "%-45s" % "OnUnknownMsg (nick, data)", (nick, data); sys.stdout.flush()

def OnValidateTag(nick, data):
    if log(): print "%-45s" % "OnValidateTag (nick, data)", (nick, data); sys.stdout.flush()

def OnParsedMsgMCTo(nick, data, receiver):
    if log(): print "%-45s" % "OnParsedMsgMCTo (nick, data, receiver)", (nick, data, receiver); sys.stdout.flush()

def OnParsedMsgPM(nick, data, receiver):
    if log(): print "%-45s" % "OnParsedMsgPM (nick, data, receiver)", (nick, data, receiver); sys.stdout.flush()

def OnParsedMsgValidateNick(nick):
    if log(): print "%-45s" % "OnParsedMsgValidateNick (nick)", (nick); sys.stdout.flush()

def OnParsedMsgConnectToMe(nick, receiver, ip, port):
    if log(): print "%-45s" % "OnParsedMsgConnectToMe (nick, receiver, ip, port)", (nick, receiver, ip, port); sys.stdout.flush()

def OnCtmToHub(nick, ip, port, servport, ref):
    if log(): print "%-45s" % "OnCtmToHub (nick, ip, port, servport, ref)", (nick, ip, port, servport, ref); sys.stdout.flush()

def OnOperatorKicks(op, nick, reason):
    if log(): print "%-45s" % "OnOperatorKicks (op, nick, reason)", (op, nick, reason); sys.stdout.flush()

def OnOperatorDrops(op, nick):
    if log(): print "%-45s" % "OnOperatorDrops (op, nick)", (op, nick); sys.stdout.flush()

def OnOperatorDropsWithReason(op, nick, reason):
    if log(): print "%-45s" % "OnOperatorDropsWithReason (op, nick, reason)", (op, nick, reason); sys.stdout.flush()

def OnNewReg(op, nick, uclass):
    if log(): print "%-45s" % "OnNewReg (op, nick, uclass)", (op, nick, uclass); sys.stdout.flush()

def OnSetConfig(nick, conf, var, new_val, old_val, val_type):
    if log(): print "%-45s" % "OnSetConfig (nick, conf, var, new_val, old_val, val_type)", (nick, conf, var, new_val, old_val, val_type); sys.stdout.flush()

def OnNewBan(op, ip, nick, reason):
    if log(): print "%-45s" % "OnNewBan (op, ip, nick, reason)", (op, ip, nick, reason); sys.stdout.flush()

#def OnTimer(msecs=None):
    # if log(): print "%-45s" % "OnTimer(%s)" % msecs; sys.stdout.flush()

# def UnLoad():
    # global _log
    # vh.SetConfig("pi_python", "callback_log", str(_log))


#vh.classmc("Script vh_event_log.py is online. To start logging, write !vh_event_log 1, and to stop, !vh_event_log 0", 5, 10)

try:
    _log = int(vh.GetConfig("pi_python", "callback_log"))
except:
    pass
