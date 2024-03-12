#ifndef ASYNC__DEBUG_LOG_H_
#define ASYNC__DEBUG_LOG_H_


#ifdef ASYNC_NO_DEBUG_LOG
#  define debug_log(...)
#else
#  include <fmt/core.h>
#  include <concepts>
#  include <coroutine>


#  define debug_log(...)  fmt::print(__VA_ARGS__)

template<typename P>
static inline
const char* _h(const std::coroutine_handle<P>& obj) {
  static char str[256];
  if constexpr (std::same_as<std::coroutine_handle<P>, std::coroutine_handle<void>>) {
    snprintf(str, 255, "{&=%p,p=void,a=%p}", static_cast<const void*>(&obj), static_cast<const void*>(obj.address()));
  } else {        
    snprintf(str, 255, "{&=%p,p=%p,a=%p}", 
      static_cast<const void*>(&obj), static_cast<const void*>(&obj.promise()), static_cast<const void*>(obj.address()));
  }
  return str;
}
#endif

#endif  // ASYNC__DEBUG_LOG_H_