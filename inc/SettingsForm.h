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
#ifndef SETTINGSFORM_H_
#define SETTINGSFORM_H_

#include "BasicForm.h"
#include "Utilities.h"

class SettingsForm: public BasicForm, public ITextEventListener, public IScrollPanelEventListener {
public:
	SettingsForm(void);
	virtual ~SettingsForm(void) { }

private:
	void SetDefaultValues(void);
	void RearrangeControls(void);

	void OnSoftkeySaveClicked(const Control &src);
	void OnSoftkeyBackClicked(const Control &src);
	void OnEnclosureFieldAcceptClicked(const Control &src);
	void OnEnclosureFieldCancelClicked(const Control &src);
	void OnSomethingChanged(const Control &src);

	virtual result Initialize(void);
	virtual result Terminate(void);

	virtual result OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID);

	virtual void OnTextValueChanged(const Control &source);
	virtual void OnTextValueChangeCanceled(const Control &source);

	virtual void OnOverlayControlCreated(const Control &source);
	virtual void OnOverlayControlOpened(const Control &source) { }
	virtual void OnOverlayControlClosed(const Control &source) { __pAutoUpdateCheckBox->SetFocus(); }
	virtual void OnOtherControlSelected(const Control &source) { }

	static const int DIALOG_ID_SELECT_FOLDER = 401;

	ScrollPanel *__pMainPanel;
	CheckButton *__pAutoUpdateCheckBox;
	CheckButton *__pAutoIntervalCheckBox;
	EditField *__pUpdateIntervalField;
	CheckButton *__pDefaultEnclosureFolderCheckBox;
	EditField *__pEnclosureFolderField;
	CheckButton *__pLoadImagesByDefaultCheckBox;

	String __prevEnclosureFolderText;
};

#endif
