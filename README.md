# Deduper

Deduper finds you similar images on your local filesystem.

Deduper ...

- finds similar images, regardless of differences in dimensions and compression artifects.
- does pretty much the same thing as [libpuzzle](https://github.com/jedisct1/libpuzzle).
- is an implementation of the same article as libpuzzle.
- uses fast routines provided by OpenCV whenever possible.
- is up to 183.97% or 385.04% faster than libpuzzle, depending on task performed.
- provides a tool to manage duplicate images on your local filesystem efficiently +.
- has an image indexer to provide fast reverse image searching locally +.
- has a C++ API ("xsig") that provides low level and high level functionalities.

Deduper does not ...

- find heavily edited version of the same image, or cropped version of the image.
- detect stylistically or topically similar images. Only visually similar images can be detected.
- use any machine learning techniques. Only traditional image processing techniques are used.
- use the GPU (yet).

*: 183.97% faster when computing signature only. 385.04% faster when finding duplicates.
Duplicate detection in deduper uses a new sub-sliced variant of the original algorithm,
while the libpuzzle based implementation uses naive distance comparing. The test data set
has 3225 images of various dimensions. Both implementations are multi-threaded, run on a
dual core mobile processor with hyperthreading.

+: QDeduper. See below.

In its core, Deduper uses a variation of similar image detection algorithm described in
H. Chi Wong, M. Bern and D. Goldberg, "An image signature for any kind of image,"
Proceedings. International Conference on Image Processing, 2002, pp. I-I,
doi: 10.1109/ICIP.2002.1038047.

Deduper is still a work in progress. For the older, libpuzzle based implementation,
see https://cgit.chrisoft.org/oddities.git/tree/deduper.

## QDeduper

QDeduper is a graphical frontend for deduper. It can scan for duplicate images and manage
the duplicates found afterwards. It also provides a "reverse image search" tool. See its
documentation for details.

## License

Mozilla Public License 2.0
