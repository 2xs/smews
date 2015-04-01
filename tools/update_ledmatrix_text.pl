#!/usr/bin/perl -w
use strict;
use URI::Escape;

my $host = shift @ARGV;
my $str = join(" ", @ARGV);
$str = uri_escape($str);
my $query = "http://$host/ledmatrix/led_text?text=$str";
system("curl", $query);
