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
#ifndef ADDCHANNELFORM_H_
#define ADDCHANNELFORM_H_

#include "BasicForm.h"
#include "HttpDownloader.h"
#include "FeedCollection.h"

class AddChannelForm: public BasicForm, public ITextEventListener, public ICustomItemEventListener, public IHttpTransactionEventListener {
public:
	AddChannelForm(void);
	virtual ~AddChannelForm(void);

private:
	void OnSoftkeyAddChannelClicked(const Control &src);
	void OnSoftkeyCancelClicked(const Control &src);
	void OnFindChannelsClicked(const Control &src);

	result FetchPage(const String &url = String(L""));
	result FetchFoundChannels(void);

	virtual result Initialize(void);
	virtual result Terminate(void);

	virtual result OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID);

	virtual void OnTextValueChanged(const Control &source);
	virtual void OnTextValueChangeCanceled(const Control &source);

	virtual void OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status);
	virtual void OnItemStateChanged(const Control &source, int index, int itemId, int elementId, ItemStatus status) { }

	virtual void OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen);
	virtual void OnTransactionAborted(HttpSession &httpSession, HttpTransaction &httpTransaction, result r);
	virtual void OnTransactionReadyToWrite(HttpSession &httpSession, HttpTransaction &httpTransaction, int recommendedChunkSize) { }
	virtual void OnTransactionHeaderCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction, int headerLen, bool rs);
	virtual void OnTransactionCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction);
	virtual void OnTransactionCertVerificationRequiredN(HttpSession &httpSession, HttpTransaction &httpTransaction, String *pCert) { }

	static const int LIST_ELEMENT_CHECKBOX = 500;
	static const int LIST_ELEMENT_TITLE = 501;
	static const int LIST_ELEMENT_URL = 502;

	EditField *__pURLField;
	Button *__pFindChannelsButton;
	CustomList *__pFoundChannelsList;
	CustomListItemFormat *__pChannelsListFormat;
	bool __checkMode;

	HttpSession *__pDownloadSession;
	ByteBuffer *__pDownloadedData;

	Popup *__pDownloadPopup;
	bool __searchingChannels;
	Queue *__pChannelsToAdd;
	ArrayList *__pFoundChannels;
};

#endif
