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

#ifndef LIBREPCB_EDITOR_CMDBOARDEDIT_H
#define LIBREPCB_EDITOR_CMDBOARDEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/project/board/boarddesignrules.h>
#include <librepcb/core/project/board/drc/boarddesignrulechecksettings.h>
#include <librepcb/core/types/elementname.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;

namespace editor {

/*******************************************************************************
 *  Class CmdBoardEdit
 ******************************************************************************/

/**
 * @brief The CmdBoardEdit class
 */
class CmdBoardEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdBoardEdit() = delete;
  CmdBoardEdit(const CmdBoardEdit& other) = delete;
  explicit CmdBoardEdit(Board& board) noexcept;
  ~CmdBoardEdit() noexcept;

  // Setters
  void setName(const ElementName& name) noexcept;
  void setInnerLayerCount(int count) noexcept;
  void setDesignRules(const BoardDesignRules& rules) noexcept;
  void setDrcSettings(const BoardDesignRuleCheckSettings& settings) noexcept;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  Board& mBoard;

  ElementName mOldName;
  ElementName mNewName;
  int mOldInnerLayerCount;
  int mNewInnerLayerCount;
  BoardDesignRules mOldDesignRules;
  BoardDesignRules mNewDesignRules;
  BoardDesignRuleCheckSettings mOldDrcSettings;
  BoardDesignRuleCheckSettings mNewDrcSettings;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
