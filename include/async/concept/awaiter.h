#ifndef ASYNC_CONCEPT__AWAITER_H_
#define ASYNC_CONCEPT__AWAITER_H_

#include <concepts>
#include <coroutine>

namespace async::concept {

  /**
   * Common parts of an awaiter as described in C++20 standard section 7.6.2.3.
   */
  template<typename T>
  concept AwaiterCommon = requires(T e) {
    { e.await_ready() } -> std::same_as<bool>;
    { e.await_resume() }; // TODO: return type
  };
  
  /**
   * The coroutine flow will call `await_resume` immediately if the 
   * `await_suspend` for this type of awaiter returns true.
   */
  template<typename T, typename P=void>
  concept AwaiterConditionalResume = AwaiterCommon<T>
      && requires(T e, std::coroutine_handle<P> h) {
        { e.await_suspend(h) } -> std::same_as<bool>;
      };

  /**
   * The coroutine flow will resume once on the handle returned from 
   * `await_suspend` before suespending.
   */
  template<typename T, typename P=void, typename Z=void>
  concept AwaiterOnceResume = AwaiterCommon<T>
      && requires(T e, std::coroutine_handle<P> h) {
        { e.await_suspend(h) } -> std::same_as<std::coroutine_handle<Q>>;
      };

  /**
   * The coroutine flow will not resume immediately.
   */
  template<typename T, typename P=void>
  concept AwaiterNoResume = AwaiterCommon<T>
      && requires(T e, std::coroutine_handle<P> h) {
        { e.await_suspend(h) } -> std::same_as<void>;
      };

  template<typename T, typename P=void, typename Z=void>
  concept Awaiter = AwaiterConditionalResume<T, P> 
      || AwaiterOnceResume<T, P, Z> 
      || AwaiterNoResume<T, P>;

}  // namespace async::concept

#endif  // ASYNC_CONCEPT__AWAITER_H_