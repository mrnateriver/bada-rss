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
#ifndef OPMLPARSER_H_
#define OPMLPARSER_H_

#include "Channel.h"
#include <FXml.h>

using namespace Osp::Base::Collection;
using namespace Osp::Xml;

class OPMLParser {
public:
	OPMLParser(void) {}

	ArrayList *ParseFileN(const String &opmlFile) const;
	result SaveToFile(const String &opmlFile, const ArrayList &channels) const;

private:
	ArrayList *ParseOPMLBodyN(xmlNodePtr pBodyNode) const;
};

#endif
