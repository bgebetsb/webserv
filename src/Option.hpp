#pragma once

template < typename T >
class Option {
 public:
  Option();
  explicit Option(const T& value);
  Option(const Option& other);
  ~Option();

  bool is_some() const;
  bool is_none() const;
  T unwrap() const;

  bool operator<(const Option& other) const;

 private:
  T value_;
  bool is_some_;
};

#include "Option.tpp"
