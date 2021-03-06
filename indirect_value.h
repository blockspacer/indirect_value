#ifndef ISOCPP_P1950_INDIRECT_VALUE_H
#define ISOCPP_P1950_INDIRECT_VALUE_H

#include <memory>
#include <utility>
#include <type_traits>

namespace isocpp_p1950 {

template <class T>
struct default_copy {
  T* operator()(const T& t) const { return new T(t); }
};

template <class T, class C = default_copy<T>, bool CanBeEmptyBaseClass = std::is_empty_v<C> && !std::is_final_v<C> >
class indirect_value_base {
protected:
  template<class U = C, class = std::enable_if_t<std::is_default_constructible_v<U>>>
  indirect_value_base() noexcept(noexcept(C())) {}
  indirect_value_base(C c) : c_(std::move(c)) {}
  C& get() noexcept { return c_; }
  const C& get() const noexcept { return c_; }
  C c_;
};

template <class T, class C>
class indirect_value_base<T, C, true> : private C {
protected:
  template<class U=C, class = std::enable_if_t<std::is_default_constructible_v<U>>>
  indirect_value_base() noexcept(noexcept(C())) {}
  indirect_value_base(C c) : C(std::move(c)) {}
  C& get() noexcept { return *this; }
  const C& get() const noexcept { return *this; }
};

template <class T, class C = default_copy<T>, class D = std::default_delete<T>>
class indirect_value : private indirect_value_base<T, C> {
  
  using base = indirect_value_base<T, C>;
  
  std::unique_ptr<T, D> ptr_;

 public:
  indirect_value() = default;

  template <class... Ts>
  explicit indirect_value(std::in_place_t, Ts&&... ts) {
    ptr_ = std::unique_ptr<T, D>(new T(std::forward<Ts>(ts)...), D{});
  }

  explicit indirect_value(T* t, C c = C{}, D d = D{}) noexcept : base(std::move(c)), ptr_(std::unique_ptr<T, D>(t, std::move(d))) {
  }

  explicit indirect_value(const indirect_value& i) : base(get_c()) {
    if (i.ptr_) { 
      ptr_ = std::unique_ptr<T, D>(get_c()(*i.ptr_), D{});
    }
  }

  explicit indirect_value(indirect_value&& i) noexcept : base(std::move(i)), ptr_(std::exchange(i.ptr_, nullptr)) {}

  indirect_value& operator = (const indirect_value& i) {
    base::operator=(i);
    if (i.ptr_) { 
      if (!ptr_){
        ptr_ = std::unique_ptr<T, D>(get_c()(*i.ptr_), D{});
      }
      else{
        ptr_.reset( get_c()(*i.ptr_) );
      }
    }
    return *this;
  }

  indirect_value& operator = (indirect_value&& i) noexcept {
    base::operator=(std::move(i));
    ptr_ = std::exchange(i.ptr_, nullptr);
    return *this;
  }

  ~indirect_value() = default;

  T* operator->() noexcept { return ptr_.operator->(); }

  const T* operator->() const noexcept { return ptr_.operator->(); }

  T& operator*() { return *ptr_; }

  const T& operator*() const { return *ptr_; }

  explicit constexpr operator bool() const noexcept { return ptr_ != nullptr; }

  friend void swap(indirect_value& lhs, indirect_value& rhs) {
    using std::swap;
    swap(lhs.get_c(), rhs.get_c());
    swap(lhs.ptr_, rhs.ptr_);
  }

  private:
    C& get_c() noexcept { return base::get(); }
    const C& get_c() const noexcept { return base::get(); }
};

}  // namespace isocpp_p1950

#endif  // ISOCPP_P1950_INDIRECT_VALUE_H
