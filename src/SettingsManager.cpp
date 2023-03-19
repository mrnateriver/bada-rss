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
#include "SettingsManager.h"

extern SettingsManager *SettingsManager::__pInstance;

result SettingsManager::Initialize(AppRegistry *reg) {
	__pInstance = new SettingsManager;
	return __pInstance->Construct(reg);
}

result SettingsManager::Construct(AppRegistry *reg) {
	__pReg = reg;
	return E_SUCCESS;
}

bool SettingsManager::GetLoadImagesByDefault(void) {
	if (__pReg) {
		int val = 0;
		if (__pReg->Get(L"PREF_LOAD_IMAGES_BY_DEFAULT", val) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_LOAD_IMAGES_BY_DEFAULT", 0);
			return false;
		} else {
			return val != 0;
		}
	} else {
		return false;
	}
}

void SettingsManager::SetLoadImagesByDefault(bool val) {
	if (__pReg) {
		int tmp = 0;
		if (__pReg->Get(L"PREF_LOAD_IMAGES_BY_DEFAULT", tmp) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_LOAD_IMAGES_BY_DEFAULT", (int)val);
		} else {
			__pReg->Set(L"PREF_LOAD_IMAGES_BY_DEFAULT", (int)val);
		}
	}
}

bool SettingsManager::GetUpdateAutomatically(void) {
	if (__pReg) {
		int val = 0;
		if (__pReg->Get(L"PREF_UPDATE_FEEDS_IN_BACKGROUND", val) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_UPDATE_FEEDS_IN_BACKGROUND", 1);
			return true;
		} else {
			return val != 0;
		}
	} else {
		return false;
	}
}

void SettingsManager::SetUpdateAutomatically(bool val) {
	if (__pReg) {
		int tmp = 0;
		if (__pReg->Get(L"PREF_UPDATE_FEEDS_IN_BACKGROUND", tmp) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_UPDATE_FEEDS_IN_BACKGROUND", (int)val);
		} else {
			__pReg->Set(L"PREF_UPDATE_FEEDS_IN_BACKGROUND", (int)val);
		}
	}
}

bool SettingsManager::GetCalculateUpdateInterval(void) {
	if (__pReg) {
		int val = 0;
		if (__pReg->Get(L"PREF_CALCULATE_UPDATE_INTERVAL", val) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_CALCULATE_UPDATE_INTERVAL", 1);
			return true;
		} else {
			return val != 0;
		}
	} else {
		return false;
	}
}

void SettingsManager::SetCalculateUpdateInterval(bool val) {
	if (__pReg) {
		int tmp = 0;
		if (__pReg->Get(L"PREF_CALCULATE_UPDATE_INTERVAL", tmp) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_CALCULATE_UPDATE_INTERVAL", (int)val);
		} else {
			__pReg->Set(L"PREF_CALCULATE_UPDATE_INTERVAL", (int)val);
		}
	}
}

int SettingsManager::GetUpdateInterval(void) {
	if (__pReg) {
		int val = 15;
		if (__pReg->Get(L"PREF_UPDATE_INTERVAL", val) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_UPDATE_INTERVAL", 15);
		}
		return val;
	} else {
		return 15;
	}
}

void SettingsManager::SetUpdateInterval(int val) {
	if (__pReg) {
		int tmp = 0;
		if (__pReg->Get(L"PREF_UPDATE_INTERVAL", tmp) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_UPDATE_INTERVAL", val);
		} else {
			__pReg->Set(L"PREF_UPDATE_INTERVAL", val);
		}
	}
}

bool SettingsManager::GetDownloadToDefaultEnclosureFolder(void) {
	if (__pReg) {
		int val = 0;
		if (__pReg->Get(L"PREF_DEFAULT_ENCLOSURE_FOLDER", val) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_DEFAULT_ENCLOSURE_FOLDER", 1);
			return true;
		} else {
			return val != 0;
		}
	} else {
		return false;
	}
}

void SettingsManager::SetDownloadToDefaultEnclosureFolder(bool val) {
	if (__pReg) {
		int tmp = 0;
		if (__pReg->Get(L"PREF_DEFAULT_ENCLOSURE_FOLDER", tmp) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_DEFAULT_ENCLOSURE_FOLDER", (int)val);
		} else {
			__pReg->Set(L"PREF_DEFAULT_ENCLOSURE_FOLDER", (int)val);
		}
	}
}

String SettingsManager::GetEnclosureDownloadFolder(void) {
	if (__pReg) {
		String val = L"/Media/";
		if (__pReg->Get(L"PREF_ENCLOSURE_FOLDER", val) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_ENCLOSURE_FOLDER", L"/Media/");
		}
		return val;
	} else {
		return L"/Media/";
	}
}

void SettingsManager::SetEnclosureDownloadFolder(const String &val) {
	if (__pReg) {
		String tmp;
		if (__pReg->Get(L"PREF_ENCLOSURE_FOLDER", tmp) == E_KEY_NOT_FOUND) {
			__pReg->Add(L"PREF_ENCLOSURE_FOLDER", val);
		} else {
			__pReg->Set(L"PREF_ENCLOSURE_FOLDER", val);
		}
	}
}
