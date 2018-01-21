INSERT IGNORE INTO reglist (nick, class, reg_date, reg_op, pwd_change, note_op, auth_ip) VALUES
	('pinger', -1, UNIX_TIMESTAMP(NOW()), 'installation', 0, 'Generic pinger nick', NULL),
	('TEPinger', -1, UNIX_TIMESTAMP(NOW()), 'installation', 0, 'Team Elite Hublist', "185.97.254.203")
