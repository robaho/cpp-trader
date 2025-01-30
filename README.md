## Summary

This is a C++ FIX based financial exchange designed for algorithmic testing.

__It is a work in progress and under active development.__

ToDo:
- ~~add market/limit order support including execution reports~~
- add multicast market data support for books and trades
- ~~cancel all orders and quotes when session disconnects~~
- possibly add [Nats](https://github.com/nats-io) messaging integration for trade distribution

## Building

The project depends on these other [robaho](https://github.com/robaho) projects.
- [cpp_fixed](https://github.com/robaho/cpp_fixed)
- [cpp_orderbook](https://github.com/robaho/cpp_orderbook)
- [cpp_fix_codec](https://github.com/robaho/cpp_fix_codec)
- [cpp_fix_engine](https://github.com/robaho/cpp_fix_engine)

All should be cloned and built at the same directory level. The `Makefile` includes and library locations can be changed if they are available in a different location.

The project builds by default using `make` and CLang. There is a `Makefile.gcc` for building using GCC.

use `./makeall.sh` to build `cpp-trader` and all dependent projects.

## Testing

use `bin/cpp-trader` to start the exchange.

use `cpp_fix_engine/bin/sample_client` or `cpp_fix_engine/massquote.sh` to start quoting against the exchange.

use `cpp_fix_engine/bin/sample_sendorder` to send a new order and wait for fill or timeout.

use `kill -USR1 <pid>` where `pid` is the exchange process to dump all of the books.

use `make run_tests` to run all of the test cases.

## Performance

using the `cpp_fix_engine` boost fibers branch and

using `sample_client localhost -bench 75`, more than **165k quotes per second**.<sup>1</sup>

```
round-trip 834721 quotes, usec per quote 5.99514, quotes per sec 166801
```

as a comparison, using [go-trader](https://github.com/robaho/go-trader) and the Go fix client, it does about 45k quotes per second.

<sup>1</sup>These are ping-pong quotes, i.e. send quote, wait for quote ack, send next quote. Streaming quotes are considerably faster.

_Updated network timings coming soon._
