#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <optional>

namespace principia {
namespace astronomy {
namespace _date_time {

// Declared in _date_time, rather than in the internal namespace and
// re-exported, so that we can refer to it as date_time::Calendar when it is
// hidden by Date::Calendar below.
enum class Calendar {
  Julian = 'J',
  Gregorian = 'G',
};

namespace internal {

// Represents a calendar day.
class Date final {
 public:
  // The calendar is Gregorian by default starting in 1583; before that,
  // `calendar` is required.
  static constexpr Date YYYYMMDD(
      std::int64_t digits,
      std::optional<Calendar> calendar = std::nullopt);
  // The calendar is Gregorian by default starting in 1583; before that,
  // `calendar` is required.
  static constexpr Date YYYYDDD(
      std::int64_t digits,
      std::optional<Calendar> calendar = std::nullopt);
  // Since the calendar week number is an ISO 8601 construct, the year must be
  // at least 1583, and the calendar is Gregorian.
  static constexpr Date YYYYwwD(std::int64_t digits);

  // The calendar is Gregorian by default starting in 1583; before that,
  // `calendar` is required.
  static constexpr Date Calendar(
      int year, int month, int day,
      std::optional<Calendar> calendar = std::nullopt);
  // The calendar is Gregorian by default starting in 1583; before that,
  // `calendar` is required.
  static constexpr Date Ordinal(
      int year, int day,
      std::optional<_date_time::Calendar> calendar = std::nullopt);
  // Since the calendar week number is an ISO 8601 construct, the year must be
  // at least 1583, and the calendar is Gregorian.
  static constexpr Date Week(int year, int week, int day);

  // The julian date `jd` must not be negative; its fractional part must be one
  // half, corresponding to 00:00 on the resulting `Date`.
  static constexpr Date JD(double jd);

  constexpr int year() const;
  constexpr int month() const;
  constexpr int day() const;
  constexpr _date_time::Calendar calendar() const;

  constexpr int ordinal() const;

  // The fractional part of `jd()` is one half, corresponding to 00:00 on the
  // day represented by this object.
  constexpr double jd() const;
  constexpr int mjd() const;

  constexpr Date next_day() const;

 private:
  constexpr Date(int year, int month, int day,
                 _date_time::Calendar const calendar);

  int year_;
  int month_;
  int day_;
  _date_time::Calendar calendar_;
};

class TimeOfDay final {
 public:
  // Checks that this represents a valid time of day as per ISO 8601, thus
  // that the components are in the normal range, or that the object represents
  // a time in a leap second, or that it represents the end of the day.
  constexpr TimeOfDay(int hour, int minute, int second, int millisecond);

  static constexpr TimeOfDay hhmmss_ms(int hhmmss, int ms);

  constexpr int hour() const;
  constexpr int minute() const;
  constexpr int second() const;
  constexpr int millisecond() const;

  constexpr bool is_leap_second() const;
  // Whether `*this` is 24:00:00.
  constexpr bool is_end_of_day() const;

 private:
  int const hour_;
  int const minute_;
  int const second_;
  int const millisecond_;

  friend class TimeParser;
};

class DateTime final {
 public:
  // Checks that `time` does not represent a leap second unless `date` is the
  // last day of the month.
  constexpr DateTime(Date date, TimeOfDay time);

  static constexpr DateTime BeginningOfDay(Date const& date);

  constexpr Date const& date() const;
  constexpr TimeOfDay const& time() const;

  // If `time()` is 24:00:00, returns an equivalent DateTime where midnight is
  // expressed as 00:00:00 on the next day; otherwise, returns `*this`.
  constexpr DateTime normalized_end_of_day() const;

 private:
  Date const date_;
  TimeOfDay const time_;

  friend constexpr DateTime operator""_DateTime(char const* str,
                                                std::size_t size);
};

class JulianDate final {
 public:
  static constexpr JulianDate JD(std::int64_t digits,
                                 std::int64_t digit_count,
                                 std::int64_t fractional_digit_count);
  static constexpr JulianDate MJD(std::int64_t digits,
                                  std::int64_t digit_count,
                                  std::int64_t fractional_digit_count);

  constexpr std::int64_t day() const;
  constexpr std::int64_t fraction_numerator() const;
  constexpr std::int64_t fraction_denominator() const;

 private:
  constexpr JulianDate(std::int64_t day,
                       std::int64_t fraction_numerator,
                       std::int64_t fraction_denominator);

  // These numbers are relative to J2000.  `fraction_denominator` is a positive
  // power of 10.
  std::int64_t const day_;
  std::int64_t const fraction_numerator_;
  std::int64_t const fraction_denominator_;
};

// Dates are ordered chronologically; equivalent dates in the Julian and
// Gregorian calendars compare equal.
constexpr bool operator==(Date const& left, Date const& right);
constexpr bool operator!=(Date const& left, Date const& right);
constexpr auto operator<=>(Date const& left, Date const& right);
constexpr Date operator""_Date(char const* str, std::size_t size);
std::ostream& operator<<(std::ostream& out, Date const& date);

constexpr bool operator==(TimeOfDay const& left, TimeOfDay const& right);
constexpr bool operator!=(TimeOfDay const& left, TimeOfDay const& right);
constexpr TimeOfDay operator""_Time(char const* str, std::size_t size);
std::ostream& operator<<(std::ostream& out, TimeOfDay const& time);

// 24:00 on one day and 00:00 on the next compare equal.
constexpr bool operator==(DateTime const& left, DateTime const& right);
constexpr bool operator!=(DateTime const& left, DateTime const& right);
constexpr DateTime operator""_DateTime(char const* str, std::size_t size);
std::ostream& operator<<(std::ostream& out, DateTime const& date_time);

// Returns true if the string can be interpreted as a Julian date.
constexpr bool IsJulian(char const* str, std::size_t size);
constexpr JulianDate operator""_Julian(char const* str, std::size_t size);

}  // namespace internal

using internal::Date;
using internal::DateTime;
using internal::IsJulian;
using internal::JulianDate;
using internal::operator""_Date;
using internal::operator""_DateTime;
using internal::operator""_Julian;
using internal::operator""_Time;
using internal::TimeOfDay;
namespace date_time = _date_time;

}  // namespace _date_time
}  // namespace astronomy
}  // namespace principia

#include "astronomy/date_time_body.hpp"
