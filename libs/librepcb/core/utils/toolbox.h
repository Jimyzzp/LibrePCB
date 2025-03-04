/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBREPCB_CORE_TOOLBOX_H
#define LIBREPCB_CORE_TOOLBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../types/angle.h"
#include "../types/length.h"
#include "../types/point.h"

#include <type_traits>

#include <QtCore>
#include <QtGui>

#include <algorithm>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Toolbox
 ******************************************************************************/

/**
 * @brief The Toolbox class provides some useful general purpose methods
 */
class Toolbox final {
  Q_DECLARE_TR_FUNCTIONS(Toolbox)

public:
  // Constructors / Destructor
  Toolbox() = delete;
  Toolbox(const Toolbox& other) = delete;
  ~Toolbox() = delete;

  // Operator Overloadings
  Toolbox& operator=(const Toolbox& rhs) = delete;

  // Static Methods

  /**
   * @brief Helper method to convert a QList<T> to a QSet<T>
   *
   * Until Qt 5.13, QList::toSet() was the way to do this conversion. But since
   * Qt 5.14 this is deprecated, and a range constructor was added to QSet
   * instead. This wrapper chooses the proper method depending on the Qt
   * version to avoid raising any deprecation warnings.
   *
   * @param list  The QList to be converted.
   *
   * @return The created QSet.
   */
  template <typename T>
  static QSet<T> toSet(const QList<T>& list) noexcept {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return QSet<T>(list.begin(), list.end());
#else
    return list.toSet();
#endif
  }

  template <typename T>
  static QList<T> sortedQSet(const QSet<T>& set) noexcept {
    QList<T> list = set.values();
    std::sort(list.begin(), list.end());
    return list;
  }

  template <typename T>
  static T sorted(const T& container) noexcept {
    T copy(container);
    std::sort(copy.begin(), copy.end());
    return copy;
  }

  /**
   * @brief Sort a container of arbitrary objects using QCollators numeric mode
   *
   * @param container           A string container as supported by QCollator.
   * @param compare             Custom comparison function with the signature
   *                            `bool(const QCollator& const V&, const V&)`
   *                            where `V` represents the container item type.
   * @param caseSensitivity     Case sensitivity of comparison.
   * @param ignorePunctuation   Whether punctuation is ignored or not.
   */
  template <typename T, typename Compare>
  static void sortNumeric(
      T& container, Compare compare,
      Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive,
      bool ignorePunctuation = false) noexcept {
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(caseSensitivity);
    collator.setIgnorePunctuation(ignorePunctuation);
    std::sort(container.begin(), container.end(),
              [&collator, &compare](const typename T::value_type& lhs,
                                    const typename T::value_type& rhs) {
                return compare(collator, lhs, rhs);
              });
  }

  /**
   * @brief Sort a container of strings using QCollators numeric mode
   *
   * @param container           A string container as supported by QCollator.
   * @param caseSensitivity     Case sensitivity of comparison.
   * @param ignorePunctuation   Whether punctuation is ignored or not.
   */
  template <typename T>
  static void sortNumeric(
      T& container, Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive,
      bool ignorePunctuation = false) noexcept {
    return sortNumeric(
        container,
        [](const QCollator& collator, const typename T::value_type& lhs,
           const typename T::value_type& rhs) { return collator(lhs, rhs); },
        caseSensitivity, ignorePunctuation);
  }

  /**
   * @brief Check if a text with a given rotation is considered as upside down
   *
   * A text is considered as upside down if it is rotated counterclockwise by
   * [-90..90°[, i.e. -90° is considered as upside down, but 90° is *not*
   * considered as upside down. For mirrored texts (rotated clockwise), it
   * is the other way around. Used to determine whether a text needs to be
   * auto-rotated or not.
   *
   * @param rotation  The global (scene coordinates) rotation of the text.
   * @param mirrored  If the text is mirrored (inverted rotation!).
   * @return Whether the text is upside down or not.
   */
  static bool isTextUpsideDown(const Angle& rotation, bool mirrored) noexcept;

  static QRectF boundingRectFromRadius(qreal radius) noexcept {
    return QRectF(-radius, -radius, 2 * radius, 2 * radius);
  }

  static QRectF boundingRectFromRadius(qreal rx, qreal ry) noexcept {
    return QRectF(-rx, -ry, 2 * rx, 2 * ry);
  }

  static QRectF adjustedBoundingRect(const QRectF& rect,
                                     qreal offset) noexcept {
    return rect.adjusted(-offset, -offset, offset, offset);
  }

  static QPainterPath shapeFromPath(
      const QPainterPath& path, const QPen& pen, const QBrush& brush,
      const UnsignedLength& minWidth = UnsignedLength(0)) noexcept;

  static Length arcRadius(const Point& p1, const Point& p2,
                          const Angle& a) noexcept;
  static Point arcCenter(const Point& p1, const Point& p2,
                         const Angle& a) noexcept;

  /**
   * @brief Calculate the angle between two given points
   *
   * @param p1      First point.
   * @param p2      Second point.
   * @param center  Center of the arc. Defaults to (0, 0).
   * @return Angle counter-clockwise from p1 to p2 (0..360°).
   *         Zero if it could not be determined.
   */
  static Angle arcAngle(const Point& p1, const Point& p2,
                        const Point& center = Point(0, 0)) noexcept;

  /**
   * @brief Calculate the point on a given line which is nearest to a given
   * point
   *
   * @param p         An arbitrary point
   * @param l1        Start point of the line
   * @param l2        End point of the line
   *
   * @return Nearest point on the given line (either l1, l2, or a point between
   * them)
   *
   * @warning This method works with floating point numbers and thus the result
   * may not be perfectly precise.
   */
  static Point nearestPointOnLine(const Point& p, const Point& l1,
                                  const Point& l2) noexcept;

  /**
   * @brief Calculate the shortest distance between a given point and a given
   * line
   *
   * @param p         An arbitrary point
   * @param l1        Start point of the line
   * @param l2        End point of the line
   * @param nearest   If not `nullptr`, the nearest point is returned here
   *
   * @return Shortest distance between the given point and the given line (>=0)
   */
  static UnsignedLength shortestDistanceBetweenPointAndLine(
      const Point& p, const Point& l1, const Point& l2,
      Point* nearest = nullptr) noexcept;

  /**
   * @brief Copy a string while incrementing its contained number
   *
   * - If the string contains one or more numbers, the last one gets incremented
   * - If it does not contain a number, a "1" is appended instead
   *
   * This way, the returned number is guaranteed to be different from the input
   * string. That's useful for example to generate unique, incrementing pin
   * numbers like "X1", "X2", "X3" etc.
   *
   * @param string  The input string
   * @return A new string with the incremented number
   */
  static QString incrementNumberInString(QString string) noexcept;

  /**
   * @brief Expand ranges like "1..5" in a string to all its values
   *
   * A range is either defined by two integers with ".." in between, or two
   * ASCII letters with ".." in between. If multiple ranges are contained, all
   * combinations of them will be created.
   *
   * For example the string "X1..10_A..C" expands to the list ["X1_A", "X1_B",
   * "X1_C", ..., "X10_C"].
   *
   * @note  Minus ('-') and plus ('+') characters are not interpreted as the
   *        sign of a number because in EDA tools they often are considered as
   *        strings, not as number signs (e.g. the inputs of an OpAmp).
   *
   * @param string The input string (may or may not contain ranges).
   * @return A list with expanded ranges in all combinations. If the input
   *         string does not contain ranges, a list with one element (equal to
   *         the input) is returned.
   */
  static QStringList expandRangesInString(const QString& string) noexcept;

  /**
   * @brief Clean a user input string
   *
   * @param input             The string typed by the user
   * @param removeRegex       Regex for all patterns to remove from the string
   * @param trim              If true, leading and trailing spaces are removed
   * @param toLower           If true, all characters are converted to lowercase
   * @param toUpper           If true, all characters are converted to uppercase
   * @param spaceReplacement  All spaces are replaced by this string
   * @param maxLength         If >= 0, the string is truncated to this length
   *
   * @return The cleaned string (may be empty)
   */
  static QString cleanUserInputString(const QString& input,
                                      const QRegularExpression& removeRegex,
                                      bool trim = true, bool toLower = false,
                                      bool toUpper = false,
                                      const QString& spaceReplacement = " ",
                                      int maxLength = -1) noexcept;

  /**
   * @brief Pretty print the name of a QLocale
   *
   * To show locale names in the UI. Examples:
   *
   *  - "en_US" -> "American English (United States)"
   *  - "de" -> "Deutsch"
   *  - "eo" -> "Esperanto"
   *
   * @param code  The code of the locale to print (e.g. "en_US").
   *
   * @return String with the pretty printed locale name.
   */
  static QString prettyPrintLocale(const QString& code) noexcept;

  /**
   * @brief Convert a float or double to a localized string
   *
   * Same as QLocale::toString<float/double>(), but with omitted trailing zeros
   * and without group separators.
   */
  template <typename T>
  static QString floatToString(T value, int decimals,
                               const QLocale& locale) noexcept {
    QString s = locale.toString(value, 'f', decimals);
    for (int i = 1; (i < decimals) && s.endsWith(locale.zeroDigit()); ++i) {
      s.chop(1);
    }
    if (qAbs(value) >= 1000) {
      s.remove(locale.groupSeparator());
    }
    return s;
  }

  /**
   * @brief Convert a fixed point decimal number from an integer to a QString
   *
   * @param value    Value to convert
   * @param pointPos Number of fixed point decimal positions
   */
  template <typename T>
  static QString decimalFixedPointToString(T value, qint32 pointPos) noexcept {
    using UnsignedT = typename std::make_unsigned<T>::type;

    if (value == 0) {
      // special case
      return "0.0";
    }

    UnsignedT valueAbs;
    if (value < 0) {
      valueAbs = -static_cast<UnsignedT>(value);
    } else {
      valueAbs = static_cast<UnsignedT>(value);
    }

    QString str = QString::number(valueAbs);
    if (str.length() > pointPos) {
      // pointPos must be > 0 for this to work correctly
      str.insert(str.length() - pointPos, '.');
    } else {
      for (qint32 i = pointPos - str.length(); i != 0; i--) str.insert(0, '0');
      str.insert(0, "0.");
    }

    while (str.endsWith('0') && !str.endsWith(".0")) str.chop(1);

    if (value < 0) str.insert(0, '-');

    return str;
  }

  /**
   * @brief Convert a fixed point decimal number from a QString to an integer.
   *
   * @param str      A QString that represents the number
   * @param pointPos Number of decimal positions. If the number has more
   *                 decimal digits, this function will throw
   */
  template <typename T>
  static T decimalFixedPointFromString(const QString& str, qint32 pointPos) {
    using UnsignedT = typename std::make_unsigned<T>::type;

    const T min = std::numeric_limits<T>::min();
    const T max = std::numeric_limits<T>::max();
    const UnsignedT max_u = std::numeric_limits<UnsignedT>::max();

    enum class State {
      INVALID,
      START,
      AFTER_SIGN,
      LONELY_DOT,
      INT_PART,
      FRAC_PART,
      EXP,
      EXP_AFTER_SIGN,
      EXP_DIGITS,
    };
    State state = State::START;
    UnsignedT valueAbs = 0;
    bool sign = false;
    qint32 expOffset = pointPos;

    const quint32 maxExp = std::numeric_limits<quint32>::max();
    quint32 exp = 0;
    bool expSign = false;

    for (QChar c : str) {
      if (state == State::INVALID) {
        // break the loop, not the switch
        break;
      }
      switch (state) {
        case State::INVALID:
          // already checked, but needed to avoid compiler warnings
          break;

        case State::START:
          if (c == '-') {
            sign = true;
            state = State::AFTER_SIGN;
          } else if (c == '+') {
            state = State::AFTER_SIGN;
          } else if (c == '.') {
            state = State::LONELY_DOT;
          } else if (c.isDigit()) {
            valueAbs = static_cast<UnsignedT>(c.digitValue());
            state = State::INT_PART;
          } else {
            state = State::INVALID;
          }
          break;

        case State::AFTER_SIGN:
          if (c == '.') {
            state = State::LONELY_DOT;
          } else if (c.isDigit()) {
            valueAbs = static_cast<UnsignedT>(c.digitValue());
            state = State::INT_PART;
          } else {
            state = State::INVALID;
          }
          break;

        case State::LONELY_DOT:
          if (c.isDigit()) {
            valueAbs = static_cast<UnsignedT>(c.digitValue());
            expOffset -= 1;
            state = State::FRAC_PART;
          } else {
            state = State::INVALID;
          }
          break;

        case State::INT_PART:
          if (c == '.') {
            state = State::FRAC_PART;
          } else if (c == 'e' || c == 'E') {
            state = State::EXP;
          } else if (c.isDigit()) {
            UnsignedT digit = static_cast<UnsignedT>(c.digitValue());
            if (valueAbs > (max_u / 10)) {
              // Would overflow
              state = State::INVALID;
              break;
            }
            valueAbs *= 10;
            if (valueAbs > (max_u - digit)) {
              // Would overflow
              state = State::INVALID;
              break;
            }
            valueAbs += digit;
          } else {
            state = State::INVALID;
          }
          break;

        case State::FRAC_PART:
          if (c == 'e' || c == 'E') {
            state = State::EXP;
          } else if (c.isDigit()) {
            UnsignedT digit = static_cast<UnsignedT>(c.digitValue());
            if (valueAbs > (max_u / 10)) {
              // Would overflow
              state = State::INVALID;
              break;
            }
            valueAbs *= 10;
            if (valueAbs > (max_u - digit)) {
              // Would overflow
              state = State::INVALID;
              break;
            }
            valueAbs += digit;
            expOffset -= 1;
          } else {
            state = State::INVALID;
          }
          break;

        case State::EXP:
          if (c == '-') {
            expSign = true;
            state = State::EXP_AFTER_SIGN;
          } else if (c == '+') {
            state = State::EXP_AFTER_SIGN;
          } else if (c.isDigit()) {
            exp = static_cast<quint32>(c.digitValue());
            state = State::EXP_DIGITS;
          } else {
            state = State::INVALID;
          }
          break;

        case State::EXP_AFTER_SIGN:
          if (c.isDigit()) {
            exp = static_cast<quint32>(c.digitValue());
            state = State::EXP_DIGITS;
          } else {
            state = State::INVALID;
          }
          break;

        case State::EXP_DIGITS:
          if (c.isDigit()) {
            quint32 digit = static_cast<quint32>(c.digitValue());
            if (exp > (maxExp / 10)) {
              // Would overflow
              state = State::INVALID;
              break;
            }
            exp *= 10;
            if (exp > (maxExp - digit)) {
              // Would overflow
              state = State::INVALID;
              break;
            }
            exp += digit;
          } else {
            state = State::INVALID;
          }
      }
    }

    bool ok = true;
    switch (state) {
      case State::INVALID:
      case State::START:
      case State::AFTER_SIGN:
      case State::LONELY_DOT:
      case State::EXP:
      case State::EXP_AFTER_SIGN:
        ok = false;
        break;

      case State::INT_PART:
      case State::FRAC_PART:
      case State::EXP_DIGITS:
        break;
    }

    if (ok) {
      quint32 expOffsetAbs;
      if (expOffset < 0) {
        expOffsetAbs = -static_cast<quint32>(expOffset);
      } else {
        expOffsetAbs = static_cast<quint32>(expOffset);
      }

      if (expSign == (expOffset < 0)) {
        if (exp > (maxExp - expOffsetAbs)) {
          // would overflow
          ok = false;
        } else {
          exp += expOffsetAbs;
        }
      } else {
        if (exp < expOffsetAbs) {
          // would overflow
          ok = false;
        } else {
          exp -= expOffsetAbs;
        }
      }
    }

    T result = 0;
    if (ok) {
      // No need to apply exponent or sign if valueAbs is zero
      if (valueAbs != 0) {
        if (expSign) {
          for (quint32 i = 0; i < exp; i++) {
            if ((valueAbs % 10) != 0) {
              // more decimal digits than allowed
              ok = false;
              break;
            }
            valueAbs /= 10;
          }
        } else {
          for (quint32 i = 0; i < exp; i++) {
            if (valueAbs > (max_u / 10)) {
              // would overflow
              ok = false;
              break;
            }
            valueAbs *= 10;
          }
        }
        if (ok) {
          if (sign) {
            if (valueAbs > static_cast<UnsignedT>(min)) {
              ok = false;
            } else {
              result = static_cast<T>(-valueAbs);
            }
          } else {
            if (valueAbs > static_cast<UnsignedT>(max)) {
              ok = false;
            } else {
              result = static_cast<T>(valueAbs);
            }
          }
        }
      }
    }

    if (!ok) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("Invalid fixed point number string: \"%1\"").arg(str));
    }
    return result;
  }

private:
  /**
   * @brief Internal helper function for #expandRangesInString(const QString&)
   */
  static QStringList expandRangesInString(
      const QString& input,
      const QVector<std::tuple<int, int, QStringList>>& replacements) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
