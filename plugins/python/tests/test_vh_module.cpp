/*
	Copyright (C) 2025 Verlihub Team

	Test suite for vh module - restored Python 3 bidirectional API
*/

#include <gtest/gtest.h>
#include "../wrapper.h"
#include <string>
#include <vector>

// Test fixture for vh module tests
class VHModuleTest : public ::testing::Test {
protected:
	static w_Tcallback callbacks[W_MAX_CALLBACKS];
	static std::vector<std::string> call_log;
	
	void SetUp() override {
		call_log.clear();
		w_Begin(callbacks);
	}
	
	void TearDown() override {
		w_End();
	}
	
	// Mock callback for testing
	static w_Targs* MockCallback_ReturnLong(int id, w_Targs* args) {
		call_log.push_back("MockCallback_ReturnLong");
		return w_pack("l", (long)42);
	}
	
	static w_Targs* MockCallback_ReturnString(int id, w_Targs* args) {
		call_log.push_back("MockCallback_ReturnString");
		return w_pack("s", (char*)"test_result");
	}
	
	static w_Targs* MockCallback_ReturnBool(int id, w_Targs* args) {
		call_log.push_back("MockCallback_ReturnBool");
		return w_pack("l", (long)1);
	}
	
	static w_Targs* MockCallback_ReturnStringList(int id, w_Targs* args) {
		call_log.push_back("MockCallback_ReturnStringList");
		static const char* list[] = {"user1", "user2", "user3", NULL};
		return w_pack("L", (char**)list);
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
	FILE* f = fopen("/tmp/test_vh_import.py", "w");
	ASSERT_NE(f, nullptr);
	fprintf(f, "import vh\n");
	fprintf(f, "print('vh module imported successfully')\n");
	fprintf(f, "print('vh.myid =', vh.myid)\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", 
		(long)id, 
		(char*)"/tmp/test_vh_import.py",
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
	FILE* f = fopen("/tmp/test_vh_getuserclass.py", "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    user_class = vh.GetUserClass('testuser')\n");
	fprintf(f, "    assert user_class == 42\n");
	fprintf(f, "    return user_class\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)"/tmp/test_vh_getuserclass.py",
		(char*)"Bot", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	// Call the test function
	w_Targs* result = w_CallFunction(id, "test", w_pack(""));
	ASSERT_NE(result, nullptr);
	
	long ret;
	ASSERT_TRUE(w_unpack(result, "l", &ret));
	EXPECT_EQ(ret, 42);
	EXPECT_EQ(call_log.size(), 1);
	EXPECT_EQ(call_log[0], "MockCallback_ReturnLong");
	
	free(result);
	w_Unload(id);
}

TEST_F(VHModuleTest, GetConfigReturnsString) {
	callbacks[W_GetConfig] = MockCallback_ReturnString;
	
	int id = w_ReserveID();
	FILE* f = fopen("/tmp/test_vh_getconfig.py", "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    value = vh.GetConfig('config', 'hub_name')\n");
	fprintf(f, "    assert value == 'test_result'\n");
	fprintf(f, "    return value\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)"/tmp/test_vh_getconfig.py",
		(char*)"Bot", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* result = w_CallFunction(id, "test", w_pack(""));
	ASSERT_NE(result, nullptr);
	
	char* ret;
	ASSERT_TRUE(w_unpack(result, "s", &ret));
	EXPECT_STREQ(ret, "test_result");
	EXPECT_EQ(call_log.size(), 1);
	
	free(result);
	w_Unload(id);
}

TEST_F(VHModuleTest, SetConfigReturnsBool) {
	callbacks[W_SetConfig] = MockCallback_ReturnBool;
	
	int id = w_ReserveID();
	FILE* f = fopen("/tmp/test_vh_setconfig.py", "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    result = vh.SetConfig('config', 'max_users', '100')\n");
	fprintf(f, "    assert result == True\n");
	fprintf(f, "    return result\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)"/tmp/test_vh_setconfig.py",
		(char*)"Bot", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* result = w_CallFunction(id, "test", w_pack(""));
	ASSERT_NE(result, nullptr);
	
	// Python True converts to PyObject*, check if callback was called
	EXPECT_EQ(call_log.size(), 1);
	EXPECT_EQ(call_log[0], "MockCallback_ReturnBool");
	
	free(result);
	w_Unload(id);
}

TEST_F(VHModuleTest, GetNickListReturnsPythonList) {
	callbacks[W_GetNickList] = MockCallback_ReturnStringList;
	
	int id = w_ReserveID();
	FILE* f = fopen("/tmp/test_vh_getnicklist.py", "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    nicklist = vh.GetNickList()\n");
	fprintf(f, "    assert isinstance(nicklist, list)\n");
	fprintf(f, "    assert len(nicklist) == 3\n");
	fprintf(f, "    assert nicklist[0] == 'user1'\n");
	fprintf(f, "    assert nicklist[1] == 'user2'\n");
	fprintf(f, "    assert nicklist[2] == 'user3'\n");
	fprintf(f, "    return len(nicklist)\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)"/tmp/test_vh_getnicklist.py",
		(char*)"Bot", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* result = w_CallFunction(id, "test", w_pack(""));
	ASSERT_NE(result, nullptr);
	
	long count;
	ASSERT_TRUE(w_unpack(result, "l", &count));
	EXPECT_EQ(count, 3);
	EXPECT_EQ(call_log.size(), 1);
	
	free(result);
	w_Unload(id);
}

TEST_F(VHModuleTest, ModuleHasConstants) {
	int id = w_ReserveID();
	FILE* f = fopen("/tmp/test_vh_constants.py", "w");
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
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)"/tmp/test_vh_constants.py",
		(char*)"TestBot", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* result = w_CallFunction(id, "test", w_pack(""));
	ASSERT_NE(result, nullptr);
	
	free(result);
	w_Unload(id);
}

TEST_F(VHModuleTest, MultipleScriptsHaveIsolatedModules) {
	int id1 = w_ReserveID();
	int id2 = w_ReserveID();
	
	// Script 1
	FILE* f = fopen("/tmp/test_vh_script1.py", "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def get_my_id():\n");
	fprintf(f, "    return vh.myid\n");
	fprintf(f, "def get_botname():\n");
	fprintf(f, "    return vh.botname\n");
	fclose(f);
	
	// Script 2
	f = fopen("/tmp/test_vh_script2.py", "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def get_my_id():\n");
	fprintf(f, "    return vh.myid\n");
	fprintf(f, "def get_botname():\n");
	fprintf(f, "    return vh.botname\n");
	fclose(f);
	
	w_Targs* load1 = w_pack("lssssls", (long)id1, (char*)"/tmp/test_vh_script1.py",
		(char*)"Bot1", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load1);
	free(load1);
	
	w_Targs* load2 = w_pack("lssssls", (long)id2, (char*)"/tmp/test_vh_script2.py",
		(char*)"Bot2", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load2);
	free(load2);
	
	// Each script should see its own ID and botname
	w_Targs* res1 = w_CallFunction(id1, "get_my_id", w_pack(""));
	long script1_id;
	w_unpack(res1, "l", &script1_id);
	EXPECT_EQ(script1_id, id1);
	free(res1);
	
	w_Targs* res2 = w_CallFunction(id2, "get_my_id", w_pack(""));
	long script2_id;
	w_unpack(res2, "l", &script2_id);
	EXPECT_EQ(script2_id, id2);
	free(res2);
	
	w_Targs* name1 = w_CallFunction(id1, "get_botname", w_pack(""));
	char* botname1;
	w_unpack(name1, "s", &botname1);
	EXPECT_STREQ(botname1, "Bot1");
	free(name1);
	
	w_Targs* name2 = w_CallFunction(id2, "get_botname", w_pack(""));
	char* botname2;
	w_unpack(name2, "s", &botname2);
	EXPECT_STREQ(botname2, "Bot2");
	free(name2);
	
	w_Unload(id1);
	w_Unload(id2);
}

TEST_F(VHModuleTest, AllFunctionsExist) {
	int id = w_ReserveID();
	FILE* f = fopen("/tmp/test_vh_all_functions.py", "w");
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
	fprintf(f, "        'name_and_version', 'StopHub'\n");
	fprintf(f, "    ]\n");
	fprintf(f, "    missing = [f for f in functions if not hasattr(vh, f)]\n");
	fprintf(f, "    if missing:\n");
	fprintf(f, "        raise AssertionError(f'Missing functions: {missing}')\n");
	fprintf(f, "    return len(functions)\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)"/tmp/test_vh_all_functions.py",
		(char*)"Bot", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* result = w_CallFunction(id, "test", w_pack(""));
	ASSERT_NE(result, nullptr);
	
	long count;
	ASSERT_TRUE(w_unpack(result, "l", &count));
	EXPECT_EQ(count, 56);  // All 56 functions exist (53 original + 3 list-returning variants)
	
	free(result);
	w_Unload(id);
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
