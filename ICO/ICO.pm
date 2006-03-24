package Imager::File::ICO;
use strict;
use Imager;
use vars qw($VERSION @ISA);

BEGIN {
  $VERSION = "0.01";
  
  eval {
    require XSLoader;
    XSLoader::load('Imager::File::ICO', $VERSION);
    1;
  } or do {
    require DynaLoader;
    push @ISA, 'DynaLoader';
    bootstrap Imager::File::ICO $VERSION;
  };
}

Imager->register_reader
  (
   type=>'ico',
   single => 
   sub { 
     my ($im, $io, %hsh) = @_;
     $im->{IMG} = i_readico_single($io, $hsh{page} || 0);

     unless ($im->{IMG}) {
       $im->_set_error(Imager->_error_as_msg);
       return;
     }
     return $im;
   },
   multiple =>
   sub {
     my ($io, %hsh) = @_;
     
     my @imgs = i_readico_multi($io);
     unless (@imgs) {
       Imager->_set_error(Imager->_error_as_msg);
       return;
     }
     return map { 
       bless { IMG => $_, DEBUG => $Imager::DEBUG, ERRSTR => undef }, 'Imager'
     } @imgs;
   },
  );

1;

__END__

=head1 NAME

Imager::File::ICO - read MS Icon files

=head1 SYNOPSIS

  use Imager;

  my $img = Imager->new;
  $img->read(file=>"foo.ico")
    or die $img->errstr;

  my @imgs = Imager->read_multi(file => "foo.ico")
    or die Imager->errstr;

=head1 DESCRIPTION



=head1 AUTHOR

=head1 SEE ALSO

Imager, Imager::Files.

=cut
