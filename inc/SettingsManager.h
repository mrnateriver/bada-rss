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
#ifndef SETTINGSMANAGER_H_
#define SETTINGSMANAGER_H_

#include <FApp.h>
#include <FBase.h>

using namespace Osp::App;
using namespace Osp::Base;

class SettingsManager {
public:
	static result Initialize(AppRegistry *reg);
	static SettingsManager *GetInstance(void) {
		return __pInstance;
	}
	static void RemoveInstance(void) {
		if (__pInstance) {
			delete __pInstance;
			__pInstance = null;
		}
	}

private:
	static SettingsManager *__pInstance;

public:
	SettingsManager(void) {
		__pReg = null;
	}
	~SettingsManager(void) { }

	result Construct(AppRegistry *reg);

	bool GetLoadImagesByDefault(void);
	void SetLoadImagesByDefault(bool val);

	bool GetUpdateAutomatically(void);
	void SetUpdateAutomatically(bool val);

	bool GetCalculateUpdateInterval(void);
	void SetCalculateUpdateInterval(bool val);

	int GetUpdateInterval(void);
	void SetUpdateInterval(int val);

	bool GetDownloadToDefaultEnclosureFolder(void);
	void SetDownloadToDefaultEnclosureFolder(bool val);

	String GetEnclosureDownloadFolder(void);
	void SetEnclosureDownloadFolder(const String &val);

	AppRegistry *__pReg;
};

#endif
