#!/usr/bin/env perl
# AweMUD NG - Next Generation AwesomePlay MUD
# Copyright (C) 2000-2004  AwesomePlay Productions, Inc.
# See the file COPYING for license details
# http://www.awemud.net
#
# Set this up to receive email for ingame sent bug reports,
# abuse reports, and so on.  Them modify the end to use the
# data to do whatever you want.  Variables:
#
# $type   : type of reprot ('BUG', 'ABUSE')
# $host   : host the reporting server is running on
# $player : who put in the report
# $abuser : if $type eq 'ABUSE', who is the report against
# $body   : body of the report
#
# Modify this script to insert into your bug tracking
# database or whatever you need.

# ---- CORE - DONT MODIFY - SEE BELOW ----

# states:
#  0 - none
#  1 - mail header
#  2 - report header
#  3 - report body

$mode = 1;
$player = '';
$body = '';
$type = '';
$host = '';
$abuser = '';

while ($line = <>) {
	chomp($line);
	if ($mode == 1) { # mail header
		if ($line eq '') { # blank line! end of mail header
			$mode = 0;
		}
	} elsif ($mode == 0) { # none/unknown
		if ($line eq '# -- HEADER --') { # begin report header
			$mode = 2;
		} elsif ($line eq '# -- BODY --') { # begin report body
			$mode = 3;
			$body = '';
		}
	} elsif ($mode == 2) { # report header
		if ($line eq '# -- END --') { # end of header
			$mode = 0;
		} elsif ($line =~ /^Issue: (.*)$/) { # de type
			$type = $1;
		} elsif ($line =~ /^From: (.*)$/) { # de player
			$player = $1;
		} elsif ($line =~ /^Host: (.*)$/) { # de player
			$host = $1;
		} elsif ($type eq 'ABUSE' && $line =~ /^About: (.*)$/) { # reported player
			$abuser = $1;
		}
	} elsif ($mode == 3) { # report body
		if ($line eq '# -- END --') { # end of body
			$mode = 0;
		} else {
			$body .= $line . "\n";
		}
	}
}

# ---- MODIFY BELOW HERE ----

print "Type: $type\n";
print "Host: $host\n";
print "From: $player\n";
if ($type eq 'ABUSE') {
	print "Abuser: $abuser\n";
}
print "Body:\n$body";
