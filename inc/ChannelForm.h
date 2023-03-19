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
#ifndef CHANNELFORM_H_
#define CHANNELFORM_H_

#include "BasicForm.h"
#include "HttpDownloader.h"
#include "Channel.h"

class ChannelForm: public BasicForm, public ICustomItemEventListener, public IGroupedItemEventListener, public IHttpTransactionEventListener, public ITimerEventListener, public IRunnable {
public:
	ChannelForm();
	virtual ~ChannelForm();

	result RefreshForm(void);
	void CancelDownload(bool deleteThread = true);

private:
	void OnOptionMenuClicked(const Control &src);
	void OnOptionMarkAllClicked(const Control &src);
	void OnSoftkeyBackClicked(const Control &src);

	result FetchPage(const String &url = String(L""));

	virtual result Initialize(void);
	virtual result Terminate(void);

	virtual result OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID);

	virtual void OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status);
	virtual void OnItemStateChanged(const Control &source, int item_group_index, int item_id_index, int item_elem_Id, ItemStatus status);
	virtual void OnItemStateChanged(const Control &source, int groupIndex, int itemIndex, int itemId, int elementId, ItemStatus status) { }

	virtual void OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen);
	virtual void OnTransactionAborted(HttpSession &httpSession, HttpTransaction &httpTransaction, result r);
	virtual void OnTransactionReadyToWrite(HttpSession &httpSession, HttpTransaction &httpTransaction, int recommendedChunkSize) { }
	virtual void OnTransactionHeaderCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction, int headerLen, bool rs);
	virtual void OnTransactionCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction);
	virtual void OnTransactionCertVerificationRequiredN(HttpSession &httpSession, HttpTransaction &httpTransaction, String *pCert) { }

	virtual void OnTimerExpired(Timer &timer);

	virtual Object *Run(void);

	static const int DIALOG_ID_VIEW_CHANNEL_INFO = 401;
	static const int DIALOG_ID_VIEW_FEED_ITEM = 402;

	static const int LIST_HEADER_ELEMENT_INFO_ICON = 501;
	static const int LIST_HEADER_ELEMENT_TITLE = 502;
	static const int LIST_HEADER_ELEMENT_PUBDATE = 503;
	static const int LIST_HEADER_ELEMENT_BUTTON = 504;

	static const int LIST_ELEMENT_TITLE = 505;
	static const int LIST_ELEMENT_DESCRIPTION = 506;
	static const int LIST_ELEMENT_UNREAD_BADGE = 507;
	static const int LIST_LOADING_ELEMENT_SPINNER = 508;
	static const int LIST_LOADING_ELEMENT_TITLE = 509;

	static const int ANIM_INTERVAL = 500/8;

	OptionMenu *__pOptionMenu;
	CustomList *__pHeaderList;
	GroupedList *__pItemsList;
	CustomListItemFormat *__pHeaderFormat;
	CustomListItemFormat *__pItemFormat;
	CustomListItemFormat *__pLoadingHeaderFormat;

	const Channel *__pChannel;

	HttpSession *__pDownloadSession;
	ByteBuffer *__pDownloadedData;

	Thread *__pParsingThread;
	bool __fetchingPage;
	bool __fetchingIcon;

	Timer *__pAnimTimer;
	struct {
		Bitmap *__pProgressBitmap1;
		Bitmap *__pProgressBitmap2;
		Bitmap *__pProgressBitmap3;
		Bitmap *__pProgressBitmap4;
		Bitmap *__pProgressBitmap5;
		Bitmap *__pProgressBitmap6;
		Bitmap *__pProgressBitmap7;
		Bitmap *__pProgressBitmap8;
	} __loadingAnimData;
};

#endif
