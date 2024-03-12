#include "async/debug_log.h"

#include <coroutine>
#include <exception>

#include <cassert>
#include <concepts>
#include <iostream>
#include <map>

#include <variant>

#include <thread>
#include <chrono>

// TODO
#define NO_OLD_CHANGES 0


template<typename T>
struct mycoro_promise;

template<typename R>
struct mycoro_promise_common2;


struct polymorphic_awaiter
{
  [[nodiscard]]  virtual bool is_resumable() noexcept = 0;

  [[nodiscard]] virtual bool is_inner_done() = 0; // TODO: noexcept

  virtual void resume_inner() = 0;

  virtual ~polymorphic_awaiter() {}

  virtual const void* inner_promise() = 0; // TODO: remove
};




template<typename T = void>
struct [[nodiscard]] mycoro final
{
private:    
  //promise_type* promise_ref_;
  mycoro_promise_common2<T>* promise_ref_;

public:
  using promise_type = ::mycoro_promise<T>;
  using value_type = T;

  

  mycoro() = delete;

  //mycoro(promise_type* promise_ref)
  mycoro(mycoro_promise_common2<T>* promise_ref)
  : promise_ref_{promise_ref}
  {
    debug_log("MYCORO mycoro promise_ref_={}\n", (void*)promise_ref_);
  }

  mycoro(const mycoro&) = delete;

  mycoro(mycoro&& o) noexcept {
    this->promise_ref_ = o.promise_ref_;
    o.promise_ref_ = nullptr;
  }

  mycoro& operator=(const mycoro&) = delete;

  mycoro& operator=(mycoro&& o) {
    if (&o != this) {
      if (!o.promise_ref_) {
        throw std::logic_error("Coroutine from which we are assigning should have valid context");
      }
      assert(this->promise_ref_); // Lifecycle management of mycoro should ensure promise_ref_ to be valid.
      promise_ref_.destroy(); 
      promise_ref_ = o.promise_ref_;
      o.promise_ref_ = nullptr;
    }
  }

  ~mycoro() {
    // TODO: destroy
    debug_log("MYCORO ~mycoro promise_ref_={}\n", static_cast<void*>(promise_ref_));
#if NO_OLD_CHANGES
    if (promise_ref_) {
      auto handle = std::coroutine_handle<promise_type>::from_promise(*promise_ref_);
      std::cout << "MYCORO ~mycoro handle=" << handle << "\n";
      handle.destroy();
      promise_ref_ = nullptr;
    }
#endif
  }

  bool is_resumable() const {
    return promise_ref_->is_resumable();
  }

  bool done() const {
    return promise_ref_->done();
  }

  std::exception_ptr exception() const noexcept {
    return promise_ref_->exception();
  }

  void resume() const {
    promise_ref_->resume();
  }

#if 0
  decltype(auto) result() {
    return promise_ref_->result();
  }

  const auto result() const {
    return promise_ref_->result();
  }
#endif

  // TODO: Concept for CoroPromiseType
  struct mycoro_awaiter final : public polymorphic_awaiter
  {    
    mycoro_promise_common2<T>* const inner_;

    mycoro_awaiter() = delete;

    mycoro_awaiter(mycoro_promise_common2<T>* inner) noexcept
		: inner_(inner)
		{
      debug_log("AWAITER(mycoro): mycoro_awaiter inner={}\n", static_cast<void*>(inner_));
    }

    ~mycoro_awaiter() noexcept {
      debug_log("AWAITER(mycoro): ~mycoro_awaiter inner={}\n", static_cast<void*>(inner_));
    }

    bool await_ready() const noexcept {
      debug_log("AWAITER(mycoro): await_ready inner={}\n", static_cast<void*>(inner_));
      return inner_->done();
    }

    template<typename OuterPromise>
    void await_suspend(std::coroutine_handle<OuterPromise> handle_from_outer) noexcept {
      debug_log("AWAITER(mycoro): await_suspend inner={} outer={}\n", static_cast<void*>(inner_), _h(handle_from_outer));
      debug_log("AWAITER(mycoro): await_suspend outer.done={} inner.done={}\n", handle_from_outer.done(), inner_->done());
      handle_from_outer.promise().set_inner2(this);
    }
    
    decltype(auto) await_resume() {
      debug_log("AWAITER(mycoro): await_resume\n");
      return inner_->result();
    }

    [[nodiscard]] constexpr bool is_resumable() noexcept override { return true; }
  
    [[nodiscard]] bool is_inner_done() override { return inner_->done(); }

    void resume_inner() override { return inner_->resume(); }

    const void* inner_promise() override { return static_cast<const void*>(inner_); }
    
  };

  mycoro_awaiter operator co_await() const noexcept {
    debug_log("MYCORO: co_await\n");
    // TODO: is this really necessary?
    return mycoro_awaiter(promise_ref_);
  }
#if 0
  std::coroutine_handle<> get_handle() const noexcept {      
      return promise_ref_->handle();
  }
#endif
};



template<typename R>
struct mycoro_promise_common2 {

  static_assert(!std::is_same_v<R, std::exception_ptr>); // Promise does not support value type std::exception_ptr
  
  static constexpr bool return_type_is_void = std::is_same_v<R, void>;
  static constexpr bool return_type_is_reference = std::is_lvalue_reference_v<R>;
  
  using value_type = std::conditional_t< return_type_is_void, std::monostate, 
      std::conditional_t<return_type_is_reference, std::reference_wrapper<std::remove_reference_t<R>>, R> >;
  using return_type = R;

  polymorphic_awaiter* current_awaiter_; // TODO: raw pointer
  const std::coroutine_handle<void> handle_;
  std::variant<std::monostate, value_type, std::exception_ptr> value_;
  
  

  mycoro_promise_common2(std::coroutine_handle<void> handle)
  : current_awaiter_(nullptr),
    handle_(handle)
  {
    debug_log("PROMISE(T): mycoro_promise handle={} T={}\n", _h(handle_), typeid(R).name());
  }

  bool is_resumable() {
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

  /* TODO constexpr*/ std::suspend_never initial_suspend() const noexcept {
    debug_log("PROMISE(common): initial_suspend\n");
    return {};
  }
  /* TODO constexpr*/ std::suspend_always final_suspend() const noexcept {
    debug_log("PROMISE(common): final_suspend\n");
    return {};
  }
  void unhandled_exception() noexcept {
    debug_log("PROMISE(common): unhandled_exception\n");
    value_ = std::current_exception();    
  }
#if 0
  void set_inner(std::coroutine_handle<> inner) {
    debug_log("PROMISE(common): set_inner old={} new={}\n", _h(handle_), _h(inner));        
    inner_ = inner;
  }
#else
  void set_inner2(polymorphic_awaiter* awaiter) {
    debug_log("PROMISE(common): set_inner2 inner={} outer={}\n", 
        awaiter->inner_promise(), static_cast<const void*>(this));
    current_awaiter_ = awaiter;
  }
#endif


#if 0
  std::coroutine_handle<> handle() const noexcept {
    return inner_? inner_ : handle_;
  }

  bool done()
  {
    debug_log("PROMISE(common): done handle={}\n", _h(handle_));
    if (inner_) {
      if (inner_.done()) {
        inner_ = std::coroutine_handle<>();
      } else {
        return false;
      }
    }
    return handle_ ? handle_.done() : false;
  }
  void resume() const
  {
    debug_log("PROMISE(common): resume\n");

    if (!handle_) { // TODO: remove this. Constructors guarantee this is always present
      throw std::logic_error("Coroutine handle for promise should be set before awaiting or resuming");
    }
    if (inner_) {
      inner_.resume();
    } else {
      handle_.resume();
    }
  }
#else
  bool done()
  {
    const void* inner = current_awaiter_ == nullptr? nullptr: current_awaiter_->inner_promise();
    debug_log("PROMISE(common): done inner={} outer={}\n", inner, static_cast<const void*>(this));
    if (current_awaiter_ != nullptr) {
      if (current_awaiter_->is_inner_done()) {
        current_awaiter_ = nullptr;
      } else {        
        return false;
      }
    }
    assert(handle_); // Handle should never be null at this point
    return handle_.done();
  }
  void resume() const
  {
    const void* inner = current_awaiter_ == nullptr? nullptr: current_awaiter_->inner_promise();
    debug_log("PROMISE(common): resume inner={} outer={}\n", inner, static_cast<const void*>(this));
    if (current_awaiter_ != nullptr) {
      current_awaiter_->resume_inner();
    } else {      
      assert(handle_); // Handle should never be null at this point
      handle_.resume();
    }
  }
#endif
  

  mycoro<return_type> get_return_object() noexcept
  {
    debug_log("PROMISE(T): get_return_object handle={} T={}\n", _h(handle_), typeid(R).name());
    return mycoro<return_type>(this);
  }

  auto result()
  {   std::exception_ptr* exc = std::get_if<std::exception_ptr>(&value_);
      if (exc != nullptr) {
        std::rethrow_exception(*exc);
      }      
      if constexpr (!return_type_is_void) {
        if constexpr (return_type_is_reference) {
          return std::get<value_type>(value_).value();
        } else {
          return std::get<value_type>(value_); // TODO: ok returning value? or should i return const value_type&
        }
      }
  }

  

};

template<typename R>
struct mycoro_promise final : public mycoro_promise_common2<R>
{
  mycoro_promise()
  : mycoro_promise_common2<R>(std::coroutine_handle<mycoro_promise>::from_promise(*this))
  {}

  /* TODO: do we need to call destructor since base type is non polymorphic? */
    
  void return_value(R value) // TODO: convertible to R types
  {
    debug_log("PROMISE(T): return_value T={} value={}\n", typeid(R).name(), value);
    mycoro_promise_common2<R>::value_ = value; // TODO: assignment optimisations? maybe move when large value?
  }
};

template<>
struct mycoro_promise<void> final : public mycoro_promise_common2<void>
{
  mycoro_promise()
  : mycoro_promise_common2<void>(std::coroutine_handle<mycoro_promise>::from_promise(*this))
  {}

  /* TODO: do we need to call destructor since base type is non polymorphic? */
    
  void return_void()
  {
    debug_log("PROMISE(T): return_void handle={} T=void\n", _h(handle_));
  }
};


#if 0
struct mycoro_promise_common {

  /* TODO constexpr*/ std::suspend_never initial_suspend() const noexcept {
    debug_log("PROMISE(common): initial_suspend\n");
    return {};
  }

  /* TODO constexpr*/ std::suspend_always final_suspend() const noexcept {
    debug_log("PROMISE(common): final_suspend\n");
    return {};
  }

  void unhandled_exception() noexcept {
    debug_log("PROMISE(common): unhandled_exception\n");
    exception_ = std::current_exception();
    state_ = promise_state::eException;
  }

  void set_inner(std::coroutine_handle<> inner) {
    debug_log("PROMISE(common): set_inner old={} new={}\n", _h(handle_), _h(inner));        
    inner_ = inner;
  }

  std::coroutine_handle<> handle() const noexcept {
    return inner_? inner_ : handle_;
  }


  bool done() {
    debug_log("PROMISE(common): done handle={}\n", _h(handle_));
    if (inner_) {
      if (inner_.done()) {
        inner_ = std::coroutine_handle<>();
      } else {
        return false;
      }
    }
    return handle_ ? handle_.done() : false;
  }

  void resume() const {
    debug_log("PROMISE(common): resume\n");

    if (!handle_) { // TODO: remove this. Constructors guarantee this is always present
      throw std::logic_error("Coroutine handle for promise should be set before awaiting or resuming");
    }
    if (inner_) {
      inner_.resume();
    } else {
      handle_.resume();
    }
  }

  enum class promise_state { eEmpty, eValue, eException };
  promise_state state_ = promise_state::eEmpty;
  std::exception_ptr exception_ = nullptr;
  std::coroutine_handle<> handle_ = nullptr;
  std::coroutine_handle<> inner_ = nullptr;
};
 
template<typename T>
struct mycoro_promise final : public mycoro_promise_common
{  
  union {
    T value_;
    char unused_;
  };

  mycoro_promise()
  {
    handle_ = std::coroutine_handle<mycoro_promise>::from_promise(*this);
    debug_log("PROMISE(T): mycoro_promise handle={} T={}\n", _h(handle_), typeid(T).name());
  }

  ~mycoro_promise() {
    debug_log("PROMISE(T): ~mycoro_promise\n");
    if (state_ == promise_state::eValue) {
      value_.~T();
    } else if (state_ == promise_state::eException) {
      exception_.~exception_ptr();
    }
  }
  
  mycoro<T> get_return_object() noexcept {
    //using promise_type = mycoro_promise<T>;
    //auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
    debug_log("PROMISE(T): get_return_object handle={} T={}\n", _h(handle_), typeid(T).name());
    return mycoro<T>(this);
  }  

  template<std::convertible_to<T> ValueType>
  void return_value(ValueType&& e) noexcept(std::is_nothrow_constructible_v<T, ValueType&&>) {
    // TODO: worry about alignment of T
    debug_log("PROMISE(T): get_return_object handle={} T={} value={}\n", _h(handle_), typeid(T).name(), e);
    ::new (static_cast<void*>(&value_)) T(std::forward<ValueType>(e));
    state_ = promise_state::eValue;
  }

  T& result() {
    if (state_ == promise_state::eException) std::rethrow_exception(exception_);
    return value_;
  }
};

template<>
struct mycoro_promise<void> : public mycoro_promise_common
{

  mycoro_promise()
  {
    handle_ = std::coroutine_handle<mycoro_promise>::from_promise(*this);
    debug_log("PROMISE(T): mycoro_promise handle={} T=void\n", _h(handle_));
  }

  ~mycoro_promise() {
    debug_log("PROMISE(T): ~mycoro_promise handle={} T=void\n", _h(handle_));
    if (state_ == promise_state::eException) {
      exception_.~exception_ptr();
    }
  }
  
  mycoro<void> get_return_object() noexcept {
    //using promise_type = mycoro_promise<void>;
    //auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
    debug_log("PROMISE(T): get_return_object handle={} T=void\n", _h(handle_));
    
    return mycoro<void>(this);
  }

  void return_void() {
    debug_log("PROMISE(T): return_void handle={} T=void\n", _h(handle_));
  }

  void result() {
    if (state_ == promise_state::eException) std::rethrow_exception(exception_);
  }
};


template<typename T>
struct mycoro_promise<T&> : public mycoro_promise_common
{
  T* value_ref_ = nullptr;
  
  mycoro<T&> get_return_object() noexcept { 
    using promise_type = mycoro_promise<T&>;
    return { std::coroutine_handle<promise_type>::from_promise(*this) };
  }

  void return_value(T& e) noexcept {
    value_ref_ = &e;
  }

  T& result() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
    return *value_ref_;
  }
};
#endif










struct my_awaiter
{
  bool await_ready() const noexcept { 
    debug_log("my_awaiter::await_ready\n");
    return false; 
  }
  
  void await_suspend(std::coroutine_handle<>) const noexcept {
    debug_log("my_awaiter::await_suspend\n");
  }

  void await_resume() const noexcept {
    debug_log("my_awaiter::await_resume\n");
  }
};





mycoro<int> coro_example(int loops) {  
  debug_log("START coro_example\n");
  for (int i = 0; i < loops; ++i) {
    debug_log("coro_example i={}\n", i);
    co_await my_awaiter{};
  }
  debug_log("END coro_example\n");
  co_return 13;
}

mycoro<void> coro_void_example(int loops) {  
  debug_log("START coro_void_example\n");
  for (int i = 0; i < loops; ++i) {
    debug_log("coro_void_example i={}\n", i);
    co_await std::suspend_always{};
  }
  debug_log("END coro_void_example\n");
  co_return;
}


mycoro<void> coro_outer() {
  debug_log("START coro_outer\n");  
  int result = co_await coro_example(3);
  debug_log("coro_outer - result={}\n", result);
  int result2 = co_await coro_example(5);
  debug_log("coro_outer - result2={}\n", result2);
  debug_log("END coro_outer\n");
  co_return;
}


template<typename T>
void run_continuation(mycoro<T>&& continuation) {
  debug_log("::run - begin\n");
  while (true) {    
    //debug_log("::run - loop coro={}\n", _h(continuation.get_handle()));
    if (continuation.done()) break;
    debug_log("::run - before resume\n");
    if (continuation.is_resumable()) {
      continuation.resume();
    } else {
      debug_log("::run - will sleep for 10ms\n");
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }    
    debug_log("::run - after resume\n");
  }
}


int main() {

  debug_log("START main\n");

  // Lifecycle type - 1
  /*
  {
    auto continuation = coro_example(3);
    run_continuation(std::move(continuation));
  }
  */

  // Lifecycle type - 2
  run_continuation(coro_outer());

  // Lifecycle type - 3
  /*
  auto continuation2 = coro_example(5);
  run_continuation(std::move(continuation2));
  */


  debug_log("END main\n");


  return EXIT_SUCCESS;
}
