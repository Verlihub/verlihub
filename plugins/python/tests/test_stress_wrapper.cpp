#include <gtest/gtest.h>
#include "wrapper.h"
#include "test_utils.h"
#include <fstream>
#include <string>
#include <vector>
#include <chrono>  // For timing if needed
#include <unistd.h>  // For getpid()

// Mock callbacks (empty functions for the callback table)
w_Targs* mock_callback(int id, w_Targs* args) { return nullptr; }
w_Tcallback mock_callbacks[W_MAX_CALLBACKS + 1];  // +1 to avoid out-of-bounds

// Global test environment for Python init/finalize
class GlobalPythonEnv : public ::testing::Environment {
public:
    void SetUp() override {
        // Initialize mock callbacks
        for (int i = 0; i <= W_MAX_CALLBACKS; ++i) {
            mock_callbacks[i] = mock_callback;
        }
        w_Begin(mock_callbacks);
        w_LogLevel(2);  // Set log level to match the provided log (shows calls and args)
    }
    void TearDown() override {
        w_End();
    }
};

// Script content as a multi-line string (contents of test_script.py)
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

// Test fixture for per-test setup
class PythonWrapperTest : public ::testing::Test {
protected:
    std::string script_path;

    void SetUp() override {
        // Create unique filename using PID to avoid conflicts in parallel execution
        script_path = std::string(BUILD_DIR) + "/test_script_stress_" + std::to_string(getpid()) + ".py";
        
        // Write script content to file
        std::ofstream script_file(script_path);
        script_file << script_content;
        script_file.close();
    }

    void TearDown() override {
        // Remove test script
        std::remove(script_path.c_str());
    }

    // Helper to load script and return ID
    int LoadScript() {
        int id = w_ReserveID();
        EXPECT_GE(id, 0);

        // Mock args: id, path, botname, opchatname, config_dir, starttime, config_name
        w_Targs* args = w_pack("lssssls", id, script_path.c_str(), "TestBot", "OpChat", ".", (long)0, "config");
        EXPECT_EQ(id, w_Load(args));
        free(args);  // Use free(), not w_free_args() - w_Load() copies strings with strdup()
        return id;
    }

    // Helper to call hook and free resources
    void CallAndFree(int id, int hook, w_Targs* params) {
        if (w_HasHook(id, hook)) {
            w_Targs* res = w_CallHook(id, hook, params);
            if (res) {
                // Unpack if needed (assuming return is long)
                long ret_val = 0;
                w_unpack(res, "l", &ret_val);
                free(res);
            }
        }
        free(params);  // Use free(), not w_free_args() - params are simple
    }
};

// Test dummy for init/end (handled globally)
TEST_F(PythonWrapperTest, InitAndEnd) {
    EXPECT_TRUE(true);
}

// Test script loading and unloading
TEST_F(PythonWrapperTest, LoadAndUnloadScript) {
    int id = LoadScript();

    // Check some hooks
    EXPECT_TRUE(w_HasHook(id, W_OnTimer));
    EXPECT_TRUE(w_HasHook(id, W_OnParsedMsgChat));

    EXPECT_EQ(1, w_Unload(id));
}

// Test calling a hook
TEST_F(PythonWrapperTest, CallHook) {
    int id = LoadScript();

    // Call OnTimer (expects double, but script returns 1)
    w_Targs* params = w_pack("d", 123.456);
    w_Targs* res = w_CallHook(id, W_OnTimer, params);
    free(params);  // Use free(), not w_free_args()

    ASSERT_NE(res, nullptr);
    long ret_val;
    w_unpack(res, "l", &ret_val);
    EXPECT_EQ(1, ret_val);  // As per script
    free(res);

    // Call OnParsedMsgChat (ss: nick, data)
    params = w_pack("ss", "test_user", "hello");
    res = w_CallHook(id, W_OnParsedMsgChat, params);
    free(params);  // Use free(), not w_free_args()

    ASSERT_NE(res, nullptr);
    w_unpack(res, "l", &ret_val);
    EXPECT_EQ(1, ret_val);
    free(res);

    w_Unload(id);
}

// Stress test to simulate long stream of messages and potentially crash for debugging
TEST_F(PythonWrapperTest, StressTest) {
    int id = LoadScript();

    // Start memory tracking
    MemoryTracker tracker;
    tracker.start();
    std::cout << "\n=== Stress Test Memory Tracking Started ===" << std::endl;
    std::cout << "Initial: " << tracker.initial.to_string() << std::endl;

    double current_time = 1759447563.749;  // Starting time from log
    const int iterations = 1000000;  // Large number to stress; adjust as needed

    for (int i = 0; i < iterations; ++i) {
        // OnTimer (most frequent)
        w_Targs* params = w_pack("d", current_time);
        CallAndFree(id, W_OnTimer, params);
        current_time += 1.043;  // Approximate increment from log

        // OnParsedMsgAny (e.g., empty message)
        if (i % 10 == 0) {
            params = w_pack("ss", "auynlyian_2", "");
            CallAndFree(id, W_OnParsedMsgAny, params);
        }

        // OnParsedMsgSearch
        if (i % 15 == 0) {
            params = w_pack("ss", "asdkjhgftyuir", "$Search Hub:asdkjhgftyuir F?T?0?9?TTH:VVPFZS7ZRRGR6N2BJLRFTETA3LW7HAVRFGL6UIY");
            CallAndFree(id, W_OnParsedMsgSearch, params);
        }

        // Simulate pinger connection sequence
        if (i % 30 == 0) {
            // OnNewConn
            params = w_pack("s", "127.0.0.1");
            CallAndFree(id, W_OnNewConn, params);

            // OnParsedMsgSupports (assuming "sss" based on log; adjust if needed)
            params = w_pack("sss", "127.0.0.1", "$Supports BotINFO", "HubINFO ");
            CallAndFree(id, W_OnParsedMsgSupports, params);

            // OnParsedMsgValidateNick
            params = w_pack("s", "$ValidateNick ufoDcPinger");
            CallAndFree(id, W_OnParsedMsgValidateNick, params);

            // OnParsedMsgVersion
            params = w_pack("ss", "127.0.0.1", "$Version 1,0091");
            CallAndFree(id, W_OnParsedMsgVersion, params);

            // OnParsedMsgMyINFO
            params = w_pack("ssssss", "ufoDcPinger", "www.ufo-modus.com Chat Only ", "<++ V:7.25.4,M:A,H:1/0/0,S:15>", "LAN(T3)l", "reg.ufo-modus.com:2501", "57886211615");
            CallAndFree(id, W_OnParsedMsgMyINFO, params);

            // OnFirstMyINFO (same args)
            params = w_pack("ssssss", "ufoDcPinger", "www.ufo-modus.com Chat Only ", "<++ V:7.25.4,M:A,H:1/0/0,S:15>", "LAN(T3)l", "reg.ufo-modus.com:2501", "57886211615");
            CallAndFree(id, W_OnFirstMyINFO, params);

            // OnValidateTag
            params = w_pack("ss", "ufoDcPinger", "<++ V:7.25.4,M:A,H:1/0/0,S:15>");
            CallAndFree(id, W_OnValidateTag, params);

            // OnUserInList
            params = w_pack("s", "ufoDcPinger");
            CallAndFree(id, W_OnUserInList, params);

            // OnUserLogin
            params = w_pack("s", "ufoDcPinger");
            CallAndFree(id, W_OnUserLogin, params);

            // OnCloseConn
            params = w_pack("s", "127.0.0.1");
            CallAndFree(id, W_OnCloseConn, params);

            // OnCloseConnEx
            params = w_pack("sls", "127.0.0.1", 0L, "ufoDcPinger");
            CallAndFree(id, W_OnCloseConnEx, params);

            // OnUserLogout
            params = w_pack("s", "ufoDcPinger");
            CallAndFree(id, W_OnUserLogout, params);
        }

        // Optional: Add more variety from log, e.g., another search
        if (i % 50 == 0) {
            params = w_pack("ss", "y2b4k698df328djei3_R102", "$Search 173.2.131.81:60060 F?T?0?9?TTH:EO5QD5X5FQCJE6VPMFFHBULKZZGZQJXNCTQBKFQ");
            CallAndFree(id, W_OnParsedMsgSearch, params);
        }

        // Sample memory every 10,000 iterations
        if (i % 10000 == 0) {
            tracker.sample();
            std::cout << "Stress test iteration: " << i << " / " << iterations << std::endl;
        }
    }

    // Final memory sample
    tracker.sample();
    
    // Print comprehensive memory report
    tracker.print_report();

    w_Unload(id);
    EXPECT_TRUE(true);  // If no crash, test passes
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new GlobalPythonEnv());
    return RUN_ALL_TESTS();
}