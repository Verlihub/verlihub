#include <gtest/gtest.h>
#include <gmock/gmock.h>  // For mocking
#include "src/cserverdc.h"  // From Verlihub API
#include "src/cdcproto.h"
#include "src/cconndc.h"
#include "src/cconfigbase.h"
#include "src/cvhpluginmgr.h"
#include "src/cmysql.h"
#include "plugins/python/cpipython.h"  // Python plugin header
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>  // For sleep if needed

using namespace testing;  // For _

nVerliHub::nSocket::cServerDC* g_server = nullptr;

// Mock MySQL class
class MockMySQL : public nVerliHub::nMySQL::cMySQL {
public:
    std::string dummy_host = "dummy";
    std::string dummy_user = "dummy";
    std::string dummy_pass = "dummy";
    std::string dummy_data = "dummy";
    std::string dummy_charset = "dummy";

    MockMySQL() : nVerliHub::nMySQL::cMySQL(dummy_host, dummy_user, dummy_pass, dummy_data, dummy_charset) {}  // Pass lvalue refs

    MOCK_METHOD(bool, Connect, (), (override));
    MOCK_METHOD(void, Disconnect, (), (override));
    MOCK_METHOD(unsigned long, Query, (const std::string &), (override));
    MOCK_METHOD(MYSQL_RES *, StoreQueryResult, (), (override));
    MOCK_METHOD(void, FreeResult, (MYSQL_RES *), (override));
    MOCK_METHOD(MYSQL_ROW, FetchRow, (MYSQL_RES *), (override));
    // Add more mocks as needed
};

// Global test environment for server init/finalize
class VerlihubEnv : public ::testing::Environment {
public:
    nVerliHub::nSocket::cServerDC* server = nullptr;
    MockMySQL* mock_mysql = nullptr;
    std::string config_dir = "./test_config/";
    std::string db_config = "dbconfig";
    std::string hub_config = "config";
    std::string plugin_config = "plugins.conf";
    std::string plugins_dir = config_dir + "plugins/";

    void SetUp() override {
        // Create temp config dir and plugins subdir
        system(("mkdir -p " + plugins_dir).c_str());

        // Minimal DB config (mocked, dummy values, empty host to skip connect)
        std::ofstream db_file(config_dir + db_config);
        db_file << "db_host=\n"
                << "db_user=test\n"
                << "db_pass=test\n"
                << "db_data=test\n"
                << "db_conf=test\n"
                << "db_port=3306\n";
        db_file.close();

        // Minimal hub config
        std::ofstream hub_file(config_dir + hub_config);
        hub_file << "hub_name=TestHub\n"
                 << "hub_host=localhost\n"
                 << "max_users=100\n"
                 << "listen_port=411\n"
                 << "listen_addr=127.0.0.1\n"
                 << "use_dns=0\n"
                 << "min_share=0\n"
                 << "max_share=0\n";
        hub_file.close();

        // Plugins config: Load Python plugin
        std::ofstream plugin_file(config_dir + plugin_config);
        plugin_file << "python_pi\n";  // Plugin identifier
        plugin_file.close();

        // Initialize server
        server = new nVerliHub::nSocket::cServerDC(config_dir, "");
        g_server = server;

        // Replace with mock MySQL
        delete server->mMySQL;
        mock_mysql = new MockMySQL();
        server->mMySQL = mock_mysql;

        // Set expectations
        EXPECT_CALL(*mock_mysql, Connect()).WillRepeatedly(Return(true));
        EXPECT_CALL(*mock_mysql, Disconnect()).Times(AnyNumber());
        EXPECT_CALL(*mock_mysql, Query(_)).WillRepeatedly(Return(0UL));
        EXPECT_CALL(*mock_mysql, StoreQueryResult()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*mock_mysql, FreeResult(_)).Times(AnyNumber());
        EXPECT_CALL(*mock_mysql, FetchRow(_)).WillRepeatedly(Return(nullptr));

        // Connect DB (mocked, but since host empty, may skip internally)
        server->DbConnect();

        // Load plugins
        server->mPluginManager.LoadAll();
    }

    void TearDown() override {
        if (server) {
            server->mPluginManager.UnLoadAll();
            delete server;
            server = nullptr;
            g_server = nullptr;
        }
        // Cleanup configs
        system("rm -rf ./test_config");
    }
};

// Script content (full)
const char* script_content = R"python(
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
)python";

// Test fixture
class VerlihubIntegrationTest : public ::testing::Test {
protected:
    nVerliHub::nSocket::cServerDC* server;
    std::string script_path = "./test_script.py";

    void SetUp() override {
        server = g_server;

        // Write script to file
        std::ofstream script_file(script_path);
        script_file << script_content;
        script_file.close();

        // Get Python plugin
        nVerliHub::nPythonPlugin::cpiPython* py_plugin = dynamic_cast<nVerliHub::nPythonPlugin::cpiPython*>(
            server->mPluginManager.FindPlugin(PYTHON_PI_IDENTIFIER));
        ASSERT_NE(py_plugin, nullptr);

        // Load script
        nVerliHub::nPythonPlugin::cPythonInterpreter* interp = new nVerliHub::nPythonPlugin::cPythonInterpreter(script_path);
        py_plugin->AddData(interp);
        interp->Init();
    }

    void TearDown() override {
        // Unload script
        nVerliHub::nPythonPlugin::cpiPython* py_plugin = dynamic_cast<nVerliHub::nPythonPlugin::cpiPython*>(
            server->mPluginManager.FindPlugin(PYTHON_PI_IDENTIFIER));
        if (py_plugin) {
            py_plugin->RemoveByName(script_path);
        }
        std::remove(script_path.c_str());
    }

    // Helper to create mock connection
    nVerliHub::nSocket::cConnDC* CreateMockConn(const std::string& ip = "127.0.0.1") {
        nVerliHub::nSocket::cConnDC* conn = new nVerliHub::nSocket::cConnDC(0, server);
        conn->SetIP(ip.c_str());
        return conn;
    }

    // Helper to parse and treat message
    void SendMessage(nVerliHub::nSocket::cConnDC* conn, const std::string& raw_msg) {
        nVerliHub::nProtocol::cMessageParser* parser = server->mProto->CreateParser();
        parser->mStr = raw_msg;
        parser->mLen = raw_msg.size();
        parser->Parse();
        server->mProto->TreatMsg(parser, conn);
        server->mProto->DeleteParser(parser);
    }
};

// Stress test calling TreatMsg with sequence from log
TEST_F(VerlihubIntegrationTest, StressTreatMsg) {
    nVerliHub::nSocket::cConnDC* conn = CreateMockConn();
    const int iterations = 100000;  // Stress level

    // Expanded sequence of raw messages from the log
    std::vector<std::string> messages = {
        "$Supports BotINFO HubINFO|",  // Triggers OnParsedMsgSupports
        "$ValidateNick ufoDcPinger|",  // OnParsedMsgValidateNick
        "$Version 1,0091|",           // OnParsedMsgVersion
        "$MyINFO $ALL ufoDcPinger www.ufo-modus.com Chat Only <++ V:7.25.4,M:A,H:1/0/0,S:15>$ $LAN(T3)l$reg.ufo-modus.com:2501$57886211615|",  // OnParsedMsgMyINFO
        "<transfix> .|",               // Chat message -> OnParsedMsgChat
        "$Search Hub:asdkjhgftyuir F?T?0?9?TTH:VVPFZS7ZRRGR6N2BJLRFTETA3LW7HAVRFGL6UIY|",  // OnParsedMsgSearch
        "<transfix> !onplug python|",  // OnOperatorCommand
        "$Search Hub:asdkjhgftyuir F?T?0?9?TTH:PEMW564WU7QG4TKVHSUZAGCGGFG2K6LCZJIMI7Y|",  // Another search
        "$Search 173.2.131.81:60060 F?T?0?9?TTH:EO5QD5X5FQCJE6VPMFFHBULKZZGZQJXNCTQBKFQ|",  // Passive search
        "$SP VVPFZS7ZRRGR6N2BJLRFTETA3LW7HAVRFGL6UIY asdkjhgftyuir|",  // SP command if parsed as such
        "$SA EO5QD5X5FQCJE6VPMFFHBULKZZGZQJXNCTQBKFQ 173.2.131.81:60060|",  // SA command
        "$SP PEMW564WU7QG4TKVHSUZAGCGGFG2K6LCZJIMI7Y asdkjhgftyuir|",  // Another SP
        // Add more variations from log or generic
        "<auynlyian_2> |",             // Empty chat -> OnParsedMsgAny
        "<asdkjhgftyuir> $SP VVPFZS7ZRRGR6N2BJLRFTETA3LW7HAVRFGL6UIY asdkjhgftyuir|",  // Chat with SP
        "$ValidateNick transfix|",     // Another validate
        // ... Expand with more if needed
    };

    for (int i = 0; i < iterations; ++i) {
        // Rotate through messages
        std::string msg = messages[i % messages.size()];

        SendMessage(conn, msg);

        // Simulate close/open for connection hooks every 30 iterations
        if (i % 30 == 0) {
            // Trigger close hooks
            server->mCBL_CloseConn.CallAll(conn);

            delete conn;
            conn = CreateMockConn();    // New conn
            server->mCBL_NewConn.CallAll(conn);
        }

        // Progress
        if (i % 1000 == 0) {
            std::cout << "Iteration: " << i << " / " << iterations << std::endl;
        }
    }

    // Cleanup
    delete conn;
    EXPECT_TRUE(true);  // Pass if no crash
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new VerlihubEnv);
    return RUN_ALL_TESTS();
}