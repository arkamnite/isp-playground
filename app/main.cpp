#include <bayer.hpp>

/**
 * Goal: write a pipeline which converts from raw Bayer data into a viewable
 * image. We can probably start with something simple, like writing to a PPM
 * format image, since we don't have enough time to actually write PNGs or JPEGs
 * (although those would both be interesting). PPM should also be compatible
 * with whatever framebuffer/image type we come up with and perform our
 * transformations on.
 *
 * 1. De-mosaic a Bayer RAW image
 * Simple approach is to use bilinear filtering, which tries to fill in the gaps
 * between pixels of the same colour. This fails when used on straight/sharp
 * edges though:
 *
 * _______    _______     _______
 * |3|3|3|    |3|x|3|     |3|3|3|
 * |1|1|1| => |x|x|x| =>  |2|2|2|
 * |1|1|1|    |1|x|1|     |1|1|1|
 * -------    -------     -------
 *
 * Can be implemented simply by... I don't know.
 */

int main() { return 0; }
