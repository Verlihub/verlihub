# PerlScript test suite

use vh;
use strict;

my $botname;

sub Main {
  #$botname = vh::GetConfig("config", "hub_security");
  $botname = vh::GetHubSecAlias;

  vh::SendToClass("<$botname> Welcome to PerlScript test suite script. Run !perltest command to run all tests.|", 10, 10);

  return 1;
}

sub RunTests; 1;

sub VH_OnOperatorCommand {
  my $nick = shift;
  my $data = shift;

  if ($data =~ /^\!perltest\s*$/) {
    RunTests $nick;
    return 0;
  }
  return 1;
}

sub Say {
  my $msg = shift;
  $msg=~s/\&/&#36;/gmx;
  $msg=~s/\|/&#124;/gmx;
  vh::SendToClass($msg."|", 10, 10);
}

sub Div {
  Say " ";
}

sub RunTests {
  my $opnick = shift;
  Say "============= TEST =============";
  Div;
  Say "Test vh::VHDBConnect...";
  my $dbh = vh::VHDBConnect;
  my $query = "SELECT class, COUNT(nick) FROM reglist GROUP BY class ORDER BY class ASC";
  Say "  Executing query: $query";
  Say "  Result:";
  my $sth = $dbh->prepare($query);
  $sth->execute;
  while(my ($c, $q) = $sth->fetchrow_array) {
    Say sprintf("    %2d %4d", $c, $q);
  }
  $sth->finish;
  $dbh->disconnect;
  Div;
  Say "Test vh::SQLQuery/Fetch/Free...";
  Say "  Executing query: $query";
  my $rows = vh::SQLQuery $query;
  Say "    Rows: ".$rows;
  Say "  Result:";
  for(my $i = 0; $i < $rows; $i++) {
    my ($c, $q) = vh::SQLFetch($i);
    Say sprintf("    %2d %4d", $c, $q);
  }
  vh::SQLFree;
  Div;
  Say "Test NULL in vh::SQLQuery";
  my $query = "SELECT NULL UNION SELECT 0 UNION SELECT 1";
  Say "  Executing query: $query";
  my $rows = vh::SQLQuery $query;
  Say "    Rows: ".$rows;
  Say "  Result:";
  for(my $i = 0; $i < $rows; $i++) {
    my ($v) = vh::SQLFetch($i);
    $v = "*undef*" unless defined $v;
    Say "    ".$v;
  }
  vh::SQLFree;
  Div;
  Say "Test vh::GetNickList...";
  Say "  Result:";
  Say "    ".vh::GetNickList;
  Div;
  Say "Test vh::GetNickListArray...";
  Say "  Result:";
  Say "    ".join(" ",vh::GetNickListArray);
  Div;
  Say "Test vh::GetOPList...";
  Say "  Result:";
  Say "    ".vh::GetOPList;
  Div;
  Say "Test vh::GetOPListArray...";
  Say "  Result:";
  Say "    ".join(" ",vh::GetOPListArray);
  Div;
  Say "Test vh::GetBotList...";
  Say "  Result:";
  Say "    ".vh::GetBotList;
  Div;
  Say "Test vh::GetBotListArray...";
  Say "  Result:";
  Say "    ".join(" ",vh::GetBotListArray);
  Div;
  Say "Test vh::GetVHCfgDir...";
  Say "  Result:";
  Say "    ".vh::GetVHCfgDir;
  Div;
  Say "Test vh::GetTotalShareSize...";
  Say "  Result:";
  Say "    ".vh::GetTotalShareSize;
  Say "Test vh::GetUsersCount...";
  Say "  Result:";
  Say "    ".vh::GetUsersCount;
  Say "Test vh::GetHubIp...";
  Say "    ".vh::GetHubIp;
  Say "Test vh::GetHubSecAlias...";
  Say "    ".vh::GetHubSecAlias;
  Say "Test vh::GetUpTime...";
  Say "    ".vh::GetUpTime;
  Div;
  Say "Test vh::GetConfig...";
  Say "  vh::GetConfig('config', 'hub_name') result:";
  Say "    ".vh::GetConfig("config", "hub_name");
  Div;
  Say "Test vh::ScriptCommand";
  Say "  vh::ScriptCommand('!testtesttest', 'testtesttest')";
  vh::ScriptCommand("!testtesttest", "testtesttest");
  Div;
  Say "Test robot functions";
  Say "  Test vh::AddRobot...";
  vh::AddRobot("testtesttest",2,"test","123","test\@test","123456");
  Say "    Nicklist: ".join(" ",vh::GetNickListArray);
  Say "    MyINFO: ".vh::GetMyINFO("testtesttest");
  Say "  Test vh::EditBot...";
  vh::EditBot("testtesttest",5,"111","222","333\@444","654321");
  Say "    Nicklist: ".join(" ",vh::GetNickListArray);
  Say "    MyINFO: ".vh::GetMyINFO("testtesttest");
  Say "  Test vh::DelRobot...";
  vh::DelRobot("testtesttest");
  Say "    Nicklist: ".join(" ",vh::GetNickListArray);
  Say "    MyINFO: ".vh::GetMyINFO("testtesttest");
  Div;
  Say "Test user functions";
  Say "  vh::GetUserCC($opnick) result:";
  Say "    ".vh::GetUserCC($opnick);
  Say "  vh::GetUserIP($opnick) result:";
  my $ip = vh::GetUserIP($opnick);
  Say "    ".$ip;
  Say "  vh::IsBot($opnick) result:";
  Say "    ".vh::IsBot($opnick);
  Say "  vh::IsBot($botname) result:";
  Say "    ".vh::IsBot($botname);
  Say "  vh::GetMyINFO($opnick)";
  Say "    ".vh::GetMyINFO($opnick);
  Say "  vh::IsUserOnline($opnick)";
  Say "    ".vh::IsUserOnline($opnick);
  Say "  vh::IsUserOnline($botname)";
  Say "    ".vh::IsUserOnline($botname);
  Say "  vh::IsUserOnline(testtesttest)";
  Say "    ".vh::IsUserOnline(testtesttest);
  Div;
  Say "Test IP functions";
  Say "  vh::GetIPCC($ip) result";
  Say "    ".vh::GetIPCC($ip);
  Say "  vh::GetIPCN($ip) result";
  Say "    ".vh::GetIPCN($ip);
  Div;
  Say "Test topic functions";
  Say "  vh::GetTopic...";
  my $topic = vh::GetTopic;
  Say "    topic=".$topic;
  Say "  vh::SetTopic...";
  vh::SetTopic "[perltest] ".$topic;
  Say "  vh::GetTopic again...";
  Say "    topic=".vh::GetTopic;
  Say "  vh::SetTopic to old topic";
  vh::SetTopic $topic;
  Div;
  Say "Test constants";
  Say "  vh::eUC_PINGER=".vh::eUC_PINGER;
  Say "  vh::eUC_NORMUSER=".vh::eUC_NORMUSER;
  Say "  vh::eUC_REGUSER=".vh::eUC_REGUSER;
  Say "  vh::eUC_VIPUSER=".vh::eUC_VIPUSER;
  Say "  vh::eUC_OPERATOR=".vh::eUC_OPERATOR;
  Say "  vh::eUC_CHEEF=".vh::eUC_CHEEF;
  Say "  vh::eUC_ADMIN=".vh::eUC_ADMIN;
  Say "  vh::eUC_MASTER=".vh::eUC_MASTER;
  Div;
  Say "Test vh::InUserSupports";
  Say "  vh::InUserSupports($opnick,'TTHSearch')";
  Say "    ".vh::InUserSupports($opnick,'TTHSearch');
  Say "  vh::InUserSupports($opnick,vh::eSF_TTHSEARCH)";
  Say "    ".vh::InUserSupports($opnick,vh::eSF_TTHSEARCH);
  Say "  vh::InUserSupports($opnick,'SomethingWrong')";
  Say "    ".vh::InUserSupports($opnick,'SomethingWrong');
  Say "============= FINISH =============";
}

sub VH_OnScriptCommand {
  my $cmd = shift;
  my $data = shift;
  my $plug = shift;
  my $script = shift;
  Say "    (test.pl:VH_OnScriptCommand) cmd=[$cmd] data=[$data] plug=[$plug] script=[$script]";
  return 1;
}
