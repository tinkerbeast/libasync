#ifndef ASYNC__POLYMORPHIC_AWAITER_H_
#define ASYNC__POLYMORPHIC_AWAITER_H_

#include <atomic>

#include <cassert>

namespace async {

  struct polymorphic_awaiter {
    [[nodiscard]] virtual bool is_resumable() const noexcept = 0;
    [[nodiscard]] virtual bool is_inner_done() const = 0;
    virtual void resume_inner() const = 0;
    virtual const void* inner_promise() = 0; // TODO: remove
    virtual ~polymorphic_awaiter() {}
  };
 
}  // namespace async

#endif  // ASYNC__POLYMORPHIC_AWAITER_H_
