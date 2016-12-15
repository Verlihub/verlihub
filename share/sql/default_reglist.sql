INSERT IGNORE INTO reglist (nick, class, reg_date, reg_op, pwd_change, note_op) VALUES
	('pinger', -1, UNIX_TIMESTAMP(NOW()), 'installation', 0, 'Generic pinger nick'),
	('TEPinger', -1, UNIX_TIMESTAMP(NOW()), 'installation', 0, 'Team Elite Hublist')
