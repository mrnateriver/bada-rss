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
#ifndef _SAVEFORM_H_
#define _SAVEFORM_H_

#include "BasicForm.h"
#include "Utilities.h"

enum SaveFormMode {
	MODE_OPEN_FILE,
	MODE_SAVE_FILE,
	MODE_CHOOSE_DIRECTORY
};

class SaveForm: public BasicForm, public ICustomItemEventListener, public ITextEventListener
{
public:
	SaveForm(void);
	virtual ~SaveForm(void);

private:
	struct FileBitmaps {
		Bitmap *pOther;
		Bitmap *pAudio;
		Bitmap *pVideo;
		Bitmap *pImage;
		Bitmap *pRss;
		Bitmap *pXml;
	};

	void OnLeftSoftkeyClicked(const Control &src);
	void OnRightSoftkeyClicked(const Control &src);

	bool CheckControls(void) const;
	int GetDirectoryElementsCount(const String &dir) const;
	result FillRootDirList(void);
	result FillDirList(const String &dir);
	result NavigateToPath(const String &path);

	virtual result Initialize(void);
	virtual result Terminate(void);

	virtual result OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID);

	virtual void OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status);
	virtual void OnItemStateChanged(const Control &source, int index, int itemId, int elementId, ItemStatus status) {}

	virtual void OnTextValueChanged(const Control &source);
	virtual void OnTextValueChangeCanceled(const Control &source);

	static const int LIST_ELEMENT_BITMAP = 500;
	static const int LIST_ELEMENT_NAME = 501;
	static const int LIST_ELEMENT_ELEMENTS = 502;

	EditField *__pFilenameField;
	Label *__pDirListCaption;
	CustomList *__pDirList;

	CustomListItemFormat *__pDirListItemFormat;
	const String *__pCurrentDir;
	LinkedList *__pCurrentDirList;
	LinkedList *__pCurrentFileList;
	Stack *__pNavigationHistory;

	String __startingPath;
	SaveFormMode __formMode;
	FileBitmaps __bmps;
};

#endif
