=head1 NAME

Imager::Filters - Entire Image Filtering Operations

=head1 SYNOPSIS

  use Imager;

  $img = ...;

  $img->filter(type=>'autolevels');
  $img->filter(type=>'autolevels', lsat=>0.2);
  $img->filter(type=>'turbnoise')

  # and lots others

  load_plugin("dynfilt/dyntest.so")
    or die "unable to load plugin\n";

  $img->filter(type=>'lin_stretch', a=>35, b=>200);

  unload_plugin("dynfilt/dyntest.so")
    or die "unable to load plugin\n";


=head1 DESCRIPTION

Filters are operations that have similar calling interface.

=head2 Types of Filters

Here is a list of the filters that are always avaliable in Imager.
This list can be obtained by running the C<filterlist.perl> script
that comes with the module source.

  Filter          Arguments   Default value
  autolevels      lsat        0.1
                  usat        0.1
                  skew        0

  bumpmap         bump lightx lighty
                  elevation   0
                  st          2

  bumpmap_complex bump
                  channel     0
                  tx          0
                  ty          0
                  Lx          0.2
                  Ly          0.4
                  Lz          -1 
                  cd          1.0 
                  cs          40.0
                  n           1.3
                  Ia          (0 0 0)
                  Il          (255 255 255)
                  Is          (255 255 255)

  contrast        intensity

  conv            coef

  fountain        xa ya xb yb
                  ftype        linear
                  repeat       none
                  combine      none
                  super_sample none
                  ssample_param 4
                  segments(see below)

  gaussian        stddev

  gradgen         xo yo colors dist

  hardinvert

  mosaic          size         20

  noise           amount       3
                  subtype      0

  postlevels      levels       10

  radnoise        xo           100
                  yo           100
                  ascale       17.0
                  rscale       0.02

  turbnoise       xo           0.0
                  yo           0.0
                  scale        10.0

  unsharpmask     stddev       2.0
                  scale        1.0

  watermark       wmark
                  pixdiff      10
                  tx           0
                  ty           0

All parameters must have some value but if a parameter has a default
value it may be omitted when calling the filter function.

A reference of the filters follows:

=over

=item autolevels

scales the value of each channel so that the values in the image will
cover the whole possible range for the channel.  I<lsat> and I<usat>
truncate the range by the specified fraction at the top and bottom of
the range respectivly.

=item bumpmap

uses the channel I<elevation> image I<bump> as a bumpmap on your
image, with the light at (I<lightx>, I<lightty>), with a shadow length
of I<st>.

=item bumpmap_complex

uses the channel I<channel> image I<bump> as a bumpmap on your image.
If Lz<0 the three L parameters are considered to be the direction of
the light.  If Lz>0 the L parameters are considered to be the light
position.  I<Ia> is the ambient colour, I<Il> is the light colour,
I<Is> is the color of specular highlights.  I<cd> is the diffuse
coefficient and I<cs> is the specular coefficient.  I<n> is the
shininess of the surface.

=item contrast

scales each channel by I<intensity>.  Values of I<intensity> < 1.0
will reduce the contrast.

=item conv

performs 2 1-dimensional convolutions on the image using the values
from I<coef>.  I<coef> should be have an odd length.

=item fountain

renders a fountain fill, similar to the gradient tool in most paint
software.  The default fill is a linear fill from opaque black to
opaque white.  The points A(xa, ya) and B(xb, yb) control the way the
fill is performed, depending on the ftype parameter:

=over

=item linear

the fill ramps from A through to B.

=item bilinear

the fill ramps in both directions from A, where AB defines the length
of the gradient.

=item radial

A is the center of a circle, and B is a point on it's circumference.
The fill ramps from the center out to the circumference.

=item radial_square

A is the center of a square and B is the center of one of it's sides.
This can be used to rotate the square.  The fill ramps out to the
edges of the square.

=item revolution

A is the centre of a circle and B is a point on it's circumference.  B
marks the 0 and 360 point on the circle, with the fill ramping
clockwise.

=item conical

A is the center of a circle and B is a point on it's circumference.  B
marks the 0 and point on the circle, with the fill ramping in both
directions to meet opposite.

=back

The I<repeat> option controls how the fill is repeated for some
I<ftype>s after it leaves the AB range:

=over

=item none

no repeats, points outside of each range are treated as if they were
on the extreme end of that range.

=item sawtooth

the fill simply repeats in the positive direction

=item triangle

the fill repeats in reverse and then forward and so on, in the
positive direction

=item saw_both

the fill repeats in both the positive and negative directions (only
meaningful for a linear fill).

=item tri_both

as for triangle, but in the negative direction too (only meaningful
for a linear fill).

=back

By default the fill simply overwrites the whole image (unless you have
parts of the range 0 through 1 that aren't covered by a segment), if
any segments of your fill have any transparency, you can set the
I<combine> option to 'normal' to have the fill combined with the
existing pixels.  See the description of I<combine> in L<Imager/Fill>.

If your fill has sharp edges, for example between steps if you use
repeat set to 'triangle', you may see some aliased or ragged edges.
You can enable super-sampling which will take extra samples within the
pixel in an attempt anti-alias the fill.

The possible values for the super_sample option are:

=over

=item none

no super-sampling is done

=item grid

a square grid of points are sampled.  The number of points sampled is
the square of ceil(0.5 + sqrt(ssample_param)).

=item random

a random set of points within the pixel are sampled.  This looks
pretty bad for low ssample_param values.

=item circle

the points on the radius of a circle within the pixel are sampled.
This seems to produce the best results, but is fairly slow (for now).

=back

You can control the level of sampling by setting the ssample_param
option.  This is roughly the number of points sampled, but depends on
the type of sampling.

The segments option is an arrayref of segments.  You really should use
the Imager::Fountain class to build your fountain fill.  Each segment
is an array ref containing:

=over

=item start

a floating point number between 0 and 1, the start of the range of fill parameters covered by this segment.

=item middle

a floating point number between start and end which can be used to
push the color range towards one end of the segment.

=item end

a floating point number between 0 and 1, the end of the range of fill
parameters covered by this segment.  This should be greater than
start.

=item c0

=item c1

The colors at each end of the segment.  These can be either
Imager::Color or Imager::Color::Float objects.

=item segment type

The type of segment, this controls the way the fill parameter varies
over the segment. 0 for linear, 1 for curved (unimplemented), 2 for
sine, 3 for sphere increasing, 4 for sphere decreasing.

=item color type

The way the color varies within the segment, 0 for simple RGB, 1 for
hue increasing and 2 for hue decreasing.

=back

Don't forget to use Imager::Fountain instead of building your own.
Really.  It even loads GIMP gradient files.

=item gaussian

performs a gaussian blur of the image, using I<stddev> as the standard
deviation of the curve used to combine pixels, larger values give
bigger blurs.  For a definition of Gaussian Blur, see:

  http://www.maths.abdn.ac.uk/~igc/tch/mx4002/notes/node99.html

=item gradgen

renders a gradient, with the given I<colors> at the corresponding
points (x,y) in I<xo> and I<yo>.  You can specify the way distance is
measured for color blendeing by setting I<dist> to 0 for Euclidean, 1
for Euclidean squared, and 2 for Manhattan distance.

=item hardinvert

inverts the image, black to white, white to black.  All channels are
inverted, including the alpha channel if any.

=item mosaic

produces averaged tiles of the given I<size>.

=item noise

adds noise of the given I<amount> to the image.  If I<subtype> is
zero, the noise is even to each channel, otherwise noise is added to
each channel independently.

=item radnoise

renders radiant Perlin turbulent noise.  The centre of the noise is at
(I<xo>, I<yo>), I<ascale> controls the angular scale of the noise ,
and I<rscale> the radial scale, higher numbers give more detail.

=item postlevels

alters the image to have only I<levels> distinct level in each
channel.

=item turbnoise

renders Perlin turbulent noise.  (I<xo>, I<yo>) controls the origin of
the noise, and I<scale> the scale of the noise, with lower numbers
giving more detail.

=item unsharpmask

performs an unsharp mask on the image.  This is the result of
subtracting a gaussian blurred version of the image from the original.
I<stddev> controls the stddev parameter of the gaussian blur.  Each
output pixel is: in + I<scale> * (in - blurred).

=item watermark

applies I<wmark> as a watermark on the image with strength I<pixdiff>,
with an origin at (I<tx>, I<ty>)

=back

A demonstration of most of the filters can be found at:

  http://www.develop-help.com/imager/filters.html

(This is a slow link.)


=head2 Plugins

It is possible to add filters to the module without recompiling the
module itself.  This is done by using DSOs (Dynamic shared object)
avaliable on most systems.  This way you can maintain our own filters
and not have to get me to add it, or worse patch every new version of
the Module.  Modules can be loaded AND UNLOADED at runtime.  This
means that you can have a server/daemon thingy that can do something
like:

  load_plugin("dynfilt/dyntest.so")
    or die "unable to load plugin\n";

  $img->filter(type=>'lin_stretch', a=>35, b=>200);

  unload_plugin("dynfilt/dyntest.so")
    or die "unable to load plugin\n";

Someone decides that the filter is not working as it should -
dyntest.c modified and recompiled.

  load_plugin("dynfilt/dyntest.so")
    or die "unable to load plugin\n";

  $img->filter(%hsh);

An example plugin comes with the module - Please send feedback to
addi@umich.edu if you test this.

Note: This seems to test ok on the following systems:
Linux, Solaris, HPUX, OpenBSD, FreeBSD, TRU64/OSF1, AIX.
If you test this on other systems please let me know.


=cut
