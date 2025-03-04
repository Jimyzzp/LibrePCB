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
#include "symbolinstancepropertiesdialog.h"

#include "../../project/cmd/cmdcomponentinstanceedit.h"
#include "../../project/cmd/cmdsymbolinstanceeditall.h"
#include "../../undocommand.h"
#include "../../undostack.h"
#include "../../workspace/desktopservices.h"
#include "ui_symbolinstancepropertiesdialog.h"

#include <librepcb/core/attribute/attributetype.h>
#include <librepcb/core/attribute/attributeunit.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolInstancePropertiesDialog::SymbolInstancePropertiesDialog(
    Workspace& ws, Project& project, ComponentInstance& cmp, SI_Symbol& symbol,
    UndoStack& undoStack, const LengthUnit& lengthUnit,
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mWorkspace(ws),
    mProject(project),
    mComponentInstance(cmp),
    mSymbol(symbol),
    mUndoStack(undoStack),
    mAttributes(mComponentInstance.getAttributes()),
    mUi(new Ui::SymbolInstancePropertiesDialog) {
  mUi->setupUi(this);
  mUi->edtSymbInstPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                  settingsPrefix % "/pos_x");
  mUi->edtSymbInstPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                  settingsPrefix % "/pos_y");
  mUi->edtSymbInstRotation->setSingleStep(90.0);  // [°]
  setWindowTitle(tr("Properties of %1").arg(mSymbol.getName()));
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &SymbolInstancePropertiesDialog::buttonBoxClicked);

  // Component Instance Attributes
  mUi->edtCompInstName->setText(*mComponentInstance.getName());
  mUi->edtCompInstValue->setText(mComponentInstance.getValue());
  mUi->attributeListEditorWidget->setReferences(nullptr, &mAttributes);

  const QStringList& localeOrder = mProject.getLocaleOrder();

  // Component Library Element Attributes
  QString htmlLink("<a href=\"%1\">%2<a>");
  mUi->lblCompLibName->setText(
      htmlLink.arg(
          mComponentInstance.getLibComponent()
              .getDirectory()
              .getAbsPath()
              .toStr(),
          *mComponentInstance.getLibComponent().getNames().value(localeOrder)) +
      " (" +
      tr("symbol variant \"%1\"")
          .arg(*mComponentInstance.getSymbolVariant().getNames().value(
              localeOrder)) +
      ")");
  mUi->lblCompLibName->setToolTip(
      mComponentInstance.getLibComponent().getDescriptions().value(
          localeOrder) +
      "<p>" +
      mComponentInstance.getLibComponent()
          .getDirectory()
          .getAbsPath()
          .toNative());
  connect(mUi->lblCompLibName, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mWorkspace.getSettings(), this);
            ds.openLocalPath(FilePath(url));
          });

  // Symbol Instance Attributes
  mUi->lblSymbInstName->setText(mSymbol.getName());
  mUi->edtSymbInstPosX->setValue(mSymbol.getPosition().getX());
  mUi->edtSymbInstPosY->setValue(mSymbol.getPosition().getY());
  mUi->edtSymbInstRotation->setValue(mSymbol.getRotation());
  mUi->cbxMirror->setChecked(mSymbol.getMirrored());

  // Symbol Library Element Attributes
  mUi->lblSymbLibName->setText(
      htmlLink.arg(mSymbol.getLibSymbol().getDirectory().getAbsPath().toStr(),
                   *mSymbol.getLibSymbol().getNames().value(localeOrder)));
  mUi->lblSymbLibName->setToolTip(
      mSymbol.getLibSymbol().getDescriptions().value(localeOrder) + "<p>" +
      mSymbol.getLibSymbol().getDirectory().getAbsPath().toNative());
  connect(mUi->lblSymbLibName, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mWorkspace.getSettings(), this);
            ds.openLocalPath(FilePath(url));
          });

  // List Devices
  try {
    tl::optional<Uuid> device = mComponentInstance.getDefaultDeviceUuid();
    // Add devices from project library first (higher priority)
    QHash<Uuid, Device*> prjLibDevices =
        mProject.getLibrary().getDevicesOfComponent(
            mComponentInstance.getLibComponent().getUuid());
    foreach (const Device* device, prjLibDevices) {
      Q_ASSERT(device);
      QString name = *device->getNames().value(localeOrder);
      mUi->cbxPreselectedDevice->addItem(name, device->getUuid().toStr());
    }
    // Then add remaining devices from workspace library (lower priority)
    QSet<Uuid> wsLibDevices = mWorkspace.getLibraryDb().getComponentDevices(
        mComponentInstance.getLibComponent().getUuid());  // can throw
    wsLibDevices -= Toolbox::toSet(prjLibDevices.keys());  // avoid duplicates
    foreach (const Uuid& deviceUuid, wsLibDevices) {
      FilePath devFp =
          mWorkspace.getLibraryDb().getLatest<Device>(deviceUuid);  // can throw
      if (devFp.isValid()) {
        QString name;
        mWorkspace.getLibraryDb().getTranslations<Device>(devFp, localeOrder,
                                                          &name);  // can throw
        mUi->cbxPreselectedDevice->addItem(name, deviceUuid.toStr());
      }
    }
    // If the selected device was not found, show its UUID instead.
    if ((device) && (!prjLibDevices.contains(*device)) &&
        (!wsLibDevices.contains(*device))) {
      mUi->cbxPreselectedDevice->addItem(device->toStr(), device->toStr());
    }
    mUi->cbxPreselectedDevice->model()->sort(0, Qt::AscendingOrder);
    mUi->cbxPreselectedDevice->insertItem(0, "");
    mUi->cbxPreselectedDevice->setCurrentIndex(
        device ? mUi->cbxPreselectedDevice->findData(device->toStr()) : 0);
  } catch (const Exception& e) {
    // If something went wrong, disable the combobox to avoid breaking something
    qCritical()
        << "Failed to list devices in symbol instance properties dialog:"
        << e.getMsg();
    mUi->cbxPreselectedDevice->setEnabled(false);
  }

  // set focus to component instance name
  mUi->edtCompInstName->selectAll();
  mUi->edtCompInstName->setFocus();
}

SymbolInstancePropertiesDialog::~SymbolInstancePropertiesDialog() noexcept {
  mUi->attributeListEditorWidget->setReferences(nullptr, nullptr);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SymbolInstancePropertiesDialog::buttonBoxClicked(
    QAbstractButton* button) noexcept {
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ApplyRole:
      applyChanges();
      break;
    case QDialogButtonBox::AcceptRole:
      if (applyChanges()) {
        accept();
      }
      break;
    case QDialogButtonBox::RejectRole:
      reject();
      break;
    default:
      Q_ASSERT(false);
      break;
  }
}

void SymbolInstancePropertiesDialog::accept() {
  if (applyChanges()) {
    QDialog::accept();
  }
}

bool SymbolInstancePropertiesDialog::applyChanges() noexcept {
  try {
    UndoStackTransaction transaction(
        mUndoStack, tr("Change properties of %1").arg(mSymbol.getName()));

    // Component Instance
    QScopedPointer<CmdComponentInstanceEdit> cmdCmp(
        new CmdComponentInstanceEdit(mProject.getCircuit(),
                                     mComponentInstance));
    cmdCmp->setName(CircuitIdentifier(
        mUi->edtCompInstName->text().trimmed()));  // can throw
    cmdCmp->setValue(mUi->edtCompInstValue->toPlainText());
    cmdCmp->setAttributes(mAttributes);
    if (mUi->cbxPreselectedDevice->isEnabled()) {
      cmdCmp->setDefaultDeviceUuid(Uuid::tryFromString(
          mUi->cbxPreselectedDevice->currentData().toString()));
    }
    transaction.append(cmdCmp.take());

    // Symbol Instance
    bool mirrored = mUi->cbxMirror->isChecked();
    QScopedPointer<CmdSymbolInstanceEditAll> cmdSym(
        new CmdSymbolInstanceEditAll(mSymbol));
    cmdSym->setPosition(Point(mUi->edtSymbInstPosX->getValue(),
                              mUi->edtSymbInstPosY->getValue()),
                        false);
    cmdSym->setRotation(mUi->edtSymbInstRotation->getValue(), false);
    cmdSym->setMirrored(mirrored, false);
    transaction.append(cmdSym.take());

    transaction.commit();  // can throw
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
