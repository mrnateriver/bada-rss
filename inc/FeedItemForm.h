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
#ifndef FEEDITEMFORM_H_
#define FEEDITEMFORM_H_

#include "BasicForm.h"
#include "Channel.h"
#include <FWeb.h>
#include <FNet.h>
#include <FIo.h>

using namespace Osp::Io;
using namespace Osp::Base::Runtime;
using namespace Osp::Web::Controls;
using namespace Osp::Net::Http;

enum ControlsState {
	STATE_DOWNLOAD_BUTTON,
	STATE_DOWNLOAD_ANIMATION,
	STATE_DOWNLOADING,
	STATE_DOWNLOADED,
	STATE_ERROR,
	STATE_NOTHING
};

class FeedItemForm: public BasicForm, public ILoadingListener, public ITouchEventListener, public ITimerEventListener, public IHttpTransactionEventListener {
public:
	FeedItemForm();
	virtual ~FeedItemForm();

	result RefreshData(void);

	bool IsBusy(void) const {
		return __pDownloadFile != null;
	}

private:
	void OnPrevButtonClicked(const Control &src);
	void OnBackButtonClicked(const Control &src);
	void OnNextButtonClicked(const Control &src);
	void OnShowImagesButtonClicked(const Control &src);
	void OnGuidButtonClicked(const Control &src);
	void OnEmailButtonClicked(const Control &src);
	void OnDownloadEnclosureButtonClicked(const Control &src);
	void OnOpenEnclosureClicked(const Control &src);

	void SetDownloadError(bool storage = false);
	void FetchEnclosure(const String &url = L"");
	String FormatPlaybackTime(long val) const;
	void SetControlsToState(ControlsState state, bool storage_error = false);

	virtual result Initialize(void);
	virtual result Terminate(void);

	virtual result OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID);

	virtual bool OnHttpAuthenticationRequestedN(const String &host, const String &realm, const AuthenticationChallenge &authentication) { return false; }
	virtual void OnHttpAuthenticationCanceled(void) { }
	virtual void OnLoadingStarted(void) { }
	virtual void OnLoadingCanceled(void) { }
	virtual void OnLoadingErrorOccurred(LoadingErrorType error, const Osp::Base::String &reason) { }
	virtual void OnLoadingCompleted(void) { }
	virtual void OnEstimatedProgress(int progress) { }
	virtual void OnPageTitleReceived(const String &title) { }
	virtual bool OnLoadingRequested(const String &url, WebNavigationType type);
	virtual DecisionPolicy OnWebDataReceived(const String &mime, const HttpHeader &httpHeader) { return WEB_DECISION_IGNORE; }

	virtual void OnTouchPressed(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo);
	virtual void OnTouchLongPressed(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) { }
	virtual void OnTouchReleased(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo);
	virtual void OnTouchMoved(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) { }
	virtual void OnTouchDoublePressed(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) { }
	virtual void OnTouchFocusIn(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo);
	virtual void OnTouchFocusOut(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo);

	virtual void OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen);
	virtual void OnTransactionAborted(HttpSession &httpSession, HttpTransaction &httpTransaction, result r);
	virtual void OnTransactionReadyToWrite(HttpSession &httpSession, HttpTransaction &httpTransaction, int recommendedChunkSize) { }
	virtual void OnTransactionHeaderCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction, int headerLen, bool rs);
	virtual void OnTransactionCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction);
	virtual void OnTransactionCertVerificationRequiredN(HttpSession &httpSession, HttpTransaction &httpTransaction, String *pCert) { }

	virtual void OnTimerExpired(Timer &timer);

	static const int ANIM_INTERVAL = 500/8;

	Label *__pTitleLabel;
	Label *__pPubLabel;
	Label *__pSepLabel;
	Label *__pFeedMenuIconLabel;
	Label *__pHeaderLabel;
	Button *__pPrevButton;
	Button *__pNextButton;
	Button *__pShowImagesButton;
	Web *__pContentView;
	ContextMenu *__pConMenu;

	Label *__pEnclosureBg;
	Label *__pEnclosureLoadingText;
	Label *__pEnclosureMIMEIcon;
	Progress *__pDownloadProgressBar;
	Button *__pDownloadEnclosureButton;
	Label *__pEnclosureMIMEText;
	Label *__pEnclosureDuration;
	Label *__pEnclosureBitrate;
	Button *__pOpenEnclosureButton;

	Bitmap *__pHeaderBitmapNormal;
	Bitmap *__pHeaderBitmapPressed;
	bool __canTriggerPress;
	bool __isGuidPresent;

	HttpSession *__pDownloadSession;
	String __pDownloadFilePath;
	File *__pDownloadFile;
	int __downloadLength;
	int __downloadedAlready;

	ArrayListT<int> *__pItemIDs;
	int __curIndex;
	Channel *__pChannel;
	String __curGuidLink;

	String __appControlEnclosurePreviousPath;
	String __appControlEnclosurePath;

	ArrayListT<int> *__pReadItemIDs;

	int __bgRiseCurrentY;
	Timer *__pRiseTimer;
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
