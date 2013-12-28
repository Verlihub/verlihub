INSERT IGNORE INTO pi_plug (nick,autoload,path,detail) VALUES
	("isp"    ,0,"/usr/local/lib/libisp_pi.so"   ,"Hub restrictions by ISP."),
	("lua"    ,0,"/usr/local/lib/liblua_pi.so"   ,"Support for Lua scripts."),
	("python"   ,0,"/usr/local/lib/libpython_pi.so"  ,"Support for Python scripts."),
	("msg"    ,0,"/usr/local/lib/libmessenger_pi.so","Offline messenger system."),
	("flood"  ,0,"/usr/local/lib/libfloodprot_pi.so","Advanced flood protection."),
	("log"    ,0,"/usr/local/lib/libiplog_pi.so","IP and nick logger with history."),
	("forbid" ,0,"/usr/local/lib/libforbid_pi.so","Filter chat from forbidden words."),
	("chat"   ,0,"/usr/local/lib/libchatroom_pi.so","Multiple chatrooms to separate chat topics."),
	("replace",0,"/usr/local/lib/libreplace_pi.so","Replace some words by other."),
	("stats"  ,0,"/usr/local/lib/libstats_pi.so","Hub statistics, trace diverse values in the database.")
