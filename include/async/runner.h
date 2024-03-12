#ifndef ASYNC__RUNNER_H_
#define ASYNC__RUNNER_H_

#include <concepts>
#include <iostream> // TODO: remove

namespace async
{
  // TODO: make this variadic
  template<typename T>
  T run(coro<T>&& continuation) {
    const coro_promise_common<T>&  promise = continuation.get_promise_ref();
    while (true) {
      if (promise.done()) break;
      if (promise.is_resumable()) {
        promise.resume();
      } else {
        // TODO: ???
        std::cout << "async::run in not resumable part\n";
      }
    }
    std::cout << "async::run before result\n";
    if constexpr (std::same_as<T, void>) {
      promise.result();
    } else {
      return promise.result();
    }
  }

  template<typename T>
  void run_noreturn(coro<T>&& continuation) {
    const coro_promise_common<T>&  promise = continuation.get_promise_ref();
    while (true) {
      if (promise.done()) break;
      if (promise.is_resumable()) {
        promise.resume();
      } else {
        // TODO: ???
        std::cout << "async::run_noreturn in not resumable part\n";
      }
    }
    std::cout << "async::run_noreturn before result\n";
    promise.result();    
  }


}  // namespace async

#endif  // ASYNC__RUNNER_H_