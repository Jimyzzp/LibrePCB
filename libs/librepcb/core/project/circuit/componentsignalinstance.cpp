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
#include "componentsignalinstance.h"

#include "../../attribute/attributesubstitutor.h"
#include "../../exceptions.h"
#include "../../library/cmp/component.h"
#include "../../utils/scopeguardlist.h"
#include "../board/items/bi_footprintpad.h"
#include "../project.h"
#include "../schematic/items/si_symbolpin.h"
#include "circuit.h"
#include "componentinstance.h"
#include "netsignal.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentSignalInstance::ComponentSignalInstance(
    Circuit& circuit, ComponentInstance& cmpInstance,
    const ComponentSignal& cmpSignal, NetSignal* netsignal)
  : QObject(&cmpInstance),
    mCircuit(circuit),
    mComponentInstance(cmpInstance),
    mComponentSignal(cmpSignal),
    mIsAddedToCircuit(false),
    mNetSignal(netsignal) {
}

ComponentSignalInstance::~ComponentSignalInstance() noexcept {
  Q_ASSERT(!mIsAddedToCircuit);
  Q_ASSERT(!isUsed());
  Q_ASSERT(!arePinsOrPadsUsed());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool ComponentSignalInstance::isNetSignalNameForced() const noexcept {
  return mComponentSignal.isNetSignalNameForced();
}

QString ComponentSignalInstance::getForcedNetSignalName() const noexcept {
  return AttributeSubstitutor::substitute(mComponentSignal.getForcedNetName(),
                                          &mComponentInstance);
}

int ComponentSignalInstance::getRegisteredElementsCount() const noexcept {
  int count = 0;
  count += mRegisteredSymbolPins.count();
  count += mRegisteredFootprintPads.count();
  return count;
}

bool ComponentSignalInstance::arePinsOrPadsUsed() const noexcept {
  foreach (const SI_SymbolPin* pin, mRegisteredSymbolPins) {
    if (pin->isUsed()) {
      return true;
    }
  }
  foreach (const BI_FootprintPad* pad, mRegisteredFootprintPads) {
    if (pad->isUsed()) {
      return true;
    }
  }
  return false;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ComponentSignalInstance::setNetSignal(NetSignal* netsignal) {
  if (netsignal == mNetSignal) {
    return;
  }
  if (!mIsAddedToCircuit) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (arePinsOrPadsUsed()) {
    throw LogicError(
        __FILE__, __LINE__,
        tr("The net signal of the component signal \"%1:%2\" cannot be "
           "changed because it is still in use!")
            .arg(*mComponentInstance.getName(), *mComponentSignal.getName()));
  }
  ScopeGuardList sgl;
  if (mNetSignal) {
    mNetSignal->unregisterComponentSignal(*this);  // can throw
    sgl.add([&]() { mNetSignal->registerComponentSignal(*this); });
  }
  if (netsignal) {
    netsignal->registerComponentSignal(*this);  // can throw
    sgl.add([&]() { netsignal->unregisterComponentSignal(*this); });
  }
  NetSignal* old = mNetSignal;
  mNetSignal = netsignal;
  sgl.dismiss();
  emit netSignalChanged(old, mNetSignal);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ComponentSignalInstance::addToCircuit() {
  if (mIsAddedToCircuit || isUsed()) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mNetSignal) {
    mNetSignal->registerComponentSignal(*this);  // can throw
  }
  mIsAddedToCircuit = true;
}

void ComponentSignalInstance::removeFromCircuit() {
  if (!mIsAddedToCircuit) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (isUsed()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The component \"%1\" cannot be removed "
                          "because it is still in use!")
                           .arg(*mComponentInstance.getName()));
  }
  if (mNetSignal) {
    mNetSignal->unregisterComponentSignal(*this);  // can throw
  }
  mIsAddedToCircuit = false;
}

void ComponentSignalInstance::registerSymbolPin(SI_SymbolPin& pin) {
  if ((!mIsAddedToCircuit) || (pin.getCircuit() != mCircuit) ||
      (mRegisteredSymbolPins.contains(&pin))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredSymbolPins.append(&pin);
}

void ComponentSignalInstance::unregisterSymbolPin(SI_SymbolPin& pin) {
  if ((!mIsAddedToCircuit) || (!mRegisteredSymbolPins.contains(&pin))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredSymbolPins.removeOne(&pin);
}

void ComponentSignalInstance::registerFootprintPad(BI_FootprintPad& pad) {
  if ((!mIsAddedToCircuit) || (pad.getCircuit() != mCircuit) ||
      (mRegisteredFootprintPads.contains(&pad))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredFootprintPads.append(&pad);
}

void ComponentSignalInstance::unregisterFootprintPad(BI_FootprintPad& pad) {
  if ((!mIsAddedToCircuit) || (!mRegisteredFootprintPads.contains(&pad))) {
    throw LogicError(__FILE__, __LINE__);
  }
  mRegisteredFootprintPads.removeOne(&pad);
}

void ComponentSignalInstance::serialize(SExpression& root) const {
  root.appendChild(mComponentSignal.getUuid());
  root.appendChild(
      "net",
      mNetSignal ? tl::make_optional(mNetSignal->getUuid()) : tl::nullopt);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
