#!/usr/bin/perl -w

use Image::Resize;
use File::Fetch;
use POSIX;
use open qw/ :std :encoding(utf-8) /;

BEGIN { $| = 1; }

my $imgurl = $ARGV[0];
print "Downloading $imgurl....";
my $download = File::Fetch->new(uri => $imgurl);
my $file = $download->fetch() or die $download->error;
File::Copy::move($file, 'productimage.png');
print "Done\n\nResizing....";
my $img = Image::Resize->new('productimage.png');
my $gd = $img->resize(200,200);
$gd->_file('productimage.png');
print "Done\n\nSaved as productimage.png\n";
