# Deduper

Deduper finds you similar images on your local filesystem.

Deduper ...

- finds similar images, regardless of differences in dimensions and compression artifects.
- does pretty much the same thing as [libpuzzle](https://github.com/jedisct1/libpuzzle).
- is an implementation of the same article as libpuzzle.
- uses fast routines provided by OpenCV whenever possible.
- is up to 3 times faster than libpuzzle *.
- provides a tool to manage duplicate images on your local filesystem efficiently.
- has an image indexer to provide fast reverse image searching locally **.
- has a C++ API that provides low level and high level functionalities.

*: When using a sub-sliced, signature hash assisted implementation to search for
duplicate images in a dataset containing 4000 images, against a plain, distance
comparing implementation. Both implementations are multi-threaded, run on a dual
core mobile processor with hyperthreading.

**: Planned.

In its core, Deduper uses a variation of similar image detection algorithm described in
H. Chi Wong, M. Bern and D. Goldberg, "An image signature for any kind of image,"
Proceedings. International Conference on Image Processing, 2002, pp. I-I,
doi: 10.1109/ICIP.2002.1038047.

## License

Mozilla Public License 2.0
