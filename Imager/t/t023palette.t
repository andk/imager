#!perl -w
# some of this is tested in t01introvert.t too
use strict;
my $loaded;
BEGIN { 
  require "t/testtools.pl";
  $| = 1; print "1..57\n";
}
END { okx(0, "loading") unless $loaded; }
use Imager;
$loaded = 1;

okx(1, "Loaded");

my $img = Imager->new(xsize=>50, ysize=>50, type=>'paletted');

okx($img, "paletted image created");

okx($img->type eq 'paletted', "got a paletted image");

my $black = Imager::Color->new(0,0,0);
my $red = Imager::Color->new(255,0,0);
my $green = Imager::Color->new(0,255,0);
my $blue = Imager::Color->new(0,0,255);

my $white = Imager::Color->new(255,255,255);

# add some color
my $blacki = $img->addcolors(colors=>[ $black, $red, $green, $blue ]);

print "# blacki $blacki\n";
okx(defined $blacki && $blacki == 0, "we got the first color");

okx($img->colorcount() == 4, "should have 4 colors");
my ($redi, $greeni, $bluei) = 1..3;

my @all = $img->getcolors;
okx(@all == 4, "all colors is 4");
coloreq($all[0], $black, "first black");
coloreq($all[1], $red, "then red");
coloreq($all[2], $green, "then green");
coloreq($all[3], $blue, "and finally blue");

# keep this as an assignment, checking for scalar context
# we don't want the last color, otherwise if the behaviour changes to
# get all up to the last (count defaulting to size-index) we'd get a
# false positive
my $one_color = $img->getcolors(start=>$redi);
okx($one_color->isa('Imager::Color'), "check scalar context");
coloreq($one_color, $red, "and that it's what we want");

# make sure we can find colors
okx(!defined($img->findcolor(color=>$white)), 
    "shouldn't be able to find white");
okx($img->findcolor(color=>$black) == $blacki, "find black");
okx($img->findcolor(color=>$red) == $redi, "find red");
okx($img->findcolor(color=>$green) == $greeni, "find green");
okx($img->findcolor(color=>$blue) == $bluei, "find blue");

# various failure tests for setcolors
okx(!defined($img->setcolors(start=>-1, colors=>[$white])),
    "expect failure: low index");
okx(!defined($img->setcolors(start=>1, colors=>[])),
    "expect failure: no colors");
okx(!defined($img->setcolors(start=>5, colors=>[$white])),
    "expect failure: high index");

# set the green index to white
okx($img->setcolors(start => $greeni, colors => [$white]),
    "set a color");
# and check it
coloreq(scalar($img->getcolors(start=>$greeni)), $white,
	"make sure it was set");
okx($img->findcolor(color=>$white) == $greeni, "and that we can find it");
okx(!defined($img->findcolor(color=>$green)), "and can't find the old color");

# write a few colors
okx(scalar($img->setcolors(start=>$redi, colors=>[ $green, $red])),
	   "save multiple");
coloreq(scalar($img->getcolors(start=>$redi)), $green, "first of multiple");
coloreq(scalar($img->getcolors(start=>$greeni)), $red, "second of multiple");

# put it back
$img->setcolors(start=>$red, colors=>[$red, $green]);

# draw on the image, make sure it stays paletted when it should
okx($img->box(color=>$red, filled=>1), "fill with red");
okx($img->type eq 'paletted', "paletted after fill");
okx($img->box(color=>$green, filled=>1, xmin=>10, ymin=>10,
	      xmax=>40, ymax=>40), "green box");
okx($img->type eq 'paletted', 'still paletted after box');
# an AA line will almost certainly convert the image to RGB, don't use
# an AA line here
okx($img->line(color=>$blue, x1=>10, y1=>10, x2=>40, y2=>40),
    "draw a line");
okx($img->type eq 'paletted', 'still paletted after line');

# draw with white - should convert to direct
okx($img->box(color=>$white, filled=>1, xmin=>20, ymin=>20, 
	      xmax=>30, ymax=>30), "white box");
okx($img->type eq 'direct', "now it should be direct");

# various attempted to make a paletted image from our now direct image
my $palimg = $img->to_paletted;
okx($palimg, "we got an image");
# they should be the same pixel for pixel
okx(Imager::i_img_diff($img->{IMG}, $palimg->{IMG}) == 0, "same pixels");

# strange case: no color picking, and no colors
# this was causing a segmentation fault
$palimg = $img->to_paletted(colors=>[ ], make_colors=>'none');
okx(!defined $palimg, "to paletted with an empty palette is an error");
print "# ",$img->errstr,"\n";
okx(scalar($img->errstr =~ /no colors available for translation/),
    "and got the correct msg");

okx(!Imager->new(xsize=>1, ysize=>-1, type=>'paletted'), 
    "fail on -ve height");
matchx(Imager->errstr, qr/Image sizes must be positive/,
       "and correct error message");
okx(!Imager->new(xsize=>-1, ysize=>1, type=>'paletted'), 
    "fail on -ve width");
matchx(Imager->errstr, qr/Image sizes must be positive/,
       "and correct error message");
okx(!Imager->new(xsize=>-1, ysize=>-1, type=>'paletted'), 
    "fail on -ve width/height");
matchx(Imager->errstr, qr/Image sizes must be positive/,
       "and correct error message");

okx(!Imager->new(xsize=>1, ysize=>1, type=>'paletted', channels=>0),
    "fail on 0 channels");
matchx(Imager->errstr, qr/Channels must be positive and <= 4/,
       "and correct error message");
okx(!Imager->new(xsize=>1, ysize=>1, type=>'paletted', channels=>5),
    "fail on 5 channels");
matchx(Imager->errstr, qr/Channels must be positive and <= 4/,
       "and correct error message");

{
  # https://rt.cpan.org/Ticket/Display.html?id=8213
  # check for handling of memory allocation of very large images
  # only test this on 32-bit machines - on a 64-bit machine it may
  # result in trying to allocate 4Gb of memory, which is unfriendly at
  # least and may result in running out of memory, causing a different
  # type of exit
  use Config;
  if ($Config{ivsize} == 4) {
    my $uint_range = 256 ** $Config{ivsize};
    my $dim1 = int(sqrt($uint_range))+1;
    
    my $im_b = Imager->new(xsize=>$dim1, ysize=>$dim1, channels=>1, type=>'paletted');
    isx($im_b, undef, "integer overflow check - 1 channel");
    
    $im_b = Imager->new(xisze=>$dim1, ysize=>1, channels=>1, type=>'paletted');
    okx($im_b, "but same width ok");
    $im_b = Imager->new(xisze=>1, ysize=>$dim1, channels=>1, type=>'paletted');
    okx($im_b, "but same height ok");
    matchx(Imager->errstr, qr/integer overflow/,
           "check the error message");

    # do a similar test with a 3 channel image, so we're sure we catch
    # the same case where the third dimension causes the overflow
    # for paletted images the third dimension can't cause an overflow
    # but make sure we didn't anything too dumb in the checks
    my $dim3 = $dim1;
    
    $im_b = Imager->new(xsize=>$dim3, ysize=>$dim3, channels=>3, type=>'paletted');
    isx($im_b, undef, "integer overflow check - 3 channel");
    
    $im_b = Imager->new(xisze=>$dim3, ysize=>1, channels=>3, type=>'paletted');
    okx($im_b, "but same width ok");
    $im_b = Imager->new(xisze=>1, ysize=>$dim3, channels=>3, type=>'paletted');
    okx($im_b, "but same height ok");

    matchx(Imager->errstr, qr/integer overflow/,
           "check the error message");
  }
  else {
    skipx(8, "don't want to allocate 4Gb");
  }
}

sub coloreq {
  my ($left, $right, $comment) = @_;

  my ($rl, $gl, $bl, $al) = $left->rgba;
  my ($rr, $gr, $br, $ar) = $right->rgba;

  print "# comparing color($rl,$gl,$bl,$al) with ($rr,$gr,$br,$ar)\n";
  okx($rl == $rr && $gl == $gr && $bl == $br && $al == $ar,
      $comment);
}
