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
#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <FBase.h>
#include <FUi.h>
#include <FLocales.h>
#include <FGraphics.h>

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::Locales;
using namespace Osp::Graphics;

class Channel;

struct SDKVersionInfo {
	int major;
	int minor;
};

struct AppVersionInfo {
	int major;
	int minor;
	int revision;
};

namespace {
	typedef struct {
		SDKVersionInfo __sdkVer;
		DateTime __epoch;
		DateTimeFormatter *__pDTFormatter;
		Bitmap *__pNoFavicon;
	} __UtilitiesEx;

	static const wchar_t __pCP1251ToUCS2CodeTable[] = {
		0,1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
		22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
		43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
		85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105,
		106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
		127, 1026, 1027, 8218, 1107, 8222, 8230, 8224, 8225, 8364, 8240, 1033, 8249, 1034, 1036, 1035, 1039, 1106, 8216, 8217, 8220,
		8221, 8226, 8211, 8212, 0, 8482, 1113, 8250, 1114, 1116, 1115, 1119, 160, 1038, 1118, 1032, 164, 1168, 166, 167, 1025,
		169, 1028, 171, 172, 173, 174, 1031, 176, 177, 1030, 1110, 1169, 181, 182, 183, 1105, 8470, 1108, 187, 1112, 1029,
		1109, 1111, 1040, 1041, 1042, 1043, 1044, 1045, 1046, 1047, 1048, 1049, 1050, 1051, 1052, 1053, 1054, 1055, 1056, 1057, 1058,
		1059, 1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068, 1069, 1070, 1071, 1072, 1073, 1074, 1075, 1076, 1077, 1078, 1079,
		1080, 1081, 1082, 1083, 1084, 1085, 1086, 1087, 1088, 1089, 1090, 1091, 1092, 1093, 1094, 1095, 1096, 1097, 1098, 1099, 1100,
		1101, 1102, 1103
	};
}

class Utilities {
public:
	static void Initialize(void);

	static AppVersionInfo GetAppVersion(void);
	static String GetAppVersionString(void);
	static String GetScreenResolutionString(void);
	static bool IsFirmwareLaterThan10(void);

	static bool IsVersionLater(AppVersionInfo version, AppVersionInfo than);

	static DateTime GetCurrentUTCDateTime(void);
	static DateTime GetCurrentLocalDateTime(void);
	static DateTime GetLocalDateTimeFromEpoch(long long ticks);
	static long long GetUTCUnixTicksFromLocalDateTime(const DateTime &dt);
	static long long GetUTCUnixTicksFromUTCDateTime(const DateTime &dt);
	static long long GetCurrentUTCUnixTicks(void);

	static int GetCurrentLocalDayOfWeek(void);

	static String FormatUTCDateTimeRFC822(const DateTime &dt);
	static String FormatDateTime(const DateTime &dt);
	static String FormatDate(const DateTime &dt);
	static String FormatDateTimeFromEpoch(long long ticks);
	static String FormatCurrentLocalDateTime(void);

	static DateTime AdjustUTCDateTimeToLocal(const DateTime &dt);
	static DateTime AdjustLocalDateTimeToUTC(const DateTime &dt);

	static DateTime ParseRFC822DateTimeToUTC(const String &str);
	static DateTime ParseRFC3339DateTimeToUTC(const String &str);

	static MessageBoxModalResult ShowMessageBox(const String &title, const String &msg, MessageBoxStyle style);

	static Bitmap *GetBitmapN(const String &name);
	static Bitmap *GetBitmapN(const String &name, bool non_res);
	static String GetString(const String &id);

	static result EncodeBitmap(Bitmap *bmp, const String &path);

	static int GetLinesForText(const String &text, int txtSize, int width);

	static Bitmap *GetStaticFavicon(void);
	static String SaveFavicon(Bitmap *bmp, const Channel &url);

	static const wchar_t *ConvertCP1251ToUCS2(const byte* buf, int len);

	static int GetGroupedListItemCount(const GroupedList &list);

private:
	static void RemoveInstance(void);

	static __UtilitiesEx __data;
	friend class Aggregator;
};

#endif
