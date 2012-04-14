/* Copyright (C) 2001-2010 by Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include "CCMetaData.h"

#include <QFile>
#include <QDomDocument>
#include <QDomElement>

using namespace std;


namespace CCX
{


template <typename T>
static bool ReadElmAttr(QDomElement elm, QString sAttr, T (*pf)(QString), T& rValue)
{
	if (!elm.hasAttribute(sAttr))
		return false;
	rValue = (*pf)(elm.attribute(sAttr));
	return true;
}


inline static QString ParseString(QString s)
{
	return s;
}

inline static int ParseInt(QString s)
{
	return s.toInt();
}

inline static QColor ParseColor(QString s)
{
	return QColor(s);
}

static Qt::AlignmentFlag ParseHAlign(QString s)
{
	return (s == "right") ? Qt::AlignRight : (s == "center") ? Qt::AlignHCenter : Qt::AlignLeft;
}

static Qt::AlignmentFlag ParseVAlign(QString s)
{
	return (s == "bottom") ? Qt::AlignBottom : (s == "middle") ? Qt::AlignVCenter : Qt::AlignTop;
}

static Compatibility ParseCompat(QString s)
{
	return (s == "yes") ? COMPAT_YES : (s == "no") ? COMPAT_NO : COMPAT_UNKNOWN;
}

static TextFormat ParseFormat(QString s)
{
	return (s == "html") ? TEXT_HTML : TEXT_PLAIN;
}


void RulesetCompatibility::ReadXML(QDomElement elm)
{
	ReadElmAttr(elm, "ms",       &ParseCompat, eMS);
	ReadElmAttr(elm, "lynx",     &ParseCompat, eLynx);
	ReadElmAttr(elm, "pedantic", &ParseCompat, ePedantic);
}


void PageProperties::ReadXML(QDomElement elm)
{
	ReadElmAttr(elm, "format",  &ParseFormat, eFormat);
	ReadElmAttr(elm, "align",   &ParseHAlign, align);
	ReadElmAttr(elm, "valign",  &ParseVAlign, valign);
	ReadElmAttr(elm, "color",   &ParseColor , color);
	ReadElmAttr(elm, "bgcolor", &ParseColor , bgcolor);
}


void Page::ReadXML(QDomElement elm, const Levelset& levelset)
{
	sText = elm.text();

	pageProps = levelset.pageProps;
	pageProps.ReadXML(elm);
}


void Text::ReadXML(QDomElement elm, const Levelset& levelset)
{
	vecPages.clear();
	QDomNodeList lstElmPages = elm.elementsByTagName("page");
	for (int i = 0; i < int(lstElmPages.length()); ++i)
	{
		QDomElement elmPage = lstElmPages.item(i).toElement();
		Page page;
		page.ReadXML(elmPage, levelset);
		vecPages.push_back(page);
	}
}


void Level::ReadXML(QDomElement elm, const Levelset& levelset)
{
	sAuthor = levelset.sAuthor;
	ReadElmAttr(elm, "author", &ParseString, sAuthor);
	
	ruleCompat = levelset.ruleCompat;
	ruleCompat.ReadXML(elm);
	
	QDomNodeList lstElm;
	lstElm = elm.elementsByTagName("prologue");
	if (lstElm.length() != 0)
		txtPrologue.ReadXML(lstElm.item(0).toElement(), levelset);
	lstElm = elm.elementsByTagName("epilogue");
	if (lstElm.length() != 0)
		txtEpilogue.ReadXML(lstElm.item(0).toElement(), levelset);
}


void Levelset::ReadXML(QDomElement elm)
{
	ReadElmAttr(elm, "description", &ParseString, sDescription);
	ReadElmAttr(elm, "copyright",   &ParseString, sCopyright);
	ReadElmAttr(elm, "author",      &ParseString, sAuthor);

	ruleCompat.ReadXML(elm);
	pageProps.ReadXML(elm);
	
	for (int i = 0; i < int(vecLevels.size()); ++i)
	{
		Level& rLevel = vecLevels[i];
		rLevel.sAuthor = sAuthor;
		rLevel.ruleCompat = ruleCompat;
	}

	// vecLevels.clear();
	QDomNodeList lstElmLevels = elm.elementsByTagName("level");
	for (int i = 0; i < int(lstElmLevels.length()); ++i)
	{
		QDomElement elmLevel = lstElmLevels.item(i).toElement();
		int nNumber = 0;
		if (!ReadElmAttr(elmLevel, "number", &ParseInt, nNumber))
			continue;
		if ( ! (nNumber >= 1  &&  nNumber < int(vecLevels.size())) )
			continue;
		Level& rLevel = vecLevels[nNumber];
		rLevel.ReadXML(elmLevel, *this);
	}
	
	QDomNodeList lstElmStyle = elm.elementsByTagName("style");
	if (lstElmStyle.length() != 0)
	{
		QDomElement elmStyle = lstElmStyle.item(0).toElement();
		if (elmStyle.parentNode() == elm)
			sStyleSheet = elmStyle.text();
	}
}


bool Levelset::ReadFile(QString sFilePath, int nLevels)
{
	Clear();
	
	vecLevels.resize(1+nLevels);
	
	QFile file(sFilePath);
	if (!file.exists())
		return true;
	if (!file.open(QIODevice::ReadOnly|QIODevice::Text))
		return false;
	
	QDomDocument doc;
	if (!doc.setContent(&file))
		return false;
		
	QDomElement elmRoot = doc.documentElement();
	if (elmRoot.tagName() != "levelset")
		return false;
		
	ReadXML(elmRoot);
	
	file.close();
	
	return true;
}


void Levelset::Clear()
{
	*this = Levelset();
}


}
