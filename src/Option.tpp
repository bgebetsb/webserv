#pragma once

#include <stdexcept>
#include "Option.hpp"

template < typename T >
Option< T >::Option() : value_(T()), is_some_(false)
{}

template < typename T >
Option< T >::Option(const T& value) : value_(value), is_some_(true)
{}

template < typename T >
Option< T >::Option(const Option& other)
    : value_(other.value_), is_some_(other.is_some_)
{}

template < typename T >
Option< T >::~Option()
{}

template < typename T >
bool Option< T >::is_some() const
{
  return is_some_;
}

template < typename T >
bool Option< T >::is_none() const
{
  return !is_some_;
}

template < typename T >
T Option< T >::unwrap() const
{
  if (!is_some_)
  {
    throw std::runtime_error("Called unwrap on None value");
  }
  return value_;
}

template < typename T >
bool Option< T >::operator<(const Option& other) const
{
  if (other.is_none())
  {
    return false;
  }

  return is_none() || (unwrap() < other.unwrap());
}
