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
#include "Aggregator.h"
#include "FeedManager.h"
#include "SettingsManager.h"
#include "FeedItemForm.h"
#include "ChannelForm.h"
#include <typeinfo>

Aggregator::Aggregator() {
	__pMainForm = null;
	__isBackground = false;
	__notifiedOfLowCharge = false;
	__pUpdateTimer = null;
}

Aggregator::~Aggregator() {
	if (__pUpdateTimer) {
		__pUpdateTimer->Cancel();
		delete __pUpdateTimer;
	}
}

Application *Aggregator::CreateInstance(void) {
	return new Aggregator();
}

bool Aggregator::OnAppInitializing(AppRegistry& appRegistry) {
	FormManager::Initialize(GetAppFrame()->GetFrame());
	Utilities::Initialize();
	SettingsManager::Initialize(&appRegistry);

	result res = FeedManager::Initialize();
	if (IsFailed(res)) {
		AppLogDebug("Aggregator::OnAppInitializing: [%s]: failed to initialize feed manager", GetErrorMessage(res));
		return false;
	}
	FeedManager::GetInstance()->CacheChannels();
	FeedManager::GetInstance()->CacheFeedItems();

	String lang = Utilities::GetString(L"LANG_IDENT");
	if (!lang.Equals(L"en", false)) {
		AppRegistry *reg = GetAppRegistry();
		int added = 0;
		if (IsFailed(reg->Get(L"ADDED_BADABLOG_FOR_RUSSIAN_LANG", added))) {
			Channel *pChan = new Channel(L"badablog - everything bada in one blog", L"samsung bada, samsung wave, games, apps", L"http://badablog.ru/index.php", L"http://badablog.ru/feed");

			String favicon = L"/Home/favicons/" + pChan->GetUniqueString() + L".png";

			Directory::Create(L"/Home/favicons/", true);
			File::Copy(L"/Res/badablog.png", favicon, false);

			pChan->SetFavicon(Utilities::GetBitmapN(favicon, true));
			pChan->SetFaviconPath(favicon);

			FeedManager::GetInstance()->AddChannel(pChan);

			delete pChan;

			reg->Add(L"ADDED_BADABLOG_FOR_RUSSIAN_LANG", 1);
		}
	}

	__pUpdateTimer = new Timer;
	res = __pUpdateTimer->Construct(*this);
	if (IsFailed(res)) {
		AppLogDebug("Aggregator::OnAppInitializing: [%s]: failed to initialize update timer", GetErrorMessage(res));
		return false;
	}

	__pMainForm = new MainForm;
	res = FormManager::GetInstance()->SwitchToForm(__pMainForm, null);
	if (IsFailed(res)) {
		return false;
	} else return true;
}

bool Aggregator::OnAppTerminating(AppRegistry &appRegistry, bool forcedTermination) {
	FormManager::RemoveInstance();
	Utilities::RemoveInstance();
	SettingsManager::RemoveInstance();

	result res = FeedManager::GetInstance()->SerializeAll();
	FeedManager::RemoveInstance();
	if (IsFailed(res)) {
		return false;
	} else {
		return true;
	}
}

void Aggregator::HandleLowBattery(void) {
	if (!__isBackground && !__notifiedOfLowCharge) {
		__notifiedOfLowCharge = true;

		String mTitle = L"Warning";
		GetAppResource()->GetString(L"IDS_LOW_BATTERY_MSG_TITLE", mTitle);
		String mText = L"Low battery";
		GetAppResource()->GetString(L"IDS_LOW_BATTERY_MSG_TEXT", mText);

		int res = 0;
		MessageBox mBox;
		mBox.Construct(mTitle, mText, MSGBOX_STYLE_OK);
		mBox.ShowAndWait(res);
	}
}

void Aggregator::OnForeground(void) {
	__isBackground = false;

	BatteryLevel eBatterLevel = BATTERY_EMPTY;
	bool isCharging = false;

	Battery::GetCurrentLevel(eBatterLevel);
	RuntimeInfo::GetValue(L"IsCharging", isCharging);

	__pUpdateTimer->Cancel();
	if ((eBatterLevel != BATTERY_CRITICAL && eBatterLevel != BATTERY_EMPTY) || isCharging) {
		//cancel update timer
	} else if (eBatterLevel == BATTERY_EMPTY) {
		Terminate();
	} else {
		HandleLowBattery();
	}
}

void Aggregator::OnBackground(void) {
	__isBackground = true;

	if (SettingsManager::GetInstance()->GetUpdateAutomatically()) {
		if (SettingsManager::GetInstance()->GetCalculateUpdateInterval()) {
			__pUpdateTimer->Start(FeedManager::GetInstance()->GetUpdateInterval() * 60 * 1000);//in millisecs
		} else {
			__pUpdateTimer->Start(SettingsManager::GetInstance()->GetUpdateInterval() * 60 * 1000);//in millisecs
		}
	}
}

void Aggregator::OnLowMemory(void) {
}

void Aggregator::OnBatteryLevelChanged(BatteryLevel batteryLevel) {
   	bool isCharging = false;
   	RuntimeInfo::GetValue(L"IsCharging", isCharging);

	if (batteryLevel == BATTERY_CRITICAL && !isCharging) {
		HandleLowBattery();
	} else if (batteryLevel == BATTERY_EMPTY && !isCharging) {
		Terminate();
	}
}

void Aggregator::OnTimerExpired(Timer &timer) {
	BasicForm *pCurForm = FormManager::GetInstance()->GetActiveForm();
	if (typeid(*pCurForm) == typeid(FeedItemForm)) {
		FeedItemForm *itemForm = static_cast<FeedItemForm *>(pCurForm);
		if (!itemForm->IsBusy()) {
			FormManager::GetInstance()->SwitchToTop(null, REASON_DIALOG_CANCEL, false);
			FormManager::GetInstance()->DisposeForm(pCurForm);
		} else {
			__pUpdateTimer->Start(FeedManager::GetInstance()->GetUpdateInterval() * 60 * 1000);//in millisecs
			return;
		}
	} else if (typeid(*pCurForm) == typeid(ChannelForm)) {
		ChannelForm *chanForm = static_cast<ChannelForm *>(pCurForm);
		chanForm->CancelDownload();
		FormManager::GetInstance()->SwitchToTop(null, REASON_DIALOG_CANCEL, false);
	} else {
		FormManager::GetInstance()->SwitchToTop(null, REASON_DIALOG_CANCEL, false);
	}

	//update channels
	__pMainForm->OnOptionUpdateChannelsClicked(*__pMainForm);
	if (SettingsManager::GetInstance()->GetUpdateAutomatically()) {
		if (SettingsManager::GetInstance()->GetCalculateUpdateInterval()) {
			__pUpdateTimer->Start(FeedManager::GetInstance()->GetUpdateInterval() * 60 * 1000);//in millisecs
		} else {
			__pUpdateTimer->Start(SettingsManager::GetInstance()->GetUpdateInterval() * 60 * 1000);//in millisecs
		}
	}
}
