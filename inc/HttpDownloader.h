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
#ifndef HTTPDOWNLOADER_H_
#define HTTPDOWNLOADER_H_

#include <FBase.h>
#include <FNet.h>

using namespace Osp::Base;
using namespace Osp::Base::Runtime;
using namespace Osp::Net::Http;

class HttpDownloader: public IHttpTransactionEventListener, public ITimerEventListener {
public:
	HttpDownloader(void);
	virtual ~HttpDownloader(void);

	result Construct(void);

	result CloseActiveTransactions(void);
	result CloseSession(void);

	result Download(const String &url, ByteBuffer *&data, int pingTimeout = 40);
	result Download(const String &url, const IHttpTransactionEventListener &handler);

private:
	virtual void OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen);
	virtual void OnTransactionAborted(HttpSession &httpSession, HttpTransaction &httpTransaction, result r) { delete &httpTransaction; }
	virtual void OnTransactionReadyToWrite(HttpSession &httpSession, HttpTransaction &httpTransaction, int recommendedChunkSize) { }
	virtual void OnTransactionHeaderCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction, int headerLen, bool rs) { }
	virtual void OnTransactionCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction) { delete &httpTransaction; }
	virtual void OnTransactionCertVerificationRequiredN(HttpSession &httpSession, HttpTransaction &httpTransaction, String *pCert) { }

	virtual void OnTimerExpired(Timer &timer);

	HttpSession *__pHttpSession;
	Timer *__pPingTimer;

	volatile bool __downloadCompleted;
	ByteBuffer *__pDownloadedData;
	volatile bool __timeout;
};

#endif
