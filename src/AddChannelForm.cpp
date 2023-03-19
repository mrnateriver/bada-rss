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
#include "AddChannelForm.h"
#include "Channel.h"
#include "Utilities.h"
#include <FGraphics.h>
#include <FXml.h>

using namespace Osp::Base::Utility;
using namespace Osp::Graphics;
using namespace Osp::Xml;

AddChannelForm::AddChannelForm(void) {
	__pURLField = null;
	__pFindChannelsButton = null;
	__pFoundChannelsList = null;
	__pChannelsListFormat = null;
	__checkMode = false;

	__pDownloadSession = null;
	__pDownloadedData = null;

	__pDownloadPopup = null;
	__searchingChannels = false;
	__pChannelsToAdd = null;
	__pFoundChannels = null;
}

AddChannelForm::~AddChannelForm(void) {
	if (__pChannelsListFormat) {
		delete __pChannelsListFormat;
	}
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
	}
	if (__pDownloadedData) {
		delete __pDownloadedData;
	}
	if (__pDownloadPopup) {
		delete __pDownloadPopup;
	}
	if (__pChannelsToAdd) {
		__pChannelsToAdd->RemoveAll(true);
		delete __pChannelsToAdd;
	}
	if (__pFoundChannels) {
		IEnumerator *pEnum = __pFoundChannels->GetEnumeratorN();
		while (!IsFailed(pEnum->MoveNext())) {
			Channel *chan = static_cast<Channel *>(pEnum->GetCurrent());

			FeedItemCollection *pCol = const_cast<FeedItemCollection *>(chan->GetItems());
			if (pCol) {
				pCol->RemoveAll(true);
				delete pCol;
			}
		}
		delete pEnum;

		__pFoundChannels->RemoveAll(true);
		delete __pFoundChannels;
	}
}

void AddChannelForm::OnSoftkeyAddChannelClicked(const Control &src) {
	if (!__checkMode) {
		__searchingChannels = false;
		FetchPage();
	} else {
		ArrayList *res = new ArrayList;
		res->Construct(__pFoundChannels->GetCount());

		int curCheckedIndex = __pFoundChannelsList->GetFirstCheckedItemIndex();
		if (curCheckedIndex >= 0) {
			do {
				int itemId = __pFoundChannelsList->GetItemIdAt(curCheckedIndex);

				Channel *chan = static_cast<Channel *>(__pFoundChannels->GetAt(itemId));
				res->Add(*(new Channel(*chan)));

				curCheckedIndex = __pFoundChannelsList->GetNextCheckedItemIndexAfter(curCheckedIndex);
			} while (curCheckedIndex >= 0);
		}

		GetFormManager()->SwitchBack(res, REASON_DIALOG_SUCCESS);

		IEnumerator *pEnum = res->GetEnumeratorN();
		while (!IsFailed(pEnum->MoveNext())) {
			Channel *chan = static_cast<Channel *>(pEnum->GetCurrent());
			FeedItemCollection *col = const_cast<FeedItemCollection *>(chan->GetItems());

			col->RemoveAll(true);
			delete col;
		}
		res->RemoveAll(true);
		delete res;
	}
}

void AddChannelForm::OnSoftkeyCancelClicked(const Control &src) {
	GetFormManager()->SwitchBack(null, REASON_DIALOG_CANCEL);
}

void AddChannelForm::OnFindChannelsClicked(const Control &src) {
	OnTextValueChanged(src);

	__searchingChannels = true;
	FetchPage();
}

result AddChannelForm::FetchPage(const String &url) {
	String fetchUrl = L"";
	if (url.IsEmpty()) {
		fetchUrl = __pURLField->GetText();
		if (!fetchUrl.StartsWith(L"http://", 0)) {
			fetchUrl.Insert(L"http://", 0);
		}
		if (fetchUrl.EndsWith(L"/")) {
			fetchUrl.Remove(fetchUrl.GetLength() - 1, 1);
		}
	} else {
		fetchUrl = url;
	}

	if (!__pChannelsToAdd) {
		__pChannelsToAdd = new Queue;
		__pChannelsToAdd->Construct(5);
	}
	__pChannelsToAdd->RemoveAll(true);

	if (!__pFoundChannels) {
		__pFoundChannels = new ArrayList;
		__pFoundChannels->Construct(5);
	}
	__pFoundChannels->RemoveAll(true);

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
			if (!__pDownloadPopup) {
				__pDownloadPopup = new Popup();
				__pDownloadPopup->Construct(false, Dimension(388,130));

				Label *pLabel = new Label;
				pLabel->Construct(Rectangle(54,15,280,70), Utilities::GetString(L"IDS_ADDCHANNELFORM_MBOX_DOWNLOAD"));
				pLabel->SetTextConfig(pLabel->GetTextSize() - 3, LABEL_TEXT_STYLE_NORMAL);
				pLabel->SetTextHorizontalAlignment(ALIGNMENT_CENTER);
				pLabel->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
				__pDownloadPopup->AddControl(*pLabel);
			}

			__pDownloadPopup->SetShowState(true);
			__pDownloadPopup->Show();
		}
		return E_SUCCESS;
	}

	Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_ERROR_CONNECT_FAILED"), MSGBOX_STYLE_OK);
	return res;
}

result AddChannelForm::FetchFoundChannels(void) {
	const String *fetchUrl = static_cast<const String *>(__pChannelsToAdd->Peek());

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
		res = __pDownloadSession->Construct(NET_HTTP_SESSION_MODE_NORMAL, null, *fetchUrl, null);
		if (IsFailed(res)) {
			AppLogDebug("AddChannelForm::FetchFoundChannels: [%s]: failed to construct http session", GetErrorMessage(res));
			break;
		}

		HttpTransaction *pTransaction = __pDownloadSession->OpenTransactionN();
		if (!pTransaction) {
			res = GetLastResult();
			AppLogDebug("AddChannelForm::FetchFoundChannels: [%s]: failed to construct http session", GetErrorMessage(res));
			break;
		}
		pTransaction->AddHttpTransactionListener(*this);

		HttpRequest *pRequest = const_cast<HttpRequest*>(pTransaction->GetRequest());
		pRequest->SetUri(*fetchUrl);
		pRequest->GetHeader()->AddField("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)");
		pRequest->SetMethod(NET_HTTP_METHOD_GET);

		res = pTransaction->Submit();
		if (IsFailed(res)) {
			AppLogDebug("AddChannelForm::FetchFoundChannels: [%s]: failed to submit http transaction", GetErrorMessage(res));

			delete pTransaction;
			break;
		}
		return E_SUCCESS;
	}

	return res;
}

result AddChannelForm::Initialize(void) {
	result res = Construct(FORM_STYLE_INDICATOR | FORM_STYLE_TITLE | FORM_STYLE_SOFTKEY_0 | FORM_STYLE_SOFTKEY_1);
	if (IsFailed(res)) {
		AppLogDebug("AddChannelForm::Initialize: [%s]: failed to construct parent form", GetErrorMessage(res));
		return res;
	}

	SetTitleText(Utilities::GetString(L"IDS_MAINFORM_OPTIONMENU_ADD_CHANNEL"), ALIGNMENT_CENTER);
	SetBackgroundColor(Color(0x0e0e0e, false));

	SetSoftkeyText(SOFTKEY_0, Utilities::GetString(L"IDS_ADDCHANNELFORM_ADD_SOFTKEY"));
	SetSoftkeyActionId(SOFTKEY_0, HANDLER(AddChannelForm::OnSoftkeyAddChannelClicked));
	AddSoftkeyActionListener(SOFTKEY_0, *this);

	SetSoftkeyText(SOFTKEY_1, Utilities::GetString(L"IDS_SAVEFORM_CANCEL_BUTTON"));
	SetSoftkeyActionId(SOFTKEY_1, HANDLER(AddChannelForm::OnSoftkeyCancelClicked));
	AddSoftkeyActionListener(SOFTKEY_1, *this);

	Label *pHint = new Label;
	res = pHint->Construct(Rectangle(10,12,460,80), Utilities::GetString(L"IDS_ADDCHANNELFORM_HINT_LABEL"));
	if (IsFailed(res)) {
		AppLogDebug("AddChannelForm::Initialize: [%s]: failed to construct hint label", GetErrorMessage(res));
		return res;
	}
	pHint->SetTextColor(Color::COLOR_WHITE);
	pHint->SetTextConfig(25, LABEL_TEXT_STYLE_NORMAL);
	pHint->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	pHint->SetTextVerticalAlignment(ALIGNMENT_TOP);

	int fieldY = 102;
	String lang = Utilities::GetString(L"LANG_IDENT");
	if (lang.Equals(L"en", false)) {
		fieldY -= 25;
	}

	__pURLField = new EditField;
	res = __pURLField->Construct(Rectangle(10,fieldY,460,110), EDIT_FIELD_STYLE_URL, INPUT_STYLE_FULLSCREEN, true, 1000, GROUP_STYLE_SINGLE);
	if (IsFailed(res)) {
		AppLogDebug("AddChannelForm::Initialize: [%s]: failed to construct URL field", GetErrorMessage(res));
		return res;
	}
	__pURLField->SetTitleText(Utilities::GetString(L"IDS_ADDCHANNELFORM_URL_FIELD_TITLE"));
	__pURLField->AddTextEventListener(*this);

	__pFindChannelsButton = new Button;
	res = __pFindChannelsButton->Construct(Rectangle(10,fieldY + 125,460,80), Utilities::GetString(L"IDS_ADDCHANNELFORM_FIND_CHANNELS_BUTTON"));
	if (IsFailed(res)) {
		AppLogDebug("AddChannelForm::Initialize: [%s]: failed to construct button to find channels", GetErrorMessage(res));
		return res;
	}
	__pFindChannelsButton->SetTextColor(Color::COLOR_WHITE);
	__pFindChannelsButton->SetTextHorizontalAlignment(ALIGNMENT_CENTER);
	__pFindChannelsButton->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
	Bitmap *pBmp = Utilities::GetBitmapN(L"search_channels_button.png");
	if (pBmp) {
		__pFindChannelsButton->SetNormalBackgroundBitmap(*pBmp);
		delete pBmp;
	}
	pBmp = Utilities::GetBitmapN(L"search_channels_button_pressed.png");
	if (pBmp) {
		__pFindChannelsButton->SetPressedBackgroundBitmap(*pBmp);
		delete pBmp;
	}
	__pFindChannelsButton->SetActionId(HANDLER(AddChannelForm::OnFindChannelsClicked));
	__pFindChannelsButton->AddActionEventListener(*this);

	__pFoundChannelsList = new CustomList;
	res = __pFoundChannelsList->Construct(Rectangle(0,fieldY + 230,480,403 - fieldY), CUSTOM_LIST_STYLE_MARK, true);
	if (IsFailed(res)) {
		AppLogDebug("AddChannelForm::Initialize: [%s]: failed to construct found channels list", GetErrorMessage(res));
		return res;
	}
	__pFoundChannelsList->SetTextOfEmptyList(L" ");
	__pFoundChannelsList->AddCustomItemEventListener(*this);

	__pChannelsListFormat = new CustomListItemFormat;
	res = __pChannelsListFormat->Construct();
	if (IsFailed(res)) {
		AppLogDebug("AddChannelForm::Initialize: [%s]: failed to construct found channels list format", GetErrorMessage(res));
		return res;
	}
	__pChannelsListFormat->AddElement(LIST_ELEMENT_CHECKBOX, Rectangle(15,15,50,50));
	__pChannelsListFormat->AddElement(LIST_ELEMENT_TITLE, Rectangle(80, 15, 385, 25), 25, Color::COLOR_WHITE, Color::COLOR_WHITE);
	__pChannelsListFormat->AddElement(LIST_ELEMENT_URL, Rectangle(80, 45, 385, 20), 20, Color::COLOR_GREY, Color::COLOR_GREY);

	AddControl(*pHint);
	AddControl(*__pURLField);
	AddControl(*__pFindChannelsButton);
	AddControl(*__pFoundChannelsList);

	SetFormStyle(GetFormStyle() & ~FORM_STYLE_SOFTKEY_0);
	__pFindChannelsButton->SetShowState(false);

	return E_SUCCESS;
}

result AddChannelForm::Terminate(void) {
	return E_SUCCESS;
}

result AddChannelForm::OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID) {
	return E_SUCCESS;
}

void AddChannelForm::OnTextValueChanged(const Control &source) {
	if (__pURLField->GetTextLength() > 0) {
		String url = __pURLField->GetText();
		if (Utilities::IsFirmwareLaterThan10()) {
			url.ToLowerCase();
		} else {
			url.ToLower();
		}
		__pURLField->SetText(url);
		bool isDirectLink = false;

		int slashIndex = -1;
		url.LastIndexOf(L'/', url.GetLength() - 1, slashIndex);
		if (slashIndex > 0) {
			result res = E_SUCCESS;
			if (slashIndex >= url.GetLength() - 1) {
				res = url.LastIndexOf(L'/', slashIndex - 1, slashIndex);
			}
			if (!IsFailed(res) && slashIndex > 0) {
				String suffix;
				url.SubString(slashIndex + 1, suffix);

				if (!suffix.IsEmpty()) {
					if (Utilities::IsFirmwareLaterThan10()) {
						suffix.ToLowerCase();
					} else {
						suffix.ToLower();
					}
					if (suffix.EndsWith(L".xml") || suffix.EndsWith(L".rdf")) {
						isDirectLink = true;
					} else if (!IsFailed(suffix.IndexOf(L"rss", 0, slashIndex)) || !IsFailed(suffix.IndexOf(L"feed", 0, slashIndex))
					|| !IsFailed(suffix.IndexOf(L"atom", 0, slashIndex))) {
						isDirectLink = true;
					}
				}
			}
		}

		if (isDirectLink) {
			SetFormStyle(GetFormStyle() | FORM_STYLE_SOFTKEY_0);
			__pFindChannelsButton->SetShowState(false);
		} else {
			SetFormStyle(GetFormStyle() & ~FORM_STYLE_SOFTKEY_0);
			__pFindChannelsButton->SetShowState(true);
		}
	} else {
		SetFormStyle(GetFormStyle() & ~FORM_STYLE_SOFTKEY_0);
		__pFindChannelsButton->SetShowState(false);
	}

	__pFoundChannelsList->RemoveAllItems();
	__checkMode = false;

	SetFocus();
	RequestRedraw(true);
}

void AddChannelForm::OnTextValueChangeCanceled(const Control &source) {
	SetFocus();
}

void AddChannelForm::OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status) {
	if (__checkMode) {
		int checkedCount = 0;
		for (int i = 0; i < __pFoundChannelsList->GetItemCount(); i++) {
			if (__pFoundChannelsList->IsItemChecked(i)) {
				checkedCount++;
			}
		}
		if (checkedCount > 0) {
			SetFormStyle(GetFormStyle() | FORM_STYLE_SOFTKEY_0);
		} else {
			SetFormStyle(GetFormStyle() & ~FORM_STYLE_SOFTKEY_0);
		}
		RequestRedraw(true);
	}
}

void AddChannelForm::OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen) {
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
			AppLogDebug("AddChannelForm::OnTransactionReadyToRead: [%s]: failed to get HTTP body", GetErrorMessage(res));
			return;
		}
	} else {
		AppLogDebug("AddChannelForm::OnTransactionReadyToRead: failed to get proper HTTP response, status code: [%d], text: [%S]", (int)pHttpResponse->GetStatusCode(), pHttpResponse->GetStatusText().GetPointer());
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

void AddChannelForm::OnTransactionHeaderCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction, int headerLen, bool rs) {
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
				if (__pChannelsToAdd->GetCount() > 0) {
					__pChannelsToAdd->Dequeue();
					while (__pChannelsToAdd->GetCount() > 0) {
						if (!IsFailed(FetchFoundChannels())) {
							return;
						} else {
							__pChannelsToAdd->Dequeue();
						}
					}
				}

				__pDownloadPopup->SetShowState(false);
				if (__pFoundChannels->GetCount() == 0) {
					Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_FAILED_TO_FIND_RSS_MBOX"), MSGBOX_STYLE_OK);

					__pChannelsToAdd->RemoveAll(true);
					__pFoundChannels->RemoveAll(true);
					__pFoundChannelsList->RemoveAllItems();
				} else {
					__checkMode = true;
				}
				RequestRedraw(true);

				redirect_count = 0;
				return;
			}
			redirect_count++;
			if (__pChannelsToAdd->GetCount() > 0) {
				Queue *pTmp = new Queue;
				pTmp->Construct(__pChannelsToAdd->GetCount());
				pTmp->Enqueue(*(new String(redirect_url)));

				delete __pChannelsToAdd->Dequeue();
				if (__pChannelsToAdd->GetCount() > 0) {
					IEnumerator *pEnum = __pChannelsToAdd->GetEnumeratorN();
					while (!IsFailed(pEnum->MoveNext())) {
						pTmp->Enqueue(*(pEnum->GetCurrent()));
					}
					delete pEnum;
				}

				__pChannelsToAdd->RemoveAll(false);
				delete __pChannelsToAdd;

				__pChannelsToAdd = pTmp;

				while (__pChannelsToAdd->GetCount() > 0) {
					if (!IsFailed(FetchFoundChannels())) {
						return;
					} else {
						__pChannelsToAdd->Dequeue();
					}
				}

				__pDownloadPopup->SetShowState(false);
				if (__pFoundChannels->GetCount() == 0) {
					Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_FAILED_TO_FIND_RSS_MBOX"), MSGBOX_STYLE_OK);

					__pChannelsToAdd->RemoveAll(true);
					__pFoundChannels->RemoveAll(true);
					__pFoundChannelsList->RemoveAllItems();
				} else {
					__checkMode = true;
				}
				RequestRedraw(true);

				redirect_count = 0;
			} else {
				FetchPage(redirect_url);
			}
		} else {
			if (__pChannelsToAdd->GetCount() > 0) {
				__pChannelsToAdd->Dequeue();
				while (__pChannelsToAdd->GetCount() > 0) {
					if (!IsFailed(FetchFoundChannels())) {
						return;
					} else {
						__pChannelsToAdd->Dequeue();
					}
				}
			}

			__pDownloadPopup->SetShowState(false);
			if (__pFoundChannels->GetCount() == 0) {
				Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_FAILED_TO_FIND_RSS_MBOX"), MSGBOX_STYLE_OK);

				__pChannelsToAdd->RemoveAll(true);
				__pFoundChannels->RemoveAll(true);
				__pFoundChannelsList->RemoveAllItems();
			} else {
				__checkMode = true;
			}
			RequestRedraw(true);

			redirect_count = 0;
		}
		return;
	}
	redirect_count = 0;
}

void AddChannelForm::OnTransactionAborted(HttpSession &httpSession, HttpTransaction &httpTransaction, result r) {
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
		__pDownloadSession = null;
	}

	if (__pChannelsToAdd->GetCount() > 0) {
		__pChannelsToAdd->Dequeue();
		while (__pChannelsToAdd->GetCount() > 0) {
			if (!IsFailed(FetchFoundChannels())) {
				return;
			} else {
				__pChannelsToAdd->Dequeue();
			}
		}

		if (__pFoundChannels->GetCount() > 0) {
			__pDownloadPopup->SetShowState(false);
			RequestRedraw(true);

			__checkMode = true;
			return;
		}
	}
	__pDownloadPopup->SetShowState(false);

	if (r == E_TIMEOUT || r == E_NOT_RESPONDING) {
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_ERROR_TIMEOUT"), MSGBOX_STYLE_OK);
	} else if (r == E_NETWORK_UNAVAILABLE) {
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_ERROR_CONNECT_FAILED"), MSGBOX_STYLE_OK);
	} else if (r == E_HOST_UNREACHABLE || r == E_DNS) {
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_ERROR_DNS"), MSGBOX_STYLE_OK);
	} else {
		String msg = Utilities::GetString(L"IDS_ADDCHANNELFORM_ERROR_UNKNOWN");
		msg.Replace(L"%s", String(GetErrorMessage(r)));
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), msg, MSGBOX_STYLE_OK);
	}

	__pChannelsToAdd->RemoveAll(true);
	__pFoundChannels->RemoveAll(true);
	__pFoundChannelsList->RemoveAllItems();

	RequestRedraw(true);
}

void AddChannelForm::OnTransactionCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction) {
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
		__pDownloadSession = null;
	}

	if (__pDownloadedData && __pDownloadedData->GetLimit() > 0) {
		__pDownloadedData->SetPosition(0);

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
				if (__pChannelsToAdd->GetCount() > 0) {
					__pChannelsToAdd->Dequeue();
					while (__pChannelsToAdd->GetCount() > 0) {
						if (!IsFailed(FetchFoundChannels())) {
							xmlFreeDoc(doc);
							return;
						} else {
							__pChannelsToAdd->Dequeue();
						}
					}
				}
				__pDownloadPopup->SetShowState(false);
				if (__pFoundChannels->GetCount() == 0) {
					Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_FAILED_TO_FIND_RSS_MBOX"), MSGBOX_STYLE_OK);

					__pChannelsToAdd->RemoveAll(true);
					__pFoundChannels->RemoveAll(true);
					__pFoundChannelsList->RemoveAllItems();
				} else {
					__checkMode = true;
				}
				RequestRedraw(true);
				return;
			}
			xmlFreeDoc(doc);
		}
		if (isXML && !isRSS) {
			if (__pChannelsToAdd->GetCount() > 0) {
				__pChannelsToAdd->Dequeue();
				while (__pChannelsToAdd->GetCount() > 0) {
					if (!IsFailed(FetchFoundChannels())) {
						return;
					} else {
						__pChannelsToAdd->Dequeue();
					}
				}
			}
			__pDownloadPopup->SetShowState(false);
			if (__pFoundChannels->GetCount() == 0) {
				Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_FAILED_TO_FIND_RSS_MBOX"), MSGBOX_STYLE_OK);

				__pChannelsToAdd->RemoveAll(true);
				__pFoundChannels->RemoveAll(true);
				__pFoundChannelsList->RemoveAllItems();
			} else {
				__checkMode = true;
			}
			RequestRedraw(true);
			return;
		}

		if (isRSS) {
			Channel *pChannel = new Channel;
			pChannel->SetDataFromXML((const char*)__pDownloadedData->GetPointer(), __pDownloadedData->GetLimit());

			String fetchUrl = L"";
			if (__pChannelsToAdd->GetCount() > 0) {
				String *chan = static_cast<String *>(__pChannelsToAdd->Dequeue());
				fetchUrl = *chan;
				delete chan;
			} else {
				fetchUrl = __pURLField->GetText();
			}
			if (!fetchUrl.StartsWith(L"http://", 0)) {
				fetchUrl.Insert(L"http://", 0);
			}
			if (fetchUrl.EndsWith(L"/")) {
				fetchUrl.Remove(fetchUrl.GetLength() - 1, 1);
			}
			pChannel->SetURL(fetchUrl);

			if (__searchingChannels) {
				__pFoundChannels->Add(*pChannel);

				CustomListItem *pItem = new CustomListItem;
				pItem->Construct(80);
				pItem->SetItemFormat(*__pChannelsListFormat);
				pItem->SetCheckBox(LIST_ELEMENT_CHECKBOX);
				pItem->SetElement(LIST_ELEMENT_TITLE, pChannel->GetTitle());
				pItem->SetElement(LIST_ELEMENT_URL, pChannel->GetURL());
				__pFoundChannelsList->AddItem(*pItem, __pFoundChannels->GetCount() - 1);

				while (__pChannelsToAdd->GetCount() > 0) {
					if (!IsFailed(FetchFoundChannels())) {
						return;
					} else {
						__pChannelsToAdd->Dequeue();
					}
				}

				__pDownloadPopup->SetShowState(false);
				if (__pFoundChannels->GetCount() == 0) {
					Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_FAILED_TO_FIND_RSS_MBOX"), MSGBOX_STYLE_OK);

					__pChannelsToAdd->RemoveAll(true);
					__pFoundChannels->RemoveAll(true);
					__pFoundChannelsList->RemoveAllItems();
				} else {
					__checkMode = true;
				}
				RequestRedraw(true);
			} else {
				ArrayList *chans = new ArrayList;
				chans->Construct(1);
				chans->Add(*pChannel);

				GetFormManager()->SwitchBack(chans, REASON_DIALOG_SUCCESS);

				FeedItemCollection *pCol = const_cast<FeedItemCollection *>(pChannel->GetItems());
				if (pCol) {
					pCol->RemoveAll(true);
					delete pCol;
				}
				chans->RemoveAll(true);
				delete chans;
			}
		} else {
			const wchar_t *strData = Utilities::ConvertCP1251ToUCS2(__pDownloadedData->GetPointer(), __pDownloadedData->GetLimit());
			String data = String(strData);
			delete []strData;

			int htmlInd = -1;
			if (!IsFailed(data.IndexOf(L"<html", 0, htmlInd)) && htmlInd >= 0) {
				//assume it's html
				String filters[] = {
					String(L"type=\"application/rss+xml\""),
					String(L"type=\"application/atom+xml\"")
				};

				for(int i = 0; i < 2; i++) {
					String pattern = filters[i];

					int startIndex = 0;
					while (true) {
						int linkIndex = -1;
						if (IsFailed(data.IndexOf(L"<link", startIndex, linkIndex))) {
							break;
						}

						int firstCloseBracket = -1;
						if (IsFailed(data.IndexOf(L">", linkIndex + 5, firstCloseBracket))) {
							break;
						}

						int typeIndex = -1;
						if (IsFailed(data.IndexOf(pattern, linkIndex + 5, typeIndex))) {
							break;
						}

						int hrefIndex = -1;
						if (IsFailed(data.IndexOf(L"href", linkIndex + 5, hrefIndex))) {
							break;
						}

						startIndex = firstCloseBracket + 1;
						if (typeIndex >= firstCloseBracket || hrefIndex >= firstCloseBracket) {
							continue;
						}

						int firstQuoteIndex = -1;
	 					if (IsFailed(data.IndexOf(L"\"", hrefIndex + 5, firstQuoteIndex))) {
							break;
						}

						int secondQuoteIndex = -1;
						if (IsFailed(data.IndexOf(L"\"", firstQuoteIndex + 1, secondQuoteIndex))) {
							break;
						}

						if (firstQuoteIndex >= firstCloseBracket || secondQuoteIndex >= firstCloseBracket) {
							continue;
						}

						String *link = new String;
						data.SubString(firstQuoteIndex + 1, secondQuoteIndex - firstQuoteIndex - 1, *link);

						__pChannelsToAdd->Enqueue(*link);
					}
				}

				if (__pChannelsToAdd->GetCount() > 0) {
					if (!__searchingChannels) {
						SetFormStyle(GetFormStyle() & ~FORM_STYLE_SOFTKEY_0);
						__pFindChannelsButton->SetShowState(true);
						RequestRedraw(true);

						__searchingChannels = true;
					}
					while (__pChannelsToAdd->GetCount() > 0) {
						if (!IsFailed(FetchFoundChannels())) {
							return;
						} else {
							__pChannelsToAdd->Dequeue();
						}
					}
					__pDownloadPopup->SetShowState(false);

					Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_ERROR_CONNECT_FAILED"), MSGBOX_STYLE_OK);

					__pChannelsToAdd->RemoveAll(true);
					__pFoundChannels->RemoveAll(true);
					__pFoundChannelsList->RemoveAllItems();

					RequestRedraw(true);
				} else {
					__pDownloadPopup->SetShowState(false);

					Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_FAILED_TO_FIND_RSS_MBOX"), MSGBOX_STYLE_OK);

					__pChannelsToAdd->RemoveAll(true);
					__pFoundChannels->RemoveAll(true);
					__pFoundChannelsList->RemoveAllItems();

					RequestRedraw(true);
				}
			} else {
				if (__pChannelsToAdd->GetCount() > 0) {
					__pChannelsToAdd->Dequeue();
					while (__pChannelsToAdd->GetCount() > 0) {
						if (!IsFailed(FetchFoundChannels())) {
							return;
						} else {
							__pChannelsToAdd->Dequeue();
						}
					}
				}

				__pDownloadPopup->SetShowState(false);
				if (__pFoundChannels->GetCount() == 0) {
					Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_FAILED_TO_FIND_RSS_MBOX"), MSGBOX_STYLE_OK);

					__pChannelsToAdd->RemoveAll(true);
					__pFoundChannels->RemoveAll(true);
					__pFoundChannelsList->RemoveAllItems();
				} else {
					__checkMode = true;
				}
				RequestRedraw(true);
			}
		}
	} else {
		if (__pChannelsToAdd->GetCount() > 0) {
			__pChannelsToAdd->Dequeue();
			while (__pChannelsToAdd->GetCount() > 0) {
				if (!IsFailed(FetchFoundChannels())) {
					return;
				} else {
					__pChannelsToAdd->Dequeue();
				}
			}
		}
		__pDownloadPopup->SetShowState(false);
		if (__pFoundChannels->GetCount() == 0) {
			Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_ADDCHANNELFORM_FAILED_TO_FIND_RSS_MBOX"), MSGBOX_STYLE_OK);

			__pChannelsToAdd->RemoveAll(true);
			__pFoundChannels->RemoveAll(true);
			__pFoundChannelsList->RemoveAllItems();
		} else {
			__checkMode = true;
		}
		RequestRedraw(true);
	}
}
