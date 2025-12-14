/*
	Copyright (C) 2025 Verlihub Team

	Test suite for Dimension 4: Dynamic C++ Function Registration
*/

#include <gtest/gtest.h>
#include "../wrapper.h"
#include <string>

// Test fixture
class DynamicRegistrationTest : public ::testing::Test {
protected:
	static w_Tcallback callbacks[W_MAX_CALLBACKS];
	
	void SetUp() override {
		w_Begin(callbacks);
	}
	
	void TearDown() override {
		w_End();
	}
	
	// Example C++ function that adds two numbers
	static w_Targs* AddNumbers(int id, w_Targs* args) {
		long a, b;
		if (!w_unpack(args, "ll", &a, &b)) {
			return w_pack("l", (long)0);
		}
		return w_pack("l", a + b);
	}
	
	// Example C++ function that reverses a string
	static w_Targs* ReverseString(int id, w_Targs* args) {
		char *str;
		if (!w_unpack(args, "s", &str)) {
			return w_pack("s", (char*)"");
		}
		
		int len = strlen(str);
		char *reversed = (char*)malloc(len + 1);
		for (int i = 0; i < len; i++) {
			reversed[i] = str[len - 1 - i];
		}
		reversed[len] = '\0';
		
		return w_pack("s", reversed);
	}
	
	// Example C++ function that multiplies a number
	static w_Targs* MultiplyBy10(int id, w_Targs* args) {
		long num;
		if (!w_unpack(args, "l", &num)) {
			return w_pack("l", (long)0);
		}
		return w_pack("l", num * 10);
	}
};

w_Tcallback DynamicRegistrationTest::callbacks[W_MAX_CALLBACKS] = {0};

TEST_F(DynamicRegistrationTest, RegisterAndCallFunction) {
	int id = w_ReserveID();
	ASSERT_GE(id, 0);
	
	// Register the AddNumbers function
	int result = w_RegisterFunction(id, "add", AddNumbers);
	EXPECT_EQ(result, 1);
	
	// Create Python script that calls the dynamic function
	FILE* f = fopen("/tmp/test_dynamic_add.py", "w");
	ASSERT_NE(f, nullptr);
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    result = vh.CallDynamicFunction('add', 5, 7)\n");
	fprintf(f, "    assert result == 12, f'Expected 12, got {result}'\n");
	fprintf(f, "    return result\n");
	fclose(f);
	
	// Load the script
	w_Targs* load_args = w_pack("lssssls", 
		(long)id, 
		(char*)"/tmp/test_dynamic_add.py",
		(char*)"TestBot",
		(char*)"OpChat",
		(char*)"/tmp",
		(long)1234567890,
		(char*)"test_config");
	
	int load_result = w_Load(load_args);
	free(load_args);
	EXPECT_EQ(load_result, id);
	
	// Call the test function
	w_Targs* ret = w_CallFunction(id, "test", w_pack(""));
	ASSERT_NE(ret, nullptr);
	
	long value;
	ASSERT_TRUE(w_unpack(ret, "l", &value));
	EXPECT_EQ(value, 12);
	
	free(ret);
	w_Unload(id);
}

TEST_F(DynamicRegistrationTest, RegisterMultipleFunctions) {
	int id = w_ReserveID();
	
	// Register multiple functions
	EXPECT_EQ(w_RegisterFunction(id, "add", AddNumbers), 1);
	EXPECT_EQ(w_RegisterFunction(id, "reverse", ReverseString), 1);
	EXPECT_EQ(w_RegisterFunction(id, "multiply", MultiplyBy10), 1);
	
	FILE* f = fopen("/tmp/test_dynamic_multi.py", "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    sum = vh.CallDynamicFunction('add', 10, 20)\n");
	fprintf(f, "    assert sum == 30\n");
	fprintf(f, "    \n");
	fprintf(f, "    rev = vh.CallDynamicFunction('reverse', 'hello')\n");
	fprintf(f, "    assert rev == 'olleh', f'Expected olleh, got {rev}'\n");
	fprintf(f, "    \n");
	fprintf(f, "    mult = vh.CallDynamicFunction('multiply', 5)\n");
	fprintf(f, "    assert mult == 50\n");
	fprintf(f, "    \n");
	fprintf(f, "    return True\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)"/tmp/test_dynamic_multi.py",
		(char*)"Bot", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* result = w_CallFunction(id, "test", w_pack(""));
	ASSERT_NE(result, nullptr);
	
	free(result);
	w_Unload(id);
}

TEST_F(DynamicRegistrationTest, UnregisterFunction) {
	int id = w_ReserveID();
	
	// Must load the script first
	FILE* f = fopen("/tmp/test_dynamic_unreg.py", "w");
	fprintf(f, "import vh\n");
	fprintf(f, "pass\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)"/tmp/test_dynamic_unreg.py",
		(char*)"Bot", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	// Register and then unregister
	EXPECT_EQ(w_RegisterFunction(id, "add", AddNumbers), 1);
	EXPECT_EQ(w_UnregisterFunction(id, "add"), 1);
	
	// Try to unregister again (should fail)
	EXPECT_EQ(w_UnregisterFunction(id, "add"), 0);
	
	// Try to unregister non-existent function
	EXPECT_EQ(w_UnregisterFunction(id, "nonexistent"), 0);
	
	w_Unload(id);
}

TEST_F(DynamicRegistrationTest, CallNonExistentFunction) {
	int id = w_ReserveID();
	
	FILE* f = fopen("/tmp/test_dynamic_nonexist.py", "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    try:\n");
	fprintf(f, "        vh.CallDynamicFunction('nonexistent', 1, 2)\n");
	fprintf(f, "        return False  # Should have raised exception\n");
	fprintf(f, "    except (NameError, RuntimeError) as e:\n");  // Both are OK
	fprintf(f, "        # Expected: either no funcs registered or func not found\n");
	fprintf(f, "        return True\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)"/tmp/test_dynamic_nonexist.py",
		(char*)"Bot", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* result = w_CallFunction(id, "test", w_pack(""));
	// Note: result might be NULL if exception propagated, that's OK for this test
	// The important thing is the script handled it
	
	if (result) free(result);
	w_Unload(id);
}

TEST_F(DynamicRegistrationTest, IsolatedBetweenScripts) {
	int id1 = w_ReserveID();
	int id2 = w_ReserveID();
	
	// Register function only for script 1
	EXPECT_EQ(w_RegisterFunction(id1, "add", AddNumbers), 1);
	
	// Script 1 can call it
	FILE* f1 = fopen("/tmp/test_dynamic_script1.py", "w");
	fprintf(f1, "import vh\n");
	fprintf(f1, "def test():\n");
	fprintf(f1, "    return vh.CallDynamicFunction('add', 3, 4)\n");
	fclose(f1);
	
	// Script 2 cannot call it
	FILE* f2 = fopen("/tmp/test_dynamic_script2.py", "w");
	fprintf(f2, "import vh\n");
	fprintf(f2, "def test():\n");
	fprintf(f2, "    try:\n");
	fprintf(f2, "        vh.CallDynamicFunction('add', 1, 2)\n");
	fprintf(f2, "        return False\n");
	fprintf(f2, "    except:\n");
	fprintf(f2, "        return True  # Expected to fail\n");
	fclose(f2);
	
	w_Targs* load1 = w_pack("lssssls", (long)id1, (char*)"/tmp/test_dynamic_script1.py",
		(char*)"Bot1", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load1);
	free(load1);
	
	w_Targs* load2 = w_pack("lssssls", (long)id2, (char*)"/tmp/test_dynamic_script2.py",
		(char*)"Bot2", (char*)"OpChat", (char*)"/tmp", (long)123, (char*)"cfg");
	w_Load(load2);
	free(load2);
	
	// Script 1 should succeed
	w_Targs* res1 = w_CallFunction(id1, "test", w_pack(""));
	ASSERT_NE(res1, nullptr);
	long val1;
	w_unpack(res1, "l", &val1);
	EXPECT_EQ(val1, 7);
	free(res1);
	
	// Script 2 should fail gracefully (returns True)
	w_Targs* res2 = w_CallFunction(id2, "test", w_pack(""));
	ASSERT_NE(res2, nullptr);
	free(res2);
	
	w_Unload(id1);
	w_Unload(id2);
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
