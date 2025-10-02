# test_script.py - Example Python 3 script for Verlihub plugin testing
# Define all hooks with print statements to log when called

import sys

def OnNewConn(*args):
    print("Called: OnNewConn with args:", args, file=sys.stderr)
    return 1  # Allow further processing

def OnCloseConn(*args):
    print("Called: OnCloseConn with args:", args, file=sys.stderr)
    return 1

def OnCloseConnEx(*args):
    print("Called: OnCloseConnEx with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgChat(*args):
    print("Called: OnParsedMsgChat with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgPM(*args):
    print("Called: OnParsedMsgPM with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgMCTo(*args):
    print("Called: OnParsedMsgMCTo with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgSearch(*args):
    print("Called: OnParsedMsgSearch with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgSR(*args):
    print("Called: OnParsedMsgSR with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgMyINFO(*args):
    print("Called: OnParsedMsgMyINFO with args:", args, file=sys.stderr)
    return 1

def OnFirstMyINFO(*args):
    print("Called: OnFirstMyINFO with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgValidateNick(*args):
    print("Called: OnParsedMsgValidateNick with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgAny(*args):
    print("Called: OnParsedMsgAny with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgAnyEx(*args):
    print("Called: OnParsedMsgAnyEx with args:", args, file=sys.stderr)
    return 1

def OnOpChatMessage(*args):
    print("Called: OnOpChatMessage with args:", args, file=sys.stderr)
    return 1

def OnPublicBotMessage(*args):
    print("Called: OnPublicBotMessage with args:", args, file=sys.stderr)
    return 1

def OnUnLoad(*args):
    print("Called: OnUnLoad with args:", args, file=sys.stderr)
    return 1

def OnCtmToHub(*args):
    print("Called: OnCtmToHub with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgSupports(*args):
    print("Called: OnParsedMsgSupports with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgMyHubURL(*args):
    print("Called: OnParsedMsgMyHubURL with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgExtJSON(*args):
    print("Called: OnParsedMsgExtJSON with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgBotINFO(*args):
    print("Called: OnParsedMsgBotINFO with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgVersion(*args):
    print("Called: OnParsedMsgVersion with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgMyPass(*args):
    print("Called: OnParsedMsgMyPass with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgConnectToMe(*args):
    print("Called: OnParsedMsgConnectToMe with args:", args, file=sys.stderr)
    return 1

def OnParsedMsgRevConnectToMe(*args):
    print("Called: OnParsedMsgRevConnectToMe with args:", args, file=sys.stderr)
    return 1

def OnUnknownMsg(*args):
    print("Called: OnUnknownMsg with args:", args, file=sys.stderr)
    return 1

def OnOperatorCommand(*args):
    print("Called: OnOperatorCommand with args:", args, file=sys.stderr)
    return 1

def OnOperatorKicks(*args):
    print("Called: OnOperatorKicks with args:", args, file=sys.stderr)
    return 1

def OnOperatorDrops(*args):
    print("Called: OnOperatorDrops with args:", args, file=sys.stderr)
    return 1

def OnOperatorDropsWithReason(*args):
    print("Called: OnOperatorDropsWithReason with args:", args, file=sys.stderr)
    return 1

def OnValidateTag(*args):
    print("Called: OnValidateTag with args:", args, file=sys.stderr)
    return 1

def OnUserCommand(*args):
    print("Called: OnUserCommand with args:", args, file=sys.stderr)
    return 1

def OnHubCommand(*args):
    print("Called: OnHubCommand with args:", args, file=sys.stderr)
    return 1

def OnScriptCommand(*args):
    print("Called: OnScriptCommand with args:", args, file=sys.stderr)
    return 1

def OnUserInList(*args):
    print("Called: OnUserInList with args:", args, file=sys.stderr)
    return 1

def OnUserLogin(*args):
    print("Called: OnUserLogin with args:", args, file=sys.stderr)
    return 1

def OnUserLogout(*args):
    print("Called: OnUserLogout with args:", args, file=sys.stderr)
    return 1

def OnTimer(*args):
    print("Called: OnTimer with args:", args, file=sys.stderr)
    return 1

def OnNewReg(*args):
    print("Called: OnNewReg with args:", args, file=sys.stderr)
    return 1

def OnNewBan(*args):
    print("Called: OnNewBan with args:", args, file=sys.stderr)
    return 1

def OnSetConfig(*args):
    print("Called: OnSetConfig with args:", args, file=sys.stderr)
    return 1

# Optional: Define name_and_version for script identification
def name_and_version():
    return "Test Script", "1.0"