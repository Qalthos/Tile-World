/* Copyright (C) 2001-2010 by Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include "TWDisplayWidget.h"

#include <QPainter>


TWDisplayWidget::TWDisplayWidget(QWidget* pParent)
	:
	QWidget(pParent)
{
}
	

void TWDisplayWidget::setPixmap(const QPixmap& pixmap)
{
	bool bResized = (pixmap.size() != m_pixmap.size());
	m_pixmap = pixmap;
	if (bResized)
		updateGeometry();
	// update();
	repaint();
}


QSize TWDisplayWidget::sizeHint() const
{
	return m_pixmap.size();
}


void TWDisplayWidget::paintEvent(QPaintEvent* pPaintEvent)
{
	QPainter painter(this);
	painter.drawPixmap(0, 0, m_pixmap);
}
