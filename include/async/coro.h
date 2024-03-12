#ifndef ASYNC__CORO_H_
#define ASYNC__CORO_H_

#include "async/debug_log.h"
#include "async/polymorphic_awaiter.h"

#include <coroutine>
#include <exception>
#include <type_traits>
#include <variant>

namespace async
{

  template<typename T>
  struct coro_promise;

  template<typename T>
  struct coro_promise_common;


  template<typename T = void>
  struct [[nodiscard]] coro final
  {
    using promise_type = coro_promise<T>;
    using value_type = T;

    coro() = delete;

    coro(const coro_promise_common<T>* promise_ref)
    : promise_ref_{promise_ref}
    {
      debug_log("MYCORO coro promise_ref_={}\n", static_cast<const void*>(promise_ref_));
    }
    
    coro(const coro&) = delete;

    coro(coro&&) = delete;

    coro& operator=(const coro&) = delete;

    coro& operator=(coro&&) = delete;

    ~coro() {      
      debug_log("MYCORO ~coro promise_ref_={}\n", static_cast<const void*>(promise_ref_));
      promise_ref_->destroy();
    }

    const coro_promise_common<T>& get_promise_ref() {
      return *promise_ref_;
    }

    // TODO: Concept for CoroPromiseType
    struct coro_awaiter final : public polymorphic_awaiter
    {    
      const coro_promise_common<T>* const inner_;

      coro_awaiter() = delete;

      coro_awaiter(const coro_promise_common<T>* inner) noexcept
      : inner_(inner)
      {
        debug_log("AWAITER(coro): coro_awaiter inner={}\n", static_cast<const void*>(inner_));
      }

      coro_awaiter(const coro_awaiter&) = delete;

      coro_awaiter(coro_awaiter&&) = delete;

      coro_awaiter& operator=(const coro_awaiter&) = delete;

      coro_awaiter& operator=(coro_awaiter&&) = delete;

      ~coro_awaiter() noexcept {
        debug_log("AWAITER(coro): ~coro_awaiter inner={}\n", static_cast<const void*>(inner_));
      }

      bool await_ready() const noexcept {
        debug_log("AWAITER(coro): await_ready inner={}\n", static_cast<const void*>(inner_));
        return inner_->done();
      }

      template<typename OuterPromise>
      void await_suspend(std::coroutine_handle<OuterPromise> handle_from_outer) /*TODO: noexcept*/ {
        // NOTE: function is not const because this becomes const.
        debug_log("AWAITER(coro): await_suspend inner={} outer={}\n", static_cast<const void*>(inner_), _h(handle_from_outer));
        handle_from_outer.promise().set_awaiter(this);
      }
      
      decltype(auto) await_resume() const {
        // TODO: decltype(auto) vs auto
        debug_log("AWAITER(coro): await_resume\n");
        return inner_->result();
      }

      [[nodiscard]] constexpr bool is_resumable() const noexcept override final { return true; }
    
      [[nodiscard]] bool is_inner_done() const override final { return inner_->done(); }

      void resume_inner() const override final { return inner_->resume(); }

      const void* inner_promise() override { return static_cast<const void*>(inner_); } // TODO: remove
      
    };

    coro_awaiter operator co_await() const noexcept {
      debug_log("MYCORO: co_await\n");
      return coro_awaiter(promise_ref_);
    }

  private:    
    const coro_promise_common<T>* const promise_ref_;
  };


  template<typename R>
  struct coro_promise_common {

    // Promise does not support value type std::exception_ptr. If it did then
    // the `result()` call could not be implemented the current way - The call
    // does a `std::get_if<std::exception_ptr>(...)` for checking if exception
    // occurred. This would always return a value if value-type was 
    // std::exception_ptr.
    static_assert(!std::is_same_v<R, std::exception_ptr>); 
    
    static constexpr bool return_type_is_void = std::is_same_v<R, void>;
    static constexpr bool return_type_is_reference = std::is_lvalue_reference_v<R>;
    
    //using value_type = std::conditional_t< return_type_is_void, std::monostate, 
    //    std::conditional_t<return_type_is_reference, std::reference_wrapper<std::remove_reference_t<R>>, R> >;
    using return_type_novoid = std::conditional_t<return_type_is_void, std::monostate, R>;
    using value_type = std::conditional_t<return_type_is_reference, 
        std::reference_wrapper<std::remove_reference_t<R>>, return_type_novoid>;

    // TODO: default constructors, etc

    coro_promise_common(std::coroutine_handle<void> handle)
    : current_awaiter_(nullptr),
      handle_(handle)
    {
      debug_log("PROMISE(common):() handle={} T={} is_lref={} is_void={}\n", _h(handle_), typeid(R).name(), return_type_is_reference, return_type_is_void);
    }

    void destroy() const {
      debug_log("PROMISE(common): destroy handle={} T={}\n", _h(handle_), typeid(R).name());
      // TODO: very bad throwing from destructor
      if (current_awaiter_ != nullptr) throw std::logic_error("Cannot destroy while current_awaiter_ is set");
      handle_.destroy();
    }

    ~coro_promise_common() {
      debug_log("PROMISE(common):~() handle={} T={}\n", _h(handle_), typeid(R).name());
    }

    // TODO: [[nodiscard]] for internally called functions initial_suspend, final_suspend, get_return_object

    constexpr std::suspend_never initial_suspend() const noexcept { return {}; }

    constexpr std::suspend_always final_suspend() const noexcept { return {}; }

    void unhandled_exception() noexcept {
      debug_log("PROMISE(common): unhandled_exception\n");
      value_ = std::current_exception();
    }

    coro<R> get_return_object() const noexcept {
      debug_log("PROMISE(common): get_return_object handle={} T={}\n", _h(handle_), typeid(R).name());
      return coro<R>(this);
    }

    void set_awaiter(polymorphic_awaiter* awaiter) /*TODO: noexcept*/ {
      debug_log("PROMISE(common): set_awaiter inner={} outer={}\n", 
          awaiter->inner_promise(), static_cast<const void*>(this));
      if (current_awaiter_ != nullptr) throw std::logic_error("set_awaiter can't be called nestedly");
      current_awaiter_ = awaiter;
    }

    [[nodiscard]] bool is_resumable() const noexcept {
      const void* inner = current_awaiter_ == nullptr? nullptr: current_awaiter_->inner_promise();
      debug_log("PROMISE(common): is_resumable inner={} outer={}\n", inner, static_cast<const void*>(this));
      if (current_awaiter_ != nullptr) {
        return current_awaiter_->is_resumable();
      } else {
        // Case 1: task type suspended after initial_suspend
        // Case 2: just came back from awaiter, continuing in outer promise
        return true;
      }
    }

    [[nodiscard]] bool done() const {
      const void* inner = current_awaiter_ == nullptr? nullptr: current_awaiter_->inner_promise();
      debug_log("PROMISE(common): done inner={} outer={}\n", inner, static_cast<const void*>(this));
      if (current_awaiter_ != nullptr) {
        if (current_awaiter_->is_inner_done()) {
          current_awaiter_ = nullptr; // NOTE: mutable needed for this point.
        } else {        
          return false;
        }
      }      
      return handle_.done(); // NOTE: handle_.done might except
    }

    void resume() const {
      const void* inner = current_awaiter_ == nullptr? nullptr: current_awaiter_->inner_promise();
      debug_log("PROMISE(common): resume inner={} outer={}\n", inner, static_cast<const void*>(this));
      if (current_awaiter_ != nullptr) {
        current_awaiter_->resume_inner();
      } else {
        handle_.resume(); // NOTE: handle_.resume might except
      }
    }
    
    /*[[nodiscard]]*/ std::conditional_t<return_type_is_void, void, const return_type_novoid&> result() const {
      // TODOP: auto vs decltype(auto)
      debug_log("PROMISE(common)::result T={}\n", typeid(R).name());
      const std::exception_ptr* exc = std::get_if<std::exception_ptr>(&value_);
      if (exc != nullptr) {
        std::rethrow_exception(*exc);
      }
      if constexpr (!return_type_is_void) {
        if constexpr (return_type_is_reference) {
          return std::get<value_type>(value_).get();
        } else {
          return std::get<value_type>(value_); // TODO: ok returning value? or should i return const value_type&
        }
      }
    }

  private:
    mutable polymorphic_awaiter* current_awaiter_; // TODO: raw pointer
    const std::coroutine_handle<void> handle_;
  protected:
    std::variant<std::monostate, value_type, std::exception_ptr> value_;
  };


  template<typename R>
  struct coro_promise final : public coro_promise_common<R>
  {
    coro_promise()
    : coro_promise_common<R>(std::coroutine_handle<coro_promise>::from_promise(*this))
    {}

    ~coro_promise() {
      debug_log("PROMISE(T): ~coro_promise<{}>\n", typeid(R).name());
    }

    /* TODO: do we need to call destructor since base type is non polymorphic? */
    
    template<std::convertible_to<R> ValueType>
    void return_value(ValueType&& value) noexcept(std::is_nothrow_constructible_v<R, ValueType&&>) { // TODO: convertible to R types
      //debug_log("PROMISE(T): return_value T={} value={}\n", typeid(R).name(), value);
      debug_log("PROMISE(T): return_value T={} value=??????\n", typeid(R).name());
      coro_promise_common<R>::value_ = R(std::forward<ValueType>(value));
    }
  };


  template<>
  struct coro_promise<void> final : public coro_promise_common<void>
  {
    coro_promise()
    : coro_promise_common<void>(std::coroutine_handle<coro_promise>::from_promise(*this))
    {}

    ~coro_promise() {
      debug_log("PROMISE(T): ~coro_promise<void>\n");
    }

    /* TODO: do we need to call destructor since base type is non polymorphic? */
      
    void return_void() {
      debug_log("PROMISE(T): return_void T=void\n");
    }
  };

}  // namespace async

#endif  // ASYNC__CORO_H_