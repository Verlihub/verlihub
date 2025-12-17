#!/usr/bin/env python3
"""
Test script for advanced type marshaling.
Tests list, dict, and complex data structures.
"""

def return_string_list():
    """Returns a list of strings to test 'L' format"""
    return ["user1", "user2", "user3", "admin", "moderator"]

def process_string_list(user_list):
    """Receives a list of strings, returns count"""
    if not isinstance(user_list, list):
        return -1
    return len(user_list)

def return_dict():
    """Returns a dictionary to test 'D' format (JSON)"""
    return {
        "total_users": 42,
        "active_users": 15,
        "hub_name": "TestHub",
        "version": "1.6.0.0",
        "uptime_seconds": 36000
    }

def process_dict(config):
    """Receives a dict, validates and returns status"""
    if not isinstance(config, dict):
        return {"status": "error", "message": "Not a dict"}
    
    required_keys = ["hub_name", "max_users"]
    missing = [k for k in required_keys if k not in config]
    
    if missing:
        return {
            "status": "incomplete",
            "missing_keys": missing
        }
    
    return {
        "status": "valid",
        "hub_name": config["hub_name"],
        "max_users": config["max_users"]
    }

def get_user_info(nick):
    """Returns complex user info as dict"""
    return {
        "nick": nick,
        "class": 3,
        "share_size": 1234567890,
        "slots": 10,
        "tags": ["OP", "TRUSTED"],
        "stats": {
            "messages": 500,
            "uptime": 7200
        }
    }

def get_multiple_values():
    """Returns tuple of mixed types"""
    return (42, "success", 3.14)

def complex_round_trip(data_dict):
    """Receives dict, modifies, returns modified dict"""
    if not isinstance(data_dict, dict):
        return {}
    
    # Add metadata
    data_dict["processed"] = True
    data_dict["processor"] = "complex_round_trip"
    
    # Modify counts if present
    if "count" in data_dict:
        data_dict["count"] += 1
    
    return data_dict

# Test counters
call_counts = {
    "return_string_list": 0,
    "process_string_list": 0,
    "return_dict": 0,
    "process_dict": 0,
    "get_user_info": 0,
    "complex_round_trip": 0
}

def increment_counter(func_name):
    if func_name in call_counts:
        call_counts[func_name] += 1

# Wrap functions to count calls
_original_return_string_list = return_string_list
def return_string_list():
    increment_counter("return_string_list")
    return _original_return_string_list()

_original_process_string_list = process_string_list
def process_string_list(user_list):
    increment_counter("process_string_list")
    return _original_process_string_list(user_list)

_original_return_dict = return_dict
def return_dict():
    increment_counter("return_dict")
    return _original_return_dict()

_original_process_dict = process_dict
def process_dict(config):
    increment_counter("process_dict")
    return _original_process_dict(config)

_original_get_user_info = get_user_info
def get_user_info(nick):
    increment_counter("get_user_info")
    return _original_get_user_info(nick)

_original_complex_round_trip = complex_round_trip
def complex_round_trip(data_dict):
    increment_counter("complex_round_trip")
    return _original_complex_round_trip(data_dict)

def get_test_stats():
    """Returns test statistics"""
    return {
        "total_calls": sum(call_counts.values()),
        "functions_tested": len([v for v in call_counts.values() if v > 0]),
        "details": call_counts
    }
