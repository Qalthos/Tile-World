/* Copyright (C) 2001-2010 by Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include "TWProgressBar.h"

#include <QPainter>
#include <QPalette>


TWProgressBar::TWProgressBar(QWidget* pParent)
	:
	QProgressBar(pParent),
	m_nValue(0),
	m_nPar(-1),
	m_bParBad(false)
{
}
	

void TWProgressBar::setValue(int nValue)
{
	if (m_nValue == nValue) return;
	m_nValue = nValue;
	update();
}

void TWProgressBar::setPar(int nPar)
{
	if (m_nPar == nPar) return;
	m_nPar = nPar;
	update();
}

void TWProgressBar::setParBad(bool bParBad)
{
	if (m_bParBad == bParBad) return;
	m_bParBad = bParBad;
	update();
}


QString TWProgressBar::text() const
{
	QString sText = format();
	sText.replace("%v", QString::number(m_nValue));
	return sText;
}


void TWProgressBar::paintEvent(QPaintEvent* pPaintEvent)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);
	const QPalette& pal = this->palette();
	QRect rect = this->rect();
	
	static const int M = 2;
	qDrawShadePanel(&painter, rect, pal, true, M);
	rect.adjust(+M, +M, -M, -M);
	painter.fillRect(rect, pal.window());

	int d = maximum() - minimum();
	if (d > 0)
	{
		bool bHasPar = (par() > minimum());
		QRect rect2 = rect;
		double v = double(value() - minimum()) / d;
		rect2.setWidth(int(v * rect.width()));
		painter.fillRect(rect2, (bHasPar && !m_bParBad) ? QColor(160, 32, 32) : pal.highlight());
		// qDrawShadePanel(&painter, rect2, pal, false, 1);
		if (bHasPar  &&  par() < value())
		{
			double p = double(par() - minimum()) / d;
			rect2.setLeft(int(p * rect.width()));
			painter.fillRect(rect2, QColor(32, 160, 32));
		}
	}
	
	if (isTextVisible())
	{
		painter.setRenderHint(QPainter::TextAntialiasing, false);
		painter.drawText(rect, alignment(), text());
	}
}
