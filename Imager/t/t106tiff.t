print "1..4\n";
use Imager qw(:all);

init_log("testout/t105gif.log",1);

$green=i_color_new(0,255,0,255);
$blue=i_color_new(0,0,255,255);
$red=i_color_new(255,0,0,255);

$img=Imager::ImgRaw::new(150,150,3);

i_box_filled($img,70,25,130,125,$green);
i_box_filled($img,20,25,80,125,$blue);
i_arc($img,75,75,30,0,361,$red);
i_conv($img,[0.1, 0.2, 0.4, 0.2, 0.1]);

my $timg = Imager::ImgRaw::new(20, 20, 4);
my $trans = i_color_new(255, 0, 0, 127);
i_box_filled($timg, 0, 0, 20, 20, $green);
i_box_filled($timg, 2, 2, 18, 18, $trans);

if (!i_has_format("tiff")) {
  print "ok 1 # skip no tiff support\n";
  print "ok 2 # skip no tiff support\n";
  print "ok 3 # skip no tiff support\n";
} else {
  open(FH,">testout/t10.tiff") || die "cannot open testout/t10.tiff for writing\n";
  binmode(FH); 
  my $IO = Imager::io_new_fd(fileno(FH));
  i_writetiff_wiol($img, $IO);
  close(FH);

  print "ok 1\n";
  
  open(FH,"testout/t10.tiff") or die "cannot open testout/t10.tiff\n";
  binmode(FH);
  $IO = Imager::io_new_fd(fileno(FH));
  $cmpimg = i_readtiff_wiol($IO, -1);

  close(FH);

  print "# tiff average mean square pixel difference: ",sqrt(i_img_diff($img,$cmpimg))/150*150,"\n";
  print "ok 2\n";

  $IO = Imager::io_new_bufchain();
  
  Imager::i_writetiff_wiol($img, $IO) or die "Cannot write to bufferchain\n";
  my $tiffdata = Imager::io_slurp($IO);

  open(FH,"testout/t10.tiff");
  my $odata;
  { local $/;
    $odata = <FH>;
  }
  
  if ($odata eq $tiffdata) {
    print "ok 3\n";
  } else {
    print "not ok 3\n";
  }

  # test Micksa's tiff writer
  # a shortish fax page
  my $faximg = Imager::ImgRaw::new(1728, 2000, 1);
  my $black = i_color_new(0,0,0,255);
  my $white = i_color_new(255,255,255,255);
  # vaguely test-patterny
  i_box_filled($faximg, 0, 0, 1728, 2000, $white);
  i_box_filled($faximg, 100,100,1628, 200, $black);
  my $width = 1;
  my $pos = 100;
  while ($width+$pos < 1628) {
    i_box_filled($faximg, $pos, 300, $pos+$width-1, 400, $black);
    $pos += $width + 20;
    $width += 2;
  }
  open FH, "> testout/t106tiff_fax.tiff"
    or die "Cannot create testout/t106tiff_fax.tiff: $!";
  binmode FH;
  $IO = Imager::io_new_fd(fileno(FH));
  i_writetiff_wiol_faxable($faximg, $IO)
    or print "not ";
  print "ok 4\n";
  close FH;
}

