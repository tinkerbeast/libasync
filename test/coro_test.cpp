#include "async/coro.h"
#include "async/runner.h"

#include "gtest/gtest.h"

namespace async::coro_test {

  int xyz_default_constructor = 0;
  int xyz_copy_constructor = 0;
  int xyz_move_constructor = 0;
  int xyz_destructor = 0;
  int xyz_copy_assignment = 0;
  int xyz_move_assignment = 0;

  void xyz_reset() {
    xyz_default_constructor = 0;
    xyz_copy_constructor = 0;
    xyz_move_constructor = 0;
    xyz_destructor = 0;
    xyz_copy_assignment = 0;
    xyz_move_assignment = 0;
  }

  struct Xyz {

    Xyz() { ++xyz_default_constructor; }

    ~Xyz() { ++xyz_destructor; }

    Xyz(const Xyz&) { ++xyz_copy_constructor; }

    Xyz(Xyz&&) { ++xyz_move_constructor; }

    Xyz& operator=(const Xyz&) {
      ++xyz_copy_assignment;
      return *this;
    }

    Xyz& operator=(Xyz&&) {
      ++xyz_move_assignment;
      return *this;
    }
  };


TEST(coro_test, destructor_on_lvalue)
{  
  xyz_reset();
  {    
    async::run_noreturn([]() -> async::coro<Xyz> {
      co_return Xyz{};
    }());
  }
  int constructs = (xyz_default_constructor + xyz_copy_constructor + xyz_move_constructor);
  ASSERT_EQ(constructs, xyz_destructor);
}

TEST(coro_test, destructor_on_reference)
{
  xyz_reset();
  {
    Xyz obj;
    async::run_noreturn([&]() -> async::coro<Xyz&> {
      co_return obj;
    }());
  }
  int constructs = (xyz_default_constructor + xyz_copy_constructor + xyz_move_constructor);
  ASSERT_EQ(constructs, xyz_destructor);
}

}