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
#include "MainForm.h"
#include "SaveForm.h"
#include "AddChannelForm.h"
#include "SettingsForm.h"
#include "FormManager.h"
#include "OpmlParser.h"
#include "FeedManager.h"
#include <FMedia.h>
#include <typeinfo>

using namespace Osp::Media;
using namespace Osp::Base::Utility;

MainForm::MainForm(void) {
	__pChannelForm = null;

	__pEmptyListIcon = null;
	__pEmptyListLabel = null;
	__pOptionMenu = null;
	__pMainPanel = null;
	__pSearchField = null;
	__pSearchLabel = null;
	__pChannelList = null;
	__pChannelListFormat = null;
	__pChannelListEditFormat = null;

	__softkeyEditDoneActionId = -1;
	__softkeyEditCancelActionId = -1;
	__pOrderSelectedItemNormal = null;
	__pOrderSelectedItemPressed = null;
	__orderIndex = -1;
	__pLowerButton = null;
	__pHigherButton = null;
	__editMode = MODE_NONE;

	__pSpinnerLabel = null;
	__pBackgroundLabel = null;
	__pTitleLabel = null;
	__pCancelButton = null;

	__cancelUpdateActionId = -1;
	__pAnimTimer = null;
	unsigned char *ptr = reinterpret_cast<unsigned char *>(&__loadingAnimData);
	for (unsigned int i = 0; i < sizeof(__loadingAnimData); i++) {
		*(ptr + i) = 0;
	}

	__pDownloadSession = null;
	__pDownloadedData = null;

	__pUpdateQueue = null;
	__pIconUpdateChannel = null;
	__updating = false;
	__newItemsCount = 0;

	__animItemIndex = -1;
	ptr = reinterpret_cast<unsigned char *>(&__smallLoadingAnimData);
	for (unsigned int i = 0; i < sizeof(__smallLoadingAnimData); i++) {
		*(ptr + i) = 0;
	}
}

MainForm::~MainForm(void) {
	if (__pChannelListFormat) {
		delete __pChannelListFormat;
	}
	if (__pChannelListEditFormat) {
		delete __pChannelListEditFormat;
	}
	if (__pOptionMenu) {
		delete __pOptionMenu;
	}
	if (__pAnimTimer) {
		__pAnimTimer->Cancel();
		delete __pAnimTimer;
	}
	if (__pUpdateQueue) {
		__pUpdateQueue->RemoveAll();
		delete __pUpdateQueue;
	}
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
	}
	if (__pDownloadedData) {
		delete __pDownloadedData;
	}
	if (__pOrderSelectedItemNormal) {
		delete __pOrderSelectedItemNormal;
		__pOrderSelectedItemNormal = null;
	}
	if (__pOrderSelectedItemPressed) {
		delete __pOrderSelectedItemPressed;
		__pOrderSelectedItemPressed = null;
	}
}

void MainForm::OnOptionMenuClicked(const Control &src) {
	if (__pOptionMenu) {
		__pOptionMenu->SetShowState(true);
		__pOptionMenu->Show();
	}
}

void MainForm::OnOptionAddChannelClicked(const Control &src) {
	AddChannelForm *pForm = new AddChannelForm;
	if (IsFailed(GetFormManager()->SwitchToForm(pForm, null, DIALOG_ID_ADD_CHANNEL))) {
		GetFormManager()->DisposeForm(pForm);
	}
}

void MainForm::OnCancelUpdateClicked(const Control &src) {
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
		__pDownloadSession = null;
	}
	if (__pDownloadedData) {
		delete __pDownloadedData;
		__pDownloadedData = null;
	}

	if (__animItemIndex >= 0) {
		const Channel *curChan = FeedManager::GetInstance()->GetChannel(__pChannelList->GetItemIdAt(__animItemIndex));
		if (curChan && curChan->GetItems()) {
			int count = curChan->GetItems()->GetUnreadItemsCount();

			CustomListItem *item = const_cast<CustomListItem *>(__pChannelList->GetItemAt(__animItemIndex));
			if (item && count > 0) {
				Bitmap *pUnreadBadge = Utilities::GetBitmapN(L"unread_badge.png");
				if (pUnreadBadge) {
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *pUnreadBadge, null);
					delete pUnreadBadge;
				}
				item->SetElement(LIST_ELEMENT_UNREAD_COUNT, count < 10 ? L" " + Integer::ToString(count) : (count > 99 ? L" ..." : Integer::ToString(count)));
				__pChannelList->RefreshItem(__animItemIndex);
			} else if (item) {
				Bitmap *pEmpty = Utilities::GetBitmapN(L"empty.png");
				if (pEmpty) {
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *pEmpty, null);
					delete pEmpty;
				}
				__pChannelList->RefreshItem(__animItemIndex);
			}
		}
	}

	__pIconUpdateChannel = null;
	__pUpdateQueue->RemoveAll();

	__pAnimTimer->Cancel();

	__animItemIndex = -1;

	__pSearchField->SetShowState(true);
	__pSearchLabel->SetShowState(true);

	__pMainPanel->SetPosition(0, 0);
	__pMainPanel->SetSize(480, 712);
	__pChannelList->SetPosition(0, 50);
	__pChannelList->SetSize(480, 662);

	SetFormStyle(GetFormStyle() | FORM_STYLE_OPTIONKEY);

	RemoveControl(*__pBackgroundLabel);
	RemoveControl(*__pCancelButton);
	RemoveControl(*__pSpinnerLabel);
	RemoveControl(*__pTitleLabel);

	__pBackgroundLabel = null;
	__pCancelButton = null;
	__pSpinnerLabel = null;
	__pTitleLabel = null;

	Draw();
	Show();

    NotificationManager* pNotiMgr = new NotificationManager();
    if (!IsFailed(pNotiMgr->Construct())) {
    	if (__newItemsCount > 0) {
    		String str = Utilities::GetString(L"IDS_NOTIFICATION_MESSAGE");
    		str.Replace(L"%s", Integer::ToString(__newItemsCount));

    		pNotiMgr->Notify(str, FeedManager::GetInstance()->GetGlobalUnreadCount());
    	}
    	__newItemsCount = 0;
    }
    delete pNotiMgr;

	__updating = false;
}

void MainForm::OnOptionUpdateChannelsClicked(const Control &src) {
	if (__updating || __pChannelList->GetItemCount() <= 0) {
		return;
	}

	__pBackgroundLabel = new Label;
	if (IsFailed(__pBackgroundLabel->Construct(Rectangle(0,0,400,80), L""))) {
		delete __pBackgroundLabel;
		__pBackgroundLabel = null;
		return;
	}

	Bitmap *pBG = Utilities::GetBitmapN(L"channel_list_item_bg.png");
	if (pBG) {
		__pBackgroundLabel->SetBackgroundBitmap(*pBG);
		delete pBG;
	}

	__pCancelButton = new Button;
	if (IsFailed(__pCancelButton->Construct(Rectangle(400,0,80,80), L""))) {
		delete __pBackgroundLabel;
		__pBackgroundLabel = null;
		delete __pCancelButton;
		__pCancelButton = null;
		return;
	}

	Bitmap *pButton = Utilities::GetBitmapN(L"mainform_header_update_cancel.png");
	Bitmap *pButtonPressed = Utilities::GetBitmapN(L"mainform_header_update_cancel_pressed.png");
	if (pButton) {
		__pCancelButton->SetNormalBackgroundBitmap(*pButton);
		delete pButton;
	}
	if (pButtonPressed) {
		__pCancelButton->SetPressedBackgroundBitmap(*pButtonPressed);
		delete pButtonPressed;
	}
	if (__cancelUpdateActionId < 0) {
		__cancelUpdateActionId = HANDLER(MainForm::OnCancelUpdateClicked);
	}
	__pCancelButton->SetActionId(__cancelUpdateActionId);
	__pCancelButton->AddActionEventListener(*this);

	__pSpinnerLabel = new Label;
	if (IsFailed(__pSpinnerLabel->Construct(Rectangle(14,14,56,56), L""))) {
		delete __pBackgroundLabel;
		__pBackgroundLabel = null;
		delete __pCancelButton;
		__pCancelButton = null;
		delete __pSpinnerLabel;
		__pSpinnerLabel = null;
		return;
	}
	__pSpinnerLabel->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap1);

	__pTitleLabel = new Label;
	if (IsFailed(__pTitleLabel->Construct(Rectangle(80,25,320,30), Utilities::GetString(L"IDS_MAINFORM_HEADER_UPDATING")))) {
		delete __pBackgroundLabel;
		__pBackgroundLabel = null;
		delete __pCancelButton;
		__pCancelButton = null;
		delete __pSpinnerLabel;
		__pSpinnerLabel = null;
		delete __pTitleLabel;
		__pTitleLabel = null;
		return;
	}
	__pTitleLabel->SetTextConfig(30, LABEL_TEXT_STYLE_NORMAL);
	__pTitleLabel->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	__pTitleLabel->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);

	__pSearchField->SetShowState(false);
	__pSearchLabel->SetShowState(false);

	__pMainPanel->SetPosition(0, 80);
	__pMainPanel->SetSize(480, 632);
	__pChannelList->SetPosition(0, 0);
	__pChannelList->SetSize(480, 632);

	SetFormStyle(GetFormStyle() & ~FORM_STYLE_OPTIONKEY);

	AddControl(*__pBackgroundLabel);
	AddControl(*__pCancelButton);
	AddControl(*__pSpinnerLabel);
	AddControl(*__pTitleLabel);

	Draw();
	Show();

	__pAnimTimer->Start(ANIM_INTERVAL);

	if (!__pUpdateQueue) {
		__pUpdateQueue = new QueueT<Channel *>;
		__pUpdateQueue->Construct(20);
	}

	Channel *firstChan = null;
	ArrayList *chans = FeedManager::GetInstance()->GetChannelsN();
	IEnumerator *pEnum = chans->GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *pChannel = static_cast<Channel *>(obj);
			__pUpdateQueue->Enqueue(pChannel);

			if (!firstChan) {
				firstChan = pChannel;
			}
		}
	}
	delete chans;

	__animItemIndex = 0;
	CustomListItem *item = const_cast<CustomListItem *>(__pChannelList->GetItemAt(__animItemIndex));
	if (item) {
		item->SetElement(LIST_ELEMENT_UNREAD_COUNT, L"");
		__pChannelList->RefreshItem(__animItemIndex);
	}

	__newItemsCount = 0;

	result res = FetchPage(firstChan->GetURL());
	if (IsFailed(res)) {
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_ERROR_CONNECT_FAILED"), MSGBOX_STYLE_OK);
		OnCancelUpdateClicked(src);
	}

	__updating = true;
}

result MainForm::FetchPage(const String &url, bool resetIcon) {
	if (resetIcon) {
		__pIconUpdateChannel = null;
	}

	result res = E_SUCCESS;
	while(true) {
		if (__pDownloadSession) {
			__pDownloadSession->CloseAllTransactions();
			delete __pDownloadSession;
			__pDownloadSession = null;
		}
		if (__pDownloadedData) {
			delete __pDownloadedData;
			__pDownloadedData = null;
		}

		__pDownloadSession = new HttpSession();
		res = __pDownloadSession->Construct(NET_HTTP_SESSION_MODE_NORMAL, null, url, null);
		if (IsFailed(res)) {
			AppLogDebug("MainForm::FetchPage: [%s]: failed to construct http session", GetErrorMessage(res));
			break;
		}

		HttpTransaction *pTransaction = __pDownloadSession->OpenTransactionN();
		if (!pTransaction) {
			res = GetLastResult();
			AppLogDebug("MainForm::FetchPage: [%s]: failed to construct http session", GetErrorMessage(res));
			break;
		}
		pTransaction->AddHttpTransactionListener(*this);

		HttpRequest *pRequest = const_cast<HttpRequest*>(pTransaction->GetRequest());
		pRequest->SetUri(url);
		pRequest->GetHeader()->AddField("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)");
		pRequest->SetMethod(NET_HTTP_METHOD_GET);

		res = pTransaction->Submit();
		if (IsFailed(res)) {
			AppLogDebug("MainForm::FetchPage: [%s]: failed to submit http transaction", GetErrorMessage(res));

			delete pTransaction;
			break;
		}
		return E_SUCCESS;
	}
	return res;
}

result MainForm::FetchNextChannel(void) {
	Channel *chan = null;
	__pUpdateQueue->Peek(chan);

	result res = E_SUCCESS;
	while (chan && __pUpdateQueue->GetCount() > 0) {
		res = FetchPage(chan->GetURL());
		if (IsFailed(res)) {
			SpinNextChannel(true);

			__pUpdateQueue->Dequeue(chan);
			if (chan) {
				int itemInd = __pChannelList->GetItemIndexFromItemId(chan->GetID());
				CustomListItem *item = const_cast<CustomListItem *>(__pChannelList->GetItemAt(itemInd));
				if (item) {
					Bitmap *icon = Utilities::GetBitmapN(L"update_error_badge.png");

					item->SetElement(LIST_ELEMENT_UNREAD_COUNT, L"");
					if (icon) {
						item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *icon, null);
						delete icon;
					}

					__pChannelList->RefreshItem(itemInd);
				}
			}
			__pUpdateQueue->Peek(chan);
		} else {
			return E_SUCCESS;
		}
	}

	OnCancelUpdateClicked(*this);//we've run out of channels
	return E_UNDERFLOW;
}

void MainForm::SetChannelUpdateError(void) {
	SpinNextChannel();

	Channel *chan = null;
	__pUpdateQueue->Dequeue(chan);

	if (!__pIconUpdateChannel && chan) {
		int itemInd = __pChannelList->GetItemIndexFromItemId(chan->GetID());
		CustomListItem *item = const_cast<CustomListItem *>(__pChannelList->GetItemAt(itemInd));
		if (item) {
			Bitmap *icon = Utilities::GetBitmapN(L"update_error_badge.png");

			item->SetElement(LIST_ELEMENT_UNREAD_COUNT, L"");
			if (icon) {
				item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *icon, null);
				delete icon;
			}

			__pChannelList->RefreshItem(itemInd);
		}
	}

	FetchNextChannel();
}

void MainForm::SpinNextChannel(bool error) {
	if (__animItemIndex + 1 >= __pChannelList->GetItemCount() || __animItemIndex < 0) {
		__animItemIndex = -1;
	} else {
		__animItemIndex++;
	}

	if (!error) {
		Channel *curChan = null;
		__pUpdateQueue->Peek(curChan);
		if (curChan && curChan->GetItems()) {
			int count = curChan->GetItems()->GetUnreadItemsCount();

			int itemInd = __pChannelList->GetItemIndexFromItemId(curChan->GetID());
			CustomListItem *item = const_cast<CustomListItem *>(__pChannelList->GetItemAt(itemInd));
			if (item && count > 0) {
				Bitmap *pUnreadBadge = Utilities::GetBitmapN(L"unread_badge.png");
				if (pUnreadBadge) {
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *pUnreadBadge, null);
					delete pUnreadBadge;
				}
				item->SetElement(LIST_ELEMENT_UNREAD_COUNT, count < 10 ? L" " + Integer::ToString(count) : (count > 99 ? L" ..." : Integer::ToString(count)));
				__pChannelList->RefreshItem(itemInd);
			} else if (item) {
				Bitmap *pEmpty = Utilities::GetBitmapN(L"empty.png");
				if (pEmpty) {
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *pEmpty, null);
					delete pEmpty;
				}
				__pChannelList->RefreshItem(itemInd);
			}
		}
	}

	if (__animItemIndex >= 0) {
		CustomListItem *item = const_cast<CustomListItem *>(__pChannelList->GetItemAt(__animItemIndex));
		if (item) {
			item->SetElement(LIST_ELEMENT_UNREAD_COUNT, L"");
			__pChannelList->RefreshItem(__animItemIndex);
		}
	}
}

void MainForm::OnOptionDeleteFeedsClicked(const Control &src) {
	ArrayList *chans = FeedManager::GetInstance()->GetChannelsN();
	FillChannelList(*chans, MODE_DELETING);
	delete chans;

	if (__pChannelList->GetItemCount() <= 0) {
		return;
	}

	SetFormStyle((GetFormStyle() | FORM_STYLE_SOFTKEY_0 | FORM_STYLE_SOFTKEY_1) & ~FORM_STYLE_OPTIONKEY);
	SetTitleIcon(null);
	SetTitleText(Utilities::GetString(L"IDS_MAINFORM_OPTIONMENU_EDIT_LIST"), ALIGNMENT_CENTER);

	if (__softkeyEditDoneActionId < 0) {
		__softkeyEditDoneActionId = HANDLER(MainForm::OnSoftkeyDoneClicked);
		AddSoftkeyActionListener(SOFTKEY_0, *this);
		SetSoftkeyActionId(SOFTKEY_0, __softkeyEditDoneActionId);
		SetSoftkeyText(SOFTKEY_0, Utilities::GetString(L"IDS_MAINFORM_SOFTKEY_FINISH_EDIT_LIST"));
	} else {
		SetSoftkeyText(SOFTKEY_0, Utilities::GetString(L"IDS_MAINFORM_SOFTKEY_FINISH_EDIT_LIST"));
	}

	if (__softkeyEditCancelActionId < 0) {
		__softkeyEditCancelActionId = HANDLER(MainForm::OnSoftkeyCancelClicked);
		AddSoftkeyActionListener(SOFTKEY_1, *this);
		SetSoftkeyActionId(SOFTKEY_1, __softkeyEditCancelActionId);
		SetSoftkeyText(SOFTKEY_1, Utilities::GetString(L"IDS_SAVEFORM_CANCEL_BUTTON"));
	}

	__pSearchField->SetShowState(false);
	__pSearchLabel->SetShowState(false);
	__pChannelList->SetPosition(0, 0);
	__pChannelList->SetSize(480, 712);

	SetFormStyle(GetFormStyle() & ~FORM_STYLE_SOFTKEY_0);
	RequestRedraw(true);
	__editMode = MODE_DELETING;
}

void MainForm::OnOptionChangeOrderClicked(const Control &src) {
	ArrayList *chans = FeedManager::GetInstance()->GetChannelsN();
	FillChannelList(*chans, MODE_CHANGING_ORDER);
	delete chans;

	if (__pChannelList->GetItemCount() <= 0) {
		return;
	}

	SetFormStyle((GetFormStyle() | FORM_STYLE_SOFTKEY_0 | FORM_STYLE_SOFTKEY_1) & ~FORM_STYLE_OPTIONKEY);
	SetTitleIcon(null);
	SetTitleText(Utilities::GetString(L"IDS_MAINFORM_OPTIONMENU_EDIT_LIST_ORDER"), ALIGNMENT_CENTER);

	if (__softkeyEditDoneActionId < 0) {
		__softkeyEditDoneActionId = HANDLER(MainForm::OnSoftkeyDoneClicked);
		AddSoftkeyActionListener(SOFTKEY_0, *this);
		SetSoftkeyActionId(SOFTKEY_0, __softkeyEditDoneActionId);
		SetSoftkeyText(SOFTKEY_0, Utilities::GetString(L"IDS_SETTINGSFORM_SOFTKEY_APPLY"));
	} else {
		SetSoftkeyText(SOFTKEY_0, Utilities::GetString(L"IDS_SETTINGSFORM_SOFTKEY_APPLY"));
	}

	if (__softkeyEditCancelActionId < 0) {
		__softkeyEditCancelActionId = HANDLER(MainForm::OnSoftkeyCancelClicked);
		AddSoftkeyActionListener(SOFTKEY_1, *this);
		SetSoftkeyActionId(SOFTKEY_1, __softkeyEditCancelActionId);
		SetSoftkeyText(SOFTKEY_1, Utilities::GetString(L"IDS_SAVEFORM_CANCEL_BUTTON"));
	}

	if (__pOrderSelectedItemNormal) {
		delete __pOrderSelectedItemNormal;
	}
	if (__pOrderSelectedItemPressed) {
		delete __pOrderSelectedItemPressed;
	}
	__pOrderSelectedItemNormal = Utilities::GetBitmapN(L"channel_list_item_bg_sel.png");
	__pOrderSelectedItemPressed = Utilities::GetBitmapN(L"channel_list_item_bg_pressed_sel.png");

	__pSearchField->SetShowState(false);
	__pSearchLabel->SetShowState(false);
	__pLowerButton->SetShowState(true);
	__pHigherButton->SetShowState(true);
	__pChannelList->SetPosition(0, 95);
	__pChannelList->SetSize(480, 617);

	__pHigherButton->SetEnabled(false);
	__pLowerButton->SetEnabled(false);

	SetFormStyle(GetFormStyle() & ~FORM_STYLE_SOFTKEY_0);
	RequestRedraw(true);
	__editMode = MODE_CHANGING_ORDER;
}

void MainForm::OnButtonLowerClicked(const Control &src) {
	if (__orderIndex >= (__pChannelList->GetItemCount() - 1)) {
		return;
	}

	int curId = __pChannelList->GetItemIdAt(__orderIndex);
	if (curId >= 0) {
		const Channel *pChannel = FeedManager::GetInstance()->GetChannel(curId);
		if (pChannel) {
			CustomListItem *pItem = new CustomListItem;
			pItem->Construct(110);
			pItem->SetItemFormat(*__pChannelListFormat);

			if (__pOrderSelectedItemNormal) {
				pItem->SetNormalItemBackgroundBitmap(*__pOrderSelectedItemNormal);
			}
			if (__pOrderSelectedItemPressed) {
				pItem->SetFocusedItemBackgroundBitmap(*__pOrderSelectedItemPressed);
			}

			pItem->SetElement(LIST_ELEMENT_ICON, *pChannel->GetFavicon(), null);
			pItem->SetElement(LIST_ELEMENT_TITLE, pChannel->GetTitle());
			pItem->SetElement(LIST_ELEMENT_DESCRIPTION, pChannel->GetDescription());

			__pChannelList->RemoveItemAt(__orderIndex);
			__pChannelList->InsertItemAt(__orderIndex + 1, *pItem, pChannel->GetID());

			__orderIndex++;
			if (__orderIndex <= 0) {
				__pHigherButton->SetEnabled(false);
				__pLowerButton->SetEnabled(true);
			} else if (__orderIndex >= (__pChannelList->GetItemCount() - 1)) {
				__pHigherButton->SetEnabled(true);
				__pLowerButton->SetEnabled(false);
			} else {
				__pHigherButton->SetEnabled(true);
				__pLowerButton->SetEnabled(true);
			}
			SetFormStyle(GetFormStyle() | FORM_STYLE_SOFTKEY_0);
			__pChannelList->Draw();
			__pChannelList->Show();
			RequestRedraw(true);
		}
	}
}

void MainForm::OnButtonHigherClicked(const Control &src) {
	if (__orderIndex <= 0) {
		return;
	}

	int curId = __pChannelList->GetItemIdAt(__orderIndex);
	if (curId >= 0) {
		const Channel *pChannel = FeedManager::GetInstance()->GetChannel(curId);
		if (pChannel) {
			CustomListItem *pItem = new CustomListItem;
			pItem->Construct(110);
			pItem->SetItemFormat(*__pChannelListFormat);

			if (__pOrderSelectedItemNormal) {
				pItem->SetNormalItemBackgroundBitmap(*__pOrderSelectedItemNormal);
			}
			if (__pOrderSelectedItemPressed) {
				pItem->SetFocusedItemBackgroundBitmap(*__pOrderSelectedItemPressed);
			}

			pItem->SetElement(LIST_ELEMENT_ICON, *pChannel->GetFavicon(), null);
			pItem->SetElement(LIST_ELEMENT_TITLE, pChannel->GetTitle());
			pItem->SetElement(LIST_ELEMENT_DESCRIPTION, pChannel->GetDescription());

			__pChannelList->RemoveItemAt(__orderIndex);
			__pChannelList->InsertItemAt(__orderIndex - 1, *pItem, pChannel->GetID());

			__orderIndex--;
			if (__orderIndex <= 0) {
				__pHigherButton->SetEnabled(false);
				__pLowerButton->SetEnabled(true);
			} else if (__orderIndex >= (__pChannelList->GetItemCount() - 1)) {
				__pHigherButton->SetEnabled(true);
				__pLowerButton->SetEnabled(false);
			} else {
				__pHigherButton->SetEnabled(true);
				__pLowerButton->SetEnabled(true);
			}
			SetFormStyle(GetFormStyle() | FORM_STYLE_SOFTKEY_0);
			__pChannelList->Draw();
			__pChannelList->Show();
			RequestRedraw(true);
		}
	}
}

void MainForm::OnSoftkeyDoneClicked(const Control &src) {
	if (__editMode == MODE_DELETING) {
		int curCheckedIndex = __pChannelList->GetFirstCheckedItemIndex();
		if (curCheckedIndex >= 0) {
			do {
				int itemId = __pChannelList->GetItemIdAt(curCheckedIndex);
				FeedManager::GetInstance()->DeleteChannel(itemId);

				curCheckedIndex = __pChannelList->GetNextCheckedItemIndexAfter(curCheckedIndex);
			} while (curCheckedIndex >= 0);
		}

		NotificationManager* pNotiMgr = new NotificationManager();
		if (!IsFailed(pNotiMgr->Construct())) {
			pNotiMgr->Notify(FeedManager::GetInstance()->GetGlobalUnreadCount());
		}
		delete pNotiMgr;
	} else if (__editMode == MODE_CHANGING_ORDER) {
		int *ids = new int[__pChannelList->GetItemCount()];
		for (int i = 0; i < __pChannelList->GetItemCount(); i++) {
			*(ids + i) = __pChannelList->GetItemIdAt(i);
		}
		FeedManager::GetInstance()->UpdateChannelsOrder(ids, __pChannelList->GetItemCount());
	}
	OnSoftkeyCancelClicked(src);
}

void MainForm::OnSoftkeyCancelClicked(const Control &src) {
	ArrayList *chans = FeedManager::GetInstance()->GetChannelsN();
	FillChannelList(*chans, MODE_NONE);
	delete chans;

	SetFormStyle((GetFormStyle() & ~FORM_STYLE_SOFTKEY_0 & ~FORM_STYLE_SOFTKEY_1) | FORM_STYLE_OPTIONKEY);
	Bitmap *pIcon = Utilities::GetBitmapN(L"mainform_icon.png");
	SetTitleIcon(pIcon);
	if (pIcon) {
		delete pIcon;
	}
	SetTitleText(Utilities::GetString(L"IDS_MAINFORM_TITLE"), ALIGNMENT_LEFT);

	if (__pOrderSelectedItemNormal) {
		delete __pOrderSelectedItemNormal;
		__pOrderSelectedItemNormal = null;
	}
	if (__pOrderSelectedItemPressed) {
		delete __pOrderSelectedItemPressed;
		__pOrderSelectedItemPressed = null;
	}

	__pSearchField->SetShowState(true);
	__pSearchLabel->SetShowState(true);
	__pLowerButton->SetShowState(false);
	__pHigherButton->SetShowState(false);
	__pChannelList->SetPosition(0, 50);
	__pChannelList->SetSize(480, 662);

	RequestRedraw(true);
	__editMode = MODE_NONE;
}

void MainForm::OnOptionExportClicked(const Control &src) {
	ArrayList *pArgs = new ArrayList;
	pArgs->Construct(2);
	pArgs->Add(*(new String(L"")));
	pArgs->Add(*(new Integer((int)MODE_SAVE_FILE)));

	SaveForm *pForm = new SaveForm;
	if (IsFailed(GetFormManager()->SwitchToForm(pForm, pArgs, DIALOG_ID_EXPORT))) {
		GetFormManager()->DisposeForm(pForm);
	}

	pArgs->RemoveAll(true);
	delete pArgs;
}

void MainForm::OnOptionImportClicked(const Control &src) {
	ArrayList *pArgs = new ArrayList;
	pArgs->Construct(2);
	pArgs->Add(*(new String(L"")));
	pArgs->Add(*(new Integer((int)MODE_OPEN_FILE)));

	SaveForm *pForm = new SaveForm;
	if (IsFailed(GetFormManager()->SwitchToForm(pForm, pArgs, DIALOG_ID_IMPORT))) {
		GetFormManager()->DisposeForm(pForm);
	}

	pArgs->RemoveAll(true);
	delete pArgs;
}

void MainForm::OnOptionSettingsClicked(const Control &src) {
	SettingsForm *pForm = new SettingsForm;
	if (IsFailed(GetFormManager()->SwitchToForm(pForm, null, DIALOG_ID_EXPORT))) {
		GetFormManager()->DisposeForm(pForm);
	}
}

void MainForm::OnKeypadSearchClicked(const Control &src) {
	__pMainPanel->CloseOverlayWindow();
	__pSearchField->RequestRedraw(true);

	ArrayList *chans = FeedManager::GetInstance()->FindChannelsN(__pSearchField->GetText());
	FillChannelList(*chans, MODE_NONE, false);
	delete chans;

	RequestRedraw(true);
}

void MainForm::OnKeypadClearClicked(const Control &src) {
	__pSearchField->Clear();
	OnKeypadSearchClicked(src);
}

result MainForm::FillChannelList(const ArrayList &pChannels, EditMode editMode, bool clearSearch) {
	__pChannelList->RemoveAllItems();

	Bitmap *pNorm = Utilities::GetBitmapN(L"channel_list_item_bg.png");
	Bitmap *pPressed = Utilities::GetBitmapN(L"channel_list_item_bg_pressed.png");
	Bitmap *pUnreadBadge = Utilities::GetBitmapN(L"unread_badge.png");

	IEnumerator *pEnum = pChannels.GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Object *obj = pEnum->GetCurrent();
		if (typeid(*obj) == typeid(Channel)) {
			Channel *pChannel = static_cast<Channel *>(obj);

			CustomListItem *pItem = new CustomListItem;
			pItem->Construct(110);
			pItem->SetItemFormat(*(editMode == MODE_DELETING ? __pChannelListEditFormat : __pChannelListFormat));
			if (pNorm) {
				pItem->SetNormalItemBackgroundBitmap(*pNorm);
			}
			if (pPressed) {
				pItem->SetFocusedItemBackgroundBitmap(*pPressed);
			}

			pItem->SetElement(LIST_ELEMENT_ICON, *pChannel->GetFavicon(), null);
			pItem->SetElement(LIST_ELEMENT_TITLE, pChannel->GetTitle());
			pItem->SetElement(LIST_ELEMENT_DESCRIPTION, pChannel->GetDescription());
			if (editMode == MODE_DELETING) {
				pItem->SetCheckBox(LIST_ELEMENT_DELETE_CHECKBOX);
			} else if (editMode == MODE_NONE) {
				if (pChannel->GetItems() && pChannel->GetItems()->GetUnreadItemsCount() > 0) {
					if (pUnreadBadge) {
						pItem->SetElement(LIST_ELEMENT_UNREAD_BADGE, *pUnreadBadge, null);
					}
					int count = pChannel->GetItems()->GetUnreadItemsCount();
					pItem->SetElement(LIST_ELEMENT_UNREAD_COUNT, count < 10 ? L" " + Integer::ToString(count) : (count > 99 ? L" ..." : Integer::ToString(count)));
				}
			}

			__pChannelList->AddItem(*pItem, pChannel->GetID());
		}
	}
	delete pEnum;

	if (pNorm) {
		delete pNorm;
	}
	if (pPressed) {
		delete pPressed;
	}
	if (pUnreadBadge) {
		delete pUnreadBadge;
	}

	if (clearSearch) {
		__pSearchField->Clear();
	}

	if (__pChannelList->GetItemCount() > 0) {
		__pEmptyListIcon->SetShowState(false);
		__pEmptyListLabel->SetShowState(false);
	} else if (__pSearchField->GetTextLength() <= 0) {
		__pEmptyListIcon->SetShowState(true);
		__pEmptyListLabel->SetShowState(true);
	}
	return E_SUCCESS;
}

result MainForm::Initialize(void) {
	result res = Construct(FORM_STYLE_INDICATOR | FORM_STYLE_TITLE | FORM_STYLE_OPTIONKEY);
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to construct parent form", GetErrorMessage(res));
		return res;
	}

	Bitmap *pIcon = Utilities::GetBitmapN(L"mainform_icon.png");
	SetTitleIcon(pIcon);
	if (pIcon) {
		delete pIcon;
	}
	SetTitleText(Utilities::GetString(L"IDS_MAINFORM_TITLE"), ALIGNMENT_LEFT);
	SetBackgroundColor(Color(0x0e0e0e, false));

	__pOptionMenu = new OptionMenu;
	res = __pOptionMenu->Construct();
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to construct option menu", GetErrorMessage(res));
		return res;
	}
	__pOptionMenu->AddActionEventListener(*this);
	AddOptionkeyActionListener(*this);
	SetOptionkeyActionId(HANDLER(MainForm::OnOptionMenuClicked));

	__pOptionMenu->AddItem(Utilities::GetString(L"IDS_MAINFORM_OPTIONMENU_ADD_CHANNEL"), HANDLER(MainForm::OnOptionAddChannelClicked));
	__pOptionMenu->AddItem(Utilities::GetString(L"IDS_MAINFORM_OPTIONMENU_EDIT_LIST"), HANDLER(MainForm::OnOptionDeleteFeedsClicked));
	__pOptionMenu->AddItem(Utilities::GetString(L"IDS_MAINFORM_OPTIONMENU_IMPORT_FROM_OPML"), HANDLER(MainForm::OnOptionImportClicked));
	__pOptionMenu->AddItem(Utilities::GetString(L"IDS_MAINFORM_OPTIONMENU_EDIT_LIST_ORDER"), HANDLER(MainForm::OnOptionChangeOrderClicked));
	__pOptionMenu->AddItem(Utilities::GetString(L"IDS_MAINFORM_OPTIONMENU_EXPORT_TO_OPML"), HANDLER(MainForm::OnOptionExportClicked));
	__pOptionMenu->AddItem(Utilities::GetString(L"IDS_MAINFORM_OPTIONMENU_SETTINGS"), HANDLER(MainForm::OnOptionSettingsClicked));
	__pOptionMenu->AddItem(Utilities::GetString(L"IDS_MAINFORM_OPTIONMENU_UPDATE_ALL_CHANNELS"), HANDLER(MainForm::OnOptionUpdateChannelsClicked));

	__pMainPanel = new ScrollPanel;
	res = __pMainPanel->Construct(Rectangle(0,0,480,712));
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to construct scroll panel", GetErrorMessage(res));
		return res;
	}
	AddControl(*__pMainPanel);

	Bitmap *pButtonNormal = Utilities::GetBitmapN(L"button_bg_unpressed.png");
	Bitmap *pButtonPressed = Utilities::GetBitmapN(L"button_bg_pressed.png");

	__pLowerButton = new Button;
	res = __pLowerButton->Construct(Rectangle(10, 10, 225, 75), Utilities::GetString(L"IDS_MAINFORM_LOWER_BUTTON"));
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to construct lower button", GetErrorMessage(res));
		return res;
	}
	if (pButtonNormal) {
		__pLowerButton->SetNormalBackgroundBitmap(*pButtonNormal);
	}
	if (pButtonPressed) {
		__pLowerButton->SetPressedBackgroundBitmap(*pButtonPressed);
	}
	__pLowerButton->SetActionId(HANDLER(MainForm::OnButtonLowerClicked));
	__pLowerButton->AddActionEventListener(*this);
	__pMainPanel->AddControl(*__pLowerButton);

	__pHigherButton = new Button;
	res = __pHigherButton->Construct(Rectangle(245, 10, 225, 75), Utilities::GetString(L"IDS_MAINFORM_HIGHER_BUTTON"));
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to construct higher button", GetErrorMessage(res));
		return res;
	}
	if (pButtonNormal) {
		__pHigherButton->SetNormalBackgroundBitmap(*pButtonNormal);
	}
	if (pButtonPressed) {
		__pHigherButton->SetPressedBackgroundBitmap(*pButtonPressed);
	}
	__pHigherButton->SetActionId(HANDLER(MainForm::OnButtonHigherClicked));
	__pHigherButton->AddActionEventListener(*this);
	__pMainPanel->AddControl(*__pHigherButton);

	if (pButtonNormal) {
		delete pButtonNormal;
	}
	if (pButtonPressed) {
		delete pButtonPressed;
	}

	__pSearchField = new EditField;
	res = __pSearchField->Construct(Rectangle(0,0,430,50), EDIT_FIELD_STYLE_NORMAL_SMALL, INPUT_STYLE_OVERLAY, false, 1000, GROUP_STYLE_NONE);
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to construct search field", GetErrorMessage(res));
		return res;
	}
	__pSearchField->SetGuideText(Utilities::GetString(L"IDS_MAINFORM_SEARCH_FIELD_GUIDE"));
	__pSearchField->SetOverlayKeypadCommandButton(COMMAND_BUTTON_POSITION_RIGHT, Utilities::GetString(L"IDS_MAINFORM_SEARCH_KEYPAD_CLEAR"), HANDLER(MainForm::OnKeypadClearClicked));
	__pSearchField->SetOverlayKeypadCommandButton(COMMAND_BUTTON_POSITION_LEFT, Utilities::GetString(L"IDS_MAINFORM_SEARCH_KEYPAD_SEARCH"), HANDLER(MainForm::OnKeypadSearchClicked));
	__pSearchField->AddActionEventListener(*this);
	__pSearchField->AddScrollPanelEventListener(*this);
	__pMainPanel->AddControl(*__pSearchField);

	__pSearchLabel = new Label;
	res = __pSearchLabel->Construct(Rectangle(430,0,50,50), L"");
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to construct search field icon", GetErrorMessage(res));
		return res;
	}
	Bitmap *pSearchIcon = Utilities::GetBitmapN(L"search_icon.png");
	if (pSearchIcon) {
		__pSearchLabel->SetBackgroundBitmap(*pSearchIcon);
		delete pSearchIcon;
	}
	__pMainPanel->AddControl(*__pSearchLabel);

	__pEmptyListIcon = new Label;
	res = __pEmptyListIcon->Construct(Rectangle(20,349,64,64), L" ");
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to construct empty list icon", GetErrorMessage(res));
		return res;
	}
	Bitmap *pEmptyBmp = Utilities::GetBitmapN(L"empty_list_icon.png");
	if (pEmptyBmp) {
		__pEmptyListIcon->SetBackgroundBitmap(*pEmptyBmp);
		delete pEmptyBmp;
	}
	__pMainPanel->AddControl(*__pEmptyListIcon);

	__pEmptyListLabel = new Label;
	res = __pEmptyListLabel->Construct(Rectangle(104,349,356,64), Utilities::GetString(L"IDS_MAINFORM_EMPTY_LIST_TEXT"));
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to construct empty list label", GetErrorMessage(res));
		return res;
	}
	__pEmptyListLabel->SetTextColor(Color::COLOR_WHITE);
	__pEmptyListLabel->SetTextConfig(32, LABEL_TEXT_STYLE_NORMAL);
	__pEmptyListLabel->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	__pEmptyListLabel->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
	__pMainPanel->AddControl(*__pEmptyListLabel);

	__pChannelList = new CustomList;
	res = __pChannelList->Construct(Rectangle(0,50,480,662), CUSTOM_LIST_STYLE_MARK, false);
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to construct channels list", GetErrorMessage(res));
		return res;
	}
	__pChannelList->SetTextOfEmptyList(L" ");
	__pChannelList->AddCustomItemEventListener(*this);
	__pMainPanel->AddControl(*__pChannelList);

	__pChannelListFormat = new CustomListItemFormat;
	__pChannelListFormat->Construct();
	__pChannelListFormat->AddElement(LIST_ELEMENT_ICON, Rectangle(10,17,16,16));
	__pChannelListFormat->AddElement(LIST_ELEMENT_TITLE, Rectangle(35,10,410,30), 32);
	__pChannelListFormat->AddElement(LIST_ELEMENT_DESCRIPTION, Rectangle(10,50,460,50), 25, Color::COLOR_GREY, Color::COLOR_GREY);
	__pChannelListFormat->AddElement(LIST_ELEMENT_UNREAD_BADGE, Rectangle(445,7,32,32));
	__pChannelListFormat->AddElement(LIST_ELEMENT_UNREAD_COUNT, Rectangle(452,13,30,20), 20);

	__pChannelListEditFormat = new CustomListItemFormat;
	__pChannelListEditFormat->Construct();
	__pChannelListEditFormat->AddElement(LIST_ELEMENT_ICON, Rectangle(10,17,16,16));
	__pChannelListEditFormat->AddElement(LIST_ELEMENT_TITLE, Rectangle(35,10,365,30), 32);
	__pChannelListEditFormat->AddElement(LIST_ELEMENT_DESCRIPTION, Rectangle(10,50,390,50), 25, Color::COLOR_GREY, Color::COLOR_GREY);
	__pChannelListEditFormat->AddElement(LIST_ELEMENT_DELETE_CHECKBOX, Rectangle(415,30,50,50));

	__pAnimTimer = new Timer;
	res = __pAnimTimer->Construct(*this);
	if (IsFailed(res)) {
		AppLogDebug("MainForm::Initialize: [%s]: failed to initialize animation timer", GetErrorMessage(res));
		return res;
	}

	__loadingAnimData.__pProgressBitmap1 = Utilities::GetBitmapN("progressing00.png");
	__loadingAnimData.__pProgressBitmap2 = Utilities::GetBitmapN("progressing01.png");
	__loadingAnimData.__pProgressBitmap3 = Utilities::GetBitmapN("progressing02.png");
	__loadingAnimData.__pProgressBitmap4 = Utilities::GetBitmapN("progressing03.png");
	__loadingAnimData.__pProgressBitmap5 = Utilities::GetBitmapN("progressing04.png");
	__loadingAnimData.__pProgressBitmap6 = Utilities::GetBitmapN("progressing05.png");
	__loadingAnimData.__pProgressBitmap7 = Utilities::GetBitmapN("progressing06.png");
	__loadingAnimData.__pProgressBitmap8 = Utilities::GetBitmapN("progressing07.png");

	Bitmap *bmps = reinterpret_cast<Bitmap *>(&__loadingAnimData);
	for (int i = 0; i < 8; i++) {
		if (!(bmps + i)) {
			AppLogDebug("MainForm::Initialize: failed to get bitmaps for loading animation");
			return E_FAILURE;
		}
	}

	__smallLoadingAnimData.__pProgressBitmap1 = Utilities::GetBitmapN("small_progressing00.png");
	__smallLoadingAnimData.__pProgressBitmap2 = Utilities::GetBitmapN("small_progressing01.png");
	__smallLoadingAnimData.__pProgressBitmap3 = Utilities::GetBitmapN("small_progressing02.png");
	__smallLoadingAnimData.__pProgressBitmap4 = Utilities::GetBitmapN("small_progressing03.png");
	__smallLoadingAnimData.__pProgressBitmap5 = Utilities::GetBitmapN("small_progressing04.png");
	__smallLoadingAnimData.__pProgressBitmap6 = Utilities::GetBitmapN("small_progressing05.png");
	__smallLoadingAnimData.__pProgressBitmap7 = Utilities::GetBitmapN("small_progressing06.png");
	__smallLoadingAnimData.__pProgressBitmap8 = Utilities::GetBitmapN("small_progressing07.png");

	bmps = reinterpret_cast<Bitmap *>(&__smallLoadingAnimData);
	for (int i = 0; i < 8; i++) {
		if (!(bmps + i)) {
			AppLogDebug("MainForm::Initialize: failed to get bitmaps for small loading animation");
			return E_FAILURE;
		}
	}

	__pChannelForm = new ChannelForm;
	if (IsFailed(GetFormManager()->AddToFrame(__pChannelForm))) {
		delete __pChannelForm;
		__pChannelForm = null;
	}

	return E_SUCCESS;
}

result MainForm::Terminate(void) {
	return E_SUCCESS;
}

result MainForm::OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID) {
	if (srcID == DIALOG_ID_EXPORT && reason == REASON_DIALOG_SUCCESS) {
		if (pArgs && pArgs->GetCount() > 0) {
			Object *obj = pArgs->GetAt(0);
			if (typeid(*obj) == typeid(String)) {
				String *str = static_cast<String *> (obj);
				ArrayList *chans = FeedManager::GetInstance()->GetChannelsN();
				if (chans) {
					OPMLParser parser;
					result res = parser.SaveToFile(*str, *chans);
					if (IsFailed(res)) {
						if (res == E_INVALID_FORMAT) {
							Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_SAVEFORM_MBOX_FILE_ERROR"), MSGBOX_STYLE_OK);
						} else {
							String msg = Utilities::GetString(L"IDS_SAVEFORM_MBOX_EXPORT_ERROR");
							msg.Replace(L"%s", String(GetErrorMessage(res)));
							Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), msg, MSGBOX_STYLE_OK);
						}
						delete chans;
						return res;
					} else {
						Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_FILE_SAVING_SUCCESS_MBOX_TITLE"), Utilities::GetString(L"IDS_SAVEFORM_FILE_SAVING_SUCCESS_MBOX_MSG"), MSGBOX_STYLE_OK);
					}
					delete chans;
				}
			}
		}
	} else if (srcID == DIALOG_ID_IMPORT && reason == REASON_DIALOG_SUCCESS) {
		__pSearchField->Clear();
		if (pArgs && pArgs->GetCount() > 0) {
			Object *obj = pArgs->GetAt(0);
			if (typeid(*obj) == typeid(String)) {
				String *str = static_cast<String *>(obj);

				OPMLParser parser;
				ArrayList *pChannels = parser.ParseFileN(*str);
				result res = GetLastResult();
				if (IsFailed(res)) {
					String str = Utilities::GetString(L"IDS_SAVEFORM_MBOX_FILE_IMPORT_ERROR");
					Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), str, MSGBOX_STYLE_OK);
					return res;
				} else {
					res = FeedManager::GetInstance()->AddChannels(*pChannels);
					if (!IsFailed(res)) {
						ArrayList *chans = FeedManager::GetInstance()->GetChannelsN();
						if (chans) {
							FillChannelList(*chans);
							delete chans;
						}
					}

					IEnumerator *pEnum = pChannels->GetEnumeratorN();
					while (!IsFailed(pEnum->MoveNext())) {
						Channel *chan = static_cast<Channel *>(pEnum->GetCurrent());

						FeedItemCollection *pCol = const_cast<FeedItemCollection *>(chan->GetItems());
						if (pCol) {
							pCol->RemoveAll(true);
							delete pCol;
						}
					}
					delete pEnum;

					pChannels->RemoveAll(true);
					delete pChannels;

					return res;
				}
			}
		}
	} else if (srcID == DIALOG_ID_ADD_CHANNEL && reason == REASON_DIALOG_SUCCESS) {
		__pSearchField->Clear();
		if (pArgs && pArgs->GetCount() > 0) {
			int prev = FeedManager::GetInstance()->GetGlobalUnreadCount();
			result res = FeedManager::GetInstance()->AddChannels(*pArgs);
			if (!IsFailed(res)) {
				ArrayList *chans = FeedManager::GetInstance()->GetChannelsN();
				FillChannelList(*chans);
				delete chans;

			    NotificationManager* pNotiMgr = new NotificationManager();
			    if (!IsFailed(pNotiMgr->Construct())) {
			    	int count = FeedManager::GetInstance()->GetGlobalUnreadCount() - prev;
			    	if (count > 0) {
			    		String str = Utilities::GetString(L"IDS_NOTIFICATION_MESSAGE");
			    		str.Replace(L"%s", Integer::ToString(count));

			    		pNotiMgr->Notify(str, FeedManager::GetInstance()->GetGlobalUnreadCount());
			    	}
			    }
			    delete pNotiMgr;
			} else {
				return res;
			}
		}
	} else if (reason == REASON_NONE || srcID == DIALOG_ID_VIEW_CHANNEL) {
		__pSearchField->Clear();
		ArrayList *chans = FeedManager::GetInstance()->GetChannelsN();
		if (chans) {
			FillChannelList(*chans);
			delete chans;

			__pLowerButton->SetShowState(false);
			__pHigherButton->SetShowState(false);
		} else {
			return GetLastResult();
		}
	}
	return E_SUCCESS;
}

void MainForm::OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status) {
	if (__editMode == MODE_DELETING) {
		int checkedCount = 0;
		for (int i = 0; i < __pChannelList->GetItemCount(); i++) {
			if (__pChannelList->IsItemChecked(i)) {
				checkedCount++;
			}
		}
		if (checkedCount > 0) {
			SetFormStyle(GetFormStyle() | FORM_STYLE_SOFTKEY_0);
		} else {
			SetFormStyle(GetFormStyle() & ~FORM_STYLE_SOFTKEY_0);
		}
		RequestRedraw(true);
	} else if (__editMode == MODE_CHANGING_ORDER) {
		if (__orderIndex >= 0) {
			CustomListItem *item = const_cast<CustomListItem *>(__pChannelList->GetItemAt(__orderIndex));
			if (item) {
				Bitmap *pNorm = Utilities::GetBitmapN(L"channel_list_item_bg.png");
				Bitmap *pPressed = Utilities::GetBitmapN(L"channel_list_item_bg_pressed.png");

				if (pNorm) {
					item->SetNormalItemBackgroundBitmap(*pNorm);
					delete pNorm;
				}
				if (pPressed) {
					item->SetFocusedItemBackgroundBitmap(*pPressed);
					delete pPressed;
				}

				__pChannelList->RefreshItem(__orderIndex);
			}
		}

		CustomListItem *item = const_cast<CustomListItem *>(__pChannelList->GetItemAt(index));
		if (item) {
			if (__pOrderSelectedItemNormal) {
				item->SetNormalItemBackgroundBitmap(*__pOrderSelectedItemNormal);
			}
			if (__pOrderSelectedItemPressed) {
				item->SetFocusedItemBackgroundBitmap(*__pOrderSelectedItemPressed);
			}

			__pChannelList->RefreshItem(index);
		}
		__orderIndex = index;

		if (__orderIndex <= 0) {
			__pHigherButton->SetEnabled(false);
			__pLowerButton->SetEnabled(true);
		} else if (__orderIndex >= (__pChannelList->GetItemCount() - 1)) {
			__pHigherButton->SetEnabled(true);
			__pLowerButton->SetEnabled(false);
		} else {
			__pHigherButton->SetEnabled(true);
			__pLowerButton->SetEnabled(true);
		}
		__pChannelList->Draw();
		__pChannelList->Show();
		RequestRedraw(true);
	} else if (!__updating) {
		//go to channel
		const Channel *chan = FeedManager::GetInstance()->GetChannel(itemId);
		if (chan) {
			ArrayList *pArgs = new ArrayList;
			pArgs->Construct(1);
			pArgs->Add(*chan);

			if (!__pChannelForm) {
				__pChannelForm = new ChannelForm;
			}
			GetFormManager()->SwitchToForm(__pChannelForm, pArgs, DIALOG_ID_VIEW_CHANNEL);
			__pChannelForm->RefreshForm();

			pArgs->RemoveAll(false);
			delete pArgs;
		}
	}
}

void MainForm::OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen) {
	ByteBuffer *pData = null;

	HttpResponse* pHttpResponse = httpTransaction.GetResponse();
	if(!pHttpResponse) {
		return;
	}

	HttpHeader *pHeader = pHttpResponse->GetHeader();
	if (!pHeader) {
		return;
	}

	if (pHttpResponse->GetStatusCode() == NET_HTTP_STATUS_OK) {
		if (!__pDownloadedData) {
			int downloadLen = 0;
			String content_length = L"";
			IList *pFields = pHeader->GetFieldNamesN();
			IEnumerator *pEnum = pFields->GetEnumeratorN();
			while (!IsFailed(pEnum->MoveNext())) {
				String *field = static_cast<String *>(pEnum->GetCurrent());
				if (field->Equals(L"Content-Length", false)) {
					IEnumerator *pVals = pHeader->GetFieldValuesN(*field);
					if (!IsFailed(pVals->MoveNext())) {
						content_length = *static_cast<String *>(pVals->GetCurrent());
					}
					delete pVals;
					break;
				}
			}
			delete pEnum;
			delete pFields;

			result res = E_SUCCESS;
			__pDownloadedData = new ByteBuffer;
			if (content_length.IsEmpty()) {
				res = __pDownloadedData->Construct(availableBodyLen);
			} else {
				res = Integer::Parse(content_length, downloadLen);
				if (!IsFailed(res)) {
					res = __pDownloadedData->Construct(downloadLen);
				}
			}
			if (IsFailed(res)) {
				delete __pDownloadedData;
				__pDownloadedData = null;
			}
		}
		pData = pHttpResponse->ReadBodyN();
		if (!pData) {
			AppLogDebug("MainForm::OnTransactionReadyToRead: [%s]: failed to get HTTP body", GetErrorMessage(GetLastResult()));
			return;
		}
	} else {
		AppLogDebug("MainForm::OnTransactionReadyToRead: failed to get proper HTTP response, status code: [%d], text: [%S]", (int)pHttpResponse->GetStatusCode(), pHttpResponse->GetStatusText().GetPointer());
	}

	if (pData && availableBodyLen > 0) {
		if (__pDownloadedData) {
			if (__pDownloadedData->GetPosition() >= __pDownloadedData->GetLimit()) {
				ByteBuffer *tmp = new ByteBuffer;
				if (IsFailed(tmp->Construct(__pDownloadedData->GetCapacity() + availableBodyLen))) {
					delete tmp;
				} else {
					__pDownloadedData->SetPosition(0);
					tmp->ReadFrom(*__pDownloadedData);

					delete __pDownloadedData;
					__pDownloadedData = tmp;
				}
			}
			__pDownloadedData->ReadFrom(*pData);
		}
		delete pData;
	}
}

void MainForm::OnTransactionAborted(HttpSession &httpSession, HttpTransaction &httpTransaction, result r) {
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
		__pDownloadSession = null;
	}

	SetChannelUpdateError();
}

void MainForm::OnTransactionHeaderCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction, int headerLen, bool rs) {
	static int redirect_count = 0;

	HttpResponse* pHttpResponse = httpTransaction.GetResponse();
	if(!pHttpResponse) {
		return;
	}

	if (pHttpResponse->GetStatusCode() == NET_HTTP_STATUS_MOVED_PERMANENTLY
		|| pHttpResponse->GetStatusCode() == NET_HTTP_STATUS_MOVED_TEMPORARILY) {
		HttpHeader *pHeader = pHttpResponse->GetHeader();

		String redirect_url;
		IList *pFields = pHeader->GetFieldNamesN();
		IEnumerator *pEnum = pFields->GetEnumeratorN();
		while (!IsFailed(pEnum->MoveNext())) {
			String *field = static_cast<String *>(pEnum->GetCurrent());
			if (field->Equals(L"Location", false)) {
				IEnumerator *pVals = pHeader->GetFieldValuesN(*field);
				if (!IsFailed(pVals->MoveNext())) {
					redirect_url = *static_cast<String *>(pVals->GetCurrent());
				}
				delete pVals;
				break;
			}
		}
		delete pEnum;
		delete pFields;

		if (__pDownloadSession) {
			__pDownloadSession->CloseAllTransactions();
			delete __pDownloadSession;
			__pDownloadSession = null;
		}

		if (!redirect_url.IsEmpty()) {
			if (redirect_count >= 10) {
				redirect_count = 0;
				SetChannelUpdateError();
				return;
			}
			redirect_count++;

			//download redirected url
			result res = FetchPage(redirect_url, false);
			if (IsFailed(res)) {
				AppLogDebug("MainForm::OnTransactionHeaderCompleted: [%s]: failed to initialize redirected download", GetErrorMessage(res));

				redirect_count = 0;
				SetChannelUpdateError();
				return;
			}
		} else {
			redirect_count = 0;
			SetChannelUpdateError();
		}
		return;
	}
	redirect_count = 0;
}

void MainForm::OnTransactionCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction) {
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
		__pDownloadSession = null;
	}

	if (__pDownloadedData && __pDownloadedData->GetLimit() > 0) {
		__pDownloadedData->SetPosition(0);
		if (__pIconUpdateChannel) {
			result res = FeedManager::GetInstance()->SetFaviconForChannel(__pIconUpdateChannel, *__pDownloadedData);
			if (!IsFailed(res)) {
				int itemInd = __pChannelList->GetItemIndexFromItemId(__pIconUpdateChannel->GetID());
				CustomListItem *item = const_cast<CustomListItem *>(__pChannelList->GetItemAt(itemInd));
				if (item) {
					item->SetElement(LIST_ELEMENT_ICON, *__pIconUpdateChannel->GetFavicon(), null);
					__pChannelList->RefreshItem(itemInd);
				}
			}

			__pIconUpdateChannel = null;

			SpinNextChannel();

			Channel *tmp = null;
			__pUpdateQueue->Dequeue(tmp);

			FetchNextChannel();
			return;
		}

		bool isXML = false;
		bool isRSS = false;
		xmlDocPtr doc = xmlReadMemory((const char*)__pDownloadedData->GetPointer(), __pDownloadedData->GetLimit(), null, null, XML_PARSE_NOCDATA);
		if (xmlLastError.code == XML_ERR_UNSUPPORTED_ENCODING) {
			const wchar_t *strData = Utilities::ConvertCP1251ToUCS2(__pDownloadedData->GetPointer(), __pDownloadedData->GetLimit());
			String conv_data = String(strData);

			int xmlInfoStartIndex = -1;
			int xmlInfoEndIndex = -1;
			if (!IsFailed(conv_data.IndexOf(L"encoding=\"", 0, xmlInfoStartIndex)) && !IsFailed(conv_data.IndexOf(L"\"", xmlInfoStartIndex + 10, xmlInfoEndIndex))) {
				conv_data.Remove(xmlInfoStartIndex + 10, xmlInfoEndIndex - xmlInfoStartIndex - 10);
				conv_data.Insert(L"utf-8", xmlInfoStartIndex + 10);

				ByteBuffer *buf = StringUtil::StringToUtf8N(conv_data);
				if (buf) {
					delete __pDownloadedData;
					__pDownloadedData = buf;

					doc = xmlReadMemory((const char*)__pDownloadedData->GetPointer(), __pDownloadedData->GetLimit(), null, null, XML_PARSE_NOCDATA);
				}
				delete []strData;
				xmlResetLastError();
			}
		}
		if (doc) {
			xmlNodePtr root = xmlDocGetRootElement(doc);
			if (root) {
				isXML = true;
				if (!strcmp((const char*)root->name, "rss") || !strcmp((const char*)root->name, "feed")) {
					isRSS = true;
				}
			} else {
				SetChannelUpdateError();
				return;
			}
			xmlFreeDoc(doc);
		} else {
			SetChannelUpdateError();
			return;
		}
		if (isRSS) {
			Channel *curChan = null;
			result res = __pUpdateQueue->Peek(curChan);
			if (!IsFailed(res)) {
				__newItemsCount += FeedManager::GetInstance()->UpdateChannelFromXML(curChan, (const char*)__pDownloadedData->GetPointer(), __pDownloadedData->GetLimit());

				String link = curChan->GetLink();
				if (link.StartsWith(L"http://", 0) && !__pIconUpdateChannel) {
					link.Remove(0, 7);

					int slashInd = -1;
					if (!IsFailed(link.IndexOf(L'/', 0, slashInd)) && slashInd >= 0) {
						link.Remove(slashInd, link.GetLength() - slashInd);
					}
					link.Insert(L"http://www.google.com/s2/favicons?domain=", 0);

					__pIconUpdateChannel = curChan;
					//download link (favicon)
					res = FetchPage(link, false);
					if (IsFailed(res)) {
						AppLogDebug("MainForm::OnTransactionCompleted: [%s]: failed to initialize icon download", GetErrorMessage(res));
					} else {
						return;
					}
				}
			}
			SpinNextChannel();

			__pUpdateQueue->Dequeue(curChan);
			FetchNextChannel();

			return;
		}
	}
	SetChannelUpdateError();
}

void MainForm::OnTimerExpired(Timer &timer) {
	if (__pSpinnerLabel) {
		static int progress = 0;
		int index = progress % 8;

		switch (index)
		{
		case 0:
			__pSpinnerLabel->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap1);
			break;
		case 1:
			__pSpinnerLabel->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap2);
			break;
		case 2:
			__pSpinnerLabel->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap3);
			break;
		case 3:
			__pSpinnerLabel->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap4);
			break;
		case 4:
			__pSpinnerLabel->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap5);
			break;
		case 5:
			__pSpinnerLabel->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap6);
			break;
		case 6:
			__pSpinnerLabel->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap7);
			break;
		case 7:
			__pSpinnerLabel->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap8);
			break;
		default:
			break;
		}

		if (__animItemIndex >= 0) {
			CustomListItem *item = const_cast<CustomListItem *>(__pChannelList->GetItemAt(__animItemIndex));
			if (item) {
				switch (index)
				{
				case 0:
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *__smallLoadingAnimData.__pProgressBitmap1, null);
					break;
				case 1:
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *__smallLoadingAnimData.__pProgressBitmap2, null);
					break;
				case 2:
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *__smallLoadingAnimData.__pProgressBitmap3, null);
					break;
				case 3:
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *__smallLoadingAnimData.__pProgressBitmap4, null);
					break;
				case 4:
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *__smallLoadingAnimData.__pProgressBitmap5, null);
					break;
				case 5:
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *__smallLoadingAnimData.__pProgressBitmap6, null);
					break;
				case 6:
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *__smallLoadingAnimData.__pProgressBitmap7, null);
					break;
				case 7:
					item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *__smallLoadingAnimData.__pProgressBitmap8, null);
					break;
				default:
					break;
				}
			}
			__pChannelList->RefreshItem(__animItemIndex);
		}
		progress++;

		__pSpinnerLabel->RequestRedraw(true);

		__pAnimTimer->Start(ANIM_INTERVAL);
	}
}
