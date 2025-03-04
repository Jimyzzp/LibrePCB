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
#include "cmdtextedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdTextEdit::CmdTextEdit(Text& text) noexcept
  : UndoCommand(tr("Edit text")),
    mText(text),
    mOldLayer(&text.getLayer()),
    mNewLayer(mOldLayer),
    mOldText(text.getText()),
    mNewText(mOldText),
    mOldPosition(text.getPosition()),
    mNewPosition(mOldPosition),
    mOldRotation(text.getRotation()),
    mNewRotation(mOldRotation),
    mOldHeight(text.getHeight()),
    mNewHeight(mOldHeight),
    mOldAlign(text.getAlign()),
    mNewAlign(mOldAlign) {
}

CmdTextEdit::~CmdTextEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdTextEdit::setLayer(const Layer& layer, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLayer = &layer;
  if (immediate) mText.setLayer(*mNewLayer);
}

void CmdTextEdit::setText(const QString& text, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewText = text;
  if (immediate) mText.setText(mNewText);
}

void CmdTextEdit::setHeight(const PositiveLength& height,
                            bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewHeight = height;
  if (immediate) mText.setHeight(mNewHeight);
}

void CmdTextEdit::setAlignment(const Alignment& align,
                               bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAlign = align;
  if (immediate) mText.setAlign(mNewAlign);
}

void CmdTextEdit::setPosition(const Point& pos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPosition = pos;
  if (immediate) mText.setPosition(mNewPosition);
}

void CmdTextEdit::translate(const Point& deltaPos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPosition += deltaPos;
  if (immediate) mText.setPosition(mNewPosition);
}

void CmdTextEdit::snapToGrid(const PositiveLength& gridInterval,
                             bool immediate) noexcept {
  setPosition(mNewPosition.mappedToGrid(gridInterval), immediate);
}

void CmdTextEdit::setRotation(const Angle& angle, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewRotation = angle;
  if (immediate) mText.setRotation(mNewRotation);
}

void CmdTextEdit::rotate(const Angle& angle, const Point& center,
                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPosition.rotate(angle, center);
  mNewRotation += angle;
  if (immediate) {
    mText.setPosition(mNewPosition);
    mText.setRotation(mNewRotation);
  }
}

void CmdTextEdit::mirror(Qt::Orientation orientation, const Point& center,
                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPosition.mirror(orientation, center);
  if (orientation == Qt::Horizontal) {
    mNewRotation = Angle::deg180() - mNewRotation;
  } else {
    mNewRotation = -mNewRotation;
  }
  mNewAlign.mirrorV();
  if (immediate) {
    mText.setPosition(mNewPosition);
    mText.setRotation(mNewRotation);
    mText.setAlign(mNewAlign);
  }
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdTextEdit::performExecute() {
  performRedo();  // can throw

  if (mNewLayer != mOldLayer) return true;
  if (mNewText != mOldText) return true;
  if (mNewPosition != mOldPosition) return true;
  if (mNewRotation != mOldRotation) return true;
  if (mNewHeight != mOldHeight) return true;
  if (mNewAlign != mOldAlign) return true;
  return false;
}

void CmdTextEdit::performUndo() {
  mText.setLayer(*mOldLayer);
  mText.setText(mOldText);
  mText.setPosition(mOldPosition);
  mText.setRotation(mOldRotation);
  mText.setHeight(mOldHeight);
  mText.setAlign(mOldAlign);
}

void CmdTextEdit::performRedo() {
  mText.setLayer(*mNewLayer);
  mText.setText(mNewText);
  mText.setPosition(mNewPosition);
  mText.setRotation(mNewRotation);
  mText.setHeight(mNewHeight);
  mText.setAlign(mNewAlign);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
