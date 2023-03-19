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
#include "BasicForm.h"
#include "FormManager.h"
#include <typeinfo>

extern FormManager *FormManager::__pInstance;

void FormManager::Initialize(Frame *pFrame) {
	__pInstance = new FormManager(pFrame);
}

void FormManager::RemoveInstance(void) {
	delete __pInstance;
	__pInstance = null;
}

FormManager *FormManager::GetInstance(void) {
	return __pInstance;
}

FormManager::FormManager(Frame *pFrame) {
	__pAppFrame = pFrame;

	__pActivePopup = null;
	__activePopupID = -1;

	__pFormStack = new StackT<BasicForm *>;
	__pFormStack->Construct(10);
}

FormManager::~FormManager(void) {
	if (__pFormStack) {
		__pFormStack->RemoveAll();
		delete __pFormStack;
	}
	if (__pActivePopup) {
		delete __pActivePopup;
	}
}

BasicForm *FormManager::GetActiveForm(void) const {
	BasicForm *pCurForm = null;
	__pFormStack->Peek(pCurForm);

	return pCurForm;
}

result FormManager::AddToFrame(BasicForm *pForm) {
	result res = pForm->Initialize();
	if (IsFailed(res)) {
		return res;
	} else {
		return __pAppFrame->AddControl(*pForm);
	}
}

result FormManager::RemoveFromFrame(BasicForm *pForm) {
	result res = pForm->Terminate();
	if (IsFailed(res)) {
		return res;
	} else {
		return __pAppFrame->RemoveControl(*pForm);
	}
}

bool FormManager::IsFormAddedToFrame(BasicForm *pForm) const {
	IList *pControls = __pAppFrame->GetControls();

	bool found = false;
	IEnumerator *pEnum = pControls->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *pObj = pEnum->GetCurrent();
		if (pObj == pForm) {
			found = true;
			break;
		}
	}

	delete pEnum;
	return found;
}

void FormManager::DisposeForm(BasicForm *pForm) {
	if (IsFormAddedToFrame(pForm)) {
		RemoveFromFrame(pForm);
	} else {
		delete pForm;
	}
}

result FormManager::ShowPopup(Popup *pPopup, int id) {
	if (__pActivePopup) {
		AppLogDebug("FormManager::ShowPopup: attempt to show popup while another one is active");
		return E_INVALID_STATE;
	}

	__pActivePopup = pPopup;
	__activePopupID = id;

	pPopup->SetShowState(true);
	return E_SUCCESS;
}

result FormManager::SwitchToForm(BasicForm *pForm, IList *pArgs, int id) {
	if (__pActivePopup) {
		AppLogDebug("FormManager::SwitchToForm: attempt to switch to another form while popup is active");
		return E_INVALID_STATE;
	}

	if (__pFormStack->Contains(pForm)) {
		AppLogDebug("FormManager::SwitchToForm: form is already in the stack");
		return E_OBJ_ALREADY_EXIST;
	}

	result res = E_SUCCESS;
	if (!IsFormAddedToFrame(pForm)) {
		res = AddToFrame(pForm);
		if (IsFailed(res)) {
			AppLogDebug("FormManager::SwitchToForm: failed to add form to the frame");
			return res;
		}
	}

	BasicForm *pCurForm = null;
	__pFormStack->Peek(pCurForm);
	int cur_id = pCurForm ? pCurForm->GetFormID() : -1;

	res = pForm->OnGivenData(pArgs, REASON_NONE, cur_id);
	if (!IsFailed(res)) {
		res = __pAppFrame->SetCurrentForm(*pForm);
		if (!IsFailed(res)) {
			pForm->Draw();
			pForm->Show();

			pForm->SetFormID(id);
			__pFormStack->Push(pForm);
		} else {
			AppLogDebug("FormManager::SwitchToForm: failed to set current form");
		}
	}
	return res;
}

result FormManager::SwitchBack(IList* pArgs, DataReceiveReason reason, bool removeFromFrame) {
	if (__pActivePopup) {
		BasicForm *pCurForm = null;
		__pFormStack->Peek(pCurForm);

		result res = E_SUCCESS;
		if (pCurForm) {
			res = pCurForm->OnGivenData(pArgs, reason, __activePopupID);
		}

		__pActivePopup->SetShowState(false);
		pCurForm->RequestRedraw(true);

		delete __pActivePopup;
		__pActivePopup = null;
		__activePopupID = -1;

		return res;
	} else if (__pFormStack->GetCount() >= 2) {
		BasicForm *pCurForm = null;
		__pFormStack->Pop(pCurForm);

		BasicForm *pPrevForm = null;
		__pFormStack->Peek(pPrevForm);

		result res = E_SUCCESS;
		if (pPrevForm && pCurForm) {
			res = pPrevForm->OnGivenData(pArgs, reason, pCurForm->GetFormID());
			if (!IsFailed(res)) {
				res = __pAppFrame->SetCurrentForm(*pPrevForm);
				if (!IsFailed(res)) {
					pPrevForm->Draw();
					pPrevForm->Show();

					if (removeFromFrame && IsFailed(RemoveFromFrame(pCurForm))) {
						AppLogDebug("FormManager::SwitchBack: failed to remove current form from frame");
					}
				} else {
					__pFormStack->Push(pCurForm);
				}
			} else {
				__pFormStack->Push(pCurForm);
			}
		} else {
			AppLogDebug("FormManager::SwitchBack: either current or previous form is null");
			res = E_INVALID_DATA;
		}
		return res;
	} else {
		AppLogDebug("FormManager::SwitchBack: there is either no active popup or nothing to switch back to");
		return E_INVALID_STATE;
	}
}

result FormManager::SwitchToTop(IList* pArgs, DataReceiveReason reason, bool removeFromFrame) {
	if (__pActivePopup) {
		BasicForm *pCurForm = null;
		__pFormStack->Peek(pCurForm);

		result res = E_SUCCESS;
		if (pCurForm) {
			res = pCurForm->OnGivenData(pArgs, reason, __activePopupID);
		}

		__pActivePopup->SetShowState(false);
		pCurForm->RequestRedraw(true);

		delete __pActivePopup;
		__pActivePopup = null;
		__activePopupID = -1;

		return res;
	} else if (__pFormStack->GetCount() >= 2) {
		BasicForm *pPrevForm = null;

		result res = E_SUCCESS;
		while (__pFormStack->GetCount() > 1) {
			BasicForm *pCurForm = null;
			__pFormStack->Pop(pCurForm);

			pPrevForm = null;
			__pFormStack->Peek(pPrevForm);

			if (pPrevForm && pCurForm) {
				res = pPrevForm->OnGivenData(pArgs, reason, pCurForm->GetFormID());
				if (!IsFailed(res)) {
					if (removeFromFrame && IsFailed(RemoveFromFrame(pCurForm))) {
						AppLogDebug("FormManager::SwitchToTop: failed to remove current form from frame");
					}
				} else {
					AppLogDebug("FormManager::SwitchToTop: failed to pass data to previous form");
					__pFormStack->Push(pCurForm);
					break;
				}
			} else {
				AppLogDebug("FormManager::SwitchToTop: either current or previous form is null");
				res = E_INVALID_DATA;
				break;
			}
		}
		res = __pAppFrame->SetCurrentForm(*pPrevForm);
		if (!IsFailed(res)) {
			pPrevForm->Draw();
			pPrevForm->Show();
		}
		return res;
	} else {
		return E_SUCCESS;
	}
}
