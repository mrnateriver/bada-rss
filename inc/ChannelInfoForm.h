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
#ifndef CHANNELINFOFORM_H_
#define CHANNELINFOFORM_H_

#include "BasicForm.h"
#include "Channel.h"
#include <FGraphics.h>

using namespace Osp::Graphics;

class ChannelInfoForm: public BasicForm, public ITouchEventListener, public IScrollPanelEventListener, public IAdjustmentEventListener {
public:
	ChannelInfoForm(void);
	virtual ~ChannelInfoForm(void) { }

private:
	void OnSoftkeySaveClicked(const Control &src);
	void OnSoftkeyBackClicked(const Control &src);
	void OnKeypadEnterClicked(const Control &src);
	void OnKeypadCancelClicked(const Control &src);

	int AddEntitledLabel(const String &str, const String &title, const Point &place, bool href = false, int width = 460);

	virtual result Initialize(void);
	virtual result Terminate(void);

	virtual result OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID);

	virtual void OnTouchPressed(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) { }
	virtual void OnTouchLongPressed(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) { }
	virtual void OnTouchReleased(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo);
	virtual void OnTouchMoved(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) { }
	virtual void OnTouchDoublePressed(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) { }
	virtual void OnTouchFocusIn(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) { }
	virtual void OnTouchFocusOut(const Control &source, const Point &currentPosition, const TouchEventInfo &touchInfo) { }

	virtual void OnOverlayControlCreated(const Control &source) { }
	virtual void OnOverlayControlOpened(const Control &source) { __prevURL = __pURLField->GetText(); }
	virtual void OnOverlayControlClosed(const Control &source) { SetFocus(); }
	virtual void OnOtherControlSelected(const Control &source) { }

	virtual void OnAdjustmentValueChanged(const Control &source, int adjustment);

	ScrollPanel *__pMainPanel;
	EditField *__pURLField;
	Slider *__pTTLSlider;

	String __prevURL;
	Channel *__pChannel;
};

#endif
