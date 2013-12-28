INSERT IGNORE INTO reglist (nick, class, reg_date, reg_op, pwd_change, note_op) VALUES
	('pinger', -1, UNIX_TIMESTAMP(NOW()), 'installation', 0, 'Generic pinger nick'),
	('dchublist', -1, UNIX_TIMESTAMP(NOW()), 'installation', 0, 'DCHublist.com'),
	('TEPinger', -1, UNIX_TIMESTAMP(NOW()), 'installation', 0, 'Team Elite Hublist'),
	('PublicHublist', -1, UNIX_TIMESTAMP(NOW()), 'installation', 0, 'PublicHublist'),
	('dchublist_ru', -1, UNIX_TIMESTAMP(NOW()), 'installation', 0, 'DCHublist.ru')
