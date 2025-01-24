## Summary

This is a C++ implementation FIX based financial exchange, designed for algorithmic testing.

__It is a work in progress. It only supports quoting at the moment, but it does handle connections from `sample_client` in the `cpp_fix_engine` project.__

## Building

The project depends on
- [cpp_fixed](https://github.com/robaho/cpp_fixed)
- [cpp_orderbook](https://github.com/robaho/cpp_orderbook)
- [cpp_fix_codec](https://github.com/robaho/cpp_fix_codec)
- [cpp_fix_engine](https://github.com/robaho/cpp_fix_engine)

All should be cloned and built at the same directory level. The `Makefile` includes and library locations can be changed if they are available in a different location.

The project builds by default using `make` and CLang. There is a `Makefile.gcc` for building using GCC.

use `./makeall.sh` to build `cpp-trader` and all dependent projects.

## Testing

use `bin/cpp-trader` to start the exchange.

use `cpp_fix_engine/bin/sample_client` to start quoting against the exchange.

use `kill -USR1 <pid>` where `pid` is the exchange process to dump all of the books.
