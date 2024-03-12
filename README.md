libasync - A C++20 coroutine library
====================================

The 'libasync' library is for developers who want to use [C++ Extensions for Coroutines - N4680](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/n4680.pdf) without having to write their own primitives (types for coroutines and tasks).

Unlike existing libraries like [cppcoro](https://github.com/lewissbaker/cppcoro) and [libcoro](https://github.com/jbaldwin/libcoro) this library decouples the coroutine primitives from the scheduler (thread-pool, io-loop, etc) which allows you to easily integrate the library into your existing codebase.

