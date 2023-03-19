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
#include "FeedItemForm.h"
#include "FeedManager.h"
#include "SettingsManager.h"
#include <FContent.h>
#include <typeinfo>

using namespace Osp::Base::Utility;
using namespace Osp::Content;

FeedItemForm::FeedItemForm() {
	__pTitleLabel = null;
	__pPubLabel = null;
	__pFeedMenuIconLabel = null;
	__pSepLabel = null;
	__pHeaderLabel = null;
	__pPrevButton = null;
	__pNextButton = null;
	__pShowImagesButton = null;
	__pContentView = null;
	__pConMenu = null;

	__pEnclosureBg = null;
	__pEnclosureLoadingText = null;
	__pEnclosureMIMEIcon = null;
	__pDownloadProgressBar = null;
	__pDownloadEnclosureButton = null;
	__pEnclosureDuration = null;
	__pEnclosureBitrate = null;
	__pEnclosureMIMEText = null;
	__pOpenEnclosureButton = null;

	__pHeaderBitmapNormal = null;
	__pHeaderBitmapPressed = null;
	__canTriggerPress = false;
	__isGuidPresent = true;

	__pDownloadSession = null;
	__pDownloadFilePath = L"";
	__pDownloadFile = null;
	__downloadLength = 0;
	__downloadedAlready = 0;

	__pItemIDs = null;
	__curIndex = -1;
	__pChannel = null;
	__curGuidLink = L"";

	__appControlEnclosurePreviousPath = L"";
	__appControlEnclosurePath = L"";

	__pReadItemIDs = null;

	__bgRiseCurrentY = 712;
	__pRiseTimer = null;
	__pAnimTimer = null;
	unsigned char *ptr = reinterpret_cast<unsigned char *>(&__loadingAnimData);
	for (unsigned int i = 0; i < sizeof(__loadingAnimData); i++) {
		*(ptr + i) = 0;
	}
}

FeedItemForm::~FeedItemForm() {
	if (__pItemIDs) {
		__pItemIDs->RemoveAll();
		delete __pItemIDs;
	}
	if (__pReadItemIDs) {
		__pReadItemIDs->RemoveAll();
		delete __pReadItemIDs;
	}
	if (__pAnimTimer) {
		__pAnimTimer->Cancel();
		delete __pAnimTimer;
	}
	if (__pRiseTimer) {
		__pRiseTimer->Cancel();
		delete __pRiseTimer;
	}
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
	}
	if (__pDownloadFile) {
		delete __pDownloadFile;
		__pDownloadFile = null;
	}
	if (!__pDownloadFilePath.IsEmpty()) {
		File::Remove(__pDownloadFilePath);
	}
	if (__pHeaderBitmapNormal) {
		delete __pHeaderBitmapNormal;
	}
	if (__pHeaderBitmapPressed) {
		delete __pHeaderBitmapPressed;
	}
	if (__pConMenu) {
		delete __pConMenu;
	}
}

void FeedItemForm::OnGuidButtonClicked(const Control &src) {
	if (!__curGuidLink.IsEmpty()) {
		ArrayList* pDataList = new ArrayList();
		pDataList->Construct();

		String* pData = new String(L"url:" + __curGuidLink);
		pDataList->Add(*pData);

		AppControl* pAc = AppManager::FindAppControlN(APPCONTROL_BROWSER, "");
		if(pAc)	{
		  pAc->Start(pDataList, null);
		  delete pAc;
		}

		pDataList->RemoveAll(true);
		delete pDataList;
	}
}

void FeedItemForm::OnEmailButtonClicked(const Control &src) {
	int itemID = -1;
	__pItemIDs->GetAt(__curIndex, itemID);
	if (itemID >= 0) {
		FeedItem *item = __pChannel->GetItems()->GetByID(itemID);
		if (item) {
			ArrayList* pDataList = new ArrayList();
			pDataList->Construct(2);

			String* pData = new String(L"subject:" + item->GetTitle());
			pDataList->Add(*pData);

			String content = item->GetContent();

			content.Replace(L"&laquo;", L"«");
			content.Replace(L"&raquo;", L"»");
			content.Replace(L"&lt;", L"<");
			content.Replace(L"&gt;", L">");
			content.Replace(L"<p>", L" ");
			content.Replace(L"<br>", L"\n");
			content.Replace(L"<br />", L"\n");
			content.Replace(L"<dd>", L"\n");
			content.Replace(L"<li>", L"\n");
			content.Replace(L"&quot;", L"\"");
			content.Replace(L"&amp;", L"&");

			int tagIndex = -1;
			while (!IsFailed(content.IndexOf(L"<", 0, tagIndex)) && tagIndex < content.GetLength() && tagIndex >= 0) {
				int closeIndex = -1;
				if (!IsFailed(content.IndexOf(L'>', tagIndex, closeIndex))) {
					content.Remove(tagIndex, closeIndex - tagIndex + 1);
				}
			}

			if (content.GetLength() > 500) {
				content.Remove(500, content.GetLength() - 500);

				int lastSpaceIndex = content.GetLength() - 1;
				if (!IsFailed(content.LastIndexOf(L" ", lastSpaceIndex, lastSpaceIndex))) {
					content.Remove(lastSpaceIndex, content.GetLength() - lastSpaceIndex);
					content.Append(L"...");
				}
			}
			String num;
			int ch;
			int repInd = 0;
			int stInd = 0;
			while (!IsFailed(content.IndexOf(L"&#", stInd, repInd))) {
				repInd += 2;
				content.IndexOf(L';', repInd, stInd);

				if (!IsFailed(content.SubString(repInd, stInd - repInd, num))) {
					if (!IsFailed(Integer::Parse(num, ch))) {
						content.Remove(repInd - 2, 3 + num.GetLength());
						content.Insert((mchar)ch, repInd - 2);
					}
				}
				stInd++;
			}

			String* pData2 = new String(L"text:" + content + L"\n\n\u00A9 " + __pChannel->GetTitle());
			pDataList->Add(*pData2);

			AppControl* pAc = AppManager::FindAppControlN(APPCONTROL_EMAIL, OPERATION_EDIT);
			if(pAc) {
				pAc->Start(pDataList, null);
				delete pAc;
			}
			pDataList->RemoveAll(true);
			delete pDataList;
		}
	}
}

void FeedItemForm::OnPrevButtonClicked(const Control &src) {
	__curIndex--;

	result res = RefreshData();
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::OnNextButtonClicked: [%s]: failed to switch to previous entry");
	}
}

void FeedItemForm::OnBackButtonClicked(const Control &src) {
	ArrayList* pArgs = new ArrayList();
	pArgs->Construct(2);
	pArgs->Add(*__pReadItemIDs);

	int itemID = -1;
	__pItemIDs->GetAt(__curIndex, itemID);
	pArgs->Add(*(new Integer(itemID)));

	GetFormManager()->SwitchBack(pArgs, REASON_DIALOG_SUCCESS, true);

	pArgs->RemoveAt(1, true);
	delete pArgs;
}

void FeedItemForm::OnNextButtonClicked(const Control &src) {
	__curIndex++;

	result res = RefreshData();
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::OnNextButtonClicked: [%s]: failed to switch to next entry");
	}
}

void FeedItemForm::OnShowImagesButtonClicked(const Control &src) {
	int itemID = -1;
	__pItemIDs->GetAt(__curIndex, itemID);
	if (itemID >= 0) {
		FeedItem *item = __pChannel->GetItems()->GetByID(itemID);
		int lines = Utilities::GetLinesForText(item->GetTitle(), 30, 395);

		if (item->GetEnclosureURL().IsEmpty() && !item->IsEnclosureDownloaded()) {
			__pContentView->SetSize(480, 624 - (lines * 31 + 45));
		} else {
			__pContentView->SetSize(480, 521 - (lines * 31 + 45));
		}
		__pContentView->SetPosition(0, lines * 31 + 45);

		String html = L"<html><head><meta name=\"viewport\" content=\"initial-scale=1.0, width=device-width, height=device-height\"/></head><body bgcolor=\"#2b2b2b\" text=\"#ffffff\">";
		html.Append(item->GetContent());
		html.Append(L"</body></html>");

		__pShowImagesButton->SetShowState(false);

		ByteBuffer *buf = StringUtil::StringToUtf8N(html);
		__pContentView->LoadData(L"file:///Home/none/" + Integer::ToString(__curIndex), *buf, L"text/html", L"UTF-8");
		delete buf;

		RequestRedraw(true);
	}
}

void FeedItemForm::OnDownloadEnclosureButtonClicked(const Control &src) {
	SetControlsToState(STATE_DOWNLOAD_ANIMATION);
}

void FeedItemForm::OnOpenEnclosureClicked(const Control &src) {
	int itemID = -1;
	__pItemIDs->GetAt(__curIndex, itemID);
	if (itemID >= 0) {
		FeedItem *item = __pChannel->GetItems()->GetByID(itemID);

		ArrayList* pDataList = new ArrayList();
		pDataList->Construct();

		String url = L"";
		String tmp = item->GetEnclosureURL();
		int sepIndex = -1;
		if (!IsFailed(tmp.IndexOf(L"~%~", 0, sepIndex)) && sepIndex >= 0 && (sepIndex + 3) < tmp.GetLength()) {
			tmp.SubString(0, sepIndex, url);
		}

		String* pData = new String(L"path:" + url);
		pDataList->Add(*pData);

		if (item->GetEnclosureMIME().StartsWith(L"audio/", 0)) {
			pData = new String(L"type:audio");
			pDataList->Add(*pData);

			AppControl* pAc = AppManager::FindAppControlN(APPCONTROL_AUDIO, OPERATION_PLAY);
			if(pAc) {
				pAc->Start(pDataList, null);
				delete pAc;
			}
		} else if (item->GetEnclosureMIME().StartsWith(L"video/", 0)) {
			pData = new String(L"type:video");
			pDataList->Add(*pData);

			AppControl* pAc = AppManager::FindAppControlN(APPCONTROL_VIDEO, OPERATION_PLAY);
			if(pAc) {
				pAc->Start(pDataList, null);
				delete pAc;
			}
		}

		pDataList->RemoveAll(true);
		delete pDataList;
	}
}

void FeedItemForm::SetDownloadError(bool storage) {
	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
		__pDownloadSession = null;
	}
	if (__pDownloadFile) {
		delete __pDownloadFile;
		__pDownloadFile = null;
	}
	if (!__pDownloadFilePath.IsEmpty()) {
		File::Remove(__pDownloadFilePath);
		__pDownloadFilePath = L"";
	}

	SetControlsToState(STATE_ERROR, storage);
}

void FeedItemForm::FetchEnclosure(const String &url) {
	String enc_url = L"";
	if (url.IsEmpty()) {
		int itemID = -1;
		__pItemIDs->GetAt(__curIndex, itemID);
		if (itemID >= 0) {
			FeedItem *item = __pChannel->GetItems()->GetByID(itemID);
			if (item && !item->IsEnclosureDownloaded()) {
				enc_url = item->GetEnclosureURL();

				String path = L"/Home/enclosures/";
				path.Append(__pChannel->GetUniqueString());
				path.Append(L"/");
				path.Append(item->GetUniqueString());

				String ext = L".dat";
				int lastDotIndex = -1;
				if (!IsFailed(enc_url.LastIndexOf(L'/', enc_url.GetLength() - 1, lastDotIndex)) && lastDotIndex >= 0) {
					enc_url.SubString(lastDotIndex + 1, ext);
				}
				if (!IsFailed(ext.IndexOf(L'.', 0, lastDotIndex)) && lastDotIndex >= 0) {
					ext.SubString(lastDotIndex, ext);
				}
				//check if URL has parameters so that we don't shove it into extension
				if (!IsFailed(ext.IndexOf(L'?', 0, lastDotIndex)) && lastDotIndex >= 0) {
					ext.SubString(0, lastDotIndex, ext);
				}

				if (Utilities::IsFirmwareLaterThan10()) {
					ext.ToLowerCase();
				} else {
					ext.ToLower();
				}
				//a slight workaround for m4v codec
				if (!IsFailed(ext.IndexOf(L"m4v", 1, lastDotIndex)) || !IsFailed(item->GetEnclosureMIME().IndexOf(L"m4v", 0, lastDotIndex))) {
					ext = L".mp4";
				}
				path.Append(ext);
				//WMV, ASF, MP4, 3GP, AVI
				if ((!IsFailed(ext.IndexOf(L"wmv", 1, lastDotIndex)) || !IsFailed(ext.IndexOf(L"asf", 1, lastDotIndex)) || !IsFailed(ext.IndexOf(L"mp4", 1, lastDotIndex))
					|| !IsFailed(ext.IndexOf(L"3gp", 1, lastDotIndex)) || !IsFailed(ext.IndexOf(L"avi", 1, lastDotIndex))) && IsFailed(item->GetEnclosureMIME().IndexOf(L"video", 0, lastDotIndex))) {
					//some morons have put a video with an audio or smt MIME type
					FeedManager::GetInstance()->SetEnclosureMIME(__pChannel, itemID, L"video/piece_of_shit");
				}

				if (__pDownloadFile) {
					delete __pDownloadFile;
				}
				__pDownloadFile = new File;
				result res = __pDownloadFile->Construct(path, L"w", true);
				if (IsFailed(res)) {
					AppLogDebug("FeedItemForm::FetchEnclosure: [%s]: failed to construct downloading file", GetErrorMessage(res));
					delete __pDownloadFile;
					__pDownloadFile = null;
					enc_url = L"";
				} else {
					if (!__pDownloadFilePath.IsEmpty()) {
						File::Remove(__pDownloadFilePath);
						__pDownloadFilePath = L"";
					}
					__pDownloadFilePath = path;

					__downloadLength = item->GetEnclosureLength();
					__downloadedAlready = 0;
				}
			}
		}
	} else {
		enc_url = url;
	}

	while (true) {
		if (!enc_url.IsEmpty()) {
			if (__pDownloadSession) {
				__pDownloadSession->CloseAllTransactions();
				delete __pDownloadSession;
				__pDownloadSession = null;
			}
			__pDownloadSession = new HttpSession();
			result res = __pDownloadSession->Construct(NET_HTTP_SESSION_MODE_NORMAL, null, enc_url, null);
			if (IsFailed(res)) {
				AppLogDebug("FeedItemForm::FetchEnclosure: [%s]: failed to construct http session", GetErrorMessage(res));

				delete __pDownloadFile;
				__pDownloadFile = null;
				__pDownloadFilePath = L"";

				break;
			}

			HttpTransaction *pTransaction = __pDownloadSession->OpenTransactionN();
			if (!pTransaction) {
				AppLogDebug("FeedItemForm::FetchEnclosure: [%s]: failed to construct http session", GetErrorMessage(GetLastResult()));

				delete __pDownloadFile;
				__pDownloadFile = null;
				__pDownloadFilePath = L"";

				break;
			}
			pTransaction->AddHttpTransactionListener(*this);

			HttpRequest *pRequest = const_cast<HttpRequest*>(pTransaction->GetRequest());
			pRequest->SetUri(enc_url);
			pRequest->GetHeader()->AddField("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)");
			pRequest->SetMethod(NET_HTTP_METHOD_GET);

			res = pTransaction->Submit();
			if (IsFailed(res)) {
				AppLogDebug("FeedItemForm::FetchEnclosure: [%s]: failed to submit http transaction", GetErrorMessage(res));

				delete pTransaction;

				delete __pDownloadFile;
				__pDownloadFile = null;
				__pDownloadFilePath = L"";

				break;
			}

			SetControlsToState(STATE_DOWNLOADING);
			return;
		} else {
			break;
		}
	}

	//report error
	SetControlsToState(STATE_ERROR);
}

String FeedItemForm::FormatPlaybackTime(long val) const {
	String time;

	int recordingDuration = val / 1000;

	int seconds = recordingDuration % 60;
	int minutes = (recordingDuration - seconds) / 60;
	int hours = (minutes - (minutes % 60)) / 60;
	minutes = minutes % 60;

	time.Append(hours);
	time.Append(':');
	if (minutes > 9) {
		time.Append(minutes);
	} else {
		time.Append('0');
		time.Append(minutes);
	}
	time.Append(':');
	if (seconds > 9) {
		time.Append(seconds);
	} else {
		time.Append('0');
		time.Append(seconds);
	}

	return time;
}

void FeedItemForm::SetControlsToState(ControlsState state, bool storage_error) {
	__pAnimTimer->Cancel();
	__pRiseTimer->Cancel();

	if (state == STATE_DOWNLOAD_BUTTON || state == STATE_DOWNLOAD_ANIMATION) {
		//positions and sizes
		__pEnclosureBg->SetPosition(0, 712);

		__pEnclosureLoadingText->SetPosition(108, 712 + 14);
		__pEnclosureLoadingText->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
		__pEnclosureLoadingText->SetTextConfig(37, LABEL_TEXT_STYLE_NORMAL);
		__pEnclosureLoadingText->SetText(Utilities::GetString(L"IDS_ADDCHANNELFORM_MBOX_DOWNLOAD"));

		__pEnclosureMIMEIcon->SetPosition(26, 712 + 26);
		__pEnclosureMIMEIcon->SetSize(56, 56);
		__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap1);

		//visibility
		__pDownloadProgressBar->SetShowState(false);

		__pEnclosureMIMEText->SetShowState(false);
		__pEnclosureDuration->SetShowState(false);
		__pOpenEnclosureButton->SetShowState(false);
		__pEnclosureBitrate->SetShowState(false);

		if (state == STATE_DOWNLOAD_BUTTON) {
			__pDownloadEnclosureButton->SetShowState(true);

			__pEnclosureBg->SetShowState(false);
			__pEnclosureLoadingText->SetShowState(false);
			__pEnclosureMIMEIcon->SetShowState(false);
		} else if (state == STATE_DOWNLOAD_ANIMATION) {
			__pDownloadEnclosureButton->SetShowState(false);

			__pEnclosureBg->SetShowState(true);
			__pEnclosureLoadingText->SetShowState(true);
			__pEnclosureMIMEIcon->SetShowState(true);

			__bgRiseCurrentY = 712;
			__pRiseTimer->Start(ANIM_INTERVAL / 3);
		}
	} else if (state == STATE_DOWNLOADING) {
		//positions and sizes
		__pEnclosureBg->SetPosition(0, 521);

		__pEnclosureLoadingText->SetPosition(108, 535);
		if (__downloadLength <= 0) {
			__pEnclosureLoadingText->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
			__pDownloadProgressBar->SetShowState(false);
		} else {
			__pEnclosureLoadingText->SetTextVerticalAlignment(ALIGNMENT_TOP);
			__pDownloadProgressBar->SetShowState(true);
		}
		__pEnclosureLoadingText->SetTextConfig(37, LABEL_TEXT_STYLE_NORMAL);
		__pEnclosureLoadingText->SetText(Utilities::GetString(L"IDS_ADDCHANNELFORM_MBOX_DOWNLOAD"));

		__pEnclosureMIMEIcon->SetPosition(26, 547);
		__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap1);

		__pDownloadProgressBar->SetValue(0);

		//visibility
		__pDownloadEnclosureButton->SetShowState(false);

		__pEnclosureBg->SetShowState(true);
		__pEnclosureLoadingText->SetShowState(true);
		__pEnclosureMIMEIcon->SetShowState(true);

		__pEnclosureMIMEText->SetShowState(false);
		__pEnclosureDuration->SetShowState(false);
		__pEnclosureBitrate->SetShowState(false);
		__pOpenEnclosureButton->SetShowState(false);

		__pAnimTimer->Start(ANIM_INTERVAL);
	} else if (state == STATE_DOWNLOADED) {
		int itemID = -1;
		__pItemIDs->GetAt(__curIndex, itemID);
		if (itemID >= 0) {
			FeedItem *item = __pChannel->GetItems()->GetByID(itemID);

			String enclosure = L"";
			String tmp = item->GetEnclosureURL();
			int sepIndex = -1;
			if (!IsFailed(tmp.IndexOf(L"~%~", 0, sepIndex)) && sepIndex >= 0 && (sepIndex + 3) < tmp.GetLength()) {
				tmp.SubString(0, sepIndex, enclosure);
			}

			if (!item->IsEnclosureDownloaded()) {
				RequestRedraw(true);
				return;
			} else if (!File::IsFileExist(enclosure)) {
				FeedManager::GetInstance()->ResetEnclosure(__pChannel, itemID);
				SetControlsToState(STATE_DOWNLOAD_BUTTON);
				return;
			}

			//positions and sizes
			__pEnclosureBg->SetPosition(0, 521);

			__pEnclosureMIMEIcon->SetPosition(9, 530);
			__pEnclosureMIMEIcon->SetSize(90,90);

			bool supported_format = true;
			String bitmap_name = L"big_warning.png";

			ContentType enc_type = CONTENT_TYPE_UNKNOWN;
			if (Utilities::IsFirmwareLaterThan10()) {
				enc_type = ContentManagerUtil::CheckContentType(enclosure);
			} else {
				enc_type = ContentManagerUtil::GetContentType(enclosure);
			}

			if (enc_type == CONTENT_TYPE_AUDIO/*item->GetEnclosureMIME().StartsWith(L"audio/", 0)*/) {
				bitmap_name = L"audio_file_icon.png";

				__pEnclosureMIMEText->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_TYPE_AUDIO"));
				AudioMetadata *meta = ContentManagerUtil::GetAudioMetaN(enclosure);
				if (meta) {
					if (meta->GetDuration() > 0) {
						__pEnclosureDuration->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_DURATION_LABEL") + FormatPlaybackTime(meta->GetDuration()));
					} else {
						__pEnclosureDuration->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_DURATION_LABEL") + L"N/A");
					}
					if (meta->GetBitrate() > 0) {
						__pEnclosureBitrate->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_BITRATE_LABEL") + Integer::ToString(meta->GetBitrate()) + Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_KBPS"));
					} else {
						__pEnclosureBitrate->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_BITRATE_LABEL") + L"N/A");
					}

					delete meta;
				} else {
					__pEnclosureDuration->SetText(L"");
					__pEnclosureBitrate->SetText(L"");

					supported_format = false;
				}
			} else if (enc_type == CONTENT_TYPE_VIDEO/*item->GetEnclosureMIME().StartsWith(L"video/", 0)*/) {
				bitmap_name = L"video_file_icon.png";

				__pEnclosureMIMEText->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_TYPE_VIDEO"));
				VideoMetadata *meta = ContentManagerUtil::GetVideoMetaN(enclosure);
				if (meta) {
					if (meta->GetDuration() > 0) {
						__pEnclosureDuration->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_DURATION_LABEL") + FormatPlaybackTime(meta->GetDuration()));
					} else {
						__pEnclosureDuration->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_DURATION_LABEL") + L"N/A");
					}
					if (meta->GetBitrate() > 0) {
						__pEnclosureBitrate->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_BITRATE_LABEL") + Integer::ToString(meta->GetBitrate()) + Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_KBPS"));
					} else {
						__pEnclosureBitrate->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_ENCLOSURE_BITRATE_LABEL") + L"N/A");
					}

					delete meta;
				} else {
					__pEnclosureDuration->SetText(L"");
					__pEnclosureBitrate->SetText(L"");

					supported_format = false;
				}
			} else {
				bitmap_name = L"other_file_icon.png";
				supported_format = false;
			}
			Bitmap *bmp = Utilities::GetBitmapN(bitmap_name);
			if (bmp) {
				__pEnclosureMIMEIcon->SetBackgroundBitmap(*bmp);
				delete bmp;
			}

			__pEnclosureLoadingText->SetPosition(108, 535);
			__pEnclosureLoadingText->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
			__pEnclosureLoadingText->SetTextConfig(37, LABEL_TEXT_STYLE_NORMAL);
			if (!supported_format) {
				__pEnclosureLoadingText->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_UNSUPPORTED_ENCLOSURE"));
				__pEnclosureLoadingText->SetShowState(true);

				__pEnclosureMIMEText->SetShowState(false);
				__pEnclosureDuration->SetShowState(false);
				__pEnclosureBitrate->SetShowState(false);
				__pOpenEnclosureButton->SetShowState(false);
			} else {
				__pOpenEnclosureButton->SetShowState(true);
				__pEnclosureMIMEText->SetShowState(true);
				__pEnclosureDuration->SetShowState(true);
				__pEnclosureBitrate->SetShowState(true);

				__pEnclosureLoadingText->SetShowState(false);
			}
			//visibility
			__pDownloadEnclosureButton->SetShowState(false);

			__pEnclosureBg->SetShowState(true);
			__pEnclosureMIMEIcon->SetShowState(true);

			__pDownloadProgressBar->SetShowState(false);
		} else {
			//we don't care for sizes and positions for STATE_NOTHING
			__pDownloadEnclosureButton->SetShowState(false);

			__pEnclosureBg->SetShowState(false);
			__pEnclosureLoadingText->SetShowState(false);
			__pEnclosureMIMEIcon->SetShowState(false);
			__pDownloadProgressBar->SetShowState(false);

			__pEnclosureMIMEText->SetShowState(false);
			__pEnclosureDuration->SetShowState(false);
			__pEnclosureBitrate->SetShowState(false);
			__pOpenEnclosureButton->SetShowState(false);
		}
	} else if (state == STATE_ERROR) {
		//positions and sizes
		__pEnclosureBg->SetPosition(0, 521);

		__pEnclosureLoadingText->SetPosition(108, 535);
		__pEnclosureLoadingText->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
		__pEnclosureLoadingText->SetTextConfig(37, LABEL_TEXT_STYLE_NORMAL);
		if (!storage_error) {
			__pEnclosureLoadingText->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_DOWNLOAD_ERROR"));
		} else {
			__pEnclosureLoadingText->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_DOWNLOAD_ERROR_NO_SPACE"));
		}

		__pEnclosureMIMEIcon->SetPosition(19, 540);
		__pEnclosureMIMEIcon->SetSize(70, 70);
		Bitmap *warn = Utilities::GetBitmapN(L"big_warning.png");
		if (warn) {
			__pEnclosureMIMEIcon->SetBackgroundBitmap(*warn);
			delete warn;
		}
		//visibility
		__pDownloadEnclosureButton->SetShowState(false);

		__pEnclosureBg->SetShowState(true);
		__pEnclosureLoadingText->SetShowState(true);
		__pEnclosureMIMEIcon->SetShowState(true);
		__pDownloadProgressBar->SetShowState(false);

		__pEnclosureMIMEText->SetShowState(false);
		__pEnclosureDuration->SetShowState(false);
		__pEnclosureBitrate->SetShowState(false);
		__pOpenEnclosureButton->SetShowState(false);
	} else {
		//we don't care for sizes and positions for STATE_NOTHING
		__pDownloadEnclosureButton->SetShowState(false);

		__pEnclosureBg->SetShowState(false);
		__pEnclosureLoadingText->SetShowState(false);
		__pEnclosureMIMEIcon->SetShowState(false);
		__pDownloadProgressBar->SetShowState(false);

		__pEnclosureMIMEText->SetShowState(false);
		__pEnclosureDuration->SetShowState(false);
		__pEnclosureBitrate->SetShowState(false);
		__pOpenEnclosureButton->SetShowState(false);
	}

	RequestRedraw(true);
}

result FeedItemForm::RefreshData(void) {
	if (__pTitleLabel && __pPubLabel) {
		int itemID = -1;
		__pItemIDs->GetAt(__curIndex, itemID);
		if (itemID >= 0) {
			//in case we were downloading enclosure before switching to next item
			if (__pDownloadSession) {
				__pDownloadSession->CloseAllTransactions();
				delete __pDownloadSession;
				__pDownloadSession = null;
			}
			if (__pDownloadFile) {
				delete __pDownloadFile;
				__pDownloadFile = null;
			}
			if (!__pDownloadFilePath.IsEmpty()) {
				File::Remove(__pDownloadFilePath);
				__pDownloadFilePath = L"";
			}

			FeedItem *item = __pChannel->GetItems()->GetByID(itemID);

			if (__curIndex <= 0) {
				__pPrevButton->SetEnabled(false);
			} else if (__curIndex >= __pItemIDs->GetCount() - 1) {
				__pNextButton->SetEnabled(false);
			} else if (__curIndex == 0 && __curIndex == __pItemIDs->GetCount() - 1) {
				__pPrevButton->SetEnabled(false);
				__pNextButton->SetEnabled(false);
			} else {
				__pPrevButton->SetEnabled(true);
				__pNextButton->SetEnabled(true);
			}

			int lines = Utilities::GetLinesForText(item->GetTitle(), 30, 395);

			__pTitleLabel->SetSize(415, lines * 31);
			__pTitleLabel->SetText(item->GetTitle());

			__pPubLabel->SetPosition(0, lines * 31 + 11);
			__pPubLabel->SetText(Utilities::GetString(L"IDS_FEEDITEMFORM_PUB_LABEL") + Utilities::FormatDateTimeFromEpoch(item->GetPubDate()));

			__pHeaderLabel->SetSize(480, lines * 31 + 45);
			__pHeaderLabel->SetBackgroundBitmap(*__pHeaderBitmapNormal);

			__pSepLabel->SetPosition(0, lines * 31 + 35);

			__pConMenu->SetPosition(240, lines * 31 + 35 + 88);

			__pFeedMenuIconLabel->SetPosition(420, (36 + lines * 31) / 2 - 22);

			if (item->GetEnclosureURL().IsEmpty() && !item->IsEnclosureDownloaded()) {
				SetControlsToState(STATE_NOTHING);

				__pContentView->SetSize(480, 624 - (lines * 31 + 45));
			} else {
				__pContentView->SetSize(480, 521 - (lines * 31 + 45));
				if (item->IsEnclosureDownloaded()) {
					//setup enclosure
					SetControlsToState(STATE_DOWNLOADED);
				} else {
					SetControlsToState(STATE_DOWNLOAD_BUTTON);
				}
			}
			__pContentView->SetPosition(0, lines * 31 + 45);

			String html = L"<html><head><meta name=\"viewport\" content=\"initial-scale=1.0, width=device-width, height=device-height\"/></head><body bgcolor=\"#2b2b2b\" text=\"#ffffff\">";
			html.Append(item->GetContent());
			html.Append(L"</body></html>");

			if (!SettingsManager::GetInstance()->GetLoadImagesByDefault()) {
				int imgCount = 0;
				int imgIndex = -4;
				while (!IsFailed(html.IndexOf(L"<img", imgIndex + 4, imgIndex)) && imgIndex < html.GetLength() && imgIndex >= 0) {
					int closeIndex = -1;
					if (!IsFailed(html.IndexOf(L'>', imgIndex, closeIndex))) {
						html.Remove(imgIndex, closeIndex - imgIndex + 1);
						html.Insert(L"<img src=\"/Res/no_img.png\" width=\"16\" height=\"16\" alt=\"\">", imgIndex);

						imgCount++;
					}
				}
				if (imgCount > 0) {
					__pContentView->SetSize(480,  __pContentView->GetHeight() - 95);
					__pContentView->SetPosition(0, lines * 31 + 140);

					__pShowImagesButton->SetPosition(10, lines * 31 + 55);
					__pShowImagesButton->SetShowState(true);
				} else {
					__pShowImagesButton->SetShowState(false);
				}
			} else {
				__pShowImagesButton->SetShowState(false);
			}

			ByteBuffer *buf = StringUtil::StringToUtf8N(html);
			__pContentView->LoadData(L"file:///Home/none/" + Integer::ToString(__curIndex), *buf, L"text/html", L"UTF-8");
			delete buf;

			__curGuidLink = item->GetGuidLink();
			if (__curGuidLink.IsEmpty() || !__curGuidLink.StartsWith(L"http", 0)) {
				if (__isGuidPresent) {
					//__pFeedMenuIconLabel->SetShowState(false);
					Bitmap *pEmailIconNormal = Utilities::GetBitmapN(L"email_icon_noguid.png");
					if (pEmailIconNormal) {
						__pFeedMenuIconLabel->SetBackgroundBitmap(*pEmailIconNormal);
						delete pEmailIconNormal;
					}
				}
				__isGuidPresent = false;
			} else {
				if (!__isGuidPresent) {
					//__pFeedMenuIconLabel->SetShowState(true);
					Bitmap *guid = Utilities::GetBitmapN(L"feed_menu_icon.png");
					if (guid) {
						__pFeedMenuIconLabel->SetBackgroundBitmap(*guid);
						delete guid;
					}
				}
				__isGuidPresent = true;
			}

			if (!item->IsRead()) {
				NotificationManager* pNotiMgr = new NotificationManager();
				if (!IsFailed(pNotiMgr->Construct())) {
					int count = pNotiMgr->GetBadgeNumber();
					if (count > 0) {
						pNotiMgr->Notify(count - 1);
					}
				}
				delete pNotiMgr;
			}

			FeedManager::GetInstance()->MarkItemAsRead(__pChannel, itemID);
			if (__pReadItemIDs && !__pReadItemIDs->Contains(itemID)) {
				__pReadItemIDs->Add(itemID);
			}

			RequestRedraw(true);
			return E_SUCCESS;
		} else {
			return E_INVALID_STATE;
		}
	} else {
		return E_INVALID_STATE;
	}
}

result FeedItemForm::Initialize(void) {
	result res = Construct(FORM_STYLE_INDICATOR | FORM_STYLE_TITLE);
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct parent form", GetErrorMessage(res));
		return res;
	}
	SetBackgroundColor(Color(0x282828, false));
	Bitmap *pIcon = Utilities::GetBitmapN(L"mainform_icon.png");
	SetTitleIcon(pIcon);
	if (pIcon) {
		delete pIcon;
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
			AppLogDebug("FeedItemForm::Initialize: failed to get bitmaps for loading animation");
			return E_FAILURE;
		}
	}

	Bitmap *pButtonNormal = Utilities::GetBitmapN(L"button_bg_unpressed.png");
	Bitmap *pButtonPressed = Utilities::GetBitmapN(L"button_bg_pressed.png");

	__pPrevButton = new Button;
	res = __pPrevButton->Construct(Rectangle(10, 629, 141, 75), Utilities::GetString(L"IDS_FEEDITEMFORM_PREV_BUTTON"));
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct prev button", GetErrorMessage(res));
		return res;
	}
	if (pButtonNormal) {
		__pPrevButton->SetNormalBackgroundBitmap(*pButtonNormal);
	}
	if (pButtonPressed) {
		__pPrevButton->SetPressedBackgroundBitmap(*pButtonPressed);
	}
	__pPrevButton->SetActionId(HANDLER(FeedItemForm::OnPrevButtonClicked));
	__pPrevButton->AddActionEventListener(*this);

	Button *backButton = new Button;
	res = backButton->Construct(Rectangle(160, 629, 161, 75), Utilities::GetString(L"IDS_FEEDITEMFORM_BACK_BUTTON"));
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct back button", GetErrorMessage(res));
		return res;
	}
	if (pButtonNormal) {
		backButton->SetNormalBackgroundBitmap(*pButtonNormal);
	}
	if (pButtonPressed) {
		backButton->SetPressedBackgroundBitmap(*pButtonPressed);
	}
	backButton->SetActionId(HANDLER(FeedItemForm::OnBackButtonClicked));
	backButton->AddActionEventListener(*this);

	__pNextButton = new Button;
	res = __pNextButton->Construct(Rectangle(330, 629, 141, 75), Utilities::GetString(L"IDS_FEEDITEMFORM_NEXT_BUTTON"));
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct next button", GetErrorMessage(res));
		return res;
	}
	if (pButtonNormal) {
		__pNextButton->SetNormalBackgroundBitmap(*pButtonNormal);
	}
	if (pButtonPressed) {
		__pNextButton->SetPressedBackgroundBitmap(*pButtonPressed);
	}
	__pNextButton->SetActionId(HANDLER(FeedItemForm::OnNextButtonClicked));
	__pNextButton->AddActionEventListener(*this);

	__pShowImagesButton = new Button;
	res = __pShowImagesButton->Construct(Rectangle(10,80,460,80), Utilities::GetString(L"IDS_FEEDITEMFORM_SHOW_IMAGES_BUTTON"));
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct show images button", GetErrorMessage(res));
		return res;
	}
	Bitmap *bmp = Utilities::GetBitmapN(L"search_channels_button.png");
	if (bmp) {
		__pShowImagesButton->SetNormalBackgroundBitmap(*bmp);
		delete bmp;
	}
	bmp = Utilities::GetBitmapN(L"search_channels_button_pressed.png");
	if (bmp) {
		__pShowImagesButton->SetPressedBackgroundBitmap(*bmp);
		delete bmp;
	}
	__pShowImagesButton->SetTextColor(Color::COLOR_WHITE);
	__pShowImagesButton->SetTextHorizontalAlignment(ALIGNMENT_CENTER);
	__pShowImagesButton->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
	__pShowImagesButton->SetActionId(HANDLER(FeedItemForm::OnShowImagesButtonClicked));
	__pShowImagesButton->AddActionEventListener(*this);
	__pShowImagesButton->SetShowState(false);

	if (pButtonNormal) {
		delete pButtonNormal;
	}
	if (pButtonPressed) {
		delete pButtonPressed;
	}

	//+enclosure
	__pEnclosureBg = new Label;
	res = __pEnclosureBg->Construct(Rectangle(0, 712, 480, 191), L" ");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct enclosure bg label", GetErrorMessage(res));
		return res;
	}
	bmp = Utilities::GetBitmapN(L"feed_item_form_enclosure_bg.png");
	if (bmp) {
		__pEnclosureBg->SetBackgroundBitmap(*bmp);
		delete bmp;
	}
	__pEnclosureBg->SetShowState(false);

	__pEnclosureMIMEIcon = new Label;
	res = __pEnclosureMIMEIcon->Construct(Rectangle(26, 712 + 26, 56, 56), L" ");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct enclosure mime icon label", GetErrorMessage(res));
		return res;
	}
	__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap1);
	__pEnclosureMIMEIcon->SetShowState(false);

	__pEnclosureLoadingText = new Label;
	res = __pEnclosureLoadingText->Construct(Rectangle(108, 712 + 14, 362, 80), Utilities::GetString(L"IDS_ADDCHANNELFORM_MBOX_DOWNLOAD"));
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct enclosure downloading label", GetErrorMessage(res));
		return res;
	}
	__pEnclosureLoadingText->SetTextColor(Color::COLOR_WHITE);
	__pEnclosureLoadingText->SetTextConfig(37, LABEL_TEXT_STYLE_NORMAL);
	__pEnclosureLoadingText->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	__pEnclosureLoadingText->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
	__pEnclosureLoadingText->SetShowState(false);

	__pDownloadProgressBar = new Progress;
	res = __pDownloadProgressBar->Construct(Rectangle(108, 585, 332, 20), 0, 100);
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct enclosure downloading progress bar", GetErrorMessage(res));
		return res;
	}
	__pDownloadProgressBar->SetShowState(false);

	__pEnclosureMIMEText = new Label;
	res = __pEnclosureMIMEText->Construct(Rectangle(108,535,362,30), L"Audio");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct enclosure mime type label", GetErrorMessage(res));
		return res;
	}
	__pEnclosureMIMEText->SetTextColor(Color::COLOR_WHITE);
	__pEnclosureMIMEText->SetTextConfig(30, LABEL_TEXT_STYLE_NORMAL);
	__pEnclosureMIMEText->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	__pEnclosureMIMEText->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
	__pEnclosureMIMEText->SetShowState(false);

	__pEnclosureDuration = new Label;
	res = __pEnclosureDuration->Construct(Rectangle(108, 565, 362, 25), L"Duration: 1:38:10");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct enclosure duration label", GetErrorMessage(res));
		return res;
	}
	__pEnclosureDuration->SetTextColor(Color::COLOR_WHITE);
	__pEnclosureDuration->SetTextConfig(25, LABEL_TEXT_STYLE_NORMAL);
	__pEnclosureDuration->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	__pEnclosureDuration->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
	__pEnclosureDuration->SetShowState(false);

	__pEnclosureBitrate = new Label;
	res = __pEnclosureBitrate->Construct(Rectangle(108, 590, 362, 25), L"File size: 44,5 MB");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct enclosure duration label", GetErrorMessage(res));
		return res;
	}
	__pEnclosureBitrate->SetTextColor(Color::COLOR_WHITE);
	__pEnclosureBitrate->SetTextConfig(25, LABEL_TEXT_STYLE_NORMAL);
	__pEnclosureBitrate->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	__pEnclosureBitrate->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
	__pEnclosureBitrate->SetShowState(false);

	__pOpenEnclosureButton = new Button;
	res = __pOpenEnclosureButton->Construct(Rectangle(372, 530, 90, 90), L"");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct open enclosure button", GetErrorMessage(res));
		return res;
	}
	bmp = Utilities::GetBitmapN(L"enclosure_play_button_normal.png");
	if (bmp) {
		__pOpenEnclosureButton->SetNormalBackgroundBitmap(*bmp);
		delete bmp;
	}
	bmp = Utilities::GetBitmapN(L"enclosure_play_button_pressed.png");
	if (bmp) {
		__pOpenEnclosureButton->SetPressedBackgroundBitmap(*bmp);
		delete bmp;
	}
	__pOpenEnclosureButton->SetShowState(false);
	__pOpenEnclosureButton->SetActionId(HANDLER(FeedItemForm::OnOpenEnclosureClicked));
	__pOpenEnclosureButton->AddActionEventListener(*this);

	__pDownloadEnclosureButton = new Button;
	res = __pDownloadEnclosureButton->Construct(Rectangle(10,535,460,80), Utilities::GetString(L"IDS_FEEDITEMFORM_DOWNLOAD_ENCLOSURE"));
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct download enclosure button", GetErrorMessage(res));
		return res;
	}
	__pDownloadEnclosureButton->SetTextColor(Color::COLOR_WHITE);
	__pDownloadEnclosureButton->SetTextHorizontalAlignment(ALIGNMENT_CENTER);
	__pDownloadEnclosureButton->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
	bmp = Utilities::GetBitmapN(L"search_channels_button.png");
	if (bmp) {
		__pDownloadEnclosureButton->SetNormalBackgroundBitmap(*bmp);
		delete bmp;
	}
	bmp = Utilities::GetBitmapN(L"search_channels_button_pressed.png");
	if (bmp) {
		__pDownloadEnclosureButton->SetPressedBackgroundBitmap(*bmp);
		delete bmp;
	}
	__pDownloadEnclosureButton->SetShowState(false);
	__pDownloadEnclosureButton->SetActionId(HANDLER(FeedItemForm::OnDownloadEnclosureButtonClicked));
	__pDownloadEnclosureButton->AddActionEventListener(*this);

	AddControl(*__pEnclosureBg);
	AddControl(*__pEnclosureLoadingText);
	AddControl(*__pEnclosureMIMEIcon);
	AddControl(*__pDownloadProgressBar);
	AddControl(*__pDownloadEnclosureButton);
	AddControl(*__pEnclosureMIMEText);
	AddControl(*__pEnclosureDuration);
	AddControl(*__pEnclosureBitrate);
	AddControl(*__pOpenEnclosureButton);
	//-enclosure

	AddControl(*__pPrevButton);
	AddControl(*backButton);
	AddControl(*__pNextButton);
	AddControl(*__pShowImagesButton);

	__pTitleLabel = new Label;
	res = __pTitleLabel->Construct(Rectangle(0, 8, 415, 30), L"");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct title label", GetErrorMessage(res));
		return res;
	}
	__pTitleLabel->SetTextColor(Color::COLOR_WHITE);
	__pTitleLabel->SetTextConfig(30, LABEL_TEXT_STYLE_NORMAL);
	__pTitleLabel->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	__pTitleLabel->SetTextVerticalAlignment(ALIGNMENT_TOP);
	__pTitleLabel->AddTouchEventListener(*this);

	__pPubLabel = new Label;
	res = __pPubLabel->Construct(Rectangle(0, 41, 415, 24), L"");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct pub date label", GetErrorMessage(res));
		return res;
	}
	__pPubLabel->SetTextColor(Color::COLOR_GREY);
	__pPubLabel->SetTextConfig(24, LABEL_TEXT_STYLE_NORMAL);
	__pPubLabel->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	__pPubLabel->SetTextVerticalAlignment(ALIGNMENT_TOP);
	__pPubLabel->AddTouchEventListener(*this);

	__pHeaderLabel = new Label;
	res = __pHeaderLabel->Construct(Rectangle(0,0,480,75), L"");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct header label", GetErrorMessage(res));
		return res;
	}
	__pHeaderBitmapNormal = Utilities::GetBitmapN(L"feed_item_form_header.png");
	if (__pHeaderBitmapNormal) {
		__pHeaderLabel->SetBackgroundBitmap(*__pHeaderBitmapNormal);
	} else {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to acquire normal header bitmap", GetErrorMessage(res));
		return GetLastResult();
	}
	__pHeaderBitmapPressed = Utilities::GetBitmapN(L"channel_list_item_bg_pressed.png");
	if (!__pHeaderBitmapPressed) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to acquire pressed header bitmap", GetErrorMessage(res));
		return GetLastResult();
	}
	__pHeaderLabel->AddTouchEventListener(*this);

	__pFeedMenuIconLabel = new Label;
	res = __pFeedMenuIconLabel->Construct(Rectangle(420,8,55,55), L"");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct guid icon label", GetErrorMessage(res));
		return res;
	}
	Bitmap *guid = Utilities::GetBitmapN(L"feed_menu_icon.png");
	if (guid) {
		__pFeedMenuIconLabel->SetBackgroundBitmap(*guid);
		delete guid;
	}
	__pFeedMenuIconLabel->AddTouchEventListener(*this);

	__pSepLabel = new Label;
	res = __pSepLabel->Construct(Rectangle(0,65,480,10), L"");
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct separator label", GetErrorMessage(res));
		return res;
	}
	Bitmap *sep = Utilities::GetBitmapN(L"feed_item_form_separator.png");
	if (sep) {
		__pSepLabel->SetBackgroundBitmap(*sep);
		delete sep;
	}

	AddControl(*__pHeaderLabel);
	AddControl(*__pTitleLabel);
	AddControl(*__pPubLabel);
	AddControl(*__pFeedMenuIconLabel);
	AddControl(*__pSepLabel);

	__pContentView = new Web;
	res = __pContentView->Construct(Rectangle(0, 75, 480, 554));
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to construct content view", GetErrorMessage(res));
		return res;
	}
	__pContentView->SetLoadingListener(this);

	AddControl(*__pContentView);

	__pConMenu = new ContextMenu;
	__pConMenu->Construct(Point(377, 157), CONTEXT_MENU_STYLE_ICON);

	Bitmap *pGuidIconNormal = Utilities::GetBitmapN(L"guid_icon.png");
	Bitmap *pGuidIconPressed = Utilities::GetBitmapN(L"guid_icon_pressed.png");
	if (pGuidIconNormal) {
		__pConMenu->AddItem(*pGuidIconNormal, pGuidIconPressed, HANDLER(FeedItemForm::OnGuidButtonClicked));
		delete pGuidIconNormal;
	}
	if (pGuidIconPressed) {
		delete pGuidIconPressed;
	}

	Bitmap *pEmailIconNormal = Utilities::GetBitmapN(L"email_icon.png");
	Bitmap *pEmailIconPressed = Utilities::GetBitmapN(L"email_icon_pressed.png");
	if (pEmailIconNormal) {
		__pConMenu->AddItem(*pEmailIconNormal, pEmailIconPressed, HANDLER(FeedItemForm::OnEmailButtonClicked));
		delete pEmailIconNormal;
	}
	if (pEmailIconPressed) {
		delete pEmailIconPressed;
	}
	__pConMenu->AddActionEventListener(*this);

	__pAnimTimer = new Timer;
	res = __pAnimTimer->Construct(*this);
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to initialize animation timer", GetErrorMessage(res));
		return res;
	}

	__pRiseTimer = new Timer;
	res = __pRiseTimer->Construct(*this);
	if (IsFailed(res)) {
		AppLogDebug("FeedItemForm::Initialize: [%s]: failed to initialize rising animation timer", GetErrorMessage(res));
		return res;
	}

	__pReadItemIDs = new ArrayListT<int>;
	if (IsFailed(__pReadItemIDs->Construct(10))) {
		delete __pReadItemIDs;
		__pReadItemIDs = null;
	}

	return E_SUCCESS;
}

result FeedItemForm::Terminate(void) {
	return E_SUCCESS;
}

result FeedItemForm::OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID) {
	if (reason == REASON_NONE) {
		if (pArgs && pArgs->GetCount() > 2) {
			Object *arg = pArgs->GetAt(2);
			if (typeid(*arg) == typeid(ArrayListT<int>)) {
				__pItemIDs = static_cast<ArrayListT<int> *>(arg);
			} else {
				return E_INVALID_ARG;
			}

			arg = pArgs->GetAt(0);
			if (typeid(*arg) == typeid(Channel)) {
				__pChannel = static_cast<Channel *>(arg);
				if (!__pChannel->GetItems()) {
					return E_INVALID_ARG;
				}
				SetTitleText(__pChannel->GetTitle(), ALIGNMENT_LEFT);
			} else {
				return E_INVALID_ARG;
			}

			arg = pArgs->GetAt(1);
			if (typeid(*arg) == typeid(Integer)) {
				Integer *itemInd = static_cast<Integer *>(arg);
				__curIndex = itemInd->ToInt();
				return E_SUCCESS;
			} else {
				return E_INVALID_ARG;
			}
		} else {
			return E_INVALID_ARG;
		}
	}
	return E_SUCCESS;
}

bool FeedItemForm::OnLoadingRequested(const String &url, WebNavigationType type) {
	ArrayList* pDataList = new ArrayList();
	pDataList->Construct();

	String* pData = new String(L"url:" + url);
	pDataList->Add(*pData);

	AppControl* pAc = AppManager::FindAppControlN(APPCONTROL_BROWSER, "");
	if(pAc)	{
	  pAc->Start(pDataList, null);
	  delete pAc;
	}

	pDataList->RemoveAll(true);
	delete pDataList;

	return true;
}

void FeedItemForm::OnTouchPressed(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) {
	//if (__isGuidPresent) {
		__pHeaderLabel->SetBackgroundBitmap(*__pHeaderBitmapPressed);
		__canTriggerPress = true;

		RequestRedraw(true);
	//}
}

void FeedItemForm::OnTouchReleased(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) {
	//if (__isGuidPresent) {
		if (__canTriggerPress) {
			__pHeaderLabel->SetBackgroundBitmap(*__pHeaderBitmapNormal);
			RequestRedraw(true);

			if (__isGuidPresent) {
				__pConMenu->SetShowState(true);
				__pConMenu->Show();
			} else {
				OnEmailButtonClicked(source);
			}
		}
		__canTriggerPress = false;
	//}
}

void FeedItemForm::OnTouchFocusIn(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) {
	//if (__isGuidPresent) {
		__pHeaderLabel->SetBackgroundBitmap(*__pHeaderBitmapPressed);
		__canTriggerPress = true;

		RequestRedraw(true);
	//}
}

void FeedItemForm::OnTouchFocusOut(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) {
	//if (__isGuidPresent) {
		__pHeaderLabel->SetBackgroundBitmap(*__pHeaderBitmapNormal);
		__canTriggerPress = false;

		RequestRedraw(true);
	//}
}

void FeedItemForm::OnTimerExpired(Timer &timer) {
	if (timer.Equals(*__pAnimTimer)) {
		if (__pEnclosureMIMEIcon) {
			static int progress = 0;
			int index = progress % 8;

			switch (index)
			{
			case 0:
				__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap1);
				break;
			case 1:
				__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap2);
				break;
			case 2:
				__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap3);
				break;
			case 3:
				__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap4);
				break;
			case 4:
				__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap5);
				break;
			case 5:
				__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap6);
				break;
			case 6:
				__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap7);
				break;
			case 7:
				__pEnclosureMIMEIcon->SetBackgroundBitmap(*__loadingAnimData.__pProgressBitmap8);
				break;
			default:
				break;
			}
			progress++;

			__pEnclosureMIMEIcon->RequestRedraw(true);
			__pAnimTimer->Start(ANIM_INTERVAL);
		}
	} else {
		__bgRiseCurrentY -= 25;
		__pEnclosureBg->SetPosition(0, __bgRiseCurrentY);
		__pEnclosureLoadingText->SetPosition(108, __bgRiseCurrentY + 14);
		__pEnclosureMIMEIcon->SetPosition(26, __bgRiseCurrentY + 26);

		if (__bgRiseCurrentY <= 521) {
			__pEnclosureBg->SetPosition(0, 521);
			__pEnclosureLoadingText->SetPosition(108, 535);
			__pEnclosureMIMEIcon->SetPosition(26, 547);

			FetchEnclosure();
		} else {
			__pRiseTimer->Start(ANIM_INTERVAL / 3);
		}

		RequestRedraw(true);
	}
}

void FeedItemForm::OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen) {
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
		if (__downloadLength <= 0) {
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

			if (!IsFailed(Integer::Parse(content_length, __downloadLength))) {
				__pEnclosureLoadingText->SetTextVerticalAlignment(ALIGNMENT_TOP);
				__pDownloadProgressBar->SetShowState(true);

				__pEnclosureLoadingText->RequestRedraw(true);
				__pDownloadProgressBar->RequestRedraw(true);
			}
		}
		pData = pHttpResponse->ReadBodyN();
		result res = GetLastResult();
		if (IsFailed(res)) {
			AppLogDebug("FeedItemForm::OnTransactionReadyToRead: [%s]: failed to acquire HTTP response body", GetErrorMessage(res));
		}
	} else {
		AppLogDebug("FeedItemForm::OnTransactionReadyToRead: failed to get proper HTTP response, status code: [%d], text: [%S]", (int)pHttpResponse->GetStatusCode(), pHttpResponse->GetStatusText().GetPointer());
	}

	//parse the response
	if (pData && availableBodyLen > 0 && __pDownloadFile) {
		static int flush_count = 0;
		result res = __pDownloadFile->Write(*pData);
		if (IsFailed(res)) {
			SetDownloadError(res == E_STORAGE_FULL);
			delete pData;
			return;
		}

		flush_count++;
		if (flush_count >= 10) {
			__pDownloadFile->Flush();
			flush_count = 0;
		}

		__downloadedAlready += availableBodyLen;
		if (__downloadLength > 0) {
			__pDownloadProgressBar->SetValue((int)(((float)__downloadedAlready / __downloadLength) * 100));
			__pDownloadProgressBar->RequestRedraw(true);
		}

		delete pData;
	}
}

void FeedItemForm::OnTransactionHeaderCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction, int headerLen, bool rs) {
	static int redirect_count = 0;

	HttpResponse* pHttpResponse = httpTransaction.GetResponse();
	if(!pHttpResponse) {
		return;
	}

	HttpHeader *pHeader = pHttpResponse->GetHeader();
	if (!pHeader) {
		return;
	}

	if (pHttpResponse->GetStatusCode() == NET_HTTP_STATUS_MOVED_PERMANENTLY
		|| pHttpResponse->GetStatusCode() == NET_HTTP_STATUS_MOVED_TEMPORARILY) {
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

		if (!redirect_url.IsEmpty()) {
			if (redirect_count >= 10) {
				//too much redirects, report error
				SetDownloadError();

				redirect_count = 0;
				return;
			}
			redirect_count++;

			//init download from redirected url
			FetchEnclosure(redirect_url);
		} else {
			//received empty redirect url, report error
			SetDownloadError();
			redirect_count = 0;
		}
		return;
	}
	redirect_count = 0;
}

void FeedItemForm::OnTransactionAborted(HttpSession &httpSession, HttpTransaction &httpTransaction, result r) {
	//close and stop everything and report error
	SetDownloadError();
}

void FeedItemForm::OnTransactionCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction) {
	//dump the file and report that we've finished and are ready to rock
	if (__pDownloadFile) {
		__pDownloadFile->Flush();

		delete __pDownloadFile;
		__pDownloadFile = null;
	}

	int itemID = -1;
	__pItemIDs->GetAt(__curIndex, itemID);
	if (itemID >= 0) {
		if (!SettingsManager::GetInstance()->GetDownloadToDefaultEnclosureFolder()) {
			FeedItem *item = __pChannel->GetItems()->GetByID(itemID);
			if (item) {
				String ext = L".dat";
				int lastDotIndex = -1;
				if (!IsFailed(__pDownloadFilePath.LastIndexOf(L'.', __pDownloadFilePath.GetLength() - 1, lastDotIndex)) && lastDotIndex >= 0) {
					__pDownloadFilePath.SubString(lastDotIndex, ext);
				}

				String dest = SettingsManager::GetInstance()->GetEnclosureDownloadFolder();
				if (!dest.EndsWith(L"/")) {
					dest.Append(L'/');
				}
				dest.Append(item->GetUniqueString());

				String content_dest = L"";

				int existCounter = 2;
				result res = ContentManagerUtil::MoveToMediaDirectory(__pDownloadFilePath, dest + ext);
				content_dest = dest + ext;
				while (res == E_FILE_ALREADY_EXIST) {
					res = ContentManagerUtil::MoveToMediaDirectory(__pDownloadFilePath, dest + L"_" + Integer::ToString(existCounter++) + ext);
					content_dest = dest + L"_" + Integer::ToString(existCounter - 1) + ext;
				}
				if (IsFailed(res)) {
					Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_FEEDITEMFORM_ERROR_MBOX_FAILED_TO_MOVE_FILE"), MSGBOX_STYLE_OK);
				} else {
					__pDownloadFilePath = content_dest;

					ContentInfo *info = null;
					ContentType enc_type = CONTENT_TYPE_UNKNOWN;
					if (Utilities::IsFirmwareLaterThan10()) {
						enc_type = ContentManagerUtil::CheckContentType(content_dest);
					} else {
						enc_type = ContentManagerUtil::GetContentType(content_dest);
					}
					switch (enc_type) {
						case CONTENT_TYPE_AUDIO: {
							info = new AudioContentInfo;
							break;
						}
						case CONTENT_TYPE_IMAGE: {
							info = new ImageContentInfo;
							break;
						}
						case CONTENT_TYPE_VIDEO: {
							info = new VideoContentInfo;
							break;
						}
						default: {
							info = new OtherContentInfo;
							break;
						}
					}
					if (info) {
						info->Construct(content_dest);

						ContentManager mgr;
						res = mgr.Construct();
						if (!IsFailed(res)) {
							mgr.CreateContent(*info);
							res = GetLastResult();
						}
						if (IsFailed(res)) {
							Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_FEEDITEMFORM_ERROR_MBOX_FAILED_TO_REG_CONTENT"), MSGBOX_STYLE_OK);
						}
					}
				}
			}
		}

		FeedManager::GetInstance()->SetEnclosureAsDownloaded(__pChannel, itemID, __pDownloadFilePath);
	}

	__pDownloadFilePath = L"";

	if (__pDownloadSession) {
		__pDownloadSession->CloseAllTransactions();
		delete __pDownloadSession;
		__pDownloadSession = null;
	}

	SetControlsToState(STATE_DOWNLOADED);
}
