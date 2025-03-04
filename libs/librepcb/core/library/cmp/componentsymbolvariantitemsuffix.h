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

#ifndef LIBREPCB_CORE_COMPONENTSYMBOLVARIANTITEMSUFFIX_H
#define LIBREPCB_CORE_COMPONENTSYMBOLVARIANTITEMSUFFIX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../exceptions.h"
#include "../../serialization/sexpression.h"
#include "../../utils/toolbox.h"

#include <type_safe/constrained_type.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ComponentSymbolVariantItemSuffix
 ******************************************************************************/

inline static QString cleanComponentSymbolVariantItemSuffix(
    const QString& userInput) noexcept {
  return Toolbox::cleanUserInputString(userInput,
                                       QRegularExpression("[^0-9a-zA-Z_]"),
                                       true, false, false, "_", 16);
}

struct ComponentSymbolVariantItemSuffixVerifier {
  template <typename Value, typename Predicate>
  static constexpr auto verify(Value&& val, const Predicate& p) ->
      typename std::decay<Value>::type {
    return p(val) ? std::forward<Value>(val)
                  : (throw RuntimeError(
                         __FILE__, __LINE__,
                         QString(QCoreApplication::translate(
                                     "ComponentSymbolVariantItemSuffix",
                                     "Invalid component symbol suffix: '%1'"))
                             .arg(val)),
                     std::forward<Value>(val));
  }
};

struct ComponentSymbolVariantItemSuffixConstraint {
  bool operator()(const QString& value) const noexcept {
    return QRegularExpression("\\A[0-9a-zA-Z_]{0,16}\\z")
        .match(value, 0, QRegularExpression::PartialPreferCompleteMatch)
        .hasMatch();
  }
};

/**
 * ComponentSymbolVariantItemSuffix is a wrapper around QString which guarantees
 * to contain a valid suffix used for
 * librepcb::ComponentSymbolVariantItem
 *
 * Such a suffix is considered as valid if it:
 *   - contains only the characters [0-9a-zA-Z_-]
 *   - is not longer than 16 characters
 *
 * The constructor throws an exception if constructed from a QString which is
 * not a valid suffix according these rules.
 */
using ComponentSymbolVariantItemSuffix =
    type_safe::constrained_type<QString,
                                ComponentSymbolVariantItemSuffixConstraint,
                                ComponentSymbolVariantItemSuffixVerifier>;

inline bool operator==(const ComponentSymbolVariantItemSuffix& lhs,
                       const QString& rhs) noexcept {
  return (*lhs) == rhs;
}
inline bool operator==(const QString& lhs,
                       const ComponentSymbolVariantItemSuffix& rhs) noexcept {
  return lhs == (*rhs);
}
inline bool operator!=(const ComponentSymbolVariantItemSuffix& lhs,
                       const QString& rhs) noexcept {
  return (*lhs) != rhs;
}
inline bool operator!=(const QString& lhs,
                       const ComponentSymbolVariantItemSuffix& rhs) noexcept {
  return lhs != (*rhs);
}
inline QString operator%(const ComponentSymbolVariantItemSuffix& lhs,
                         const QString& rhs) noexcept {
  return (*lhs) % rhs;
}
inline QString operator%(const QString& lhs,
                         const ComponentSymbolVariantItemSuffix& rhs) noexcept {
  return lhs % (*rhs);
}

template <>
inline SExpression serialize(const ComponentSymbolVariantItemSuffix& obj) {
  return SExpression::createString(*obj);
}

template <>
inline ComponentSymbolVariantItemSuffix deserialize(const SExpression& node) {
  return ComponentSymbolVariantItemSuffix(node.getValue());  // can throw
}

inline QDataStream& operator<<(QDataStream& stream,
                               const ComponentSymbolVariantItemSuffix& obj) {
  stream << *obj;
  return stream;
}

inline QDebug operator<<(QDebug stream,
                         const ComponentSymbolVariantItemSuffix& obj) {
  stream << QString("ComponentSymbolVariantItemSuffix('%1'')").arg(*obj);
  return stream;
}

inline uint qHash(const ComponentSymbolVariantItemSuffix& key,
                  uint seed = 0) noexcept {
  return ::qHash(*key, seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
