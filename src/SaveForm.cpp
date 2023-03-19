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
#include "SaveForm.h"
#include <FIo.h>
#include <FContent.h>
#include <typeinfo>

using namespace Osp::Io;
using namespace Osp::Base::Utility;
using namespace Osp::Content;

SaveForm::SaveForm(void) {
	__pFilenameField = null;
	__pDirListCaption = null;
	__pDirList = null;

	__pDirListItemFormat = null;
	__pCurrentDir = null;
	__pCurrentDirList = null;
	__pCurrentFileList = null;
	__pNavigationHistory = null;

	__startingPath = L"";
	__formMode = MODE_SAVE_FILE;
}

SaveForm::~SaveForm(void) {
	if (__pDirListItemFormat) {
		delete __pDirListItemFormat;
	}
	if (__pCurrentDir) {
		delete __pCurrentDir;
	}
	if (__pCurrentDirList) {
		__pCurrentDirList->RemoveAll(true);
		delete __pCurrentDirList;
	}
	if (__pCurrentFileList) {
		__pCurrentFileList->RemoveAll(true);
		delete __pCurrentFileList;
	}
	if (__pNavigationHistory) {
		__pNavigationHistory->RemoveAll(true);
		delete __pNavigationHistory;
	}
	if (__bmps.pAudio) {
		delete __bmps.pAudio;
	}
	if (__bmps.pImage) {
		delete __bmps.pImage;
	}
	if (__bmps.pOther) {
		delete __bmps.pOther;
	}
	if (__bmps.pRss) {
		delete __bmps.pRss;
	}
	if (__bmps.pXml) {
		delete __bmps.pXml;
	}
	if (__bmps.pVideo) {
		delete __bmps.pVideo;
	}
}

int SaveForm::GetDirectoryElementsCount(const String &dir) const {
	Directory *pDir = new Directory;
	result res = pDir->Construct(dir);
	if (IsFailed(res)) {
		AppLogDebug("SaveForm::FillDirList: [%s]: failed to construct directory object for path [%S]", GetErrorMessage(res), dir.GetPointer());
		delete pDir;
		SetLastResult(res);
		return -1;
	}

	DirEnumerator *pDirEnum = pDir->ReadN();
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogDebug("SaveForm::GetDirectoryElementsCount: [%s]: failed to construct directory enumerator for path [%S]", GetErrorMessage(res), dir.GetPointer());
		delete pDir;
		SetLastResult(res);
		return -1;
	}

	int count = 0;
	while (!IsFailed(pDirEnum->MoveNext())) {
		DirEntry entry = pDirEnum->GetCurrentDirEntry();

		String entry_name = entry.GetName();
		if (entry_name.Equals(String(L"."))
		|| entry_name.Equals(String(L".."))
		|| entry_name.Equals(String(L"8m59kn4kej"))
		|| entry_name.Equals(String(L"DownloadedAppPackages"))
		|| entry_name.Equals(String(L"__@@bada_applications@@__"))) {
			continue;
		}
		count++;
	}

	delete pDir;
	delete pDirEnum;

	return count;
}

result SaveForm::FillRootDirList(void) {
	__pCurrentDirList->RemoveAll(true);
	__pCurrentFileList->RemoveAll(true);
	__pDirList->RemoveAllItems();

	Bitmap *pFolderMC = Utilities::GetBitmapN(L"folder_icon_mc.png");
	Bitmap *pFolderIntM = Utilities::GetBitmapN(L"folder_icon_intm.png");

	String *paths[2] = {
		new String(L"/Media/"),
		new String(L"/Storagecard/Media/")
	};

	String path_names[2] = {
		Utilities::GetString(L"IDS_SAVEFORM_DIR_ENTRY_INTMEM"),
		Utilities::GetString(L"IDS_SAVEFORM_DIR_ENTRY_STORAGECARD")
	};

	result res = E_SUCCESS;
	for(int i = 0; i < 2; i++) {
		CustomListItem *pItem = new CustomListItem;
		res = pItem->Construct(90);
		if (IsFailed(res)) {
			AppLogDebug("SaveForm::FillRootDirList: [%s]: failed to construct directory list item", GetErrorMessage(res));
			delete pItem;
			break;
		}

		pItem->SetItemFormat(*__pDirListItemFormat);
		if (i == 1) {
			pItem->SetElement(LIST_ELEMENT_BITMAP, *pFolderMC, pFolderMC);
		} else if (i == 0) {
			pItem->SetElement(LIST_ELEMENT_BITMAP, *pFolderIntM, pFolderIntM);
		}
		pItem->SetElement(LIST_ELEMENT_NAME, path_names[i]);

		int count = GetDirectoryElementsCount(*paths[i]);
		if (count < 0) {
			/*AppLogDebug("SaveForm::FillRootDirList: [%s]: failed to enumerate directory elements for path [%S]", GetErrorMessage(res), paths[i]->GetPointer());
			res = GetLastResult();

			delete pItem;
			break;*/
			continue;
		} else if (count > 0) {
			String elements = Utilities::GetString(L"IDS_SAVEFORM_DIR_ENTRY_INFO"); elements.Append(count);
			pItem->SetElement(LIST_ELEMENT_ELEMENTS, elements);
		} else {
			pItem->SetElement(LIST_ELEMENT_ELEMENTS, Utilities::GetString(L"IDS_SAVEFORM_DIR_ENTRY_INFO_EMPTY"));
		}

		__pDirList->AddItem(*pItem, i);
		__pCurrentDirList->Add(*paths[i]);
	}

	delete pFolderMC;
	delete pFolderIntM;

	SetSoftkeyEnabled(SOFTKEY_0, false);
	RequestRedraw(true);
	return res;
}

result SaveForm::FillDirList(const String &dir) {
	if (dir.Equals(String(L"/"))) {
		return FillRootDirList();
	}

	__pCurrentDirList->RemoveAll(true);
	__pCurrentFileList->RemoveAll(true);
	__pDirList->RemoveAllItems();

	Bitmap *pFolder = Utilities::GetBitmapN(L"folder_icon.png");
	Bitmap *pBack = Utilities::GetBitmapN(L"dir_up_icon.png");

	Directory *pDir = new Directory;
	result res = pDir->Construct(dir);
	if (IsFailed(res)) {
		AppLogDebug("SaveForm::FillDirList: [%s]: failed to construct directory object for path [%S]", GetErrorMessage(res), dir.GetPointer());

		delete pFolder;
		delete pBack;
		delete pDir;

		return res;
	}

	DirEnumerator *pEnum = pDir->ReadN();
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogDebug("SaveForm::FillDirList: [%s]: failed to construct directory enumerator for path [%S]", GetErrorMessage(res), dir.GetPointer());

		delete pFolder;
		delete pBack;
		delete pDir;

		return res;
	}

	CustomListItem *pItem = new CustomListItem;
	res = pItem->Construct(90);
	if (IsFailed(res)) {
		AppLogDebug("SaveForm::FillDirList: [%s]: failed to construct back item", GetErrorMessage(res));

		delete pItem;
		delete pFolder;
		delete pBack;
		delete pDir;

		return res;
	}
	pItem->SetItemFormat(*__pDirListItemFormat);
	pItem->SetElement(LIST_ELEMENT_NAME, Utilities::GetString(L"IDS_SAVEFORM_NAVIGATE_BACK"));
	pItem->SetElement(LIST_ELEMENT_BITMAP, *pBack, pBack);
	pItem->SetElement(LIST_ELEMENT_ELEMENTS, *(static_cast<const String*>(__pNavigationHistory->Peek())));
	__pDirList->AddItem(*pItem, -1);

	LocaleManager locMgr;
	res = locMgr.Construct();
	if (IsFailed(res)) {
		AppLogDebug("SaveForm::FillDirList: [%s]: failed to construct locale manager", GetErrorMessage(res));

		delete pItem;
		delete pFolder;
		delete pBack;
		delete pDir;

		return res;
	}

	TimeZone tz = locMgr.GetSystemTimeZone();

	int i = 0;
	int j = -2;
	while (!IsFailed(pEnum->MoveNext())) {
		DirEntry entry = pEnum->GetCurrentDirEntry();

		String entry_name = entry.GetName();
		if (entry_name.Equals(String(L"."))
		|| entry_name.Equals(String(L".."))
		|| entry_name.Equals(String(L"8m59kn4kej"))
		|| entry_name.Equals(String(L"DownloadedAppPackages"))
		|| entry_name.Equals(String(L"__@@bada_applications@@__"))) {
			continue;
		}

		pItem = new CustomListItem;
		res = pItem->Construct(90);
		if (IsFailed(res)) {
			AppLogDebug("SaveForm::FillDirList: [%s]: failed to construct directory list item", GetErrorMessage(res));
			delete pItem;
			break;
		}
		pItem->SetItemFormat(*__pDirListItemFormat);

		String path = dir;
		path.Append(entry_name);

		if (entry.IsDirectory()) {
			String str;
			entry_name.ToUpper(str);
			str = Utilities::GetString(String(L"IDS_DIRNAME_") + str);
			if (IsFailed(GetLastResult())) {
				pItem->SetElement(LIST_ELEMENT_NAME, entry_name);
			} else {
				pItem->SetElement(LIST_ELEMENT_NAME, str);
			}
			pItem->SetElement(LIST_ELEMENT_BITMAP, *pFolder, pFolder);

			path.Append('/');
			int count = GetDirectoryElementsCount(path);
			if (count < 0) {
				AppLogDebug("SaveForm::FillDirList: [%s]: failed to enumerate directory elements for path [%S]", GetErrorMessage(res), path.GetPointer());
				res = GetLastResult();

				delete pItem;
				break;
			} else if (count > 0) {
				String elements = Utilities::GetString(L"IDS_SAVEFORM_DIR_ENTRY_INFO"); elements.Append(count);
				pItem->SetElement(LIST_ELEMENT_ELEMENTS, elements);
			} else {
				pItem->SetElement(LIST_ELEMENT_ELEMENTS, Utilities::GetString(L"IDS_SAVEFORM_DIR_ENTRY_INFO_EMPTY"));
			}

			__pCurrentDirList->Add(*(new String(path)));
			__pDirList->AddItem(*pItem, i++);
		} else if (__formMode != MODE_CHOOSE_DIRECTORY) {
			Bitmap *thNail = null;
			if (entry_name.EndsWith(L".opml")) {
				thNail = __bmps.pRss;
			} else if (entry_name.EndsWith(L".xml")) {
				thNail = __bmps.pXml;
			} else {
				ContentType type;
				if (Utilities::IsFirmwareLaterThan10()) {
					type = ContentManagerUtil::CheckContentType(path);
				} else {
					type = ContentManagerUtil::GetContentType(path);
				}
				if (type == CONTENT_TYPE_AUDIO) {
					thNail = __bmps.pAudio;
				} else if (type == CONTENT_TYPE_IMAGE) {
					thNail = __bmps.pImage;
				} else if (type == CONTENT_TYPE_VIDEO) {
					thNail = __bmps.pVideo;
				} else {
					thNail = __bmps.pOther;
				}
			}

			pItem->SetElement(LIST_ELEMENT_NAME, entry_name);
			if (thNail) {
				pItem->SetElement(LIST_ELEMENT_BITMAP, *thNail, thNail);
			}

			DateTime date = tz.UtcTimeToStandardTime(entry.GetDateTime());
			if (tz.IsDstUsed()) {
				date.AddHours(1);
			}

			String info;
			info.Format(60, Utilities::GetString(L"IDS_SAVEFORM_FILE_ENTRY_INFO_FORMAT").GetPointer(), (unsigned long)Math::Round((double)entry.GetFileSize() / 1024), Utilities::FormatDateTime(date).GetPointer());

			pItem->SetElement(LIST_ELEMENT_ELEMENTS, info);

			__pCurrentFileList->Add(*(new String(entry_name)));
			__pDirList->AddItem(*pItem, j--);
		}
	}

	delete pFolder;
	delete pBack;
	delete pEnum;
	delete pDir;

	SetSoftkeyEnabled(SOFTKEY_0, true);
	RequestRedraw(true);
	return res;
}

result SaveForm::NavigateToPath(const String &path) {
	if (path.IsEmpty()) {
		return E_INVALID_ARG;
	}

	delete __pCurrentDir;
	__pNavigationHistory->RemoveAll(true);

	StringTokenizer strTok(path, L"/");

	String token;
	String *dir = new String(L"/");
	if (path.EndsWith(L"/")) {
		while (strTok.HasMoreTokens()) {
			__pNavigationHistory->Push(*(new String(*dir)));

			strTok.GetNextToken(token);
			token.Append('/');

			dir->Append(token);
		}
	} else {
		int count = strTok.GetTokenCount();
		for(int i = 0; i < count - 1; i++) {
			__pNavigationHistory->Push(*(new String(*dir)));

			strTok.GetNextToken(token);
			token.Append('/');

			dir->Append(token);
		}
		strTok.GetNextToken(token);
		__pFilenameField->SetText(token);
	}

	__pCurrentDir = dir;
	return FillDirList(*__pCurrentDir);
}

void SaveForm::OnLeftSoftkeyClicked(const Control &src) {
	String *pFile = new String(*__pCurrentDir);
	if (__formMode != MODE_CHOOSE_DIRECTORY) {
		if (__pFilenameField->GetTextLength() == 0) {
			Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_SAVEFORM_MBOX_FILE_MSG"), MSGBOX_STYLE_OK);
			delete pFile;
			return;
		}
		if ((__formMode == MODE_SAVE_FILE) && Utilities::IsFirmwareLaterThan10() && !__pFilenameField->GetText().EndsWith(L".txt")) {
			Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE"), Utilities::GetString(L"IDS_SAVEFORM_FIRMWARE_ERROR_MBOX_TEXT"), MSGBOX_STYLE_OK);
			delete pFile;
			return;
		}

		result res = E_SUCCESS;

		pFile->Append(__pFilenameField->GetText());

		if (__formMode == MODE_OPEN_FILE) {
			if (!File::IsFileExist(*pFile)) {
				Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_SAVEFORM_MBOX_TEXT_OPENMODE"), MSGBOX_STYLE_OK);
				delete pFile;
				return;
			}
		} else {
			if (File::IsFileExist(*pFile)) {
				int mres = Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE"), Utilities::GetString(L"IDS_SAVEFORM_MBOX_TEXT"), MSGBOX_STYLE_YESNO);

				res = GetLastResult();
				if (IsFailed(res)) {
					AppLogDebug("SaveForm::OnLeftSoftkeyClicked: [%s]: failed to construct message box, selected file will be overwritten", GetErrorMessage(res));
				} else if (mres != MSGBOX_RESULT_YES) {
					delete pFile;
					return;
				}
			}
		}
	} else if (pFile->Equals(L"/", false)) {
		Utilities::ShowMessageBox(Utilities::GetString(L"IDS_SAVEFORM_MBOX_TITLE_OPENMODE"), Utilities::GetString(L"IDS_SAVEFORM_MBOX_TEXT_DIRMODE"), MSGBOX_STYLE_OK);
		delete pFile;
		return;
	}

	ArrayList *pArgs = new ArrayList;
	pArgs->Construct(1);
	pArgs->Add(*pFile);

	GetFormManager()->SwitchBack(pArgs, REASON_DIALOG_SUCCESS);

	pArgs->RemoveAll(true);
	delete pArgs;
}

void SaveForm::OnRightSoftkeyClicked(const Control &src) {
	GetFormManager()->SwitchBack(null, REASON_DIALOG_CANCEL);
}

result SaveForm::Initialize(void) {
	result res = Construct(FORM_STYLE_INDICATOR | FORM_STYLE_TITLE | FORM_STYLE_SOFTKEY_0 | FORM_STYLE_SOFTKEY_1);
	if (IsFailed(res)) {
		AppLogException("SaveForm::Initialize: [%s]: failed to construct parent form", GetErrorMessage(res));
		return res;
	}

	SetTitleText(Utilities::GetString(L"IDS_SAVEFORM_TITLE"), ALIGNMENT_CENTER);
	SetBackgroundColor(Color(0x0e0e0e, false));
	SetSoftkeyText(SOFTKEY_0, Utilities::GetString(L"IDS_SAVEFORM_ACCEPT_BUTTON"));
	SetSoftkeyText(SOFTKEY_1, Utilities::GetString(L"IDS_SAVEFORM_CANCEL_BUTTON"));
	SetSoftkeyActionId(SOFTKEY_0, HANDLER(SaveForm::OnLeftSoftkeyClicked));
	SetSoftkeyActionId(SOFTKEY_1, HANDLER(SaveForm::OnRightSoftkeyClicked));
	AddSoftkeyActionListener(SOFTKEY_0, *this);
	AddSoftkeyActionListener(SOFTKEY_1, *this);

	__pFilenameField = new EditField;
	res = __pFilenameField->Construct(Rectangle(9, 20, 461, 111), EDIT_FIELD_STYLE_NORMAL, INPUT_STYLE_FULLSCREEN, true, 1000, GROUP_STYLE_NONE);
	if (IsFailed(res)) {
		AppLogException("SaveForm::Initialize: [%s]: failed to construct filename field", GetErrorMessage(res));
		return res;
	}
	__pFilenameField->SetTitleText(Utilities::GetString(L"IDS_SAVEFORM_FILENAME_FIELD_TITLE"));
	__pFilenameField->AddTextEventListener(*this);
	AddControl(*__pFilenameField);

	__pDirListCaption = new Label;
	res = __pDirListCaption->Construct(Rectangle(25,139,445,40), Utilities::GetString(L"IDS_SAVEFORM_DIRLIST_TITLE"));
	if (IsFailed(res)) {
		AppLogException("SaveForm::Initialize: [%s]: failed to construct directory list caption", GetErrorMessage(res));
		return res;
	}
	__pDirListCaption->SetTextColor(Color(0x77BADD, false));
	__pDirListCaption->SetTextConfig(29, LABEL_TEXT_STYLE_NORMAL);
	__pDirListCaption->SetTextVerticalAlignment(ALIGNMENT_TOP);
	__pDirListCaption->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	AddControl(*__pDirListCaption);

	__pDirList = new CustomList;
	res = __pDirList->Construct(Rectangle(0,179,480,460), CUSTOM_LIST_STYLE_NORMAL, true);
	if (IsFailed(res)) {
		AppLogException("SaveForm::Initialize: [%s]: failed to construct directory list", GetErrorMessage(res));
		return res;
	}
	__pDirList->SetTextOfEmptyList(L" ");
	__pDirList->AddCustomItemEventListener(*this);
	AddControl(*__pDirList);

	__pDirListItemFormat = new CustomListItemFormat;
	__pDirListItemFormat->Construct();
	__pDirListItemFormat->AddElement(LIST_ELEMENT_BITMAP, Rectangle(0,0,90,90));
	__pDirListItemFormat->AddElement(LIST_ELEMENT_NAME, Rectangle(95,10,380,37));
	__pDirListItemFormat->AddElement(LIST_ELEMENT_ELEMENTS, Rectangle(95,57,373,37), 22, Color::COLOR_GREY, Color::COLOR_GREY);

	__pCurrentDir = new String(L"/");
	__pCurrentDirList = new LinkedList;
	__pCurrentFileList = new LinkedList;
	__pNavigationHistory = new Stack;

	__bmps.pOther = Utilities::GetBitmapN(L"other_file_icon.png");
	__bmps.pAudio = Utilities::GetBitmapN(L"audio_file_icon.png");
	__bmps.pVideo = Utilities::GetBitmapN(L"video_file_icon.png");
	__bmps.pImage = Utilities::GetBitmapN(L"image_file_icon.png");
	__bmps.pRss = Utilities::GetBitmapN(L"rss_file_icon.png");
	__bmps.pXml = Utilities::GetBitmapN(L"xml_file_icon.png");

	return E_SUCCESS;
}

result SaveForm::Terminate(void) {
	return E_SUCCESS;
}

result SaveForm::OnGivenData(IList *pArgs, DataReceiveReason reason, int srcID) {
	if (pArgs || pArgs->GetCount() >= 2) {
		if (typeid(*(pArgs->GetAt(0))) == typeid(String)) {
			String *str = static_cast<String *>(pArgs->GetAt(0));
			if (str) {
				__startingPath = *str;
			}
		}
		if (typeid(*(pArgs->GetAt(1))) == typeid(Integer)) {
			Integer *val = static_cast<Integer *>(pArgs->GetAt(1));
			if (val) {
				__formMode = (SaveFormMode)val->ToInt();
			}
		}
	}

	if (__formMode == MODE_OPEN_FILE) {
		SetTitleText(Utilities::GetString(L"IDS_SAVEFORM_TITLE_OPENMODE"), ALIGNMENT_CENTER);
		SetSoftkeyText(SOFTKEY_0, Utilities::GetString(L"IDS_SAVEFORM_ACCEPT_BUTTON_OPENMODE"));
	} else if (__formMode == MODE_CHOOSE_DIRECTORY) {
		SetTitleText(Utilities::GetString(L"IDS_SAVEFORM_TITLE_DIRMODE"), ALIGNMENT_CENTER);
		SetSoftkeyText(SOFTKEY_0, Utilities::GetString(L"IDS_SAVEFORM_ACCEPT_BUTTON_DIRMODE"));

		__pFilenameField->SetEnabled(false);
		__pFilenameField->SetShowState(false);

		__pDirListCaption->SetShowState(false);

		__pDirList->SetPosition(0, 0);
		__pDirList->SetSize(480,639);
	}

	if (!__startingPath.IsEmpty()) {
		return NavigateToPath(__startingPath);
	} else return FillRootDirList();
}

void SaveForm::OnItemStateChanged(const Control &source, int index, int itemId, ItemStatus status) {
	const String *path = null;
	if (itemId < -1) {
		int real_id = -(itemId + 2);
		__pFilenameField->SetText(String(*(static_cast<String*>(__pCurrentFileList->GetAt(real_id)))));

		RequestRedraw(true);
		return;
	} else if (itemId == -1) {
		delete __pCurrentDir;
		path = static_cast<const String*>(__pNavigationHistory->Pop());
	} else {
		__pNavigationHistory->Push(*__pCurrentDir);
		path = new String(*(static_cast<String*>(__pCurrentDirList->GetAt(itemId))));
	}

	result res = FillDirList(*path);
	__pCurrentDir = path;

	if (IsFailed(res)) {
		AppLogDebug("SaveForm::OnItemStateChanged: [%s]: failed to fill directory list", GetErrorMessage(res));
	}
}

void SaveForm::OnTextValueChanged(const Control &source) {
	if (__pFilenameField) {
		if (Utilities::IsFirmwareLaterThan10()) {
			if (!__pFilenameField->GetText().EndsWith(L".txt")) {
				__pFilenameField->AppendText(L".txt");
			}
		} else {
			if (!__pFilenameField->GetText().EndsWith(L".opml")) {
				__pFilenameField->AppendText(L".opml");
			}
		}
		__pDirList->SetFocus();
	}
}

void SaveForm::OnTextValueChangeCanceled(const Control &source) {
	__pDirList->SetFocus();
}
