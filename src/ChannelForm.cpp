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
#include "ChannelForm.h"
#include "FeedManager.h"
#include "ChannelInfoForm.h"
#include "FeedItemForm.h"
#include <FGraphics.h>
#include <typeinfo>

using namespace Osp::Graphics;
using namespace Osp::Base::Utility;

ChannelForm::ChannelForm() {
	__pOptionMenu = null;
	__pHeaderList = null;
	__pItemsList = null;
	__pHeaderFormat = null;
	__pItemFormat = null;
	__pLoadingHeaderFormat = null;

	__pChannel = null;

	__pDownloadSession = null;
	__pDownloadedData = null;

	__pParsingThread = null;
	__fetchingPage = false;
	__fetchingIcon = false;

	__pAnimTimer = null;
	unsigned char *ptr = reinterpret_cast<unsigned char *>(&__loadingAnimData);
	for (unsigned int i = 0; i < sizeof(__loadingAnimData); i++) {
		*(ptr + i) = 0;
	}
}

ChannelForm::~ChannelForm() {
	if (__pOptionMenu) {
		delete __pOptionMenu;
	}
	if (__pHeaderFormat) {
		delete __pHeaderFormat;
	}
	if (__pItemFormat) {
		delete __pItemFormat;
	}
	if (__pLoadingHeaderFormat) {
		delete __pLoadingHeaderFormat;
	}
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
	}
	if (__pParsingThread) {
		FeedManager::GetInstance()->GetMutex()->Acquire();
		delete __pParsingThread;
		FeedManager::GetInstance()->GetMutex()->Release();
	}
	if (__pDownloadedData) {
		delete __pDownloadedData;
	}
	if (__pAnimTimer) {
		__pAnimTimer->Cancel();
		delete __pAnimTimer;
	}
}

void ChannelForm::OnOptionMenuClicked(const Control &src) {
	if (__pOptionMenu) {
		__pOptionMenu->SetShowState(true);
		__pOptionMenu->Show();
	}
}

void ChannelForm::OnOptionMarkAllClicked(const Control &src) {
	FeedManager::GetInstance()->MarkAllItemsAsRead(const_cast<Channel *>(__pChannel));

    NotificationManager* pNotiMgr = new NotificationManager();
    if (!IsFailed(pNotiMgr->Construct())) {
   		pNotiMgr->Notify(FeedManager::GetInstance()->GetGlobalUnreadCount());
    }
    delete pNotiMgr;

	RefreshForm();
}

void ChannelForm::OnSoftkeyBackClicked(const Control &src) {
	CancelDownload();

	GetFormManager()->SwitchBack(null, REASON_DIALOG_CANCEL, false);

	__pItemsList->RemoveAllItems();
	__pItemsList->RemoveAllGroups();
}

result ChannelForm::RefreshForm(void) {
	if (!__pChannel) {
		AppLogDebug("ChannelForm::RefreshForm: attempt to refresh page without a channel set");
		return E_INVALID_STATE;
	}
	__pItemsList->RemoveAllItems();
	__pItemsList->RemoveAllGroups();

	const Channel *chan = __pChannel;

	const CustomListItem *pConstItem = __pHeaderList->GetItemAt(0);
	if (pConstItem) {
		CustomListItem *pItem = const_cast<CustomListItem *>(pConstItem);

		if (chan->GetPubDate()) {
			pItem->SetElement(LIST_HEADER_ELEMENT_PUBDATE, Utilities::FormatDateTimeFromEpoch(chan->GetPubDate()));
		} else {
			pItem->SetElement(LIST_HEADER_ELEMENT_PUBDATE, Utilities::GetString(L"IDS_CHANFORM_HEADER_NO_PUBDATE"));
		}
		__pHeaderList->RefreshItem(0);
	}

	__pItemsList->AddGroup(Utilities::GetString(L"IDS_CHANFORM_LIST_GROUP_TODAY"), null, 0);
	__pItemsList->AddGroup(Utilities::GetString(L"IDS_CHANFORM_LIST_GROUP_YESTERDAY"), null, 1);
	__pItemsList->AddGroup(Utilities::GetString(L"IDS_CHANFORM_LIST_GROUP_THIS_WEEK"), null, 2);
	__pItemsList->AddGroup(Utilities::GetString(L"IDS_CHANFORM_LIST_GROUP_LAST_WEEK"), null, 3);
	__pItemsList->AddGroup(Utilities::GetString(L"IDS_CHANFORM_LIST_GROUP_2WEEKS_AGO"), null, 4);
	__pItemsList->AddGroup(Utilities::GetString(L"IDS_CHANFORM_LIST_GROUP_3WEEKS_AGO"), null, 5);
	__pItemsList->AddGroup(Utilities::GetString(L"IDS_CHANFORM_LIST_GROUP_OLDER"), null, 6);

	Bitmap *pItemNorm = Utilities::GetBitmapN(L"entry_list_item_bg.png");
	Bitmap *pUnreadIcon = Utilities::GetBitmapN(L"not_read_icon_white.png");

	long long curDays = Utilities::GetCurrentUTCUnixTicks() / (1000 * 60 * 60 * 24);
	int dayOfWeek = Utilities::GetCurrentLocalDayOfWeek();
	if (dayOfWeek == -1) {
		//this means that today is sunday and the first day of week is monday
		dayOfWeek = 6;
	}

	if (chan->GetItems()) {
		IEnumerator *pEnum = chan->GetItems()->GetEnumeratorN();
		while (!IsFailed(pEnum->MoveNext())) {
			Object *item_arg = pEnum->GetCurrent();
			if (typeid(*item_arg) == typeid(FeedItem)) {
				FeedItem *item = static_cast<FeedItem *>(item_arg);

				int lines = Utilities::GetLinesForText(item->GetTitle(), 30, 446);

				CustomListItem *pEntry = new CustomListItem;
				pEntry->Construct(lines * 31 + 30);
				pEntry->SetItemFormat(*__pItemFormat);

				if (pItemNorm) {
					pEntry->SetNormalItemBackgroundBitmap(*pItemNorm);
				}

				pEntry->SetElement(LIST_ELEMENT_TITLE, item->GetTitle());

				if (!item->IsRead() && pUnreadIcon) {
					pEntry->SetElement(LIST_ELEMENT_UNREAD_BADGE, *pUnreadIcon, null);
				}

				long long itemDays = item->GetPubDate() / (1000 * 60 * 60 * 24);
				int group = 0;
				if (itemDays == curDays) {
					group = 0;
				} else if (curDays - itemDays == 1) {
					group = 1;
				} else if (curDays - itemDays <= dayOfWeek) {
					group = 2;
				} else if (itemDays < curDays - dayOfWeek && itemDays >= curDays - dayOfWeek - 7) {
					group = 3;
				} else if (itemDays < curDays - dayOfWeek - 7 && itemDays >= curDays - dayOfWeek - 14) {
					group = 4;
				} else if (itemDays < curDays - dayOfWeek - 14 && itemDays >= curDays - dayOfWeek - 21) {
					group = 5;
				} else {
					group = 6;
				}
				__pItemsList->AddItem(group, *pEntry, item->GetID());
			}
		}
		delete pEnum;
	}

	if (pItemNorm) {
		delete pItemNorm;
	}
	if (pUnreadIcon) {
		delete pUnreadIcon;
	}

	for (int i = 0; i < __pItemsList->GetGroupCount(); i++) {
		if (!__pItemsList->GetItemCountAt(i)) {
			__pItemsList->RemoveGroupAt(i);
			i = -1;
		}
	}

	__pItemsList->ScrollToTop();
	RequestRedraw(true);
	return E_SUCCESS;
}

result ChannelForm::FetchPage(const String &url) {
	if (!__pChannel) {
		AppLogDebug("ChannelForm::FetchPage: attempt to update channel without a channel set");
		return E_INVALID_STATE;
	}

	String fetchUrl = L"";
	if (url.IsEmpty()) {
		//fetchUrl = __pChannel->GetURL();
		/*if (!fetchUrl.StartsWith(L"http://", 0)) {
			fetchUrl.Insert(L"http://", 0);
		}*/
		String link = __pChannel->GetLink();
		if (link.StartsWith(L"http://", 0)) {
			link.Remove(0, 7);

			int slashInd = -1;
			if (!IsFailed(link.IndexOf(L'/', 0, slashInd)) && slashInd >= 0) {
				link.Remove(slashInd, link.GetLength() - slashInd);
			}
			link.Insert(L"http://www.google.com/s2/favicons?domain=", 0);

			fetchUrl = link;
			__fetchingIcon = true;
		} else {
			return E_INVALID_STATE;
		}
	} else {
		fetchUrl = url;
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
		res = __pDownloadSession->Construct(NET_HTTP_SESSION_MODE_NORMAL, null, fetchUrl, null);
		if (IsFailed(res)) {
			AppLogDebug("AddChannelForm::FetchPage: [%s]: failed to construct http session", GetErrorMessage(res));
			break;
		}

		HttpTransaction *pTransaction = __pDownloadSession->OpenTransactionN();
		if (!pTransaction) {
			res = GetLastResult();
			AppLogDebug("AddChannelForm::FetchPage: [%s]: failed to construct http session", GetErrorMessage(res));
			break;
		}
		pTransaction->AddHttpTransactionListener(*this);

		HttpRequest *pRequest = const_cast<HttpRequest*>(pTransaction->GetRequest());
		pRequest->SetUri(fetchUrl);
		pRequest->GetHeader()->AddField("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)");
		pRequest->SetMethod(NET_HTTP_METHOD_GET);

		res = pTransaction->Submit();
		if (IsFailed(res)) {
			AppLogDebug("AddChannelForm::FetchPage: [%s]: failed to submit http transaction", GetErrorMessage(res));

			delete pTransaction;
			break;
		}

		if (url.IsEmpty()) {
			CustomListItem *pHeader = new CustomListItem;
			pHeader->Construct(80);
			pHeader->SetItemFormat(*__pLoadingHeaderFormat);
			Bitmap *pNorm = Utilities::GetBitmapN(L"channel_header_item_bg.png");
			if (pNorm) {
				pHeader->SetNormalItemBackgroundBitmap(*pNorm);
				pHeader->SetFocusedItemBackgroundBitmap(*pNorm);
				delete pNorm;
			}
			pHeader->SetElement(LIST_LOADING_ELEMENT_TITLE, Utilities::GetString(L"IDS_CHANFORM_HEADER_UPDATING"));
			pHeader->SetElement(LIST_LOADING_ELEMENT_SPINNER, *__loadingAnimData.__pProgressBitmap1, null);
			__pHeaderList->AddItem(*pHeader, -2);

			__pItemsList->SetSize(480,552);
			__pItemsList->SetPosition(0,160);
			__pHeaderList->SetSize(480, 160);

			__pAnimTimer->Start(ANIM_INTERVAL);

			__pItemsList->SetEnabled(false);
			__pItemsList->ScrollToTop();

			SetFormStyle(GetFormStyle() & ~FORM_STYLE_OPTIONKEY);

			RequestRedraw(true);
		}

		__fetchingPage = true;
		return E_SUCCESS;
	}

	Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_ERROR_CONNECT_FAILED"), MSGBOX_STYLE_OK);
	return res;
}

void ChannelForm::CancelDownload(bool deleteThread) {
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
	}
	if (deleteThread) {
		//in case we try to cancel download (go back for example) while parsing xml
		FeedManager::GetInstance()->GetMutex()->Acquire();
		if (__pParsingThread) {
			delete __pParsingThread;
			__pParsingThread = null;
		}
		FeedManager::GetInstance()->GetMutex()->Release();
	}

	__fetchingPage = false;

	SetFormStyle(GetFormStyle() | FORM_STYLE_OPTIONKEY);
}

result ChannelForm::Initialize(void) {
	result res = Construct(FORM_STYLE_INDICATOR | FORM_STYLE_TITLE | FORM_STYLE_OPTIONKEY | FORM_STYLE_SOFTKEY_1);
	if (IsFailed(res)) {
		AppLogDebug("ChannelForm::Initialize: [%s]: failed to construct parent form", GetErrorMessage(res));
		return res;
	}

	Bitmap *pIcon = Utilities::GetBitmapN(L"mainform_icon.png");
	SetTitleIcon(pIcon);
	if (pIcon) {
		delete pIcon;
	}
	SetBackgroundColor(Color(0x0e0e0e, false));

	SetSoftkeyText(SOFTKEY_1, Utilities::GetString(L"IDS_CHANFORM_SOFTKEY_BACK"));
	SetSoftkeyActionId(SOFTKEY_1, HANDLER(ChannelForm::OnSoftkeyBackClicked));
	AddSoftkeyActionListener(SOFTKEY_1, *this);

	__pOptionMenu = new OptionMenu;
	res = __pOptionMenu->Construct();
	if (IsFailed(res)) {
		AppLogDebug("ChannelForm::Initialize: [%s]: failed to construct option menu", GetErrorMessage(res));
		return res;
	}
	__pOptionMenu->AddActionEventListener(*this);
	AddOptionkeyActionListener(*this);
	SetOptionkeyActionId(HANDLER(ChannelForm::OnOptionMenuClicked));

	__pOptionMenu->AddItem(Utilities::GetString(L"IDS_CHANFORM_OPTIONMENU_MARK_ALL"), HANDLER(ChannelForm::OnOptionMarkAllClicked));

	ScrollPanel *mainPanel = new ScrollPanel;
	res = mainPanel->Construct(Rectangle(0,0,480,712));
	if (IsFailed(res)) {
		AppLogDebug("ChannelForm::Initialize: [%s]: failed to construct scroll panel", GetErrorMessage(res));
		return res;
	}
	AddControl(*mainPanel);

	__pItemsList = new GroupedList;
	res = __pItemsList->Construct(Rectangle(0,80,480,632), CUSTOM_LIST_STYLE_NORMAL, false);
	if (IsFailed(res)) {
		AppLogDebug("ChannelForm::Initialize: [%s]: failed to construct items list", GetErrorMessage(res));
		return res;
	}
	__pItemsList->SetTextOfEmptyList(L" ");
	__pItemsList->AddGroupedItemEventListener(*this);

	mainPanel->AddControl(*__pItemsList);

	__pHeaderList = new CustomList;
	res = __pHeaderList->Construct(Rectangle(0,0,480,80), CUSTOM_LIST_STYLE_NORMAL, false);
	if (IsFailed(res)) {
		AppLogDebug("ChannelForm::Initialize: [%s]: failed to construct header list", GetErrorMessage(res));
		return res;
	}
	__pHeaderList->SetTextOfEmptyList(L" ");
	__pHeaderList->AddCustomItemEventListener(*this);
	mainPanel->AddControl(*__pHeaderList);

	__pHeaderFormat = new CustomListItemFormat;
	__pHeaderFormat->Construct();
	__pHeaderFormat->AddElement(LIST_HEADER_ELEMENT_INFO_ICON, Rectangle(10,24,32,32));
	__pHeaderFormat->AddElement(LIST_HEADER_ELEMENT_TITLE, Rectangle(52,10,348,30), 30);
	__pHeaderFormat->AddElement(LIST_HEADER_ELEMENT_PUBDATE, Rectangle(52,40,358,35), 33, Color::COLOR_GREY, Color::COLOR_GREY);
	__pHeaderFormat->AddElement(LIST_HEADER_ELEMENT_BUTTON, Rectangle(400,0,80,80));
	__pHeaderFormat->SetElementEventEnabled(LIST_HEADER_ELEMENT_BUTTON, true);

	__pItemFormat = new CustomListItemFormat;
	__pItemFormat->Construct();
	__pItemFormat->AddElement(LIST_ELEMENT_TITLE, Rectangle(10,15,446,200), 30);
	__pItemFormat->AddElement(LIST_ELEMENT_UNREAD_BADGE, Rectangle(456,19,16,16));

	__pLoadingHeaderFormat = new CustomListItemFormat;
	__pLoadingHeaderFormat->Construct();
	__pLoadingHeaderFormat->AddElement(LIST_LOADING_ELEMENT_SPINNER, Rectangle(14,14,56,56));
	__pLoadingHeaderFormat->AddElement(LIST_LOADING_ELEMENT_TITLE, Rectangle(80,25,390,30), 30);

	__pAnimTimer = new Timer;
	res = __pAnimTimer->Construct(*this);
	if (IsFailed(res)) {
		AppLogDebug("ChannelForm::Initialize: [%s]: failed to initialize animation timer", GetErrorMessage(res));
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
			AppLogDebug("ChannelForm::Initialize: failed to get bitmaps for loading animation");
			return E_FAILURE;
		}
	}

	CustomListItem *pHeader = new CustomListItem;
	pHeader->Construct(80);
	pHeader->SetItemFormat(*__pHeaderFormat);

	Bitmap *pNorm = Utilities::GetBitmapN(L"channel_header_item_bg.png");
	Bitmap *pPressed = Utilities::GetBitmapN(L"channel_header_item_bg_pressed.png");
	if (pNorm) {
		pHeader->SetNormalItemBackgroundBitmap(*pNorm);
		delete pNorm;
	}
	if (pPressed) {
		pHeader->SetFocusedItemBackgroundBitmap(*pPressed);
		delete pPressed;
	}

	pHeader->SetElement(LIST_HEADER_ELEMENT_TITLE, Utilities::GetString(L"IDS_CHANFORM_HEADER_TITLE"));
	pHeader->SetElement(LIST_HEADER_ELEMENT_PUBDATE, Utilities::GetString(L"IDS_CHANFORM_HEADER_NO_PUBDATE"));

	Bitmap *pButton = Utilities::GetBitmapN(L"channel_header_item_button.png");
	Bitmap *pButtonPressed = Utilities::GetBitmapN(L"channel_header_item_button_pressed.png");
	if (pButton) {
		pHeader->SetElement(LIST_HEADER_ELEMENT_BUTTON, *pButton, pButtonPressed);
		delete pButton;
	}
	if (pButtonPressed) {
		delete pButtonPressed;
	}

	Bitmap *pInfoIcon = Utilities::GetBitmapN(L"channel_header_info_icon.png");
	if (pInfoIcon) {
		pHeader->SetElement(LIST_HEADER_ELEMENT_INFO_ICON, *pInfoIcon, null);
		delete pInfoIcon;
	}

	__pHeaderList->AddItem(*pHeader, -1);

	return E_SUCCESS;
}

result ChannelForm::Terminate(void) {
	return E_SUCCESS;
}

result ChannelForm::OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID) {
	if (reason == REASON_DIALOG_SUCCESS && srcID == DIALOG_ID_VIEW_FEED_ITEM) {
		if (pArgs && pArgs->GetCount() > 0) {
			Object *arg = pArgs->GetAt(0);
			if (typeid(*arg) == typeid(ArrayListT<int>)) {
				ArrayListT<int> *read_items = static_cast<ArrayListT<int> *>(arg);

				Bitmap *pEmpty = Utilities::GetBitmapN(L"empty.png");
				if (pEmpty) {
					IEnumeratorT<int> *pEnum = read_items->GetEnumeratorN();
					while (!IsFailed(pEnum->MoveNext())) {
						int itemID = -1;
						pEnum->GetCurrent(itemID);
						if (itemID >= 0) {
							int itemIndex = -1, groupIndex = -1;
							__pItemsList->GetItemIndexFromItemId(itemID, groupIndex, itemIndex);
							if (itemIndex >= 0 && groupIndex >= 0) {
								CustomListItem *list_item = const_cast<CustomListItem *>(__pItemsList->GetItemAt(groupIndex, itemIndex));
								list_item->SetElement(LIST_ELEMENT_UNREAD_BADGE, *pEmpty, null);
							}
						}
					}
					delete pEnum;
					delete pEmpty;
				}
			} else {
				return E_INVALID_ARG;
			}
			if (pArgs->GetCount() > 1) {
				Object *ind = pArgs->GetAt(1);
				if (typeid(*ind) == typeid(Integer)) {
					Integer *go_ind = static_cast<Integer *>(ind);

					int grInd = -1, itInd = -1;
					__pItemsList->GetItemIndexFromItemId(go_ind->ToInt(), grInd, itInd);
					if (grInd >= 0 && itInd >= 0) {
						if (itInd > 3) {
							__pItemsList->ScrollToTop(grInd, itInd - 3);
						} else if (itInd == 3) {
							__pItemsList->ScrollToTop(grInd);
						} else if (grInd > 0) {
							int diff = itInd - 3;
							int prCount = __pItemsList->GetItemCountAt(grInd - 1);
							__pItemsList->ScrollToTop(grInd - 1, prCount + diff);
						} else {
							__pItemsList->ScrollToTop(0);
						}
					}
				} else {
					return E_INVALID_ARG;
				}
			}
		}
	} else if (reason == REASON_NONE) {
		if (pArgs && pArgs->GetCount() > 0) {
			Object *arg = pArgs->GetAt(0);
			if (typeid(*arg) == typeid(Channel)) {
				__pChannel = static_cast<Channel *>(arg);
				SetTitleText(__pChannel->GetTitle(), ALIGNMENT_LEFT);
			} else {
				return E_INVALID_ARG;
			}
			if (pArgs->GetCount() > 1) {
				Object *arg2 = pArgs->GetAt(1);
				if (typeid(*arg2) == typeid(Boolean)) {
					bool refresh = static_cast<Boolean *>(arg2)->ToBool();
					if (refresh) {
						RefreshForm();
					}
				}
			}
		} else {
			return E_INVALID_ARG;
		}
	}
	return E_SUCCESS;
}

void ChannelForm::OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status) {
	if (itemId == -1 && __pChannel) {
		ArrayList *pArgs = new ArrayList;
		pArgs->Construct(1);
		pArgs->Add(*__pChannel);

		ChannelInfoForm *pForm = new ChannelInfoForm;
		if (IsFailed(GetFormManager()->SwitchToForm(pForm, pArgs, DIALOG_ID_VIEW_CHANNEL_INFO))) {
			GetFormManager()->DisposeForm(pForm);
		}

		pArgs->RemoveAll(false);
		delete pArgs;
	}
}

void ChannelForm::OnItemStateChanged(const Control &source, int item_group_index, int item_id_index, int item_elem_Id, ItemStatus status) {
	if (!__fetchingPage) {
		if (source.Equals(*__pHeaderList) && item_elem_Id == LIST_HEADER_ELEMENT_BUTTON) {
			FetchPage();
		} else if (source.Equals(*__pItemsList)) {
			ArrayListT<int> *pItemIDs = new ArrayListT<int>;
			result res = pItemIDs->Construct(Utilities::GetGroupedListItemCount(*__pItemsList));
			if (!IsFailed(res)) {
				int index = 0;
				int cur_index = -1;
				for (int i = 0; i < __pItemsList->GetGroupCount(); i++) {
					for(int j = 0; j < __pItemsList->GetItemCountAt(i); j++) {
						int id = __pItemsList->GetItemIdAt(i, j);
						pItemIDs->Add(id);
						if (id == item_elem_Id) {
							cur_index = index;
						}
						index++;
					}
				}

				if (cur_index >= 0) {
					ArrayList *pArgs = new ArrayList;
					res = pArgs->Construct(3);
					if (!IsFailed(res)) {
						pArgs->Add(*__pChannel);
						pArgs->Add(*(new Integer(cur_index)));
						pArgs->Add(*pItemIDs);

						FeedItemForm *pForm = new FeedItemForm;
						if (IsFailed(GetFormManager()->SwitchToForm(pForm, pArgs, DIALOG_ID_VIEW_FEED_ITEM))) {
							GetFormManager()->DisposeForm(pForm);
						}
						pForm->RefreshData();

						pArgs->RemoveAt(1, true);
						pArgs->RemoveAll(false);
					}
					delete pArgs;
				}
			} else {
				delete pItemIDs;
			}
		}
	}
}

void ChannelForm::OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen) {
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
			result res = GetLastResult();
			AppLogDebug("ChannelForm::OnTransactionReadyToRead: [%s]: failed to get HTTP body", GetErrorMessage(res));
			return;
		}
	} else {
		AppLogDebug("ChannelForm::OnTransactionReadyToRead: failed to get proper HTTP response, status code: [%d], text: [%S]", (int)pHttpResponse->GetStatusCode(), pHttpResponse->GetStatusText().GetPointer());
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

void ChannelForm::OnTransactionAborted(HttpSession &httpSession, HttpTransaction &httpTransaction, result r) {
	CancelDownload();

	if (r == E_TIMEOUT || r == E_NOT_RESPONDING) {
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_CHANFORM_ERROR_TIMEOUT_MSG"), MSGBOX_STYLE_OK);
	} else if (r == E_NETWORK_UNAVAILABLE) {
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_ERROR_CONNECT_FAILED"), MSGBOX_STYLE_OK);
	} else if (r == E_HOST_UNREACHABLE || r == E_DNS) {
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_CHANFORM_UPDATE_ERROR_MBOX_MSG"), MSGBOX_STYLE_OK);
	} else {
		String msg = Utilities::GetString(L"IDS_CHANFORM_ERROR_UNKNOWN");
		msg.Replace(L"%s", String(GetErrorMessage(r)));
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), msg, MSGBOX_STYLE_OK);
	}
}

void ChannelForm::OnTransactionHeaderCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction, int headerLen, bool rs) {
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
				CancelDownload();

				Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_CHANFORM_UPDATE_ERROR_MBOX_MSG"), MSGBOX_STYLE_OK);

				redirect_count = 0;
				return;
			}
			redirect_count++;
			FetchPage(redirect_url);
		} else {
			CancelDownload();

			Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_CHANFORM_UPDATE_ERROR_MBOX_MSG"), MSGBOX_STYLE_OK);
			redirect_count = 0;
		}
		return;
	}
	redirect_count = 0;
}

void ChannelForm::OnTransactionCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction) {
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
		__pDownloadSession = null;
	}

	if (__pDownloadedData && __pDownloadedData->GetLimit() > 0) {
		__pDownloadedData->SetPosition(0);

		if (__fetchingIcon) {
			FeedManager::GetInstance()->SetFaviconForChannel(const_cast<Channel *>(__pChannel), *__pDownloadedData);
			__fetchingIcon = false;

			String fetchUrl = __pChannel->GetURL();
			if (!fetchUrl.StartsWith(L"http://", 0)) {
				fetchUrl.Insert(L"http://", 0);
			}
			FetchPage(fetchUrl);
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
				CancelDownload();

				Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_CHANFORM_UPDATE_FORMAT_ERROR_MSG"), MSGBOX_STYLE_OK);

				return;
			}
			xmlFreeDoc(doc);
		} else {
			CancelDownload();

			Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_CHANFORM_UPDATE_PARSE_ERROR_MSG"), MSGBOX_STYLE_OK);

			return;
		}
		if (isRSS) {
			if (__pParsingThread) {
				delete __pParsingThread;
			}

			__pParsingThread = new Thread;
			__pParsingThread->Construct(*this, 64 * 1024, THREAD_PRIORITY_LOW);

			__pParsingThread->Start();
			return;
		}
	}
	CancelDownload();

	Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_CHANFORM_UPDATE_ERROR_MBOX_MSG"), MSGBOX_STYLE_OK);
}

void ChannelForm::OnTimerExpired(Timer &timer) {
	const CustomListItem *pConstItem = __pHeaderList->GetItemAt(1);
	if (pConstItem) {
		static int progress = 0;
		int index = progress % 8;

		CustomListItem *pItem = const_cast<CustomListItem *>(pConstItem);
		switch (index)
		{
		case 0:
			pItem->SetElement(LIST_LOADING_ELEMENT_SPINNER, *__loadingAnimData.__pProgressBitmap1, null);
			break;
		case 1:
			pItem->SetElement(LIST_LOADING_ELEMENT_SPINNER, *__loadingAnimData.__pProgressBitmap2, null);
			break;
		case 2:
			pItem->SetElement(LIST_LOADING_ELEMENT_SPINNER, *__loadingAnimData.__pProgressBitmap3, null);
			break;
		case 3:
			pItem->SetElement(LIST_LOADING_ELEMENT_SPINNER, *__loadingAnimData.__pProgressBitmap4, null);
			break;
		case 4:
			pItem->SetElement(LIST_LOADING_ELEMENT_SPINNER, *__loadingAnimData.__pProgressBitmap5, null);
			break;
		case 5:
			pItem->SetElement(LIST_LOADING_ELEMENT_SPINNER, *__loadingAnimData.__pProgressBitmap6, null);
			break;
		case 6:
			pItem->SetElement(LIST_LOADING_ELEMENT_SPINNER, *__loadingAnimData.__pProgressBitmap7, null);
			break;
		case 7:
			pItem->SetElement(LIST_LOADING_ELEMENT_SPINNER, *__loadingAnimData.__pProgressBitmap8, null);
			break;
		default:
			break;
		}
		progress++;
		__pHeaderList->RefreshItem(1);

		if (__fetchingPage) {
			__pAnimTimer->Start(ANIM_INTERVAL);
		} else {
			__pHeaderList->RemoveItemAt(1);

			__pItemsList->SetSize(480,632);
			__pItemsList->SetPosition(0,80);
			__pHeaderList->SetSize(480,80);

			__pItemsList->SetEnabled(true);
		}
	}
}

Object *ChannelForm::Run(void) {
	if (__pChannel && __pDownloadedData) {
		int new_items = FeedManager::GetInstance()->UpdateChannelFromXML(const_cast<Channel *>(__pChannel), (const char*)__pDownloadedData->GetPointer(), __pDownloadedData->GetLimit());

		delete __pDownloadedData;
		__pDownloadedData = null;

		NotificationManager* pNotiMgr = new NotificationManager();
	    if (!IsFailed(pNotiMgr->Construct())) {
	    	if (new_items > 0) {
	    		String str = Utilities::GetString(L"IDS_NOTIFICATION_MESSAGE");
	    		str.Replace(L"%s", Integer::ToString(new_items));

	    		pNotiMgr->Notify(str, FeedManager::GetInstance()->GetGlobalUnreadCount());
	    	}
	    }
	    delete pNotiMgr;
	}
	CancelDownload(false);
	RefreshForm();
	return null;
}
