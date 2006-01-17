#!/usr/bin/perl -w

# massroom - generate a bunch of related rooms
# 	Larry "Dirt Road" Kollar, 14 Dec 2002
# 

my $zonewrap=0;		# TODO: implement zone wrapping
my $zonename="";

my ($width, $height, $root, $roomtitle, $roomdesc);
my $usage = "Usage: $0 <width> <height> <root> <title> file...\n";

my ($x, $y);

$width = shift || die $usage;
$height = shift || die $usage;
$root = shift || die $usage;
$roomtitle = shift || die $usage;

# ----------------------------------------------
#
# Slurp in the description
#
while( <> ) {
	$roomdesc .= $_;
}


# Print a zone header (TODO)
#
if( $zonewrap ) {
	print "<zone version=", qq("3"), ">\n";
	print "<name>", $zonename, "</name>\n";
	print "<properties/>\n";
}


# Generate the rooms
#
for( $x=1; $x <= $width; $x++ ) {
	for( $y=1; $y <= $height; $y++ ) {
		print "<room version=", qq("3"), " outdoors=", qq("1"), ">\n";
		print "  <name>", $root . $x . "_" . $y, "</name>\n";
		print "  <title>", $roomtitle, "</title>\n";
		print "  <desc>", $roomdesc, "</desc>\n";
		print "  <properties/>\n";

		addexit( 1, "north", $x, $y-1 );
		addexit( 2, "south", $x, $y+1 );
		addexit( 3, "west",  $x-1, $y );
		addexit( 4, "east",  $x+1, $y );
		addexit( 5, "northwest", $x-1, $y-1 );
		addexit( 6, "northeast", $x+1, $y-1 );
		addexit( 7, "southwest", $x-1, $y+1 );
		addexit( 8, "southeast", $x+1, $y+1 );

		print "</room>\n";
	}
}


# Close the zone (TODO)
if( $zonewrap ) {
	print "</zone>\n";
}


# Create an exit. Use a special room name ("outside")
# for rooms not landing in the block.

sub addexit {
	my $xid = shift(@_);
	my $dir = shift(@_);
	my $xdest = shift(@_);
	my $ydest = shift(@_);
	my $targ = "";

	print "  <exit id=", qq("$xid"), " dir=", qq("$dir"), ">\n";
	print "    <name>", $dir, "</name>\n";
	print "    <properties/>\n";

	if ( $xdest<1 || $ydest<1 || $xdest>$width || $ydest>$height ) {
		$targ = "outside";
	} else {
		$targ = $root . $xdest . "_" . $ydest;
	}
	print "    <target>", $targ, "</target>\n";

	print "  </exit>\n";
}

# POD follows. Create manpage using:
# 	pod2man --lax --center="AweMUD Utilities" massroom

__END__

=head1
Name

massroom - generate a large area of AweMUD rooms

=head1
Synopsis

B<massroom> width height root title [file...]

=head1
Description

B<massroom> creates an area of B<AweMUD>-format rooms, 
with the same title and description.
The intent is to quickly create large outdoor areas
such as a forest or valley.
B<massroom> sets up exits in all compass directions to
adjacent rooms, using a dummy value of B<outside> for
exits going to other rooms.

=head1
Arguments

=over 4

=item width

is the number of east-west rooms.

=item height

is the number of north-south rooms.

=item root

is the internal name of each room.
B<massroom> creates a unique name for each room
by adding the width and height index.
For example, if the root is B<forest>, the first
room name is B<forest1_1>, the room directly east
is B<forest2_1>, and so forth.

=item title

is the room title that a player sees, such as
B<Spooky Forest>.
The title is the same for all rooms.

=back

Any other arguments on the command line are
assumed to be files containing the text of
the room description.
If no file is specified, B<massroom> reads
the standard input.

=head1
Bugs

The XML output is fragmentary, not wrapped in
a B<zone> element.
This was a deliberate decision, since you
may want to add an area to an existing zone.
A later version of B<massroom> will optionally
wrap the rooms and produce a complete zone file.

=head1
To Do

Support optional zone wrapping (see above).

Support use of a prototype room (which would
supplant the B<root> and B<title> arguments,
as well as allowing other enhancements or
restrictions.
One example use would be to create streets,
which tend to run east-west only or north-south
only.
Many street rooms would require only two exits.

Support a room type, such as B<type="forest">,
as that functionality will soon be supported
by B<AweMUD>.

Come up with a way to run roads through the
middle of an area.

=head1
Version

1.0 (14 Dec 2002)

=head1
Author

Larry "Dirt Road" Kollar, lkollar@despammed.com

=cut
