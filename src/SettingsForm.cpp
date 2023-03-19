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
#include "SettingsForm.h"
#include "SaveForm.h"
#include "SettingsManager.h"
#include <FIo.h>
#include <typeinfo>

using namespace Osp::Io;

SettingsForm::SettingsForm(void) {
	__pMainPanel = null;
	__pAutoUpdateCheckBox = null;
	__pAutoIntervalCheckBox = null;
	__pUpdateIntervalField = null;
	__pDefaultEnclosureFolderCheckBox = null;
	__pLoadImagesByDefaultCheckBox = null;

	__prevEnclosureFolderText = L"";
}

void SettingsForm::OnSoftkeySaveClicked(const Control &src) {
	SettingsManager *mgr = SettingsManager::GetInstance();

	mgr->SetUpdateAutomatically(__pAutoUpdateCheckBox->IsSelected());
	mgr->SetCalculateUpdateInterval(__pAutoIntervalCheckBox->IsSelected());

	int res = 1;
	Integer::Parse(__pUpdateIntervalField->GetText(), res);
	mgr->SetUpdateInterval(res);

	mgr->SetLoadImagesByDefault(__pLoadImagesByDefaultCheckBox->IsSelected());

	mgr->SetDownloadToDefaultEnclosureFolder(__pDefaultEnclosureFolderCheckBox->IsSelected());
	mgr->SetEnclosureDownloadFolder(__pEnclosureFolderField->GetText());

	GetFormManager()->SwitchBack(null, REASON_DIALOG_SUCCESS);
}

void SettingsForm::OnSoftkeyBackClicked(const Control &src) {
	GetFormManager()->SwitchBack(null, REASON_DIALOG_CANCEL);
}

void SettingsForm::OnEnclosureFieldAcceptClicked(const Control &src) {
	String val = __pEnclosureFolderField->GetText();
	if (!val.StartsWith(L"/", 0)) {
		val.Insert(L"/", 0);
	}
	Directory dir;
	if ((!val.StartsWith(L"/Media", 0) && !val.StartsWith(L"/Storagecard/Media", 0)) || IsFailed(dir.Construct(val))) {
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_SETTINGSFORM_MANUAL_PATH_ERROR"), MSGBOX_STYLE_OK);
		return;
	}

	__pEnclosureFolderField->SetText(val);

	__pMainPanel->CloseOverlayWindow();
	__pAutoUpdateCheckBox->SetFocus();

	OnSomethingChanged(src);
}

void SettingsForm::OnEnclosureFieldCancelClicked(const Control &src) {
	__pEnclosureFolderField->SetText(__prevEnclosureFolderText);
	OnEnclosureFieldAcceptClicked(src);
}

void SettingsForm::OnSomethingChanged(const Control &src) {
	SetSoftkeyEnabled(SOFTKEY_0, true);

	if (__pUpdateIntervalField->GetText().Equals(L"0", false)) {
		__pAutoIntervalCheckBox->SetSelected(true);
	}
	if (__pEnclosureFolderField->GetText().StartsWith(L"/Home", 0) || __pEnclosureFolderField->GetTextLength() <= 0) {
		__pEnclosureFolderField->SetText(L"/Media/");
		__pDefaultEnclosureFolderCheckBox->SetSelected(true);
	}
	RearrangeControls();
}

void SettingsForm::SetDefaultValues(void) {
	SettingsManager *mgr = SettingsManager::GetInstance();

	__pAutoUpdateCheckBox->SetSelected(mgr->GetUpdateAutomatically());
	__pAutoIntervalCheckBox->SetSelected(mgr->GetCalculateUpdateInterval());
	__pUpdateIntervalField->SetText(Integer::ToString(mgr->GetUpdateInterval()));

	__pDefaultEnclosureFolderCheckBox->SetSelected(mgr->GetDownloadToDefaultEnclosureFolder());
	__pEnclosureFolderField->SetText(mgr->GetEnclosureDownloadFolder());

	__pLoadImagesByDefaultCheckBox->SetSelected(mgr->GetLoadImagesByDefault());
}

void SettingsForm::RearrangeControls(void) {
	if (!__pAutoUpdateCheckBox->IsSelected()) {
		__pAutoIntervalCheckBox->SetEnabled(false);
		__pUpdateIntervalField->SetEnabled(false);
	} else {
		__pAutoIntervalCheckBox->SetEnabled(true);
		if (__pAutoIntervalCheckBox->IsSelected()) {
			__pUpdateIntervalField->SetEnabled(false);
		} else {
			__pUpdateIntervalField->SetEnabled(true);
		}
	}
	if (!__pDefaultEnclosureFolderCheckBox->IsSelected()) {
		__pEnclosureFolderField->SetEnabled(true);
	} else {
		__pEnclosureFolderField->SetEnabled(false);
	}
	RequestRedraw(true);
}

result SettingsForm::Initialize(void) {
	result res = Construct(FORM_STYLE_INDICATOR | FORM_STYLE_TITLE | FORM_STYLE_SOFTKEY_0 | FORM_STYLE_SOFTKEY_1);
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct parent form", GetErrorMessage(res));
		return res;
	}
	SetBackgroundColor(Color(0x0e0e0e, false));
	SetTitleText(Utilities::GetString(L"IDS_SETTINGSFORM_TITLE"));

	SetSoftkeyText(SOFTKEY_1, Utilities::GetString(L"IDS_SAVEFORM_CANCEL_BUTTON"));
	SetSoftkeyActionId(SOFTKEY_1, HANDLER(SettingsForm::OnSoftkeyBackClicked));
	SetSoftkeyText(SOFTKEY_0, Utilities::GetString(L"IDS_SETTINGSFORM_SOFTKEY_APPLY"));
	SetSoftkeyActionId(SOFTKEY_0, HANDLER(SettingsForm::OnSoftkeySaveClicked));
	AddSoftkeyActionListener(SOFTKEY_0, *this);
	AddSoftkeyActionListener(SOFTKEY_1, *this);

	__pMainPanel = new ScrollPanel;
	res = __pMainPanel->Construct(Rectangle(0,0,480,712));
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct scroll panel", GetErrorMessage(res));
		return res;
	}
	AddControl(*__pMainPanel);

	int triggerID = HANDLER(SettingsForm::OnSomethingChanged);

	Label *updateSettingsLabel = new Label;
	res = updateSettingsLabel->Construct(Rectangle(10,20,460,25),Utilities::GetString(L"IDS_SETTINGSFORM_LABEL_UPDATE_SETTINGS"));
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct autoupdate settings label", GetErrorMessage(res));
		return res;
	}
	updateSettingsLabel->SetTextConfig(25, LABEL_TEXT_STYLE_NORMAL);
	__pMainPanel->AddControl(*updateSettingsLabel);

	__pAutoUpdateCheckBox = new CheckButton;
	res = __pAutoUpdateCheckBox->Construct(Rectangle(10,65,460,100), CHECK_BUTTON_STYLE_ONOFF, BACKGROUND_STYLE_DEFAULT, false, Utilities::GetString(L"IDS_SETTINGSFORM_AUTOUPDATE_CHECKBOX"), GROUP_STYLE_TOP);
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct autoupdate checkbox", GetErrorMessage(res));
		return res;
	}
	__pAutoUpdateCheckBox->SetActionId(triggerID, triggerID);
	__pAutoUpdateCheckBox->AddActionEventListener(*this);
	__pMainPanel->AddControl(*__pAutoUpdateCheckBox);

	__pAutoIntervalCheckBox = new CheckButton;
	res = __pAutoIntervalCheckBox->Construct(Rectangle(10,165,460,100), CHECK_BUTTON_STYLE_ONOFF, BACKGROUND_STYLE_DEFAULT, false, Utilities::GetString(L"IDS_SETTINGSFORM_AUTO_UPDATE_INTERVAL_CHECKBOX"), GROUP_STYLE_MIDDLE);
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct auto update interval checkbox", GetErrorMessage(res));
		return res;
	}
	__pAutoIntervalCheckBox->SetActionId(triggerID, triggerID);
	__pAutoIntervalCheckBox->AddActionEventListener(*this);
	__pMainPanel->AddControl(*__pAutoIntervalCheckBox);

	__pUpdateIntervalField = new EditField;
	res = __pUpdateIntervalField->Construct(Rectangle(10,265,460,106), EDIT_FIELD_STYLE_NORMAL, INPUT_STYLE_FULLSCREEN, true, 5, GROUP_STYLE_BOTTOM);
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct update interval field", GetErrorMessage(res));
		return res;
	}
	__pUpdateIntervalField->SetTitleText(Utilities::GetString(L"IDS_SETTINGSFORM_UPDATE_INTERVAL_FIELD_TITLE"));
	__pUpdateIntervalField->AddTextEventListener(*this);
	__pMainPanel->AddControl(*__pUpdateIntervalField);

	Label *enclosureSettingsLabel = new Label;
	res = enclosureSettingsLabel->Construct(Rectangle(10,391,460,25),Utilities::GetString(L"IDS_SETTINGSFORM_LABEL_ENCLOSURE_SETTINGS"));
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct enclosure settings label", GetErrorMessage(res));
		return res;
	}
	enclosureSettingsLabel->SetTextConfig(25, LABEL_TEXT_STYLE_NORMAL);
	__pMainPanel->AddControl(*enclosureSettingsLabel);

	//ENCLOSURE
	__pDefaultEnclosureFolderCheckBox = new CheckButton;
	res = __pDefaultEnclosureFolderCheckBox->Construct(Rectangle(10,436,460,100),CHECK_BUTTON_STYLE_ONOFF, BACKGROUND_STYLE_DEFAULT, false, Utilities::GetString(L"IDS_SETTINGSFORM_DEFAULT_ENCLOSURE_FOLDER"),GROUP_STYLE_TOP);
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct default enclosure folder checkbox", GetErrorMessage(res));
		return res;
	}
	__pDefaultEnclosureFolderCheckBox->SetActionId(triggerID, triggerID);
	__pDefaultEnclosureFolderCheckBox->AddActionEventListener(*this);
	__pMainPanel->AddControl(*__pDefaultEnclosureFolderCheckBox);

	__pEnclosureFolderField = new EditField;
	res = __pEnclosureFolderField->Construct(Rectangle(10,536,460,106), EDIT_FIELD_STYLE_NORMAL, INPUT_STYLE_OVERLAY, true, 1000, GROUP_STYLE_BOTTOM);
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct enclosure folder field", GetErrorMessage(res));
		return res;
	}
	__pEnclosureFolderField->SetTitleText(Utilities::GetString(L"IDS_SETTINGSFORM_ENCLOSURE_FOLDER_FIELD"));
	__pEnclosureFolderField->SetOverlayKeypadCommandButton(COMMAND_BUTTON_POSITION_RIGHT, Utilities::GetString(L"IDS_SAVEFORM_CANCEL_BUTTON"), HANDLER(SettingsForm::OnEnclosureFieldCancelClicked));
	__pEnclosureFolderField->SetOverlayKeypadCommandButton(COMMAND_BUTTON_POSITION_LEFT, Utilities::GetString(L"IDS_CHANNELINFOFORM_KEYPAD_ACCEPT"), HANDLER(SettingsForm::OnEnclosureFieldAcceptClicked));
	__pEnclosureFolderField->AddActionEventListener(*this);
	__pEnclosureFolderField->AddScrollPanelEventListener(*this);
	__pMainPanel->AddControl(*__pEnclosureFolderField);

	//OTHER SETTINGS
	Label *otherSettingsLabel = new Label;
	res = otherSettingsLabel->Construct(Rectangle(10,662,460,25),Utilities::GetString(L"IDS_SETTINGSFORM_LABEL_FEED_SETTINGS"));
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct other settings label", GetErrorMessage(res));
		return res;
	}
	otherSettingsLabel->SetTextConfig(25, LABEL_TEXT_STYLE_NORMAL);
	__pMainPanel->AddControl(*otherSettingsLabel);

	__pLoadImagesByDefaultCheckBox = new CheckButton;
	res = __pLoadImagesByDefaultCheckBox->Construct(Rectangle(10,707,460,100), CHECK_BUTTON_STYLE_ONOFF, BACKGROUND_STYLE_DEFAULT, false, Utilities::GetString(L"IDS_SETTINGSFORM_LOAD_IMAGES_BY_DEF_CHECKBOX"), GROUP_STYLE_SINGLE);
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct load imgs by default checkbox", GetErrorMessage(res));
		return res;
	}
	__pLoadImagesByDefaultCheckBox->SetActionId(triggerID, triggerID);
	__pLoadImagesByDefaultCheckBox->AddActionEventListener(*this);
	__pMainPanel->AddControl(*__pLoadImagesByDefaultCheckBox);

	Label *stretch = new Label;
	res = stretch->Construct(Rectangle(10,807,460,140),L" ");
	if (IsFailed(res)) {
		AppLogDebug("SettingsForm::Initialize: [%s]: failed to construct stretcher", GetErrorMessage(res));
		return res;
	}
	__pMainPanel->AddControl(*stretch);

	SetDefaultValues();

	if (!__pAutoUpdateCheckBox->IsSelected()) {
		__pAutoIntervalCheckBox->SetEnabled(false);
		__pUpdateIntervalField->SetEnabled(false);
	} else {
		__pAutoIntervalCheckBox->SetEnabled(true);
		if (__pAutoIntervalCheckBox->IsSelected()) {
			__pUpdateIntervalField->SetEnabled(false);
		} else {
			__pUpdateIntervalField->SetEnabled(true);
		}
	}
	if (!__pDefaultEnclosureFolderCheckBox->IsSelected()) {
		__pEnclosureFolderField->SetEnabled(true);
	} else {
		__pEnclosureFolderField->SetEnabled(false);
	}

	SetSoftkeyEnabled(SOFTKEY_0, false);
	return E_SUCCESS;
}

result SettingsForm::Terminate(void) {
	return E_SUCCESS;
}

result SettingsForm::OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID) {
	if (srcID == DIALOG_ID_SELECT_FOLDER && reason == REASON_DIALOG_SUCCESS) {
		if (pArgs || pArgs->GetCount() >= 1) {
			if (typeid(*(pArgs->GetAt(0))) == typeid(String)) {
				String *str = static_cast<String *>(pArgs->GetAt(0));
				if (str) {
					__pEnclosureFolderField->SetText(*str);
				}
			}
		}
		RequestRedraw(true);
	}
	return E_SUCCESS;
}

void SettingsForm::OnTextValueChanged(const Control &source) {
	if (__pUpdateIntervalField->GetTextLength() > 0) {
		String text = __pUpdateIntervalField->GetText();
		bool got_nonzero = false;
		for(int i = 0; i < text.GetLength(); i++) {
			mchar ch; text.GetCharAt(i, ch);
			if (!Character::IsDigit(ch) || (ch == L'0' && !got_nonzero)) {
				text.SetCharAt(L'*', i);
			} else {
				got_nonzero = true;
			}
		}
		text.Replace(L"*", L"", 0);
		if (text.IsEmpty()) {
			text = L"1";
		}
		__pUpdateIntervalField->SetText(text);
	} else {
		__pUpdateIntervalField->SetText(L"1");
	}
	SetFocus();
	OnSomethingChanged(source);
}

void SettingsForm::OnTextValueChangeCanceled(const Control &source) {
	SetFocus();
}

void SettingsForm::OnOverlayControlCreated(const Control &source) {
	__prevEnclosureFolderText = __pEnclosureFolderField->GetText();

	ArrayList *pArgs = new ArrayList;
	pArgs->Construct(2);
	pArgs->Add(*(new String(L"")));
	pArgs->Add(*(new Integer((int)MODE_CHOOSE_DIRECTORY)));

	SaveForm *pForm = new SaveForm;
	if (IsFailed(GetFormManager()->SwitchToForm(pForm, pArgs, DIALOG_ID_SELECT_FOLDER))) {
		GetFormManager()->DisposeForm(pForm);
	}

	pArgs->RemoveAll(true);
	delete pArgs;
}
