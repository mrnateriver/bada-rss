/*
 * Copyright (c) 2016 Evgenii Dobrovidov
 * This file is part of "RSS aggregator".
 *
 * "RSS aggregator" is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * "RSS aggregator" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with "RSS aggregator".  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BASICFORM_H_
#define BASICFORM_H_

#include "FormManager.h"
#include <FBase.h>
#include <FUi.h>

using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

class BasicForm;
typedef void (BasicForm::*EventDelegate)(const Control &src);

#define HANDLER(x) RegisterEventHandler(static_cast<EventDelegate>(&x))

class BasicForm: public Form, public IActionEventListener {
public:
	BasicForm(void);
	virtual ~BasicForm(void);

	virtual result Initialize(void) = 0;
	virtual result Terminate(void) = 0;

	virtual result OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID) = 0;

	int RegisterEventHandler(EventDelegate hnd);

protected:
	FormManager *GetFormManager(void) const {
		return __pFormManager;
	}

private:
	void OnActionPerformed(const Control &source, int actionId);

	EventDelegate *__pDelegates;
	int __regLength;
	int __regEvents;

	int __formID;
	void SetFormID(int id) {
		__formID = id;
	}
	int GetFormID(void) const {
		return __formID;
	}

	FormManager *__pFormManager;

	friend class FormManager;
};

#endif
