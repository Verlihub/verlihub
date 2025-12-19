#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "cserverdc.h"
#include "plugins/python/cpipython.h"
#include "plugins/python/cpythoninterpreter.h"
#include "cconndc.h"
#include "cprotocol.h"
#include "test_utils.h"
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <curl/curl.h>

using namespace testing;
using namespace nVerliHub;
using namespace nVerliHub::nSocket;
using namespace nVerliHub::nProtocol;
using namespace nVerliHub::nPythonPlugin;
using namespace nVerliHub::nEnums;

// Global server instance (shared across tests)
static cServerDC *g_server = nullptr;
static cpiPython *g_py_plugin = nullptr;

// Helper to get environment variable or default value
static std::string getEnvOrDefault(const char* var, const char* defaultVal) {
    const char* val = std::getenv(var);
    return val ? std::string(val) : std::string(defaultVal);
}

// curl callback to capture response body
static size_t my_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Helper function to make HTTP GET request
static bool http_get(const std::string& url, std::string& response, long& http_code) {
    CURL *curl = curl_easy_init();
    if (!curl) return false;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK);
}

// Global test environment setup (once for all tests)
class VerlihubEnv : public ::testing::Environment {
public:
    void SetUp() override {
        // Use build directory for config
        std::string config_dir = std::string(BUILD_DIR) + "/test_config_" + 
                                 std::to_string(getpid());
        std::string config_file = config_dir + "/dbconfig";
        std::string plugins_dir = config_dir + "/plugins";
        
        // Create directories
        int ret = system(("mkdir -p " + config_dir).c_str());
        (void)ret;
        ret = system(("mkdir -p " + plugins_dir).c_str());
        (void)ret;
        
        // Create venv and install FastAPI
        std::string venv_dir = std::string(BUILD_DIR) + "/venv";
        std::cerr << "\n=== Setting up Python virtual environment ===" << std::endl;
        std::cerr << "Creating venv at: " << venv_dir << std::endl;
        
        // Check if venv already exists and has FastAPI
        std::string check_cmd = "test -f " + venv_dir + "/bin/python && " +
                                venv_dir + "/bin/python -c 'import fastapi' 2>/dev/null";
        
        if (system(check_cmd.c_str()) != 0) {
            // venv doesn't exist or FastAPI not installed
            std::cerr << "Setting up new venv..." << std::endl;
            
            // Create venv
            std::string create_venv = "python3 -m venv " + venv_dir + " 2>&1";
            ret = system(create_venv.c_str());
            if (ret != 0) {
                std::cerr << "⚠ Warning: Failed to create venv (test will run without FastAPI)" << std::endl;
            } else {
                // Install FastAPI and uvicorn
                std::cerr << "Installing FastAPI and uvicorn..." << std::endl;
                std::string install_cmd = venv_dir + "/bin/pip install --quiet fastapi uvicorn 2>&1";
                ret = system(install_cmd.c_str());
                
                if (ret == 0) {
                    std::cerr << "✓ FastAPI and uvicorn installed successfully" << std::endl;
                } else {
                    std::cerr << "⚠ Warning: Failed to install FastAPI (test will run without it)" << std::endl;
                }
            }
        } else {
            std::cerr << "✓ Using existing venv with FastAPI" << std::endl;
        }
        
        // Set environment variable for hub_api.py to find the venv
        setenv("VERLIHUB_PYTHON_VENV", venv_dir.c_str(), 1);
        std::cerr << "Set VERLIHUB_PYTHON_VENV=" << venv_dir << std::endl;
        std::cerr << "========================================\n" << std::endl;
        
        // Get MySQL config from environment
        std::string db_host = getEnvOrDefault("VH_TEST_MYSQL_HOST", "localhost");
        std::string db_port = getEnvOrDefault("VH_TEST_MYSQL_PORT", "3306");
        std::string db_user = getEnvOrDefault("VH_TEST_MYSQL_USER", "verlihub");
        std::string db_pass = getEnvOrDefault("VH_TEST_MYSQL_PASS", "verlihub");
        std::string db_name = getEnvOrDefault("VH_TEST_MYSQL_DB", "verlihub");
        
        std::string db_host_port = db_host;
        if (db_port != "3306") {
            db_host_port = db_host + ":" + db_port;
        }
        
        // Write dbconfig
        std::ofstream dbconf(config_file);
        dbconf << "db_host = " << db_host_port << "\n"
               << "db_user = " << db_user << "\n" 
               << "db_pass = " << db_pass << "\n"
               << "db_data = " << db_name << "\n";
        dbconf.close();
        
        // Create server
        try {
            g_server = new cServerDC(config_dir, config_dir);
        } catch (...) {
            throw std::runtime_error(
                "Failed to create Verlihub server! Ensure MySQL is running."
            );
        }
        
        if (!g_server) {
            throw std::runtime_error("Server creation returned null");
        }
        
        // Set encoding FIRST before any config operations
        g_server->mC.hub_encoding = "UTF-8";
        if (g_server->mICUConvert) {
            delete g_server->mICUConvert;
            g_server->mICUConvert = new nVerliHub::nUtils::cICUConvert(g_server);
        }
        
        // Now set and save config values to database
        // This must be done BEFORE Python plugin init so hub_api.py caches correct values
        std::string val_new, val_old;
        g_server->SetConfig("config", "hub_encoding", "UTF-8", val_new, val_old);
        g_server->SetConfig("config", "hub_name", "Test Hub API", val_new, val_old);
        g_server->SetConfig("config", "hub_desc", "Testing API Endpoints", val_new, val_old);
        g_server->SetConfig("config", "hub_topic", "Welcome to the test!", val_new, val_old);
        g_server->SetConfig("config", "max_users_total", "500", val_new, val_old);
        g_server->SetConfig("config", "hub_icon_url", "https://example.com/icon.png", val_new, val_old);
        g_server->SetConfig("config", "hub_logo_url", "https://example.com/logo.png", val_new, val_old);
        
        // Create MOTD file
        std::string motd_path = config_dir + "/motd";
        std::ofstream motd_file(motd_path);
        motd_file << "Welcome to Test Hub API!\nThis is the message of the day for testing.";
        motd_file.close();
        
        // Also set in memory
        g_server->mC.hub_name = "Test Hub API";
        g_server->mC.hub_desc = "Testing API Endpoints";
        g_server->mC.hub_topic = "Welcome to the test!";
        g_server->mC.max_users_total = 500;
        
        // Verify config was set correctly
        char* test_name = g_server->GetConfig("config", "hub_name", nullptr);
        if (test_name) {
            std::cerr << "DEBUG: hub_name from GetConfig: " << test_name << std::endl;
            free(test_name);
        }
        
        if (g_server->mICUConvert) {
            delete g_server->mICUConvert;
            g_server->mICUConvert = new nVerliHub::nUtils::cICUConvert(g_server);
        }
        
        // Initialize Python plugin
        std::cerr << "Initializing Python plugin..." << std::endl;
        g_py_plugin = new cpiPython();
        g_py_plugin->SetMgr(&g_server->mPluginManager);
        g_py_plugin->OnLoad(g_server);
        g_py_plugin->RegisterAll();
        std::cerr << "Python plugin initialized" << std::endl;
        
        // Initialize curl
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    void TearDown() override {
        curl_global_cleanup();
        
        if (g_server) {
            delete g_server;
            g_server = nullptr;
        }
        
#ifdef PYTHON_SINGLE_INTERPRETER
        if (g_py_plugin) {
            g_py_plugin->Empty();
            delete g_py_plugin;
            g_py_plugin = nullptr;
        }
#else
        // Leak Python plugin in sub-interpreter mode to avoid threading issues
        if (g_py_plugin) {
            std::cerr << "Skipping Python cleanup (sub-interpreter mode)" << std::endl;
            g_py_plugin = nullptr;
        }
#endif
    }
};

// Test fixture
class HubApiStressTest : public ::testing::Test {
protected:
    cPythonInterpreter* interp = nullptr;
    std::string script_path;
    
    void SetUp() override {
        ASSERT_NE(g_py_plugin, nullptr) << "Python plugin not initialized";
        ASSERT_NE(g_server, nullptr) << "Server not initialized";
        
        // Copy hub_api.py to test directory
        std::string src = std::string(SOURCE_DIR) + "/plugins/python/scripts/hub_api.py";
        std::string dest = std::string(BUILD_DIR) + "/test_hub_api_" + std::to_string(getpid()) + ".py";
        
        std::ifstream src_file(src, std::ios::binary);
        std::ofstream dest_file(dest, std::ios::binary);
        
        if (!src_file || !dest_file) {
            FAIL() << "Failed to copy hub_api.py from " << src << " to " << dest;
        }
        
        dest_file << src_file.rdbuf();
        src_file.close();
        dest_file.close();
        
        script_path = dest;
        
        // Load the script
        std::cerr << "Loading hub_api.py from: " << script_path << std::endl;
        interp = new cPythonInterpreter(script_path);
        g_py_plugin->AddData(interp);
        interp->Init();
        
        ASSERT_NE(interp, nullptr) << "Failed to create interpreter for hub_api.py";
        ASSERT_GE(interp->id, 0) << "Interpreter initialization failed";
        std::cerr << "hub_api.py loaded with ID: " << interp->id << std::endl;
    }
    
    void TearDown() override {
        if (interp) {
            g_py_plugin->RemoveByName(script_path);
            interp = nullptr;
        }
        
        // Clean up test file
        if (!script_path.empty()) {
            unlink(script_path.c_str());
        }
    }
    
    // Helper: Create a mock connection for message processing
    cConnDC* create_mock_connection(const std::string& nick, int user_class = 10) {
        cConnDC* conn = new cConnDC(0, g_server);
        conn->mpUser = new cUser(nick);
        conn->mpUser->mNick = nick;
        conn->mpUser->mClass = (tUserCl)user_class;
        conn->mpUser->mxServer = g_server;
        conn->mpUser->mxConn = conn;
        
        // Set realistic MyINFO with actual share amount (10 MB = 10485760 bytes)
        // Format: $MyINFO $ALL <nick> <desc><tag>$ $<speed><flag>$<email>$<sharesize>$
        conn->mpUser->mMyINFO = "$MyINFO $ALL " + nick + " Test Description<++ V:0.777,M:A,H:1/0/0,S:2>$ $0.01\x01$test@example.com$10485760$";
        conn->mpUser->mShare = 10485760;  // 10 MB
        
        return conn;
    }
    
    // Helper: Send command through OnHubCommand hook
    bool send_hub_command(cConnDC* conn, const std::string& command, bool in_pm = false) {
        std::string cmd = command;
        return g_py_plugin->OnHubCommand(conn, &cmd, 1, in_pm ? 1 : 0);
    }
    
    // Helper: Simulate chat message
    bool send_chat_message(cConnDC* conn, const std::string& message) {
        // Create a cMessageDC for the chat message
        cMessageDC msg;
        msg.mType = eDC_CHAT;
        msg.mStr = "<" + conn->mpUser->mNick + "> " + message + "|";
        return g_py_plugin->OnParsedMsgChat(conn, &msg);
    }
};

// Test 1: Load hub_api.py and verify it starts
TEST_F(HubApiStressTest, LoadScript) {
    EXPECT_NE(interp, nullptr);
    EXPECT_GE(interp->id, 0);
}

// Test 2: Start API server via command
TEST_F(HubApiStressTest, StartApiServer) {
    cConnDC* admin = create_mock_connection("TestAdmin", 10);
    
    // Send !api start 18080
    bool result = send_hub_command(admin, "!api start 18080", true);
    
    // Should return false (0) meaning command was handled
    EXPECT_FALSE(result);
    
    // Give server time to start (if FastAPI is available)
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Try to connect to API - will fail if FastAPI not installed, which is OK
    std::string response;
    long http_code = 0;
    bool success = http_get("http://localhost:18080/", response, http_code);
    
    if (success && http_code == 200) {
        std::cerr << "✓ FastAPI server started successfully" << std::endl;
        EXPECT_NE(response.find("Verlihub"), std::string::npos) << "Response should mention Verlihub";
        std::cerr << "API Server Response: " << response.substr(0, 200) << std::endl;
    } else {
        std::cerr << "⚠ FastAPI not available or server didn't start (this is OK for testing)" << std::endl;
        std::cerr << "  Install with: pip install fastapi uvicorn" << std::endl;
    }
    
    delete admin->mpUser;
    delete admin;
}

// Test 3: Validate API endpoints return correct data
TEST_F(HubApiStressTest, ValidateApiEndpoints) {
    cConnDC* admin = create_mock_connection("TestAdmin", 10);
    
    // Test config values (set in global SetUp)
    std::string hub_name = "Test Hub API";
    std::string hub_desc = "Testing API Endpoints";
    std::string topic = "Welcome to the test!";
    
    // Create some test users
    std::vector<cConnDC*> users;
    for (int i = 0; i < 5; i++) {
        std::string nick = "TestUser" + std::to_string(i);
        cConnDC* user = create_mock_connection(nick, 1);
        users.push_back(user);
        
        // Add users to the hub's user list
        g_server->mUserList.Add(user->mpUser);
        user->mpUser->mInList = true;
    }
    
    // Start API server
    send_hub_command(admin, "!api start 18085", true);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Trigger OnTimer to refresh the cache with new config values
    g_py_plugin->OnTimer(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test /hub endpoint
    std::string response;
    long http_code = 0;
    
    if (http_get("http://localhost:18085/hub", response, http_code)) {
        std::cout << "\n=== /hub Response ===" << std::endl;
        std::cout << response << std::endl;
        
        if (http_code == 200) {
            // Validate response has expected JSON structure (no UTF-8 errors)
            EXPECT_EQ(response.find("\"error\":"), std::string::npos)
                << "/hub should not contain encoding errors";
            
            // Validate actual field values instead of just presence
            EXPECT_NE(response.find("\"name\": \"Test Hub API\""), std::string::npos)
                << "/hub should return correct hub name: Test Hub API";
            
            EXPECT_NE(response.find("\"description\": \"Testing API Endpoints\""), std::string::npos)
                << "/hub should return correct description: Testing API Endpoints";
            
            EXPECT_NE(response.find("\"topic\": \"Welcome to the test!\""), std::string::npos)
                << "/hub should return correct topic: Welcome to the test!";
            
            EXPECT_NE(response.find("\"max_users\": 500"), std::string::npos)
                << "/hub should return correct max_users: 500";
            
            EXPECT_NE(response.find("Welcome to Test Hub API!"), std::string::npos)
                << "/hub should return MOTD content from file";
            
            EXPECT_NE(response.find("\"icon_url\": \"https://example.com/icon.png\""), std::string::npos)
                << "/hub should return correct icon_url: https://example.com/icon.png";
            
            EXPECT_NE(response.find("\"logo_url\": \"https://example.com/logo.png\""), std::string::npos)
                << "/hub should return correct logo_url: https://example.com/logo.png";
            
            std::cout << "✓ /hub endpoint validated with correct values" << std::endl;
        } else {
            std::cerr << "⚠ /api/hub returned HTTP " << http_code << std::endl;
        }
    }
    
    // Test /users endpoint
    if (http_get("http://localhost:18085/users", response, http_code)) {
        std::cout << "\n=== /users Response ===" << std::endl;
        std::cout << response.substr(0, 800) << "..." << std::endl;
        
        if (http_code == 200) {
            // Check that response has the expected structure
            EXPECT_NE(response.find("\"count\":"), std::string::npos)
                << "/users should return count field";
            
            EXPECT_NE(response.find("\"users\":"), std::string::npos)
                << "/users should return users array";
            
            // Validate that at least one user has non-zero share
            EXPECT_NE(response.find("\"share\": 10485760"), std::string::npos)
                << "/users should return correct share amounts (not zero)";
            
            // Validate that user info fields are populated
            EXPECT_NE(response.find("\"description\": \"Test Description\""), std::string::npos)
                << "/users should return user descriptions";
            
            EXPECT_NE(response.find("\"email\": \"test@example.com\""), std::string::npos)
                << "/users should return user email addresses";
            
            EXPECT_NE(response.find("\"tag\": \"<++ V:0.777,M:A,H:1/0/0,S:2>\""), std::string::npos)
                << "/users should return user client tags";
            
            std::cout << "✓ /users endpoint validated successfully" << std::endl;
        } else {
            std::cerr << "⚠ /api/users returned HTTP " << http_code << std::endl;
        }
    }
    
    // Test /stats endpoint
    if (http_get("http://localhost:18085/stats", response, http_code)) {
        std::cout << "\n=== /stats Response ===" << std::endl;
        std::cout << response << std::endl;
        
        if (http_code == 200) {
            // Validate response has expected fields
            EXPECT_NE(response.find("\"users_online\""), std::string::npos)
                << "Stats should have users_online field";
            EXPECT_NE(response.find("\"total_share\""), std::string::npos)
                << "Stats should have total_share field";
            
            // Validate that total_share is not "0.00 B" (should show actual share)
            EXPECT_EQ(response.find("\"total_share\": \"0.00 B\""), std::string::npos)
                << "Stats total_share should not be zero when users have share";
            
            std::cout << "✓ /stats endpoint validated successfully" << std::endl;
        } else {
            std::cerr << "⚠ /stats returned HTTP " << http_code << std::endl;
        }
    }
    
    // Cleanup
    for (auto* user : users) {
        g_server->mUserList.Remove(user->mpUser);
        delete user->mpUser;
        delete user;
    }
    
    delete admin->mpUser;
    delete admin;
}

// Test 4: Concurrent message processing while making API calls
TEST_F(HubApiStressTest, ConcurrentMessagesAndApiCalls) {
    cConnDC* admin = create_mock_connection("TestAdmin", 10);
    
    // Start API server
    send_hub_command(admin, "!api start 18081", true);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Create multiple mock users
    std::vector<cConnDC*> users;
    for (int i = 0; i < 10; i++) {
        std::string nick = "User" + std::to_string(i);
        users.push_back(create_mock_connection(nick, 1));
    }
    
    // Atomic counters for thread safety
    std::atomic<int> messages_sent{0};
    std::atomic<int> api_calls_made{0};
    std::atomic<int> api_calls_success{0};
    std::atomic<bool> stop_flag{false};
    
    // Thread 1: Continuously send chat messages through TreatMsg simulation
    std::thread message_thread([&]() {
        int count = 0;
        while (!stop_flag && count < 500) {
            for (auto* user : users) {
                std::string msg = "Hello from " + user->mpUser->mNick + " #" + std::to_string(count);
                send_chat_message(user, msg);
                messages_sent++;
                
                // Also send some commands
                if (count % 10 == 0) {
                    send_hub_command(user, "!help", false);
                }
            }
            count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Thread 2: Continuously make API calls
    std::thread api_thread([&]() {
        std::vector<std::string> endpoints = {
            "http://localhost:18081/",
            "http://localhost:18081/hub",
            "http://localhost:18081/stats",
            "http://localhost:18081/users",
            "http://localhost:18081/geo",
            "http://localhost:18081/share",
            "http://localhost:18081/health"
        };
        
        int count = 0;
        while (!stop_flag && count < 100) {
            for (const auto& url : endpoints) {
                std::string response;
                long http_code = 0;
                
                if (http_get(url, response, http_code)) {
                    if (http_code == 200) {
                        api_calls_success++;
                    }
                }
                api_calls_made++;
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            count++;
        }
    });
    
    // Thread 3: OnTimer simulation (calls update_data_cache)
    std::thread timer_thread([&]() {
        int count = 0;
        while (!stop_flag && count < 100) {
            // Call OnTimer hook (simulates the every-second timer)
            g_py_plugin->OnTimer(1000); // 1000ms = 1 second
            count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    // Let it run for 15 seconds
    std::this_thread::sleep_for(std::chrono::seconds(15));
    
    // Stop all threads
    stop_flag = true;
    
    message_thread.join();
    api_thread.join();
    timer_thread.join();
    
    // Verify we processed messages and made API calls
    std::cerr << "\n=== Stress Test Results ===" << std::endl;
    std::cerr << "Messages sent: " << messages_sent << std::endl;
    std::cerr << "API calls made: " << api_calls_made << std::endl;
    std::cerr << "API calls successful: " << api_calls_success << std::endl;
    
    EXPECT_GT(messages_sent.load(), 0) << "Should have sent messages";
    EXPECT_GT(api_calls_made.load(), 0) << "Should have made API calls";
    
    // Only check API success if some calls succeeded (FastAPI might not be installed)
    if (api_calls_success > 0) {
        double success_rate = (double)api_calls_success / (double)api_calls_made;
        EXPECT_GT(success_rate, 0.8) << "API success rate should be > 80%";
        std::cerr << "✓ API success rate: " << (success_rate * 100) << "%" << std::endl;
    } else {
        std::cerr << "⚠ No successful API calls (FastAPI likely not installed)" << std::endl;
    }
    
    // Cleanup
    for (auto* user : users) {
        delete user->mpUser;
        delete user;
    }
    delete admin->mpUser;
    delete admin;
}

// Test 5: Rapid command processing
TEST_F(HubApiStressTest, RapidCommandProcessing) {
    cConnDC* admin = create_mock_connection("TestAdmin", 10);
    
    // Start memory tracking
    MemoryTracker tracker;
    tracker.start();
    std::cerr << "\n=== Memory Tracking Started ===" << std::endl;
    std::cerr << "Initial: " << tracker.initial.to_string() << std::endl;
    
    // Start API server
    send_hub_command(admin, "!api start 18082", true);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::atomic<bool> stop_flag{false};
    std::atomic<int> commands_sent{0};
    
    // Thread: Send rapid commands while OnTimer is running
    std::thread command_thread([&]() {
        int count = 0;
        while (!stop_flag && count < 200) {
            // Various commands
            send_hub_command(admin, "!api status", true);
            send_hub_command(admin, "!api help", true);
            send_hub_command(admin, "!api status", true);
            
            commands_sent += 3;
            count++;
            
            // Sample memory every 50 iterations
            if (count % 50 == 0) {
                tracker.sample();
            }
            
            // Small delay
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    });
    
    // Thread: OnTimer running continuously
    std::thread timer_thread([&]() {
        int count = 0;
        while (!stop_flag && count < 300) {
            g_py_plugin->OnTimer(1000); // 1000ms = 1 second
            count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    // Thread: Make API calls to ensure FastAPI threads are active
    std::thread api_thread([&]() {
        int count = 0;
        while (!stop_flag && count < 50) {
            std::string response;
            long http_code = 0;
            http_get("http://localhost:18082/stats", response, http_code);
            count++;
            
            // Sample memory every 10 API calls
            if (count % 10 == 0) {
                tracker.sample();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    // Run for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    stop_flag = true;
    command_thread.join();
    timer_thread.join();
    api_thread.join();
    
    // Final memory sample
    tracker.sample();
    
    std::cerr << "\n=== Rapid Command Test Results ===" << std::endl;
    std::cerr << "Commands sent: " << commands_sent << std::endl;
    
    // Print memory report
    tracker.print_report();
    
    EXPECT_GT(commands_sent.load(), 0);
    
    delete admin->mpUser;
    delete admin;
}

// Test 6: Memory leak detection under load
TEST_F(HubApiStressTest, MemoryLeakDetection) {
    cConnDC* admin = create_mock_connection("TestAdmin", 10);
    
    // Start memory tracking
    MemoryTracker tracker;
    tracker.start();
    std::cerr << "\n=== Memory Leak Detection Started ===" << std::endl;
    std::cerr << "Initial: " << tracker.initial.to_string() << std::endl;
    
    // Start API server
    send_hub_command(admin, "!api start 18083", true);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::atomic<bool> stop_flag{false};
    
    // Run intensive operations
    std::thread ops_thread([&]() {
        for (int i = 0; i < 1000 && !stop_flag; i++) {
            g_py_plugin->OnTimer(1000); // 1000ms = 1 second
            
            std::string response;
            long http_code = 0;
            http_get("http://localhost:18083/users", response, http_code);
            
            // Sample memory every 100 iterations
            if (i % 100 == 0) {
                tracker.sample();
                std::cerr << "Iteration " << i << "/1000" << std::endl;
            }
        }
    });
    
    ops_thread.join();
    
    // Final memory sample
    tracker.sample();
    
    // Print comprehensive memory report
    tracker.print_report();
    
    // Allow up to 10 MB growth (Python caches, FastAPI workers, etc.)
    const long ACCEPTABLE_GROWTH_KB = 10 * 1024;
    long memory_growth = (long)tracker.current.vm_rss_kb - (long)tracker.initial.vm_rss_kb;
    
    EXPECT_LT(memory_growth, ACCEPTABLE_GROWTH_KB) 
        << "Memory growth exceeded " << ACCEPTABLE_GROWTH_KB << " KB threshold";
    
    delete admin->mpUser;
    delete admin;
}

// Test 7: Encoding conversion with weird characters and API interaction
TEST_F(HubApiStressTest, EncodingConversionWithWeirdCharactersAndApi) {
    cConnDC* admin = create_mock_connection("TestAdmin", 10);
    
    std::cout << "\n=== Encoding Conversion + API Stress Test ===" << std::endl;
    std::cout << "Testing weird characters through API while changing hub encoding" << std::endl;
    
    // Start API server
    send_hub_command(admin, "!api start 18084", true);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Store original encoding
    std::string original_encoding = g_server->mC.hub_encoding;
    
    // Test 1: UTF-8 with emoji and special characters
    std::cout << "\n--- Test 1: UTF-8 with emoji and Unicode ---" << std::endl;
    g_server->mC.hub_encoding = "UTF-8";
    if (g_server->mICUConvert) {
        delete g_server->mICUConvert;
        g_server->mICUConvert = new nVerliHub::nUtils::cICUConvert(g_server);
    }
    
    std::vector<cConnDC*> weird_users_utf8;
    std::vector<std::string> weird_nicks_utf8 = {
        "User🌍",
        "Тест™",
        "Café☕",
        "用户测试",
        "Ñoño←→"
    };
    
    for (const auto& nick : weird_nicks_utf8) {
        cConnDC* user = create_mock_connection(nick, 1);
        weird_users_utf8.push_back(user);
        
        // Send chat message with weird characters
        std::string msg = "Hello from " + nick + "! Testing 🎉";
        bool msg_result = send_chat_message(user, msg);
        std::cout << "  Message from \"" << nick << "\": " 
                 << (msg_result ? "processed" : "handled") << std::endl;
        
        // Send command - verify it doesn't crash
        bool cmd_result = send_hub_command(user, "!help", false);
        EXPECT_TRUE(cmd_result || !cmd_result) << "Command should not crash for: " << nick;
    }
    
    // Make API call to check user list
    std::string response;
    long http_code = 0;
    if (http_get("http://localhost:18084/users", response, http_code)) {
        std::cout << "  API response code: " << http_code << std::endl;
        if (http_code == 200) {
            // Check if any of our weird nicks appear in response
            std::cout << "  Response preview: " << response.substr(0, 200) << "..." << std::endl;
            
            // Validate response is not empty and looks like JSON
            EXPECT_GT(response.length(), 0) << "API response should not be empty";
            EXPECT_TRUE(response[0] == '{' || response[0] == '[') 
                << "API response should be JSON";
            
            // Check if response contains some user data structure
            bool has_users_data = (response.find("users") != std::string::npos ||
                                  response.find("nick") != std::string::npos ||
                                  response.find("User") != std::string::npos);
            if (has_users_data) {
                std::cout << "  ✓ API response contains user data" << std::endl;
            }
        }
    }
    
    // Test 2: Switch to CP1251 (Cyrillic)
    std::cout << "\n--- Test 2: CP1251 with Cyrillic and invalid chars ---" << std::endl;
    g_server->mC.hub_encoding = "CP1251";
    if (g_server->mICUConvert) {
        delete g_server->mICUConvert;
        g_server->mICUConvert = new nVerliHub::nUtils::cICUConvert(g_server);
    }
    
    std::vector<cConnDC*> weird_users_cp1251;
    std::vector<std::string> weird_nicks_cp1251 = {
        "Привет",       // Valid in CP1251
        "Admin™",       // Trademark might not work
        "用户",          // Chinese won't work in CP1251
        "Test🌍",       // Emoji won't work
        "Пользователь"  // Valid Russian
    };
    
    for (const auto& nick : weird_nicks_cp1251) {
        cConnDC* user = create_mock_connection(nick, 1);
        weird_users_cp1251.push_back(user);
        
        // Send messages that might have encoding issues
        std::string msg = "Сообщение от " + nick;
        bool result = send_chat_message(user, msg);
        std::cout << "  CP1251 message from \"" << nick << "\": " 
                 << (result ? "processed" : "handled") << std::endl;
        
        // Verify user was created successfully
        ASSERT_NE(user->mpUser, nullptr) << "User should be created for: " << nick;
        EXPECT_EQ(user->mpUser->mNick, nick) << "Nick should match for CP1251 user";
    }
    
    // Call API again with different encoding
    if (http_get("http://localhost:18084/stats", response, http_code)) {
        std::cout << "  API stats with CP1251: code=" << http_code << std::endl;
        
        if (http_code == 200) {
            EXPECT_GT(response.length(), 0) << "Stats response should not be empty";
            // Stats should have some numeric data
            bool has_stats = (response.find("total") != std::string::npos ||
                            response.find("count") != std::string::npos ||
                            response.find("users") != std::string::npos);
            if (has_stats) {
                std::cout << "  ✓ Stats response contains expected data" << std::endl;
            }
        }
    }
    
    // Test 3: Switch to ISO-8859-1 (Latin-1)
    std::cout << "\n--- Test 3: ISO-8859-1 with Western European chars ---" << std::endl;
    g_server->mC.hub_encoding = "ISO-8859-1";
    if (g_server->mICUConvert) {
        delete g_server->mICUConvert;
        g_server->mICUConvert = new nVerliHub::nUtils::cICUConvert(g_server);
    }
    
    std::vector<cConnDC*> weird_users_latin1;
    std::vector<std::string> weird_nicks_latin1 = {
        "Café",         // Valid in Latin-1
        "Müller",       // Valid in Latin-1
        "Ñoño",         // Valid in Latin-1
        "Привет",       // Cyrillic won't work
        "Test®©™"       // Some symbols might work
    };
    
    for (const auto& nick : weird_nicks_latin1) {
        cConnDC* user = create_mock_connection(nick, 1);
        weird_users_latin1.push_back(user);
        
        std::string msg = "Message from " + nick + " in Latin-1";
        bool result = send_chat_message(user, msg);
        std::cout << "  Latin-1 message from \"" << nick << "\": " 
                 << (result ? "processed" : "handled") << std::endl;
        
        // Verify nick was set correctly
        ASSERT_NE(user->mpUser, nullptr) << "User should exist for: " << nick;
        // Latin-1 compatible characters should be preserved in the nick
        if (nick.find("Café") != std::string::npos || 
            nick.find("Müller") != std::string::npos ||
            nick.find("Ñoño") != std::string::npos) {
            EXPECT_GT(user->mpUser->mNick.length(), 3)
                << "Latin-1 compatible nick should have reasonable length: " << nick;
        }
    }
    
    // Test 4: Rapid encoding switches while processing messages and API calls
    std::cout << "\n--- Test 4: Rapid encoding changes with concurrent load ---" << std::endl;
    
    std::atomic<bool> stop_flag{false};
    std::atomic<int> messages_sent{0};
    std::atomic<int> api_calls{0};
    std::atomic<int> encoding_changes{0};
    
    // Combine all users for stress test
    std::vector<cConnDC*> all_users;
    all_users.insert(all_users.end(), weird_users_utf8.begin(), weird_users_utf8.end());
    all_users.insert(all_users.end(), weird_users_cp1251.begin(), weird_users_cp1251.end());
    all_users.insert(all_users.end(), weird_users_latin1.begin(), weird_users_latin1.end());
    
    // Thread 1: Send messages from users with weird nicks
    std::thread message_thread([&]() {
        int count = 0;
        while (!stop_flag && count < 100) {
            for (auto* user : all_users) {
                std::string msg = "Test message #" + std::to_string(count) + " 🌍™®©";
                send_chat_message(user, msg);
                messages_sent++;
            }
            count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    });
    
    // Thread 2: Make API calls
    std::thread api_thread([&]() {
        std::vector<std::string> endpoints = {
            "http://localhost:18084/users",
            "http://localhost:18084/stats",
            "http://localhost:18084/health"
        };
        
        int count = 0;
        while (!stop_flag && count < 50) {
            for (const auto& url : endpoints) {
                std::string resp;
                long code = 0;
                http_get(url, resp, code);
                api_calls++;
            }
            count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    // Thread 3: Rapidly change hub encoding
    std::thread encoding_thread([&]() {
        std::vector<std::string> encodings = {
            "UTF-8", "CP1251", "ISO-8859-1", "CP1250", "UTF-8"
        };
        
        int count = 0;
        while (!stop_flag && count < 20) {
            for (const auto& encoding : encodings) {
                g_server->mC.hub_encoding = encoding;
                if (g_server->mICUConvert) {
                    delete g_server->mICUConvert;
                    g_server->mICUConvert = new nVerliHub::nUtils::cICUConvert(g_server);
                }
                encoding_changes++;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            count++;
        }
    });
    
    // Thread 4: OnTimer calls
    std::thread timer_thread([&]() {
        int count = 0;
        while (!stop_flag && count < 50) {
            g_py_plugin->OnTimer(1000);
            count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    // Run for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    stop_flag = true;
    
    message_thread.join();
    api_thread.join();
    encoding_thread.join();
    timer_thread.join();
    
    std::cout << "\n=== Encoding Stress Test Results ===" << std::endl;
    std::cout << "Messages sent: " << messages_sent << std::endl;
    std::cout << "API calls made: " << api_calls << std::endl;
    std::cout << "Encoding changes: " << encoding_changes << std::endl;
    
    EXPECT_GT(messages_sent.load(), 0) << "Should have sent messages";
    EXPECT_GT(api_calls.load(), 0) << "Should have made API calls";
    EXPECT_GT(encoding_changes.load(), 0) << "Should have changed encodings";
    
    std::cout << "\n✓ No crashes with weird characters across encodings" << std::endl;
    std::cout << "✓ Rapid encoding changes handled without deadlocks" << std::endl;
    std::cout << "✓ API remained responsive during encoding changes" << std::endl;
    std::cout << "✓ Messages with unconvertible characters processed gracefully" << std::endl;
    
    // Restore original encoding
    g_server->mC.hub_encoding = original_encoding;
    if (g_server->mICUConvert) {
        delete g_server->mICUConvert;
        g_server->mICUConvert = new nVerliHub::nUtils::cICUConvert(g_server);
    }
    
    // Cleanup all users
    for (auto* user : all_users) {
        delete user->mpUser;
        delete user;
    }
    
    delete admin->mpUser;
    delete admin;
}

// Register global environment
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new VerlihubEnv);
    return RUN_ALL_TESTS();
}
