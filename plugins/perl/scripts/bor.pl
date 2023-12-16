# bor.pl
# (C) Shurik, 2011
# 
# —крипт показывает пользователю одну случайную цитату с bash.im (ex-bash.org.ru) по команде +bor

use vh;
use LWP::UserAgent;
use HTML::Entities;
use Encode;
use encoding "cp1251";
use strict;

sub VH_OnUserCommand {
  my $nick = shift;
  my $data = shift;

  if ($data =~ /^\+bor\s*$/) {
    my $ua = new LWP::UserAgent(agent=>"Mozilla/1.0");
    my $resp = $ua->get("http://bash.im/random");
    my $pg = $resp->content;
    $pg =~ m#class="text">(.+?)</div>#imx;
    my $q = "<br>".$1;
    $q =~ s#<br[\s/]*>#\n\ \ \ \ #igmx;
    $q = decode_entities($q);
    vh::SendToUser(encode("cp1251","<bash.im> —лучайна¤ цитата:\n".$q."\n|"), $nick);
    return 0;
  }

  return 1;
}
