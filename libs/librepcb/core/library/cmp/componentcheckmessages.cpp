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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "componentcheckmessages.h"

#include "componentsignal.h"
#include "componentsymbolvariant.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  MsgDuplicateSignalName
 ******************************************************************************/

MsgDuplicateSignalName::MsgDuplicateSignalName(
    const ComponentSignal& signal) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Duplicate signal name: '%1'").arg(*signal.getName()),
        tr("All component signals must have unique names, otherwise they "
           "cannot be distinguished later in the device editor. If your part "
           "has several pins which are electrically exactly equal (e.g. "
           "multiple GND pins), you should add only one of these pins as a "
           "component signal. The assignment to multiple pins should be done "
           "in the device editor instead."),
        "duplicate_signal_name") {
  mApproval.appendChild("name", *signal.getName());
}

/*******************************************************************************
 *  MsgMissingComponentDefaultValue
 ******************************************************************************/

MsgMissingComponentDefaultValue::MsgMissingComponentDefaultValue() noexcept
  : RuleCheckMessage(
        Severity::Warning, tr("No default value set"),
        tr("Most components should have a default value set. The "
           "default value becomes the component's value when adding it "
           "to a schematic. It can also contain placeholders which are "
           "substituted later in the schematic. Commonly used default "
           "values are:\n\n"
           "Generic parts (e.g. a diode): %1\n"
           "Specific parts (e.g. a microcontroller): %2\n"
           "Passive parts: Using an attribute, e.g. %3")
            .arg("'{{PARTNUMBER or DEVICE}}'",
                 "'{{PARTNUMBER or DEVICE or COMPONENT}}'", "'{{RESISTANCE}}'"),
        "empty_default_value") {
}

/*******************************************************************************
 *  MsgMissingComponentPrefix
 ******************************************************************************/

MsgMissingComponentPrefix::MsgMissingComponentPrefix() noexcept
  : RuleCheckMessage(
        Severity::Warning, tr("No component prefix set"),
        tr("Most components should have a prefix defined. The prefix is used "
           "to generate the component's name when adding it to a schematic. "
           "For example the prefix 'R' (resistor) leads to component names "
           "'R1', 'R2', 'R3' etc."),
        "empty_prefix") {
}

/*******************************************************************************
 *  MsgMissingSymbolVariant
 ******************************************************************************/

MsgMissingSymbolVariant::MsgMissingSymbolVariant() noexcept
  : RuleCheckMessage(
        Severity::Error, tr("No symbol variant defined"),
        tr("Every component requires at least one symbol variant, otherwise it "
           "can't be added to schematics."),
        "missing_variants") {
}

/*******************************************************************************
 *  MsgMissingSymbolVariantItem
 ******************************************************************************/

MsgMissingSymbolVariantItem::MsgMissingSymbolVariantItem(
    std::shared_ptr<const ComponentSymbolVariant> symbVar) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Symbol variant '%1' has no items")
            .arg(*symbVar->getNames().getDefaultValue()),
        tr("Every symbol variant requires at least one symbol item, otherwise "
           "it can't be added to schematics."),
        "missing_gates"),
    mSymbVar(symbVar) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("variant", symbVar->getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
