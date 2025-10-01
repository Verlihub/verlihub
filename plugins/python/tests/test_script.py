# test_script.py - Example Python 3 script for Verlihub plugin testing
# Define all hooks with print statements to log when called

import sys

def OnNewConn(*args):
    print("Called: OnNewConn with args:", args, file=sys.stderr)

def OnCloseConn(*args):
    print("Called: OnCloseConn with args:", args, file=sys.stderr)

def OnCloseConnEx(*args):
    print("Called: OnCloseConnEx with args:", args, file=sys.stderr)

def OnParsedMsgChat(*args):
    print("Called: OnParsedMsgChat with args:", args, file=sys.stderr)

def OnParsedMsgPM(*args):
    print("Called: OnParsedMsgPM with args:", args, file=sys.stderr)

def OnParsedMsgMCTo(*args):
    print("Called: OnParsedMsgMCTo with args:", args, file=sys.stderr)

def OnParsedMsgSearch(*args):
    print("Called: OnParsedMsgSearch with args:", args, file=sys.stderr)

def OnParsedMsgSR(*args):
    print("Called: OnParsedMsgSR with args:", args, file=sys.stderr)

def OnParsedMsgMyINFO(*args):
    print("Called: OnParsedMsgMyINFO with args:", args, file=sys.stderr)

def OnFirstMyINFO(*args):
    print("Called: OnFirstMyINFO with args:", args, file=sys.stderr)

def OnParsedMsgValidateNick(*args):
    print("Called: OnParsedMsgValidateNick with args:", args, file=sys.stderr)

def OnParsedMsgAny(*args):
    print("Called: OnParsedMsgAny with args:", args, file=sys.stderr)

def OnParsedMsgAnyEx(*args):
    print("Called: OnParsedMsgAnyEx with args:", args, file=sys.stderr)

def OnOpChatMessage(*args):
    print("Called: OnOpChatMessage with args:", args, file=sys.stderr)

def OnPublicBotMessage(*args):
    print("Called: OnPublicBotMessage with args:", args, file=sys.stderr)

def OnUnLoad(*args):
    print("Called: OnUnLoad with args:", args, file=sys.stderr)

def OnCtmToHub(*args):
    print("Called: OnCtmToHub with args:", args, file=sys.stderr)

def OnParsedMsgSupports(*args):
    print("Called: OnParsedMsgSupports with args:", args, file=sys.stderr)

def OnParsedMsgMyHubURL(*args):
    print("Called: OnParsedMsgMyHubURL with args:", args, file=sys.stderr)

def OnParsedMsgExtJSON(*args):
    print("Called: OnParsedMsgExtJSON with args:", args, file=sys.stderr)

def OnParsedMsgBotINFO(*args):
    print("Called: OnParsedMsgBotINFO with args:", args, file=sys.stderr)

def OnParsedMsgVersion(*args):
    print("Called: OnParsedMsgVersion with args:", args, file=sys.stderr)

def OnParsedMsgMyPass(*args):
    print("Called: OnParsedMsgMyPass with args:", args, file=sys.stderr)

def OnParsedMsgConnectToMe(*args):
    print("Called: OnParsedMsgConnectToMe with args:", args, file=sys.stderr)

def OnParsedMsgRevConnectToMe(*args):
    print("Called: OnParsedMsgRevConnectToMe with args:", args, file=sys.stderr)

def OnUnknownMsg(*args):
    print("Called: OnUnknownMsg with args:", args, file=sys.stderr)

def OnOperatorCommand(*args):
    print("Called: OnOperatorCommand with args:", args, file=sys.stderr)

def OnOperatorKicks(*args):
    print("Called: OnOperatorKicks with args:", args, file=sys.stderr)

def OnOperatorDrops(*args):
    print("Called: OnOperatorDrops with args:", args, file=sys.stderr)

def OnOperatorDropsWithReason(*args):
    print("Called: OnOperatorDropsWithReason with args:", args, file=sys.stderr)

def OnValidateTag(*args):
    print("Called: OnValidateTag with args:", args, file=sys.stderr)

def OnUserCommand(*args):
    print("Called: OnUserCommand with args:", args, file=sys.stderr)

def OnHubCommand(*args):
    print("Called: OnHubCommand with args:", args, file=sys.stderr)

def OnScriptCommand(*args):
    print("Called: OnScriptCommand with args:", args, file=sys.stderr)

def OnUserInList(*args):
    print("Called: OnUserInList with args:", args, file=sys.stderr)

def OnUserLogin(*args):
    print("Called: OnUserLogin with args:", args, file=sys.stderr)

def OnUserLogout(*args):
    print("Called: OnUserLogout with args:", args, file=sys.stderr)

def OnTimer(*args):
    print("Called: OnTimer with args:", args, file=sys.stderr)

def OnNewReg(*args):
    print("Called: OnNewReg with args:", args, file=sys.stderr)

def OnNewBan(*args):
    print("Called: OnNewBan with args:", args, file=sys.stderr)

def OnSetConfig(*args):
    print("Called: OnSetConfig with args:", args, file=sys.stderr)

# Optional: Define name_and_version for script identification
def name_and_version():
    return "Test Script", "1.0"