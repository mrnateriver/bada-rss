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
#ifndef _MAINFORM_H_
#define _MAINFORM_H_

#include "ChannelForm.h"
#include <FGraphics.h>

using namespace Osp::Graphics;

enum EditMode {
	MODE_NONE,
	MODE_DELETING,
	MODE_CHANGING_ORDER
};

class MainForm: public BasicForm, public ICustomItemEventListener, public IScrollPanelEventListener, public ITimerEventListener, public IHttpTransactionEventListener {
public:
	MainForm(void);
	virtual ~MainForm(void);

private:
	void OnCancelUpdateClicked(const Control &src);
	void OnOptionMenuClicked(const Control &src);
	void OnOptionAddChannelClicked(const Control &src);
	void OnOptionDeleteFeedsClicked(const Control &src);
	void OnOptionChangeOrderClicked(const Control &src);
	void OnButtonLowerClicked(const Control &src);
	void OnButtonHigherClicked(const Control &src);
	void OnSoftkeyDoneClicked(const Control &src);
	void OnSoftkeyCancelClicked(const Control &src);
	void OnOptionUpdateChannelsClicked(const Control &src);
	void OnOptionExportClicked(const Control &src);
	void OnOptionImportClicked(const Control &src);
	void OnOptionSettingsClicked(const Control &src);
	void OnKeypadSearchClicked(const Control &src);
	void OnKeypadClearClicked(const Control &src);

	result FillChannelList(const ArrayList &pChannels, EditMode editMode = MODE_NONE, bool clearSearch = true);
	result FetchPage(const String &url, bool resetIcon = true);
	result FetchNextChannel(void);
	void SetChannelUpdateError(void);
	void SpinNextChannel(bool error = false);

	virtual result Initialize(void);
	virtual result Terminate(void);

	virtual result OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID);

	virtual void OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status);
	virtual void OnItemStateChanged(const Control &source, int index, int itemId, int elementId, ItemStatus status) { }

	virtual void OnOverlayControlCreated(const Control &source) { }
	virtual void OnOverlayControlOpened(const Control &source) { }
	virtual void OnOverlayControlClosed(const Control &source) { __pChannelList->SetFocus(); }
	virtual void OnOtherControlSelected(const Control &source) { }

	virtual void OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen);
	virtual void OnTransactionAborted(HttpSession &httpSession, HttpTransaction &httpTransaction, result r);
	virtual void OnTransactionReadyToWrite(HttpSession &httpSession, HttpTransaction &httpTransaction, int recommendedChunkSize) { }
	virtual void OnTransactionHeaderCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction, int headerLen, bool rs);
	virtual void OnTransactionCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction);
	virtual void OnTransactionCertVerificationRequiredN(HttpSession &httpSession, HttpTransaction &httpTransaction, String *pCert) { }

	virtual void OnTimerExpired(Timer &timer);

	static const int DIALOG_ID_EXPORT = 401;
	static const int DIALOG_ID_IMPORT = 402;
	static const int DIALOG_ID_ADD_CHANNEL = 403;
	static const int DIALOG_ID_VIEW_CHANNEL = 404;

	static const int LIST_ELEMENT_ICON = 501;
	static const int LIST_ELEMENT_TITLE = 502;
	static const int LIST_ELEMENT_DESCRIPTION = 503;
	static const int LIST_ELEMENT_UNREAD_BADGE = 504;
	static const int LIST_ELEMENT_UNREAD_COUNT = 505;
	static const int LIST_ELEMENT_DELETE_CHECKBOX = 506;

	static const int ANIM_INTERVAL = 500/8;

	ChannelForm *__pChannelForm;

	Label *__pEmptyListIcon;
	Label *__pEmptyListLabel;
	OptionMenu *__pOptionMenu;
	ScrollPanel *__pMainPanel;
	EditField *__pSearchField;
	Label *__pSearchLabel;
	CustomList *__pChannelList;
	CustomListItemFormat *__pChannelListFormat;
	CustomListItemFormat *__pChannelListEditFormat;

	int __softkeyEditDoneActionId;
	int __softkeyEditCancelActionId;
	Bitmap *__pOrderSelectedItemNormal;
	Bitmap *__pOrderSelectedItemPressed;
	int __orderIndex;
	Button *__pLowerButton;
	Button *__pHigherButton;
	EditMode __editMode;

	Label *__pSpinnerLabel;
	Label *__pBackgroundLabel;
	Label *__pTitleLabel;
	Button *__pCancelButton;

	int __cancelUpdateActionId;
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

	HttpSession *__pDownloadSession;
	ByteBuffer *__pDownloadedData;

	QueueT<Channel *> *__pUpdateQueue;
	Channel *__pIconUpdateChannel;
	bool __updating;
	int __newItemsCount;

	int __animItemIndex;
	struct {
		Bitmap *__pProgressBitmap1;
		Bitmap *__pProgressBitmap2;
		Bitmap *__pProgressBitmap3;
		Bitmap *__pProgressBitmap4;
		Bitmap *__pProgressBitmap5;
		Bitmap *__pProgressBitmap6;
		Bitmap *__pProgressBitmap7;
		Bitmap *__pProgressBitmap8;
	} __smallLoadingAnimData;

	friend class Aggregator;
};

#endif
