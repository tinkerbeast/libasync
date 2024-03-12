#include "async/coro.h"
#include "async/runner.h"

#include <iostream>

#include <semaphore>

#if 0
namespace async {


  class Runner




  struct semaphore_awaiter final : public polymorphic_awaiter {

    std::counting_semaphore<1> sem_;
    bool is_resumable_;

    semaphore_awaiter()
    : sem_(0),
      is_resumable_(false)
    {}

    [[nodiscard]] bool is_resumable() const noexcept override final {
      return is_resumable_
    }

    [[nodiscard]] void make_resumable() const noexcept override final {
      sem_.release();
      is_resumable_ = true;
    }

    [[nodiscard]] constexpr virtual bool is_inner_done() const override final { return true; }

    constexpr void resume_inner() const override final { }

    constexpr const void* inner_promise() override final { return nullptr; }

    bool await_ready() noexcept {
      bool acquired = sem_.try_acquire();
      return acquired;
    }

    template<typename OuterPromise>
    void await_suspend(std::coroutine_handle<OuterPromise> handle_from_outer) {
      handle_from_outer.promise().set_awaiter(this);
    }

    void await_resume() noexcept {
      sem_.acquire();
    }

  };
}
#endif

async::coro<int> coro_example(int loops) {
  int sum = 0;
  std::cout << "START coro_example\n";
  for (int i = 0; i < loops; ++i) {
    std::cout << "coro_example i=" << i << "\n";
    co_await std::suspend_always{};
    sum += i;
  }
  std::cout << "END coro_example\n";
  co_return sum;
}


async::coro<double> coro_example_outer() {
  double half_sum = 0;
  std::cout << "START coro_example_outer\n";

  int result1 = co_await coro_example(3);
  half_sum += result1 / 2.0;
  std::cout << "coro_example_outer result1=" << result1 << "\n";

  int result2 = co_await coro_example(4);
  half_sum += result2 / 2.0;
  std::cout << "coro_example_outer result2=" << result2 << "\n";

  std::cout << "END coro_example_outer\n";
  co_return half_sum;
}


async::coro<void> coro_example_outer_outer() {
  std::cout << "START coro_example_outer_outer\n";
  double d = co_await coro_example_outer();
  std::cout << "coro_example_outer_outer d=" << d << "\n";
  int i = co_await coro_example(2);
  std::cout << "coro_example_outer_outer i=" << i << "\n";
  co_return;
}



struct Abc {
  Abc() {
    std::cout << "Abc()" << " this=" << this << std::endl;
  }

  ~Abc() {
    std::cout << "~Abc()" << " this=" << this << std::endl;
  }

  Abc(const Abc&) {
    std::cout << "Abc(Abc&)" << " this=" << this << std::endl;
  }

  Abc(Abc&& o) {
    std::cout << "Abc(Abc&&)" << " this=" << this << " o=" << &o << std::endl;
  }

  Abc& operator=(const Abc&) {
    std::cout << "operator=(Abc&)" << " this=" << this << std::endl;
    return *this;
  }

  Abc& operator=(Abc&&) {
    std::cout << "operator=(Abc&&)" << " this=" << this << std::endl;
    return *this;
  }
};


Abc return_rvalue() {

  return Abc{};
}

#include <string.h>


int main() {

    async::run_noreturn([]() -> async::coro<Abc> {
      co_return Abc{};
    }());



    /*auto result = */ //async::run(coro_example_outer());
    //std::cout << "main result=" << result << std::endl;
    return EXIT_SUCCESS;
}
