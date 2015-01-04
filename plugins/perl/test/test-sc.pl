# PerlScript test suite - ScriptCommand hook in perl
use vh;
use strict;

sub Say {
  my $msg = shift;
  $msg=~s/\&/&#36;/gmx;
  $msg=~s/\|/&#124;/gmx;
  vh::SendToClass($msg."|", 10, 10);
}

sub VH_OnScriptCommand {
  my $cmd = shift;
  my $data = shift;
  my $plug = shift;
  my $script = shift;
  Say "    (test-sc.pl:VH_OnScriptCommand) cmd=[$cmd] data=[$data] plug=[$plug] script=[$script]";
  return 1;
}
