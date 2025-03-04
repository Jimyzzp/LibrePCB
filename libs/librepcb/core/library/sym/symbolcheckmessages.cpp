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
#include "symbolcheckmessages.h"

#include "../../geometry/text.h"
#include "../../types/layer.h"
#include "../../utils/toolbox.h"
#include "symbolpin.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  MsgDuplicatePinName
 ******************************************************************************/

MsgDuplicatePinName::MsgDuplicatePinName(const SymbolPin& pin) noexcept
  : RuleCheckMessage(
        Severity::Error, tr("Duplicate pin name: '%1'").arg(*pin.getName()),
        tr("All symbol pins must have unique names, otherwise they cannot be "
           "distinguished later in the component editor. If your part has "
           "several pins with same functionality (e.g. multiple GND pins), you "
           "should add only one of these pins to the symbol. The assignment to "
           "multiple leads should be done in the device editor instead."),
        "duplicate_pin_name") {
  mApproval.appendChild("name", *pin.getName());
}

/*******************************************************************************
 *  MsgMissingSymbolName
 ******************************************************************************/

MsgMissingSymbolName::MsgMissingSymbolName() noexcept
  : RuleCheckMessage(
        Severity::Warning, tr("Missing text: '%1'").arg("{{NAME}}"),
        tr("Most symbols should have a text element for the component's name, "
           "otherwise you won't see that name in the schematics. There are "
           "only a few exceptions (e.g. a schematic frame) which don't need a "
           "name, for those you can ignore this message."),
        "missing_name_text") {
}

/*******************************************************************************
 *  MsgMissingSymbolValue
 ******************************************************************************/

MsgMissingSymbolValue::MsgMissingSymbolValue() noexcept
  : RuleCheckMessage(
        Severity::Warning, tr("Missing text: '%1'").arg("{{VALUE}}"),
        tr("Most symbols should have a text element for the component's value, "
           "otherwise you won't see that value in the schematics. There are "
           "only a few exceptions (e.g. a schematic frame) which don't need a "
           "value, for those you can ignore this message."),
        "missing_value_text") {
}

/*******************************************************************************
 *  MsgOverlappingSymbolPins
 ******************************************************************************/

MsgOverlappingSymbolPins::MsgOverlappingSymbolPins(
    QVector<std::shared_ptr<const SymbolPin>> pins) noexcept
  : RuleCheckMessage(
        Severity::Error, buildMessage(pins),
        tr("There are multiple pins at the same position. This is not allowed "
           "because you cannot connect wires to these pins in the schematic "
           "editor."),
        "overlapping_pins"),
    mPins(pins) {
  QVector<std::shared_ptr<const SymbolPin>> sortedPins = pins;
  std::sort(sortedPins.begin(), sortedPins.end(),
            [](const std::shared_ptr<const SymbolPin>& a,
               const std::shared_ptr<const SymbolPin>& b) {
              return a->getUuid() < b->getUuid();
            });
  foreach (const auto& pin, pins) {
    mApproval.ensureLineBreak();
    mApproval.appendChild("pin", pin->getUuid());
  }
  mApproval.ensureLineBreak();
}

QString MsgOverlappingSymbolPins::buildMessage(
    const QVector<std::shared_ptr<const SymbolPin>>& pins) noexcept {
  QStringList pinNames;
  foreach (const auto& pin, pins) {
    pinNames.append("'" % pin->getName() % "'");
  }
  Toolbox::sortNumeric(pinNames, Qt::CaseInsensitive, false);
  return tr("Overlapping pins: %1").arg(pinNames.join(", "));
}

/*******************************************************************************
 *  MsgSymbolPinNotOnGrid
 ******************************************************************************/

MsgSymbolPinNotOnGrid::MsgSymbolPinNotOnGrid(
    std::shared_ptr<const SymbolPin> pin,
    const PositiveLength& gridInterval) noexcept
  : RuleCheckMessage(
        Severity::Error,
        tr("Pin not on %1mm grid: '%2'")
            .arg(gridInterval->toMmString(), *pin->getName()),
        tr("Every pin must be placed exactly on the %1mm grid, "
           "otherwise it cannot be connected in the schematic editor.")
            .arg(gridInterval->toMmString()),
        "pin_not_on_grid"),
    mPin(pin),
    mGridInterval(gridInterval) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("pin", pin->getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  MsgWrongSymbolTextLayer
 ******************************************************************************/

MsgWrongSymbolTextLayer::MsgWrongSymbolTextLayer(
    std::shared_ptr<const Text> text, const Layer& expectedLayer) noexcept
  : RuleCheckMessage(
        Severity::Warning,
        tr("Layer of '%1' is not '%2'")
            .arg(text->getText(), expectedLayer.getNameTr()),
        tr("The text element '%1' should normally be on layer '%2'.")
            .arg(text->getText(), expectedLayer.getNameTr()),
        "unusual_text_layer"),
    mText(text),
    mExpectedLayer(&expectedLayer) {
  mApproval.ensureLineBreak();
  mApproval.appendChild("text", text->getUuid());
  mApproval.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
