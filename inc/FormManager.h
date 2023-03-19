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
#ifndef FORMMANAGER_H_
#define FORMMANAGER_H_

#include <FBase.h>
#include <FApp.h>
#include <FUi.h>

class BasicForm;

using namespace Osp::Base::Collection;
using namespace Osp::App;
using namespace Osp::Ui::Controls;

enum DataReceiveReason {
	REASON_NONE,
	REASON_DIALOG_SUCCESS,
	REASON_DIALOG_CANCEL
};

class FormManager {
public:
	static void Initialize(Frame *pFrame);
	static FormManager *GetInstance(void);
	static void RemoveInstance(void);

private:
	static FormManager *__pInstance;

public:
	FormManager(Frame *pFrame);
	~FormManager(void);

	BasicForm *GetActiveForm(void) const;

	result AddToFrame(BasicForm *pForm);
	result RemoveFromFrame(BasicForm *pForm);

	bool IsFormAddedToFrame(BasicForm *pForm) const;
	void DisposeForm(BasicForm *pForm);

	result ShowPopup(Popup *pPopup, int id = -1);
	result SwitchToForm(BasicForm *pForm, IList *pArgs, int id = -1);
	result SwitchBack(IList* pArgs, DataReceiveReason reason, bool removeFromFrame = true);
	result SwitchToTop(IList* pArgs, DataReceiveReason reason, bool removeFromFrame = true);

private:
	Frame *__pAppFrame;

	Popup *__pActivePopup;
	int __activePopupID;
	StackT<BasicForm *> *__pFormStack;
};

#endif
