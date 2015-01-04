package vh;

use 5.008;
use strict;
use warnings;
use Carp;

require Exporter;
use AutoLoader;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use vh ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	Ban
	CloseConnection
	GetUserCC
	GetMyINFO
	AddRegUser
	DelRegUser
	RegBot
	UnRegBot
	AddRobot
	DelRobot
	EditBot
	GetUserClass
	GetUserHost
	GetUserIP
	ParseCommand
	SendToClass
	SendToAll
	SendPMToAll
	KickUser
	SendDataToUser
	SetConfig
	GetConfig
	GetUsersCount
	GetNickList
	GetNickListArray
	GetOPList
	GetOPListArray
	GetBotList
	GetBotListArray
	GetBots
	GetTotalShareSize
	GetVHCfgDir

	SendToUser
	SendDataToAll
	Disconnect
	DisconnectByName

	VHDBConnect
	SQLQuery
	SQLFetch
	SQLFree

	GetUpTime
	IsBot
	IsUserOnline
	GetHubIp
	GetHubSecAlias
	StopHub
	GetTopic
	SetTopic
	GetIPCC
	GetIPCN
	InUserSupports
	ReportUser
	SendToOpChat
	ScriptCommand
	GetTempRights
	SetTempRights
) ] );

our @EXPORT_OK = ( "VH__Call__Function", @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
);

our $VERSION = '0.01';

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.

    my $constname;
    our $AUTOLOAD;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    croak "&vh::constant not defined" if $constname eq 'constant';
    my ($error, $val) = constant($constname);
    if ($error) { croak $error; }
    {
	no strict 'refs';
	# Fixed between 5.005_53 and 5.005_61
#XXX	if ($] >= 5.00561) {
#XXX	    *$AUTOLOAD = sub () { $val };
#XXX	}
#XXX	else {
	    *$AUTOLOAD = sub { $val };
#XXX	}
    }
    goto &$AUTOLOAD;
}

require XSLoader;
XSLoader::load('vh', $VERSION);

our $DBI_FOUND = 0;
eval 'use DBI; $DBI_FOUND=1;';

# Preloaded methods go here.

sub SendToUser {
  return SendDataToUser(@_);
}

sub SendDataToAll {
  return SendToClass(@_);
}

sub Disconnect {
  return CloseConnection(@_);
}

sub DisconnectByName {
  return CloseConnection(@_);
}

sub GetBots {
  carp "PerlScript plugin doesn't support GetBots!";
}

sub GetTempRights {
  carp "PerlScript plugin doesn't support GetTempRights currenlty";
}

sub SetTempRights {
  carp "PerlScript plugin doesn't support SetTempRights currenlty";
}

sub AddRobot {
  return RegBot(@_);
}

sub DelRobot {
  return UnRegBot(@_);
}

sub GetNickListArray {
  my $nl = GetNickList();
  $nl =~ s#^\$NickList\s+##;
  $nl =~ s#\$\$$##;
  my @nl = split /\$\$/, $nl;
  return @nl;
}

sub GetOPListArray {
  my $nl = GetOPList();
  $nl =~ s#^\$OpList\s+##;
  $nl =~ s#\$\$$##;
  my @nl = split /\$\$/, $nl;
  return @nl;
}

sub GetBotListArray {
  my $nl = GetBotList();
  $nl =~ s#^\$BotList\s+##;
  $nl =~ s#\$\$$##;
  my @nl = split /\$\$/, $nl;
  return @nl;
}

sub VHDBConnect {
  if (!$DBI_FOUND) {
    carp "No DBI found in your system. Install perl modules 'DBI' and 'DBD::mysql'";
    return;
  }

  use AppConfig;
  my $config = AppConfig->new({ERROR=>sub{}});

  for my $k(qw(db_host db_user db_pass db_data db_charset)) {
    $config->define("$k=s");
  }

  $config->file(vh::GetVHCfgDir()."/dbconfig");

  my $dsn = "DBI:mysql:database=".$config->db_data;
  if ($config->db_host) { $dsn.=";hostname=".$config->db_host; }
  my $dbh = DBI->connect($dsn, $config->db_user, $config->db_pass);
  $dbh->{'mysql_auto_reconnect'} = 1;
  if($config->db_charset) { $dbh->do("SET NAMES ".$config->db_charset); }

  return $dbh;
}

sub VH__Call__Function {
  my $func = shift;
  my $ref;
  my $ret;
  if (!defined &{"main::$func"}) {
    return 1;
  }
  no strict 'refs';
  return &{"main::$func"}(@_);
  use strict 'refs';
  return $ret;
}

BEGIN {
  require vh::const;
  vh::const->import();
}

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__

=head1 NAME

vh - Perl extension for use in Verlihub Perl scripts.

=head1 SYNOPSIS

Sample script:

  use vh;

  my $dbh;
  my $botname;

  sub Main {
    $dbh = vh::VHDBConnect;

    $botname = vh::GetConfig("config", "hub_security");

    return 1;
  }

  sub VH_OnOperatorCommand {
    my $nick = shift;
    my $data = shift;
  
    if ($data =~ /^\!regscount\s*$/) {
      my $sth = $dbh->prepare("SELECT class, COUNT(nick) FROM reglist GROUP BY class ORDER BY class ASC");
      $sth->execute;

      my $s = "<$botname>\n\n";
      while(my ($c,$q) = $sth->fetchrow_array) {
        $s .= "class:$c users:$q\n";
      }
      $sth->finish;

      vh::SendToUser($s."|", $nick);
      return 0;
    }
    return 1;
  }

=head1 ABSTRACT

  Perl extension for use in Verlihub Perl scripts.

=head1 DESCRIPTION

You may use these callbacks from your scripts.

  //TODO&FIXME

  bool Ban(char *nick, char *op, char *nick, unsigned howlong, int bantype)
  bool CloseConnection(char *nick)
  char *GetMyINFO(char *nick)
  char *ParseCommand(char *command_line)
  bool SendDataToAll(char *data, int min_class, int max_class)
  bool SendDataToUser(char *data, char *nick)

=head1 SEE ALSO

See PerlScript plugin documentation and demo scripts for more information.

=head1 AUTHORS

Daniel Muller E<lt>dan at verliba dot czE<gt>

Shurik E<lt>shurik at sbin dot ruE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright 2004 by Daniel Muller

Copyright 2011-2013 by Shurik

This library is free software; you can redistribute it and/or modify
it under the terms of GNU General Public License version 2 or later. 

=cut
