#!perl -w
use strict;
use Imager;
use Test::More tests => 51;
use Imager::Test qw(test_image test_image_16 is_image);
use Fcntl ':seek';

Imager::init_log('testout/20write.log', 2);

{
  my $im = test_image();
  ok($im->write(file => 'testout/20verb.rgb'), "write 8-bit verbatim")
    or print "# ", $im->errstr, "\n";
  my $im2 = Imager->new;
  ok($im2->read(file => 'testout/20verb.rgb'), "read it back")
    or print "# ", $im2->errstr, "\n";
  is_image($im, $im2, "compare");
  is($im2->tags(name => 'sgi_rle'), 0, "check not rle");
  is($im2->tags(name => 'sgi_bpc'), 1, "check bpc");
  is($im2->tags(name => 'i_comment'), undef, "no namestr");
  
  ok($im->write(file => 'testout/20rle.rgb', 
		sgi_rle => 1, 
		i_comment => "test"), "write 8-bit rle")
    or print "# ", $im->errstr, "\n";
  my $im3 = Imager->new;
  ok($im3->read(file => 'testout/20rle.rgb'), "read it back")
    or print "# ", $im3->errstr, "\n";
  is_image($im, $im3, "compare");
  is($im3->tags(name => 'sgi_rle'), 1, "check not rle");
  is($im3->tags(name => 'sgi_bpc'), 1, "check bpc");
  is($im3->tags(name => 'i_comment'), 'test', "check i_comment set");
}

{
  my $im = test_image_16();
  ok($im->write(file => 'testout/20verb16.rgb'), "write 16-bit verbatim")
    or print "# ", $im->errstr, "\n";
  my $im2 = Imager->new;
  ok($im2->read(file => 'testout/20verb16.rgb'), "read it back")
    or print "# ", $im2->errstr, "\n";
  is_image($im, $im2, "compare");
  is($im2->tags(name => 'sgi_rle'), 0, "check not rle");
  is($im2->tags(name => 'sgi_bpc'), 2, "check bpc");
  is($im2->tags(name => 'i_comment'), undef, "no namestr");
  
  ok($im->write(file => 'testout/20rle16.rgb', 
		sgi_rle => 1, 
		i_comment => "test"), "write 16-bit rle")
    or print "# ", $im->errstr, "\n";
  my $im3 = Imager->new;
  ok($im3->read(file => 'testout/20rle16.rgb'), "read it back")
    or print "# ", $im3->errstr, "\n";
  is_image($im, $im3, "compare");
  is($im3->tags(name => 'sgi_rle'), 1, "check not rle");
  is($im3->tags(name => 'sgi_bpc'), 2, "check bpc");
  is($im3->tags(name => 'i_comment'), 'test', "check i_comment set");
}

{
  # grey scale check
  my $im = test_image()->convert(preset=>'grey');
  ok($im->write(file => 'testout/20vgray8.bw'), "write 8-bit verbatim grey")
    or print "# ", $im->errstr, "\n";
  my $im2 = Imager->new;
  ok($im2->read(file => 'testout/20vgray8.bw'), "read it back")
    or print "# ", $im2->errstr, "\n";
  is_image($im, $im2, "compare");
  is($im2->tags(name => 'i_format'), 'sgi', "check we saved as SGI");
  is($im2->tags(name => 'sgi_rle'), 0, "check not rle");
  is($im2->tags(name => 'sgi_bpc'), 1, "check bpc");
  is($im2->tags(name => 'i_comment'), undef, "no namestr");
}

{
  # write failure tests
  my $rgb8 = test_image();
  my $rgb16 = test_image_16();
  my $rgb8rle = $rgb8->copy;
  $rgb8rle->settag(name => 'sgi_rle', value => 1);
  my $grey8 = $rgb8->convert(preset => 'grey');
  my $grey16 = $rgb16->convert(preset => 'grey');
  my $grey16rle = $grey16->copy;
  $grey16rle->settag(name => 'sgi_rle', value => 1);

  my @tests =
    (
     # each entry is: image, limit, expected msg, description
     [ 
      $rgb8, 500, 
      'SGI image: cannot write header', 
      'writing header' 
     ],
     [ 
      $rgb8, 1024, 
      'SGI image: error writing image data', 
      '8-bit image data' 
     ],
     [
      $grey8, 513,
      'SGI image: error writing image data',
      '8-bit image data (grey)'
     ],
     [
      $rgb8rle, 513,
      'SGI image: error writing offsets/lengths',
      'rle tables, 8 bit',
     ],
     [
      $rgb8rle, 4112,
      'SGI image: error writing RLE data',
      '8-bit rle data',
     ],
     [
      $rgb8rle, 14707,
      'SGI image: cannot write final RLE table',
      '8-bit rewrite RLE table',
     ],
     [
      $rgb16, 513,
      'SGI image: error writing image data',
      '16-bit image data',
     ],
     [
      $grey16rle, 513,
      'SGI image: error writing offsets/lengths',
      'rle tables, 16 bit',
     ],
     [
      $grey16rle, 1713,
      'SGI image: error writing RLE data',
      '16-bit rle data',
     ],
     [
      $grey16rle, 10871,
      'SGI image: cannot write final RLE table',
      '16-bit rewrite RLE table',
     ],
    );
  for my $test (@tests) {
    my ($im, $limit, $expected_msg, $desc) = @$test;
    my ($writecb, $seekcb) = limited_write($limit);
    ok(!$im->write(type => 'sgi', writecb => $writecb,
		   seekcb => $seekcb, maxbuffer => 1),
       "write should fail - $desc");
    is($im->errstr, "$expected_msg: limit reached", "check error - $desc");
  }
}

sub limited_write {
  my ($limit) = @_;

  my $pos = 0;
  my $written = 0;
  return
    (
     # write callback
     sub {
       my ($data) = @_;
       # limit total written so we can fail the offset table write for RLE
       $written += length $data;
       if ($written <= $limit) {
	 $pos += length $data;
         print "# write of ", length $data, " bytes successful (", 
	   $limit - $written, " left)\n";
         return 1;
       }
       else {
         print "# write of ", length $data, " bytes failed\n";
         Imager::i_push_error(0, "limit reached");
         return;
       }
     },
     # seek cb
     sub {
       my ($position, $whence) = @_;

       if ($whence == SEEK_SET) {
	 $pos = $position;
	 print "# seek to $pos\n";
       }
       elsif ($whence == SEEK_END) {
	 die "SEEK_END not supported\n";
       }
       elsif ($whence == SEEK_CUR) {
	 die "SEEK_CUR not supported\n";
       }
       else {
	 die "Invalid seek whence $whence";
       }

       $pos;
     }
    )
}
