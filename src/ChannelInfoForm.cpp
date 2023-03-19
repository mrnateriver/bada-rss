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
#include "ChannelInfoForm.h"
#include "Utilities.h"
#include "FeedManager.h"
#include <typeinfo>

ChannelInfoForm::ChannelInfoForm(void) {
	__pMainPanel = null;
	__pURLField = null;
	__pTTLSlider = null;

	__prevURL = L"";
	__pChannel = null;
}

void ChannelInfoForm::OnSoftkeySaveClicked(const Control &src) {
	FeedManager::GetInstance()->SetURLAndTTLForChannel(__pChannel, __pURLField->GetText(), __pTTLSlider->GetValue());
	GetFormManager()->SwitchBack(null, REASON_DIALOG_SUCCESS);
}

void ChannelInfoForm::OnSoftkeyBackClicked(const Control &src) {
	GetFormManager()->SwitchBack(null, REASON_DIALOG_CANCEL);
}

void ChannelInfoForm::OnKeypadEnterClicked(const Control &src) {
	if (!__prevURL.Equals(__pURLField->GetText(), false)) {
		SetFormStyle(GetFormStyle() | FORM_STYLE_SOFTKEY_0);
		SetSoftkeyText(SOFTKEY_1, Utilities::GetString(L"IDS_SAVEFORM_CANCEL_BUTTON"));
	}

	__pMainPanel->CloseOverlayWindow();
	RequestRedraw(true);
}

void ChannelInfoForm::OnKeypadCancelClicked(const Control &src) {
	__pURLField->SetText(__prevURL);
	OnKeypadEnterClicked(src);
}

int ChannelInfoForm::AddEntitledLabel(const String &str, const String &title, const Point &place, bool href, int width) {
	Bitmap *pLabelBGTop = Utilities::GetBitmapN(L"label_bg_top.png");
	Bitmap *pLabelBGMiddle = Utilities::GetBitmapN(L"label_bg_middle.png");
	Bitmap *pLabelBGBottom = Utilities::GetBitmapN(L"label_bg_bottom.png");

	if (!pLabelBGTop || !pLabelBGMiddle || !pLabelBGBottom) {
		AppLogDebug("ChannelInfoForm::AddEntitledLabel: failed to get required bitmaps");

		if (pLabelBGTop) {
			delete pLabelBGTop;
		}
		if (pLabelBGMiddle) {
			delete pLabelBGMiddle;
		}
		if (pLabelBGBottom) {
			delete pLabelBGBottom;
		}
		return -1;
	}

	Label *labelTop = new Label;
	result res = labelTop->Construct(Rectangle(place.x,place.y,width,7), L"");
	if (IsFailed(res)) {
		AppLogDebug("ChannelInfoForm::AddEntitledLabel: [%s]: failed to construct top label", GetErrorMessage(res));
		return res;
	}
	labelTop->SetBackgroundBitmap(*pLabelBGTop);

	int lines = Utilities::GetLinesForText(str, 27, width-52);

	Label *labelMiddle = new Label;
	res = labelMiddle->Construct(Rectangle(place.x,place.y+7,width,lines*28+60), L"");
	if (IsFailed(res)) {
		AppLogDebug("ChannelInfoForm::AddEntitledLabel: [%s]: failed to construct middle label", GetErrorMessage(res));
		return res;
	}
	labelMiddle->SetBackgroundBitmap(*pLabelBGMiddle);

	Label *labelBottom = new Label;
	res = labelBottom->Construct(Rectangle(place.x,place.y+67+lines*28,width,7), L"");
	if (IsFailed(res)) {
		AppLogDebug("ChannelInfoForm::AddEntitledLabel: [%s]: failed to construct bottom label", GetErrorMessage(res));
		return res;
	}
	labelBottom->SetBackgroundBitmap(*pLabelBGBottom);

	Label *labelTitle = new Label;
	res = labelTitle->Construct(Rectangle(place.x + 16,place.y + 15,width-32,30), title);
	if (IsFailed(res)) {
		AppLogDebug("ChannelInfoForm::AddEntitledLabel: [%s]: failed to construct title label", GetErrorMessage(res));
		return res;
	}
	labelTitle->SetTextColor(Color(0x77badd, false));
	labelTitle->SetTextConfig(30, LABEL_TEXT_STYLE_NORMAL);
	labelTitle->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	labelTitle->SetTextVerticalAlignment(ALIGNMENT_TOP);

	Label *labelText = new Label;
	res = labelText->Construct(Rectangle(place.x + 16,place.y+55,width-32,lines*28), str);
	if (IsFailed(res)) {
		AppLogDebug("ChannelInfoForm::AddEntitledLabel: [%s]: failed to construct text label", GetErrorMessage(res));
		return res;
	}
	labelText->SetTextConfig(28, LABEL_TEXT_STYLE_NORMAL);
	labelText->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	labelText->SetTextVerticalAlignment(ALIGNMENT_TOP);
	if (href) {
		labelText->AddTouchEventListener(*this);
		labelText->SetTextColor(Color(0xc5e3fc, false));
	}

	__pMainPanel->AddControl(*labelTop);
	__pMainPanel->AddControl(*labelMiddle);
	__pMainPanel->AddControl(*labelBottom);
	__pMainPanel->AddControl(*labelTitle);
	__pMainPanel->AddControl(*labelText);

	if (pLabelBGTop) {
		delete pLabelBGTop;
	}
	if (pLabelBGMiddle) {
		delete pLabelBGMiddle;
	}
	if (pLabelBGBottom) {
		delete pLabelBGBottom;
	}

	return place.y+74+lines*28;
}

result ChannelInfoForm::Initialize(void) {
	result res = Construct(FORM_STYLE_INDICATOR | FORM_STYLE_TITLE | FORM_STYLE_SOFTKEY_0 | FORM_STYLE_SOFTKEY_1);
	if (IsFailed(res)) {
		AppLogDebug("ChannelInfoForm::Initialize: [%s]: failed to construct parent form", GetErrorMessage(res));
		return res;
	}
	SetBackgroundColor(Color(0x0e0e0e, false));
	SetTitleText(Utilities::GetString(L"IDS_CHANNELINFOFORM_TITLE"));

	SetSoftkeyText(SOFTKEY_1, Utilities::GetString(L"IDS_CHANFORM_SOFTKEY_BACK"));
	SetSoftkeyActionId(SOFTKEY_1, HANDLER(ChannelInfoForm::OnSoftkeyBackClicked));
	SetSoftkeyText(SOFTKEY_0, Utilities::GetString(L"IDS_SAVEFORM_ACCEPT_BUTTON"));
	SetSoftkeyActionId(SOFTKEY_0, HANDLER(ChannelInfoForm::OnSoftkeySaveClicked));
	AddSoftkeyActionListener(SOFTKEY_0, *this);
	AddSoftkeyActionListener(SOFTKEY_1, *this);

	__pMainPanel = new ScrollPanel;
	res = __pMainPanel->Construct(Rectangle(0,0,480,712));
	if (IsFailed(res)) {
		AppLogDebug("ChannelInfoForm::Initialize: [%s]: failed to construct scroll panel", GetErrorMessage(res));
		return res;
	}
	AddControl(*__pMainPanel);

	SetFormStyle(GetFormStyle() & ~FORM_STYLE_SOFTKEY_0);
	return E_SUCCESS;
}

result ChannelInfoForm::Terminate(void) {
	return E_SUCCESS;
}

result ChannelInfoForm::OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID) {
	if (reason == REASON_NONE) {
		if (pArgs && pArgs->GetCount() > 0) {
			Object *arg = pArgs->GetAt(0);
			if (typeid(*arg) == typeid(Channel)) {
				__pChannel = static_cast<Channel *>(arg);

				int yEnd = AddEntitledLabel(__pChannel->GetTitle(), Utilities::GetString(L"IDS_CHANNELINFOFORM_CHANNEL_TITLE"), Point(10,10));
				yEnd = AddEntitledLabel(__pChannel->GetDescription(), Utilities::GetString(L"IDS_CHANNELINFOFORM_CHANNEL_DESCRIPTION"), Point(10,yEnd+10));
				yEnd = AddEntitledLabel(__pChannel->GetLink(), Utilities::GetString(L"IDS_CHANNELINFOFORM_CHANNEL_LINK"), Point(10,yEnd+10), true);
				if (__pChannel->GetPubDate()) {
					yEnd = AddEntitledLabel(Utilities::FormatDateTimeFromEpoch(__pChannel->GetPubDate()), Utilities::GetString(L"IDS_CHANFORM_HEADER_TITLE"), Point(10, yEnd+10));
				} else {
					yEnd = AddEntitledLabel(Utilities::GetString(L"IDS_CHANFORM_HEADER_NO_PUBDATE"), Utilities::GetString(L"IDS_CHANFORM_HEADER_TITLE"), Point(10, yEnd+10));
				}

				//String warnStr = Utilities::GetString(L"IDS_CHANNELINFOFORM_CHANNEL_WARNING");
				//int warnLines = Utilities::GetLinesForText(warnStr, 25, 440);

				/*Label *warnLabel = new Label;
				if (!IsFailed(warnLabel->Construct(Rectangle(10,yEnd+10,460,warnLines * 26), warnStr))) {
					warnLabel->SetTextConfig(25, LABEL_TEXT_STYLE_NORMAL);
					warnLabel->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
					warnLabel->SetTextVerticalAlignment(ALIGNMENT_TOP);

					yEnd += 10 + warnLines * 26;
					__pMainPanel->AddControl(*warnLabel);
				} else {
					delete warnLabel;
				}*/

				/*__pURLField = new EditField;
				if (!IsFailed(__pURLField->Construct(Rectangle(10,yEnd + 10,460,106), EDIT_FIELD_STYLE_URL, INPUT_STYLE_OVERLAY, true, 1000, GROUP_STYLE_SINGLE))) {
					__pURLField->SetTitleText(Utilities::GetString(L"IDS_CHANNELINFOFORM_CHANNEL_URL"));
					__pURLField->SetText(__pChannel->GetURL());

					__pURLField->SetOverlayKeypadCommandButton(COMMAND_BUTTON_POSITION_RIGHT, Utilities::GetString(L"IDS_SAVEFORM_CANCEL_BUTTON"), HANDLER(ChannelInfoForm::OnKeypadCancelClicked));
					__pURLField->SetOverlayKeypadCommandButton(COMMAND_BUTTON_POSITION_LEFT, Utilities::GetString(L"IDS_CHANNELINFOFORM_KEYPAD_ACCEPT"), HANDLER(ChannelInfoForm::OnKeypadEnterClicked));
					__pURLField->AddActionEventListener(*this);
					__pURLField->AddScrollPanelEventListener(*this);

					yEnd += 116;
					__pMainPanel->AddControl(*__pURLField);
				} else {
					delete __pURLField;
					__pURLField = null;
				}

				__pTTLSlider = new Slider;
				if (!IsFailed(__pTTLSlider->Construct(Rectangle(10,yEnd+10,460,116),BACKGROUND_STYLE_DEFAULT,true,0,180,GROUP_STYLE_SINGLE))) {
					__pTTLSlider->SetTitleText(Utilities::GetString(L"IDS_CHANNELINFOFORM_CHANNEL_TTL"));
					__pTTLSlider->SetValue(__pChannel->GetTTL() > 180 ? 180 : __pChannel->GetTTL());

					__pTTLSlider->AddAdjustmentEventListener(*this);

					yEnd += 126;
					__pMainPanel->AddControl(*__pTTLSlider);
				} else {
					delete __pTTLSlider;
					__pTTLSlider = null;
				}

				Label *spring = new Label;
				spring->Construct(Rectangle(10,yEnd + 10,460,110), L" ");
				__pMainPanel->AddControl(*spring);*/
			} else {
				return E_INVALID_ARG;
			}
		} else {
			return E_INVALID_ARG;
		}
	}
	return E_SUCCESS;
}

void ChannelInfoForm::OnTouchReleased(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) {
	ArrayList* pDataList = new ArrayList();
	pDataList->Construct();

	const Label &label = static_cast<const Label &>(source);
	String* pData = new String(L"url:" + label.GetText());
	pDataList->Add(*pData);

	AppControl* pAc = AppManager::FindAppControlN(APPCONTROL_BROWSER, "");
	if(pAc)	{
	  pAc->Start(pDataList, null);
	  delete pAc;
	}

	pDataList->RemoveAll(true);
	delete pDataList;
}

void ChannelInfoForm::OnAdjustmentValueChanged(const Control &source, int adjustment) {
	SetFormStyle(GetFormStyle() | FORM_STYLE_SOFTKEY_0);
	SetSoftkeyText(SOFTKEY_1, Utilities::GetString(L"IDS_SAVEFORM_CANCEL_BUTTON"));
	RequestRedraw(true);
}
