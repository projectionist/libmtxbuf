# libmtxbuf

Template for a threadsafe buffer of whatever you like.

* has a maximum number of elements
* blocks when trying to read from an empty buffer
* blocks when trying to write to a full buffer
* returns data on a closed, non-empty buffer
* throws an error when reading from an empty, closed buffer
* no-ops when writing to a closed buffer

Header only library.

Extracted from a video-based projection: https://github.com/projectionist/loop.

See `tests/buffer.cpp` for example usage.

### Tests

Build tests with `make tests/buffer`. Run with `tests/buffer_test`.
