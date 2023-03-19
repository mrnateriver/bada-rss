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
#include "Channel.h"
#include <FSystem.h>
#include <FMedia.h>
#include <FApp.h>
#include <FIo.h>

using namespace Osp::Base::Collection;
using namespace Osp::System;
using namespace Osp::Media;
using namespace Osp::App;
using namespace Osp::Io;

extern __UtilitiesEx Utilities::__data;

void Utilities::Initialize(void) {
	__data.__epoch = GetLocalDateTimeFromEpoch(0);

	__data.__pDTFormatter = DateTimeFormatter::CreateDateTimeFormatterN();
	result res = GetLastResult();
	if (IsFailed(res)) {
		AppLogDebug("Utilities::Initialize: [%s]: failed to create date time formatter", GetErrorMessage(res));
	}

	SDKVersionInfo ver;
    String platformVersion;
    res = SystemInfo::GetValue(L"APIVersion", platformVersion);
    if (!IsFailed(res)) {
    	do {
			int dotIndex = -1;
			res = platformVersion.IndexOf(L".", 0, dotIndex);
			if (IsFailed(res)) {
				AppLogDebug("Utilities::Initialize: [%s]: failed to get API version", GetErrorMessage(res));
		    	ver.major = 1;
		    	ver.minor = 0;
		    	break;
			}

			String majorStr(L"1");
			res = platformVersion.SubString(0, dotIndex, majorStr);
			if (IsFailed(res)) {
				AppLogDebug("Utilities::Initialize: [%s]: failed to get API version", GetErrorMessage(res));
		    	ver.major = 1;
		    	ver.minor = 0;
		    	break;
			}

			int majorVersion = 1;
			res = Integer::Parse(majorStr, majorVersion);
			if (IsFailed(res)) {
				AppLogDebug("Utilities::Initialize: [%s]: failed to get API version", GetErrorMessage(res));
		    	ver.major = 1;
		    	ver.minor = 0;
		    	break;
			}

			String minorStr(L"0");
			res = platformVersion.SubString(dotIndex + 1, minorStr);
			if (IsFailed(res)) {
				AppLogDebug("Utilities::Initialize: [%s]: failed to get API version", GetErrorMessage(res));
		    	ver.major = 1;
		    	ver.minor = 0;
		    	break;
			}

			int minorVersion = 1;
			res = Integer::Parse(minorStr, minorVersion);
			if (IsFailed(res)) {
				AppLogDebug("Utilities::Initialize: [%s]: failed to get API version", GetErrorMessage(res));
		    	ver.major = 1;
		    	ver.minor = 0;
		    	break;
			}

			ver.major = majorVersion;
			ver.minor = minorVersion;
    	} while (false);
    } else {
    	AppLogDebug("Utilities::Initialize: [%s]: failed to get API version", GetErrorMessage(res));

    	ver.major = 1;
    	ver.minor = 0;
    }
    __data.__sdkVer = ver;

	__data.__pNoFavicon = Utilities::GetBitmapN(L"no_favicon.png");
    AppLogDebug("Utilities::Initialize: API version: [%d.%d]", ver.major, ver.minor);
}

void Utilities::RemoveInstance(void) {
	if (__data.__pDTFormatter) {
		delete __data.__pDTFormatter;
		__data.__pDTFormatter = null;
	}
	if (__data.__pNoFavicon) {
		delete __data.__pNoFavicon;
		__data.__pNoFavicon = null;
	}
}

AppVersionInfo Utilities::GetAppVersion(void) {
	String ver = Application::GetInstance()->GetAppVersion();
	AppVersionInfo info = { 1, 0, 0 };

	int firstDotIndex = 0;
	result res = ver.IndexOf(L'.', 0, firstDotIndex);
	if (IsFailed(res)) {
		AppLogDebug("Utilities::GetAppVersion: failed to parse app version string");
		return info;
	}

	int secondDotIndex = 0;
	res = ver.LastIndexOf(L'.', ver.GetLength() - 1, secondDotIndex);
	if (IsFailed(res) || secondDotIndex == firstDotIndex) {
		AppLogDebug("Utilities::GetAppVersion: failed to parse app version string");
		return info;
	}

	String major;
	ver.SubString(0, firstDotIndex, major);
	String minor;
	ver.SubString(firstDotIndex + 1, secondDotIndex - firstDotIndex - 1, minor);
	String revision;
	ver.SubString(secondDotIndex + 1, ver.GetLength() - 1 - secondDotIndex, revision);

	int maj, min, rev;
	res = Integer::Parse(major, maj);
	if (IsFailed(res)) {
		AppLogDebug("Utilities::GetAppVersion: failed to parse app version string");
		return info;
	}
	res = Integer::Parse(minor, min);
	if (IsFailed(res)) {
		AppLogDebug("Utilities::GetAppVersion: failed to parse app version string");
		return info;
	}
	res = Integer::Parse(revision, rev);
	if (IsFailed(res)) {
		AppLogDebug("Utilities::GetAppVersion: failed to parse app version string");
		return info;
	}

	info.major = maj;
	info.minor = min;
	info.revision = rev;

	return info;
}

String Utilities::GetAppVersionString(void) {
	return Application::GetInstance()->GetAppVersion();
}

String Utilities::GetScreenResolutionString(void) {
	int screenHeight = 800;
	SystemInfo::GetValue(L"ScreenHeight", screenHeight);
	int screenWidth = 480;
	SystemInfo::GetValue(L"ScreenWidth", screenWidth);

	String res = Integer::ToString(screenWidth);
	res.Append('x');
	res.Append(screenHeight);

	return res;
}

bool Utilities::IsFirmwareLaterThan10(void) {
	return __data.__sdkVer.major > 1 || __data.__sdkVer.minor > 0;
}

bool Utilities::IsVersionLater(AppVersionInfo version, AppVersionInfo than) {
	return version.major > than.major || version.minor > than.minor || version.revision > than.revision;
}

DateTime Utilities::GetCurrentUTCDateTime(void) {
	DateTime dt;
	SystemTime::GetCurrentTime(dt);
	return dt;
}

DateTime Utilities::GetCurrentLocalDateTime(void) {
	return AdjustUTCDateTimeToLocal(GetCurrentUTCDateTime());
}

String Utilities::FormatUTCDateTimeRFC822(const DateTime &dt) {
	String datetime;
	result res = E_SUCCESS;

	Locale loc = Locale(LANGUAGE_ENG, COUNTRY_US);

	DateTimeFormatter *pFormatter = DateTimeFormatter::CreateDateTimeFormatterN(loc);
	pFormatter->ApplyPattern(L"EEE, dd MMM yyyy HH:mm:ss 'GMT'");

	if (pFormatter) {
		res = pFormatter->Format(dt, datetime);
	} else {
		datetime = dt.ToString();
	}

	if (IsFailed(res)) {
		AppLogDebug("Utilities::FormatUTCDateTimeRFC822: [%s]: failed to format datetime", GetErrorMessage(res));
		SetLastResult(res);

		delete pFormatter;
		return dt.ToString();
	}

	delete pFormatter;
	return datetime;
}

String Utilities::FormatDateTime(const DateTime &dt) {
	String datetime;
	result res = E_SUCCESS;

	if (__data.__pDTFormatter) {
		res = __data.__pDTFormatter->Format(dt, datetime);
	} else {
		datetime = dt.ToString();
	}

	if (IsFailed(res)) {
		AppLogDebug("Utilities::GetLocaleSpecificDatetime: [%s]: failed to format datetime", GetErrorMessage(res));

		SetLastResult(res);
		return dt.ToString();
	}
	return datetime;
}

String Utilities::FormatDate(const DateTime &dt) {
	String datetime;
	result res = E_SUCCESS;

	DateTimeFormatter *pFormatter = DateTimeFormatter::CreateDateFormatterN(DATE_TIME_STYLE_FULL);
	//pFormatter->ApplyPattern(L"EEE, dd MMM yyyy HH:mm:ss 'GMT'");

	if (pFormatter) {
		res = pFormatter->Format(dt, datetime);
	} else {
		datetime = dt.ToString();
	}

	if (IsFailed(res)) {
		AppLogDebug("Utilities::FormatDate: [%s]: failed to format datetime", GetErrorMessage(res));
		SetLastResult(res);

		delete pFormatter;
		return dt.ToString();
	}

	delete pFormatter;
	return datetime;
}

String Utilities::FormatDateTimeFromEpoch(long long ticks) {
	DateTime dt = __data.__epoch;
	dt.AddSeconds(ticks / 1000);

	return FormatDateTime(dt);
}

String Utilities::FormatCurrentLocalDateTime(void) {
	DateTime curTime;
	SystemTime::GetCurrentTime(curTime);

	curTime = AdjustUTCDateTimeToLocal(curTime);
	return FormatDateTime(curTime);
}

result Utilities::EncodeBitmap(Bitmap *bmp, const String &path) {
	if (!bmp) {
		return E_INVALID_ARG;
	}

	Image *pImage = new Image();
	result res = pImage->Construct();
	if (IsFailed(res)) {
		AppLogDebug("Utilities::EncodeBitmap: [%s]: failed to construct image encoder", GetErrorMessage(res));
		delete pImage;
		return res;
	}

	res = pImage->EncodeToFile(*bmp, IMG_FORMAT_PNG, path, true);
	if (IsFailed(res)) {
		AppLogDebug("Utilities::EncodeBitmap: [%s]: failed to encode image", GetErrorMessage(res));
		delete pImage;
		return res;
	}

	delete pImage;
	return E_SUCCESS;
}

Bitmap* Utilities::GetBitmapN(const String &name) {
	if (__data.__sdkVer.major == 1 && __data.__sdkVer.minor == 0) {
		Image *pImage = new Image();
		result res = pImage->Construct();
		if (IsFailed(res)) {
			AppLogDebug("Utilities::GetBitmapN: [%s]: failed to construct image decoder", GetErrorMessage(res));
			SetLastResult(res);

			delete pImage;
			return null;
		}

		int screenHeight = 800;
		SystemInfo::GetValue(L"ScreenHeight", screenHeight);
		int screenWidth = 480;
		SystemInfo::GetValue(L"ScreenWidth", screenWidth);

		String fullname = String(L"/Res/");
		fullname.Append(screenWidth);
		fullname.Append('x');
		fullname.Append(screenHeight);
		fullname.Append('/');
		fullname.Append(name);

		Bitmap *pBitmap = null;
		if(fullname.EndsWith(L"jpg"))
			pBitmap = pImage->DecodeN(fullname, BITMAP_PIXEL_FORMAT_RGB565);
		else if(fullname.EndsWith(L"bmp"))
			pBitmap = pImage->DecodeN(fullname, BITMAP_PIXEL_FORMAT_RGB565);
		else if(fullname.EndsWith(L"png"))
			pBitmap = pImage->DecodeN(fullname, BITMAP_PIXEL_FORMAT_ARGB8888);
		else if (fullname.EndsWith(L"gif"))
			pBitmap = pImage->DecodeN(fullname, BITMAP_PIXEL_FORMAT_RGB565);

		delete pImage;

		res = GetLastResult();
		if (IsFailed(res)) {
			AppLogDebug("Utilities::GetBitmapN: [%s]: failed to decode image: [%S]", GetErrorMessage(res), name.GetPointer());
			return null;
		}
		return pBitmap;
	} else return Application::GetInstance()->GetAppResource()->GetBitmapN(name);
}

Bitmap *Utilities::GetBitmapN(const String &name, bool non_res) {
	Image *pImage = new Image();
	result res = pImage->Construct();
	if (IsFailed(res)) {
		AppLogDebug("Utilities::GetBitmapN: [%s]: failed to construct image decoder", GetErrorMessage(res));
		SetLastResult(res);

		delete pImage;
		return null;
	}

	Bitmap *pBitmap = null;
	if(name.EndsWith(L"jpg"))
		pBitmap = pImage->DecodeN(name, BITMAP_PIXEL_FORMAT_RGB565);
	else if(name.EndsWith(L"bmp"))
		pBitmap = pImage->DecodeN(name, BITMAP_PIXEL_FORMAT_RGB565);
	else if(name.EndsWith(L"png"))
		pBitmap = pImage->DecodeN(name, BITMAP_PIXEL_FORMAT_ARGB8888);
	else if (name.EndsWith(L"gif"))
		pBitmap = pImage->DecodeN(name, BITMAP_PIXEL_FORMAT_RGB565);

	delete pImage;

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogDebug("Utilities::GetBitmapN: [%s]: failed to decode nonres image: [%S]", GetErrorMessage(res), name.GetPointer());
		return null;
	}
	return pBitmap;
}

DateTime Utilities::GetLocalDateTimeFromEpoch(long long ticks) {
	GregorianCalendar calendar;
	calendar.Construct();

	calendar.SetTimeInMillisecFromEpoch(ticks);

	return AdjustUTCDateTimeToLocal(calendar.GetTime());
}

long long Utilities::GetUTCUnixTicksFromLocalDateTime(const DateTime &dt) {
	GregorianCalendar calendar;
	calendar.Construct();
	calendar.SetTime(AdjustLocalDateTimeToUTC(dt));

	long long ticks;
	calendar.GetTimeInMillisecFromEpoch(ticks);
	return ticks;
}

long long Utilities::GetUTCUnixTicksFromUTCDateTime(const DateTime &dt) {
	GregorianCalendar calendar;
	calendar.Construct();
	calendar.SetTime(dt);

	long long ticks;
	calendar.GetTimeInMillisecFromEpoch(ticks);
	return ticks;
}

long long Utilities::GetCurrentUTCUnixTicks(void) {
	long long ticks;
	SystemTime::GetTicks(ticks);//milliseconds

	return ticks;
}

int Utilities::GetCurrentLocalDayOfWeek(void) {
	GregorianCalendar calendar;
	calendar.Construct();
	calendar.SetTime(GetCurrentLocalDateTime());

	return calendar.GetTimeField(TIME_FIELD_DAY_OF_WEEK) - calendar.GetFirstDayOfWeek();
}

DateTime Utilities::AdjustUTCDateTimeToLocal(const DateTime &dt) {
	LocaleManager locMgr;
	result res = locMgr.Construct();
	if (IsFailed(res)) {
		AppLogDebug("Utilities::AdjustUTCDateTimeToLocal: [%s]: failed to construct locale manager", GetErrorMessage(res));
		SetLastResult(res);
		return dt;
	}

	TimeZone tz = locMgr.GetSystemTimeZone();

	DateTime date = tz.UtcTimeToStandardTime(dt);
	if (tz.IsDstUsed()) {
		date.AddHours(1);
	}

	return date;
}

DateTime Utilities::AdjustLocalDateTimeToUTC(const DateTime &dt) {
	LocaleManager locMgr;
	result res = locMgr.Construct();
	if (IsFailed(res)) {
		AppLogDebug("Utilities::AdjustLocalDateTimeToUTC: [%s]: failed to construct locale manager", GetErrorMessage(res));
		SetLastResult(res);
		return dt;
	}

	TimeZone tz = locMgr.GetSystemTimeZone();

	DateTime date = tz.StandardTimeToUtcTime(dt);
	if (tz.IsDstUsed()) {
		date.AddHours(-1);
	}

	return date;
}

DateTime Utilities::ParseRFC822DateTimeToUTC(const String &str) {
	String format = str;

	int commaIndex = -1;
	if (!IsFailed(format.IndexOf(L',', 0, commaIndex)) && commaIndex >= 0) {
		format.Remove(0, commaIndex + 1);
	}
	format.Trim();

	ArrayList split_data;
	split_data.Construct(5);

	int last_space_index = 0;
	int space_index = -1;
	String *sub = null;
	while (!IsFailed(format.IndexOf(L' ', last_space_index, space_index)) && space_index >= 0) {
		sub = new String;
		format.SubString(last_space_index, space_index - last_space_index, *sub);

		sub->Trim();
		if (sub->GetLength() > 0) {
			split_data.Add(*sub);
		}
		last_space_index = space_index + 1;
	}
	sub = new String;
	result res = format.SubString(last_space_index, *sub);
	if (!IsFailed(res)) {
		split_data.Add(*sub);
	}

	if (split_data.GetCount() < 5) {
		SetLastResult(E_INVALID_FORMAT);
		split_data.RemoveAll(true);
		return DateTime();
	}

	String *day = static_cast<String *>(split_data.GetAt(0));
	String *month = static_cast<String *>(split_data.GetAt(1));
	String *year = static_cast<String *>(split_data.GetAt(2));
	String datetime = *(static_cast<String *>(split_data.GetAt(3)));
	String *tz = static_cast<String *>(split_data.GetAt(4));

	int num_day, num_year;
	int num_month = -1;

	res = Integer::Parse(*day, num_day);
	if (IsFailed(res)) {
		SetLastResult(E_INVALID_FORMAT);
		split_data.RemoveAll(true);
		return DateTime();
	}
	res = Integer::Parse(*year, num_year);
	if (IsFailed(res)) {
		SetLastResult(E_INVALID_FORMAT);
		split_data.RemoveAll(true);
		return DateTime();
	}

	String months[12] = {
		L"Jan",
		L"Feb",
		L"Mar",
		L"Apr",
		L"May",
		L"Jun",
		L"Jul",
		L"Aug",
		L"Sep",
		L"Oct",
		L"Nov",
		L"Dec"
	};

	for (int i = 0; i < 12; i++) {
		if (months[i].Equals(*month, false)) {
			num_month = i + 1;
			break;
		}
	}
	if (num_month < 0) {
		SetLastResult(E_INVALID_FORMAT);
		split_data.RemoveAll(true);
		return DateTime();
	}

	String substr;
	int ar_ind = 0;
	int time[3] = { -1, -1, 0 };
	while (datetime.GetLength() > 0) {
		datetime.SubString(0, 2, substr);
		if (datetime.GetLength() < 3) {
			datetime.Remove(0, 2);
		} else {
			datetime.Remove(0, 3);
		}

		res = Integer::Parse(substr, time[ar_ind++]);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			split_data.RemoveAll(true);
			return DateTime();
		}
	}

	DateTime almost_result;
	res = almost_result.SetValue(num_year, num_month, num_day, time[0], time[1], time[2]);
	if (IsFailed(res)) {
		SetLastResult(E_INVALID_FORMAT);
		split_data.RemoveAll(true);
		return DateTime();
	}

	String timezones[15] = {
		L"UT",
		L"GMT",
		L"EST",
		L"EDT",
		L"CST",
		L"CDT",
		L"MST",
		L"MDT",
		L"PST",
		L"PDT",
		L"Z",
		L"A",
		L"M",
		L"N",
		L"Y"
	};
	int offsets[15] = {
		0,
		0,
		-5,
		-4,
		-6,
		-5,
		-7,
		-6,
		-8,
		-7,
		0,
		-1,
		-12,
		+1,
		+12
	};

	int american_time = -1;
	for (int i = 0; i < 15; i++) {
		if (timezones[i].Equals(*tz, false)) {
			american_time = i;
			break;
		}
	}
	if (american_time >= 0) {
		almost_result.AddHours(-offsets[american_time]);
	} else {
		String houroffset;
		res = tz->SubString(1, 2, houroffset);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			split_data.RemoveAll(true);
			return DateTime();
		}
		String minoffset;
		res = tz->SubString(3, 2, minoffset);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			split_data.RemoveAll(true);
			return DateTime();
		}

		int hoff, moff;
		res = Integer::Parse(houroffset, hoff);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			split_data.RemoveAll(true);
			return DateTime();
		}
		res = Integer::Parse(minoffset, moff);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			split_data.RemoveAll(true);
			return DateTime();
		}

		if (tz->StartsWith(L"+", 0)) {
			almost_result.AddHours(-hoff);
			almost_result.AddMinutes(-moff);
		} else if (tz->StartsWith(L"-", 0)) {
			almost_result.AddHours(hoff);
			almost_result.AddMinutes(moff);
		} else {
			SetLastResult(E_INVALID_FORMAT);
			split_data.RemoveAll(true);
			return DateTime();
		}
	}

	split_data.RemoveAll(true);
	return almost_result;
}

DateTime Utilities::ParseRFC3339DateTimeToUTC(const String &str) {
	String format = str;

	int indexOfT = -1;
	if (IsFailed(format.IndexOf(L'T', 0, indexOfT)) || indexOfT < 0) {
		SetLastResult(E_INVALID_FORMAT);
		return DateTime();
	}

	String date;
	result res = format.SubString(0, indexOfT, date);
	if (IsFailed(res)) {
		SetLastResult(E_INVALID_FORMAT);
		return DateTime();
	}
	date.Append(L'-');//this is for parsing so not to parse last piece separately

	int arrIndex = 0;
	int dateArray[3] = { -1, -1, -1 };

	int last_space_index = 0;
	int space_index = -1;
	while (last_space_index < date.GetLength() && !IsFailed(date.IndexOf(L'-', last_space_index, space_index))) {
		String sub;
		date.SubString(last_space_index, space_index - last_space_index, sub);

		res = Integer::Parse(sub, dateArray[arrIndex++]);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			return DateTime();
		}
		last_space_index = space_index + 1;
	}

	String time;
	res = format.SubString(indexOfT + 1, 8, time);
	if (IsFailed(res)) {
		SetLastResult(E_INVALID_FORMAT);
		return DateTime();
	}
	time.Append(L':');

	arrIndex = 0;
	int timeArray[3] = { -1, -1, -1 };

	last_space_index = 0;
	space_index = -1;
	while (last_space_index < time.GetLength() && !IsFailed(time.IndexOf(L':', last_space_index, space_index))) {
		String sub;
		time.SubString(last_space_index, space_index - last_space_index, sub);

		res = Integer::Parse(sub, timeArray[arrIndex++]);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			return DateTime();
		}
		last_space_index = space_index + 1;
	}

	DateTime almost_result;
	res = almost_result.SetValue(dateArray[0], dateArray[1], dateArray[2], timeArray[0], timeArray[1], timeArray[2]);
	if (IsFailed(res)) {
		SetLastResult(E_INVALID_FORMAT);
		return DateTime();
	}

	int sepIndex = -1;
	mchar seps[3] = { L'Z', L'+', L'-' };
	arrIndex = 0;

	while (IsFailed(res)) {
		res = format.IndexOf(seps[arrIndex++], indexOfT + 1, sepIndex);
		if (arrIndex == 3 && IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			return DateTime();
		}
	}

	if (arrIndex > 0) {
		String houroffset;
		res = format.SubString(sepIndex + 1, 2, houroffset);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			return DateTime();
		}
		String minoffset;
		res = format.SubString(sepIndex + 4, 2, minoffset);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			return DateTime();
		}

		int hoff, moff;
		res = Integer::Parse(houroffset, hoff);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			return DateTime();
		}
		res = Integer::Parse(minoffset, moff);
		if (IsFailed(res)) {
			SetLastResult(E_INVALID_FORMAT);
			return DateTime();
		}

		if (arrIndex == 1) {
			almost_result.AddHours(-hoff);
			almost_result.AddMinutes(-moff);
		} else {
			almost_result.AddHours(hoff);
			almost_result.AddMinutes(moff);
		}
	}

	return almost_result;
}

MessageBoxModalResult Utilities::ShowMessageBox(const String &title, const String &msg, MessageBoxStyle style) {
	MessageBox *pBox = new MessageBox;
	int mres = 2;
	result res = pBox->Construct(title, msg, style);
	if (!IsFailed(res)) {
		pBox->ShowAndWait(mres);
	}
	SetLastResult(res);
	delete pBox;

	return (MessageBoxModalResult)mres;
}

String Utilities::GetString(const String &id) {
	String str = id;
	result res = Application::GetInstance()->GetAppResource()->GetString(id, str);
	if (IsFailed(res)) {
		AppLogDebug("Utilities::GetString: could not retrieve string by id [%S]", id.GetPointer());
		SetLastResult(res);
	}
	return str;
}

int Utilities::GetLinesForText(const String &text, int txtSize, int width) {
	Font font;
	font.Construct(FONT_STYLE_PLAIN, txtSize);

	TextElement elem;
	elem.Construct(text);
	elem.SetFont(font);

	EnrichedText ent;
	ent.Construct(Dimension(width, 500));
	ent.SetTextWrapStyle(TEXT_WRAP_WORD_WRAP);
	ent.Add(elem);

	return ent.GetTotalLineCount();
}

const wchar_t *Utilities::ConvertCP1251ToUCS2(const byte* buf, int len) {
	wchar_t *res = new wchar_t[len + 1];
	for(int i = 0; i < len; i++) {
		res[i] = __pCP1251ToUCS2CodeTable[buf[i]];
	}
	res[len] = L'\0';
	return res;
}

Bitmap *Utilities::GetStaticFavicon(void) {
	return __data.__pNoFavicon;
}

String Utilities::SaveFavicon(Bitmap *bmp, const Channel &url) {
	Directory::Create(L"/Home/favicons/", true);

	String path = L"/Home/favicons/";
	path.Append(url.GetUniqueString());
	path.Append(L".png");

	result res = EncodeBitmap(bmp, path);
	if (IsFailed(res)) {
		AppLogDebug("Utilities::SaveFavicon: [%s]: failed to save favicon", GetErrorMessage(res));
		SetLastResult(res);
		return L"";
	} else {
		return path;
	}
}

int Utilities::GetGroupedListItemCount(const GroupedList &list) {
	int res = 0;
	for(int i = 0; i < list.GetGroupCount(); i++) {
		res += list.GetItemCountAt(i);
	}
	return res;
}
