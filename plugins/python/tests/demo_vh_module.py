import vh

print(f"=== vh Module Demonstration ===")
print(f"Script ID: {vh.myid}")
print(f"Bot Name: {vh.botname}")
print(f"OpChat Name: {vh.opchatname}")
print()

# Event hook - C++ calls this when user enters chat
def OnParsedMsgChat(nick, message):
    print(f"[Hook] OnParsedMsgChat: {nick}: {message}")
    
    if message.startswith("!stats"):
        # Get user information
        user_class = vh.GetUserClass(nick)
        user_ip = vh.GetUserIP(nick)
        user_cc = vh.GetUserCC(nick)
        
        # Get hub information
        user_count = vh.GetUsersCount()
        hub_topic = vh.Topic()
        version = vh.name_and_version()
        
        # Send response using vh module
        response = (
            f"User Stats for {nick}:\n"
            f"Class: {user_class}\n"
            f"IP: {user_ip}\n"
            f"Country: {user_cc}\n"
            f"\nHub Stats:\n"
            f"Online Users: {user_count}\n"
            f"Topic: {hub_topic}\n"
            f"Server: {version}"
        )
        
        vh.pm(response, nick)
        return 1
    
    elif message.startswith("!users"):
        # Get list of users
        nicklist = vh.GetNickList()
        oplist = vh.GetOpList()
        
        response = (
            f"Total users: {len(nicklist)}\n"
            f"Operators: {len(oplist)}\n"
            f"First 5 users: {', '.join(nicklist[:5])}"
        )
        
        vh.pm(response, nick)
        return 1
    
    elif message.startswith("!config"):
        # Read hub configuration
        hub_name = vh.GetConfig("config", "hub_name")
        max_users = vh.GetConfig("config", "max_users_total")
        
        vh.pm(f"Hub: {hub_name}, Max users: {max_users}", nick)
        return 1
    
    return 1

# Event hook - C++ calls this when user logs in  
def OnUserLogin(nick):
    print(f"[Hook] OnUserLogin: {nick}")
    
    # Welcome message using vh module
    user_class = vh.GetUserClass(nick)
    
    if user_class >= 3:  # Operator
        vh.pm(f"Welcome back, operator {nick}!", nick)
        vh.SendToOpChat(f"{nick} has joined OpChat")
    else:
        topic = vh.Topic()
        vh.pm(f"Welcome to the hub! Topic: {topic}", nick)
    
    return 1

# Event hook - C++ calls this periodically
def OnTimer():
    # This demonstrates Python → C++ calls happening in background
    count = vh.GetUsersCount()
    if count > 100:
        vh.mc(f"Hub is busy: {count} users online!")
    return 1

print("Demo script loaded - vh module fully functional!")
print("Available functions:", dir(vh))
print()
