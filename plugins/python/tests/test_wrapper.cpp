#include <gtest/gtest.h>
#include "wrapper.h"
#include <fstream>
#include <string>
#include <vector>

// Mock callbacks (empty functions for the callback table)
w_Targs* mock_callback(int id, w_Targs* args) { return nullptr; }
w_Tcallback mock_callbacks[W_MAX_CALLBACKS] = { mock_callback };

// Global test environment for Python init/finalize
class GlobalPythonEnv : public ::testing::Environment {
public:
    void SetUp() override {
        w_Begin(mock_callbacks);
    }
    void TearDown() override {
        w_End();
    }
};

class PythonWrapperTest : public ::testing::Test {
protected:
    std::string script_path;
    
    void SetUp() override {
        // Create a test script file in build directory
        script_path = std::string(BUILD_DIR) + "/test_script_" + std::to_string(getpid()) + ".py";
        std::ofstream script_file(script_path);
        script_file << "def OnTimer():\n"
                    << "    return 42\n"
                    << "def OnParsedMsgChat(user, data):\n"
                    << "    return 'Processed: ' + data\n";
        script_file.close();
    }

    void TearDown() override {
        // Remove test script
        std::remove(script_path.c_str());
    }
};

// Test dummy for init/end (handled globally)
TEST_F(PythonWrapperTest, InitAndEnd) {
    EXPECT_TRUE(true);
}

// Test script loading and unloading
TEST_F(PythonWrapperTest, LoadAndUnloadScript) {
    int id = w_ReserveID();
    EXPECT_GE(id, 0);

    // Mock args: id, path, botname, opchatname, config_dir, starttime, config_name
    w_Targs* args = w_pack("lssssls", id, script_path.c_str(), "TestBot", "OpChat", ".", (long)0, "config");
    EXPECT_EQ(id, w_Load(args));
    free(args);  // Use free(), not w_free_args() - w_Load() copies strings with strdup()

    // Check hooks
    EXPECT_TRUE(w_HasHook(id, W_OnTimer));
    EXPECT_TRUE(w_HasHook(id, W_OnParsedMsgChat));

    EXPECT_EQ(1, w_Unload(id));
}

// Test calling a hook
TEST_F(PythonWrapperTest, CallHook) {
    int id = w_ReserveID();
    w_Targs* args = w_pack("lssssls", id, script_path.c_str(), "TestBot", "OpChat", ".", (long)0, "config");
    w_Load(args);
    free(args);  // Use free(), not w_free_args() - w_Load() copies strings with strdup()

    // Call OnTimer (no params)
    w_Targs* params = w_pack("");
    w_Targs* res = w_CallHook(id, W_OnTimer, params);
    free(params);  // Use free() for simple params

    ASSERT_NE(res, nullptr);
    long ret_val;
    w_unpack(res, "l", &ret_val);
    EXPECT_EQ(42, ret_val);
    free(res);

    // Call OnParsedMsgChat (with params: user pointer, data string)
    void* mock_user = nullptr;  // Mock user
    params = w_pack("ps", mock_user, "hello");
    res = w_CallHook(id, W_OnParsedMsgChat, params);
    free(params);  // Use free() for simple params

    ASSERT_NE(res, nullptr);
    char* ret_str;
    w_unpack(res, "s", &ret_str);
    EXPECT_STREQ("Processed: hello", ret_str);
    free(ret_str);
    free(res);

    w_Unload(id);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new GlobalPythonEnv());
    return RUN_ALL_TESTS();
}