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

#include "BasicForm.h"

BasicForm::BasicForm(void) {
	__pDelegates = new EventDelegate[30];
	__regLength = 30;
	for (int i = 0; i < 30; i++) {
		*(__pDelegates + i) = null;
	}
	__regEvents = 0;

	__pFormManager = FormManager::GetInstance();
	__formID = -1;
}

BasicForm::~BasicForm(void) {
	if (__pDelegates) {
		delete []__pDelegates;
	}
}

int BasicForm::RegisterEventHandler(EventDelegate hnd) {
	if (__pDelegates) {
		if (__regEvents == __regLength) {
			EventDelegate *pNew = new EventDelegate[__regLength + 10];
			for (int i = 0; i < __regLength; i++) {
				*(pNew + i) = *(__pDelegates + i);
			}
			for (int i = __regLength; i < __regLength + 10; i++) {
				*(pNew + i) = null;
			}
			__regLength += 10;

			delete []__pDelegates;
			__pDelegates = pNew;
		}
		*(__pDelegates + __regEvents) = hnd;
		return __regEvents++;
	} else {
		return -1;
	}
}

void BasicForm::OnActionPerformed(const Control &source, int actionId) {
	if (*(__pDelegates + actionId)) {
		(this->*(*(__pDelegates + actionId)))(source);
	}
}
