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
#ifndef _AGGREGATOR_H_
#define _AGGREGATOR_H_

#include "MainForm.h"
#include <FApp.h>
#include <FSystem.h>

using namespace Osp::App;
using namespace Osp::System;

class Aggregator: public Application, public ITimerEventListener {
public:
	static Application *CreateInstance(void);

public:
	Aggregator();
	~Aggregator();

private:
	void HandleLowBattery(void);

	virtual bool OnAppInitializing(AppRegistry &appRegistry);
	virtual bool OnAppTerminating(AppRegistry &appRegistry, bool forcedTermination = false);
	virtual void OnForeground(void);
	virtual void OnBackground(void);
	virtual void OnLowMemory(void);
	virtual void OnBatteryLevelChanged(BatteryLevel batteryLevel);

	virtual void OnTimerExpired(Timer &timer);

	MainForm *__pMainForm;
	bool __isBackground;
	bool __notifiedOfLowCharge;
	Timer *__pUpdateTimer;
};

#endif
