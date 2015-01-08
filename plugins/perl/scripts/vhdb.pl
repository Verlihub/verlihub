# vhdb.pl
# (C) Shurik, 2011
#
# Demo script that uses Verlihub database

use vh;
use strict;

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

    print $s; 
    vh::SendToUser($s."|", $nick);
    return 0;
  }
  return 1;
}

sub UnLoad {
  $dbh->disconnect;
  return 1;
}
