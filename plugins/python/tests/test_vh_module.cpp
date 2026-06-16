/*
	Copyright (C) 2025 Verlihub Team

	Test suite for vh module - restored Python 3 bidirectional API
*/

#include <gtest/gtest.h>
#include "../wrapper.h"
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>

// Helper function to generate unique test file paths in build directory
static std::string GetTestFilePath(const char* base_name) {
	std::ostringstream oss;
	oss << BUILD_DIR << "/test_vh_" << base_name << "_" << getpid() << "_" << ::testing::UnitTest::GetInstance()->current_test_info()->name() << ".py";
	return oss.str();
}

// Test fixture for vh module tests
class VHModuleTest : public ::testing::Test {
protected:
	static w_Tcallback callbacks[W_MAX_CALLBACKS];
	static std::vector<std::string> call_log;
	std::vector<std::string> temp_files;  // Track files to clean up
	
	void SetUp() override {
		call_log.clear();
		w_Begin(callbacks);
	}
	
	void TearDown() override {
		w_End();
		// Clean up temporary files
		for (const auto& file : temp_files) {
			unlink(file.c_str());
		}
	}
	
	// Helper to create temp file and track it
	std::string CreateTempFile(const char* base_name) {
		std::string path = GetTestFilePath(base_name);
		temp_files.push_back(path);
		return path;
	}
	
	// Mock callback for testing
	static w_Targs* MockCallback_ReturnLong(int id, w_Targs* args) {
		call_log.push_back("MockCallback_ReturnLong");
		return w_pack("l", (long)42);
	}
	
	static w_Targs* MockCallback_ReturnString(int id, w_Targs* args) {
		call_log.push_back("MockCallback_ReturnString");
		return w_pack("s", strdup("test_result"));
	}
	
	static w_Targs* MockCallback_ReturnBool(int id, w_Targs* args) {
		call_log.push_back("MockCallback_ReturnBool");
		return w_pack("l", (long)1);
	}
	
	static w_Targs* MockCallback_ReturnStringList(int id, w_Targs* args) {
		fprintf(stderr, "MockCallback_ReturnStringList called! id=%d\n", id);
		call_log.push_back("MockCallback_ReturnStringList");
		// vh.GetNickList() expects JSON, not a string list
		w_Targs* result = w_pack("D", strdup("[\"user1\", \"user2\", \"user3\"]"));
		fprintf(stderr, "MockCallback_ReturnStringList returning result=%p\n", result);
		return result;
	}
	
	static w_Targs* MockCallback_EchoArgs(int id, w_Targs* args) {
		call_log.push_back("MockCallback_EchoArgs");
		// Just return the args back for verification
		return args;
	}
};

w_Tcallback VHModuleTest::callbacks[W_MAX_CALLBACKS] = {0};
std::vector<std::string> VHModuleTest::call_log;

TEST_F(VHModuleTest, ModuleImportable) {
	int id = w_ReserveID();
	ASSERT_GE(id, 0);
	
	// Create a minimal Python script that imports vh
	std::string script_path = CreateTempFile("import");
	FILE* f = fopen(script_path.c_str(), "w");
	ASSERT_NE(f, nullptr);
	fprintf(f, "import vh\n");
	fprintf(f, "print('vh module imported successfully')\n");
	fprintf(f, "print('vh.myid =', vh.myid)\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", 
		(long)id, 
		(char*)script_path.c_str(),
		(char*)"TestBot",
		(char*)"OpChat",
		(char*)"/tmp",
		(long)1234567890,
		(char*)"test_config");
	
	int result = w_Load(load_args);
	free(load_args);
	
	EXPECT_EQ(result, id);
	
	w_Unload(id);
}

TEST_F(VHModuleTest, GetUserClassReturnsLong) {
	callbacks[W_GetUserClass] = MockCallback_ReturnLong;
	
	int id = w_ReserveID();
	std::string script_path = CreateTempFile("getuserclass");
	FILE* f = fopen(script_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    user_class = vh.GetUserClass('testuser')\n");
	fprintf(f, "    assert user_class == 42\n");
	fprintf(f, "    return user_class\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	// Call the test function
	w_Targs* params = w_pack(""); w_Targs* result = w_CallFunction(id, "test", params); free(params);
	ASSERT_NE(result, nullptr);
	
	long ret;
	ASSERT_TRUE(w_unpack(result, "l", &ret));
	EXPECT_EQ(ret, 42);
	EXPECT_EQ(call_log.size(), 1);
	EXPECT_EQ(call_log[0], "MockCallback_ReturnLong");
	
	w_free_args(result);
	w_Unload(id);
}

TEST_F(VHModuleTest, GetConfigReturnsString) {
	callbacks[W_GetConfig] = MockCallback_ReturnString;
	
	int id = w_ReserveID();
	std::string script_path = CreateTempFile("getconfig");
	FILE* f = fopen(script_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    value = vh.GetConfig('config', 'hub_name')\n");
	fprintf(f, "    assert value == 'test_result'\n");
	fprintf(f, "    return value\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* params = w_pack(""); w_Targs* result = w_CallFunction(id, "test", params); free(params);
	ASSERT_NE(result, nullptr);
	
	char* ret;
	ASSERT_TRUE(w_unpack(result, "s", &ret));
	EXPECT_STREQ(ret, "test_result");
	EXPECT_EQ(call_log.size(), 1);
	
	w_free_args(result);
	w_Unload(id);
}

TEST_F(VHModuleTest, SetConfigReturnsBool) {
	callbacks[W_SetConfig] = MockCallback_ReturnBool;
	
	int id = w_ReserveID();
	std::string script_path = CreateTempFile("setconfig");
	FILE* f = fopen(script_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    result = vh.SetConfig('config', 'max_users', '100')\n");
	fprintf(f, "    assert result == True\n");
	fprintf(f, "    return result\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* params = w_pack(""); w_Targs* result = w_CallFunction(id, "test", params); free(params);
	ASSERT_NE(result, nullptr);
	
	// Python True converts to PyObject*, check if callback was called
	EXPECT_EQ(call_log.size(), 1);
	EXPECT_EQ(call_log[0], "MockCallback_ReturnBool");
	
	w_free_args(result);
	w_Unload(id);
}

TEST_F(VHModuleTest, GetNickListReturnsPythonList) {
	callbacks[W_GetNickList] = MockCallback_ReturnStringList;
	
	int id = w_ReserveID();
	std::string script_path = CreateTempFile("getnicklist");
	FILE* f = fopen(script_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    nicklist = vh.GetNickList()\n");
	fprintf(f, "    print(f'DEBUG: nicklist type={type(nicklist)}, len={len(nicklist)}, value={nicklist}')\n");
	fprintf(f, "    assert isinstance(nicklist, list), f'Expected list, got {type(nicklist)}'\n");
	fprintf(f, "    assert len(nicklist) == 3, f'Expected length 3, got {len(nicklist)}'\n");
	fprintf(f, "    assert nicklist[0] == 'user1'\n");
	fprintf(f, "    assert nicklist[1] == 'user2'\n");
	fprintf(f, "    assert nicklist[2] == 'user3'\n");
	fprintf(f, "    return len(nicklist)\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* params = w_pack(""); w_Targs* result = w_CallFunction(id, "test", params); free(params);
	ASSERT_NE(result, nullptr);
	
	long count;
	ASSERT_TRUE(w_unpack(result, "l", &count));
	EXPECT_EQ(count, 3);
	std::cout << "call_log.size()=" << call_log.size() << std::endl;
	for (size_t i = 0; i < call_log.size(); ++i) {
		std::cout << "  call_log[" << i << "]=" << call_log[i] << std::endl;
	}
	EXPECT_EQ(call_log.size(), 1);
	
	w_free_args(result);
	w_Unload(id);
}

TEST_F(VHModuleTest, ModuleHasConstants) {
	int id = w_ReserveID();
	std::string script_path = CreateTempFile("constants");
	FILE* f = fopen(script_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    # Test connection close reason constants\n");
	fprintf(f, "    assert hasattr(vh, 'eCR_KICKED')\n");
	fprintf(f, "    assert hasattr(vh, 'eCR_TIMEOUT')\n");
	fprintf(f, "    # Test bot constants\n");
	fprintf(f, "    assert hasattr(vh, 'eBOT_OK')\n");
	fprintf(f, "    # Test ban constants\n");
	fprintf(f, "    assert hasattr(vh, 'eBF_NICKIP')\n");
	fprintf(f, "    # Test script attributes\n");
	fprintf(f, "    assert hasattr(vh, 'myid')\n");
	fprintf(f, "    assert hasattr(vh, 'botname')\n");
	fprintf(f, "    assert vh.myid >= 0\n");  // Just check it's valid
	fprintf(f, "    assert vh.botname == 'TestBot'\n");
	fprintf(f, "    return True\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"TestBot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* params = w_pack(""); w_Targs* result = w_CallFunction(id, "test", params); free(params);
	ASSERT_NE(result, nullptr);
	
	w_free_args(result);
	w_Unload(id);
}

#ifndef PYTHON_SINGLE_INTERPRETER
// This test expects script isolation, which is incompatible with single-interpreter mode
// In single-interpreter mode, all scripts share the same vh module instance
TEST_F(VHModuleTest, MultipleScriptsHaveIsolatedModules) {
	int id1 = w_ReserveID();
	int id2 = w_ReserveID();
	
	// Script 1
	std::string script1_path = CreateTempFile("script1");
	FILE* f = fopen(script1_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def get_my_id():\n");
	fprintf(f, "    return vh.myid\n");
	fprintf(f, "def get_botname():\n");
	fprintf(f, "    return vh.botname\n");
	fclose(f);
	
	// Script 2
	std::string script2_path = CreateTempFile("script2");
	f = fopen(script2_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def get_my_id():\n");
	fprintf(f, "    return vh.myid\n");
	fprintf(f, "def get_botname():\n");
	fprintf(f, "    return vh.botname\n");
	fclose(f);
	
	w_Targs* load1 = w_pack("lssssls", (long)id1, (char*)script1_path.c_str(),
		(char*)"Bot1", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load1);
	free(load1);
	
	w_Targs* load2 = w_pack("lssssls", (long)id2, (char*)script2_path.c_str(),
		(char*)"Bot2", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load2);
	free(load2);
	
	// Each script should see its own ID and botname
	w_Targs* p1 = w_pack(""); w_Targs* res1 = w_CallFunction(id1, "get_my_id", p1); free(p1);
	long script1_id;
	w_unpack(res1, "l", &script1_id);
	EXPECT_EQ(script1_id, id1);
	free(res1);
	
	w_Targs* p2 = w_pack(""); w_Targs* res2 = w_CallFunction(id2, "get_my_id", p2); free(p2);
	long script2_id;
	w_unpack(res2, "l", &script2_id);
	EXPECT_EQ(script2_id, id2);
	free(res2);
	
	w_Targs* p3 = w_pack(""); w_Targs* name1 = w_CallFunction(id1, "get_botname", p3); free(p3);
	char* botname1;
	w_unpack(name1, "s", &botname1);
	EXPECT_STREQ(botname1, "Bot1");
	free(name1);
	
	w_Targs* p4 = w_pack(""); w_Targs* name2 = w_CallFunction(id2, "get_botname", p4); free(p4);
	char* botname2;
	w_unpack(name2, "s", &botname2);
	EXPECT_STREQ(botname2, "Bot2");
	free(name2);
	
	w_Unload(id1);
	w_Unload(id2);
}
#endif // PYTHON_SINGLE_INTERPRETER

TEST_F(VHModuleTest, AllFunctionsExist) {
	int id = w_ReserveID();
	std::string script_path = CreateTempFile("all_functions");
	FILE* f = fopen(script_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    functions = [\n");
	fprintf(f, "        'SendToOpChat', 'SendToActive', 'SendToPassive',\n");
	fprintf(f, "        'SendToActiveClass', 'SendToPassiveClass', 'SendDataToUser',\n");
	fprintf(f, "        'SendDataToAll', 'SendPMToAll', 'CloseConnection',\n");
	fprintf(f, "        'GetMyINFO', 'SetMyINFO', 'GetUserClass',\n");
	fprintf(f, "        'GetNickList', 'GetOpList', 'GetBotList',\n");
	fprintf(f, "        'GetRawNickList', 'GetRawOpList', 'GetRawBotList',\n");
	fprintf(f, "        'GetUserHost', 'GetUserIP', 'SetUserIP',\n");
	fprintf(f, "        'SetMyINFOFlag', 'UnsetMyINFOFlag', 'GetUserHubURL',\n");
	fprintf(f, "        'GetUserExtJSON', 'GetUserCC', 'GetIPCC', 'GetIPCN',\n");
	fprintf(f, "        'GetIPASN', 'GetGeoIP', 'AddRegUser', 'DelRegUser',\n");
	fprintf(f, "        'SetRegClass', 'Ban', 'KickUser', 'DelNickTempBan',\n");
	fprintf(f, "        'DelIPTempBan', 'ParseCommand', 'ScriptCommand',\n");
	fprintf(f, "        'SetConfig', 'GetConfig', 'IsRobotNickBad',\n");
	fprintf(f, "        'AddRobot', 'DelRobot', 'SQL', 'GetServFreq',\n");
	fprintf(f, "        'GetUsersCount', 'GetTotalShareSize', 'usermc', 'pm',\n");
	fprintf(f, "        'mc', 'classmc', 'UserRestrictions', 'Topic',\n");
	fprintf(f, "        'name_and_version', 'StopHub', 'Encode', 'Decode'\n");
	fprintf(f, "    ]\n");
	fprintf(f, "    missing = [f for f in functions if not hasattr(vh, f)]\n");
	fprintf(f, "    if missing:\n");
	fprintf(f, "        raise AssertionError(f'Missing functions: {missing}')\n");
	fprintf(f, "    return len(functions)\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* params = w_pack(""); w_Targs* result = w_CallFunction(id, "test", params); free(params);
	ASSERT_NE(result, nullptr);
	
	long count;
	ASSERT_TRUE(w_unpack(result, "l", &count));
	EXPECT_EQ(count, 58);  // All 58 functions exist (53 original + 3 list variants + 2 encode/decode)
	
	w_free_args(result);
	w_Unload(id);
}

TEST_F(VHModuleTest, EncodeDecodeWorks) {
	int id = w_ReserveID();
	std::string script_path = CreateTempFile("encode_decode");
	FILE* f = fopen(script_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    # Test encoding special DC++ characters\n");
	fprintf(f, "    original = 'Hello$World|Test`More~End'\n");
	fprintf(f, "    encoded = vh.Encode(original)\n");
	fprintf(f, "    assert '&#36;' in encoded  # $\n");
	fprintf(f, "    assert '&#124;' in encoded # |\n");
	fprintf(f, "    assert '&#96;' in encoded  # `\n");
	fprintf(f, "    assert '&#126;' in encoded # ~\n");
	fprintf(f, "    \n");
	fprintf(f, "    # Test decoding back\n");
	fprintf(f, "    decoded = vh.Decode(encoded)\n");
	fprintf(f, "    assert decoded == original\n");
	fprintf(f, "    \n");
	fprintf(f, "    # Test roundtrip with control char\n");
	fprintf(f, "    test_str = 'A\\x05B'  # chr(5)\n");
	fprintf(f, "    enc = vh.Encode(test_str)\n");
	fprintf(f, "    assert '&#5;' in enc\n");
	fprintf(f, "    dec = vh.Decode(enc)\n");
	fprintf(f, "    assert dec == test_str\n");
	fprintf(f, "    \n");
	fprintf(f, "    return True\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* params = w_pack(""); w_Targs* result = w_CallFunction(id, "test", params); free(params);
	ASSERT_NE(result, nullptr);
	
	w_free_args(result);
	w_Unload(id);
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
