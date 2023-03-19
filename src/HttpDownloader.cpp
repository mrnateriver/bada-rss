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
#include "HttpDownloader.h"

using namespace Osp::Base::Collection;

HttpDownloader::HttpDownloader() {
	__pHttpSession = null;
	__pPingTimer = null;

	__downloadCompleted = false;
	__pDownloadedData = null;
	__timeout = false;
}

HttpDownloader::~HttpDownloader() {
	if (__pHttpSession) {
		delete __pHttpSession;
	}
	if (__pPingTimer) {
		delete __pPingTimer;
	}
	if (__pDownloadedData) {
		delete __pDownloadedData;
	}
}

result HttpDownloader::Construct(void) {
	__pPingTimer = new Timer;
	return __pPingTimer->Construct(*this);
}

result HttpDownloader::Download(const String &url, ByteBuffer *&data, int pingTimeout) {
	if (!__pPingTimer) {
		AppLogDebug("HttpDownloader::Download: instance is not constructed");
		return E_INVALID_STATE;
	}

	result res = Download(url, *this);
	if (IsFailed(res)) {
		AppLogDebug("HttpDownloader::Download: [%s]: failed to initialize asynchronous download", GetErrorMessage(res));
		return res;
	}

	__pPingTimer->Start(pingTimeout * 10);
	while (!__timeout || !__downloadCompleted) {
		Thread::Sleep(200);
	}

	if (__timeout) {
		AppLogDebug("HttpDownloader::Download: ping timeout");
		return E_TIMEOUT;
	} else {
		__pPingTimer->Cancel();
	}

	if (!__pDownloadedData) {
		AppLogDebug("HttpDownloader::Download: HTTP request failed");
		return E_FAILURE;
	}

	data = __pDownloadedData;

	__downloadCompleted = false;
	__pDownloadedData = null;
	__timeout = false;
	return E_SUCCESS;
}

result HttpDownloader::Download(const String &url, const IHttpTransactionEventListener &handler) {
	__pHttpSession = new HttpSession();
	result res = __pHttpSession->Construct(NET_HTTP_SESSION_MODE_NORMAL, null, url, null, NET_HTTP_COOKIE_FLAG_ALWAYS_MANUAL);
	if (IsFailed(res)) {
		AppLogDebug("HttpDownloader::Download: [%s]: failed to initialize HTTP session", GetErrorMessage(res));

		delete __pHttpSession;
		__pHttpSession = null;

		return res;
	}

	HttpTransaction *pTransaction = __pHttpSession->OpenTransactionN();
	res = GetLastResult();
	if (IsFailed(res) || !pTransaction) {
		AppLogDebug("HttpDownloader::Download: [%s]: failed to open transaction for HTTP session", GetErrorMessage(res));

		delete __pHttpSession;
		__pHttpSession = null;

		return res;
	}
	pTransaction->AddHttpTransactionListener(handler);

	HttpRequest *pRequest = pTransaction->GetRequest();
	res = GetLastResult();
	if (IsFailed(res) || !pRequest) {
		AppLogDebug("HttpDownloader::Download: [%s]: failed to initialize HTTP request", GetErrorMessage(res));

		delete __pHttpSession;
		__pHttpSession = null;
		delete pTransaction;

		return res;
	}

	pRequest->SetUri(url);
	pRequest->SetMethod(NET_HTTP_METHOD_GET);
	pRequest->GetHeader()->AddField("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)");

	res = pTransaction->Submit();
	if (IsFailed(res)) {
		AppLogDebug("HttpDownloader::Download: [%s]: failed to submit HTTP request", GetErrorMessage(res));

		delete __pHttpSession;
		__pHttpSession = null;
		delete pTransaction;

		return res;
	}

	return E_SUCCESS;
}

void HttpDownloader::OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen) {
	__pDownloadedData = null;

	HttpResponse* pHttpResponse = httpTransaction.GetResponse();
	if(!pHttpResponse) {
		__downloadCompleted = true;
		__timeout = false;
		__pPingTimer->Cancel();
		return;
	}
	if (pHttpResponse->GetStatusCode() == NET_HTTP_STATUS_OK) {
		HttpHeader* pHttpHeader = pHttpResponse->GetHeader();
		if(pHttpHeader) {
			__pDownloadedData = pHttpResponse->ReadBodyN();
		} else {
			AppLogDebug("HttpDownloader::OnTransactionReadyToRead: failed to get proper HTTP response header");
		}
	} else {
		AppLogDebug("HttpDownloader::OnTransactionReadyToRead: failed to get proper HTTP response, status code: [%d]", (int)pHttpResponse->GetStatusCode());
	}
	if (__pHttpSession) {
		delete __pHttpSession;
		__pHttpSession = null;
	}

	__downloadCompleted = true;
	__timeout = false;
	__pPingTimer->Cancel();
}

result HttpDownloader::CloseActiveTransactions(void) {
	if (__pHttpSession) {
		return __pHttpSession->CloseAllTransactions();
	} else {
		return E_SUCCESS;
	}
}

result HttpDownloader::CloseSession(void) {
	if (__pHttpSession) {
		delete __pHttpSession;
		__pHttpSession = null;
		return E_SUCCESS;
	} else {
		return E_INVALID_STATE;
	}
}

void HttpDownloader::OnTimerExpired(Timer& timer) {
	CloseActiveTransactions();
	CloseSession();
	__timeout = true;
}
