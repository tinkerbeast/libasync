#ifndef ASYNC_CONCEPT__PROMISE_H_
#define ASYNC_CONCEPT__PROMISE_H_

#include <concepts>

namespace async::concept
{
  // TODO: get_return_object_on_allocation_failure is not handled

  /**
   * Promise concept as described in C++20 standard section 9.5.4. Details in
   * item 5 which shows how a promise is used in a compiled co-routine.
   */
  template<typename T, typename R>
  concept Promise = requires(T t) {
    { t.initial_suspend() }; // TODO: return type Awaitable
    { t.final_suspend() }; // TODO: return type Awaitable
    { t.unhandled_exception() } -> std::same_as<void>;
    { t.get_return_object() };
  } && requires(T t, R r) {
    requires std::same_as<decltype(t.return_void()), void>
    || std::same_as<decltype(t.return_value(r)), void>;
  }

}  // namespace async::concept

#endif  // ASYNC_CONCEPT__PROMISE_H_
