/* oshwbind.cpp: Binds the generic module to the Qt OS/hardware layer.
 * 
 * Copyright (C) 2001-2010 by Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	"../generic/generic.h"
#include	"../gen.h"

#include <QBitmap>
#include <QPainter>

// #include <QTime>
/*
#include <QWaitCondition>
#include <QMutex>
*/

#ifdef WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif

#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include <stdio.h>


Qt_Surface::Qt_Surface()
{
	w = h = 0;
	bytesPerPixel = 0;
	pitch = 0;
	pixels = 0;
	m_bColorKeySet = false;
	m_nColorKey = 0;
}

void Qt_Surface::Init(const QPaintDevice& dev)
{
	w = dev.width();
	h = dev.height();
	bytesPerPixel = dev.depth() / 8;
	pitch = 0;
	pixels = 0;
}

void Qt_Surface::InitImage()
{
	bytesPerPixel = m_image.depth() / 8;
	pitch = m_image.bytesPerLine();
	pixels = m_image.bits();
}


void Qt_Surface::SetPixmap(const QPixmap& pixmap)
{
	m_pixmap = pixmap;
	m_image = QImage();
	Init(m_pixmap);
}

void Qt_Surface::SetImage(const QImage& image)
{
	m_image = image;
	m_pixmap = QPixmap();
	Init(m_image);
	InitImage();
}


const QPixmap& Qt_Surface::GetPixmap()
{
	if (m_pixmap.isNull())
	{
		m_pixmap = QPixmap::fromImage(m_image);
	}
	return m_pixmap;
}

const QImage& Qt_Surface::GetImage()
{
	if (m_image.isNull())
		m_image = m_pixmap.toImage();
	InitImage();
	return m_image;
}


void Qt_Surface::Lock()
{
	(void)GetImage();
	m_pixmap = QPixmap();
}

void Qt_Surface::Unlock()
{
	// NOTHING
}


void Qt_Surface::FillRect(const TW_Rect* pDstRect, uint32_t nColor)
{
	(void)GetPixmap();
	// TODO?: don't force image -> pixmap?
	// TODO?: for 8-bit?
	if (!pDstRect)
	{
		m_pixmap.fill(nColor);
	}
	else
	{
		QPainter painter(&m_pixmap);
		painter.fillRect(*pDstRect, QColor(nColor));
	}
	
	m_image = QImage();
	pixels = 0;
}


void Qt_Surface::BlitSurface(Qt_Surface* pSrc, const TW_Rect* pSrcRect,
						     Qt_Surface* pDst, const TW_Rect* pDstRect)
{
	TW_Rect srcRect = (pSrcRect ? *pSrcRect : TW_Rect(0,0, pSrc->w, pSrc->h));
	TW_Rect dstRect = (pDstRect ? *pDstRect : TW_Rect(0,0, pDst->w, pDst->h));
	if (srcRect.w == 0) srcRect.w = pSrc->w;
	if (srcRect.h == 0) srcRect.h = pSrc->h;
	if (dstRect.w == 0) dstRect.w = srcRect.w;
	if (dstRect.h == 0) dstRect.h = srcRect.h;
	if (!pDstRect)
		{dstRect.w = srcRect.w; dstRect.h = srcRect.h;}
	else if (pDstRect && !pSrcRect)
		{srcRect.w = dstRect.w; srcRect.h = dstRect.h;}

	// TODO?: don't force image -> pixmap?

	(void)pDst->GetPixmap();
	pDst->m_image = QImage();
	pDst->pixels = 0;

	QPixmap srcPix;
	if (pSrc->IsColorKeySet())
	{
		QImage image = pSrc->GetImage().copy(srcRect);
		QImage imgMask = image.createMaskFromColor(pSrc->GetColorKey());
		
		srcPix = QPixmap::fromImage(image);
		QBitmap bmpMask = QBitmap::fromImage(imgMask);
		srcPix.setMask(bmpMask);
		
		srcRect.x = srcRect.y = 0;
		
		pDst->m_pixmap = srcPix;	// @#$
		return;	// @#$
	}
	else
	{
		srcPix = pSrc->GetPixmap();
	}

	QPainter painter(&(pDst->m_pixmap));
	painter.drawPixmap(QRect(dstRect).topLeft(), srcPix, srcRect);	
}


void Qt_Surface::SetColorKey(uint32_t nColorKey)
{
	m_nColorKey = nColorKey;
	m_bColorKeySet = true;
}

void Qt_Surface::ResetColorKey()
{
	m_bColorKeySet = false;
}


Qt_Surface* Qt_Surface::DisplayFormat()
{
	Qt_Surface* pNewSurface = new Qt_Surface(*this);
	if (!m_image.isNull())
		pNewSurface->pixels = pNewSurface->m_image.bits();
	(void)pNewSurface->GetPixmap();
	return pNewSurface;
}


/* Create a fresh surface. If transparency is true, the surface is
 * created with 32-bit pixels, so as to ensure a complete alpha
 * channel. Otherwise, the surface is created with the same format as
 * the screen.
 */
extern "C" TW_Surface* TW_NewSurface(int w, int h, int bTransparent)
{
	Qt_Surface* pSurface = new Qt_Surface();

	if (bTransparent)
	{
		QImage image(w, h, QImage::Format_ARGB32);
		image.fill(0);
		pSurface->SetImage(image);
	}
	else
	{
		QPixmap pixmap(w, h);
		pixmap.fill(Qt::black);
		pSurface->SetPixmap(pixmap);
	}
	
	return pSurface;
}


extern "C" void TW_FreeSurface(TW_Surface* s)
{
	Qt_Surface* pSurface = static_cast<Qt_Surface*>(s);
	delete pSurface;
}


extern "C" void TW_LockSurface(TW_Surface* s)
{
	Qt_Surface* pSurface = static_cast<Qt_Surface*>(s);
	pSurface->Lock();
}

extern "C" void TW_UnlockSurface(TW_Surface* s)
{
	Qt_Surface* pSurface = static_cast<Qt_Surface*>(s);
	pSurface->Unlock();
}


extern "C" void TW_FillRect(TW_Surface* pDst, const TW_Rect* pDstRect, uint32_t nColor)
{
	Qt_Surface* pSurface = static_cast<Qt_Surface*>(pDst);
	pSurface->FillRect(pDstRect, nColor);
}


extern "C" int TW_BlitSurface(TW_Surface* _pSrc, const TW_Rect* pSrcRect,
						      TW_Surface* _pDst, const TW_Rect* pDstRect)
{
	Qt_Surface* pDst = static_cast<Qt_Surface*>(_pDst);
	Qt_Surface* pSrc = static_cast<Qt_Surface*>(_pSrc);

	Qt_Surface::BlitSurface(pSrc, pSrcRect, pDst, pDstRect);
	return 0;
}


extern "C" void TW_SetColorKey(TW_Surface* s, uint32_t nColorKey)
{
	Qt_Surface* pSurface = static_cast<Qt_Surface*>(s);
	pSurface->SetColorKey(nColorKey);
}

extern "C" void TW_ResetColorKey(TW_Surface* s)
{
	Qt_Surface* pSurface = static_cast<Qt_Surface*>(s);
	pSurface->ResetColorKey();
}


extern "C" TW_Surface* TW_DisplayFormat(TW_Surface* s)
{
	Qt_Surface* pSurface = static_cast<Qt_Surface*>(s);
	return pSurface->DisplayFormat();
}


extern "C" TW_Surface* TW_DisplayFormatAlpha(TW_Surface* pSurface)
{
	return TW_DisplayFormat(pSurface);
}


/* Return the color of the pixel at (x, y) on the given surface. (The
 * surface must be locked before calling this function.)
 */
extern "C" uint32_t TW_PixelAt(const TW_Surface* s, int x, int y)
{
	const Qt_Surface* pSurface = static_cast<const Qt_Surface*>(s);
	return pSurface->PixelAt(x, y);
}


extern "C" uint32_t TW_MapRGB(const TW_Surface* pSurface, uint8_t r, uint8_t g, uint8_t b)
{
	// TODO: for 8-bit
	return qRgb(r, g, b);
}


extern "C" uint32_t TW_MapRGBA(const TW_Surface* pSurface, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	// TODO: for 8-bit
	return qRgba(r, g, b, a);
}


/* Load the given bitmap file. If setscreenpalette is true, the screen palette
 * will be synchronized to the bitmap's palette.
 */
extern "C" TW_Surface* TW_LoadBMP(const char* szFilename, int bSetScreenPalette)
{
	QImage image(szFilename);
	if (image.isNull())
		return 0;
	
	image = image.convertToFormat(QImage::Format_ARGB32);
	// Doesn't seem to be necessary, but just in case...
		
	Qt_Surface* pSurface = new Qt_Surface();
	pSurface->SetImage(image);
	return pSurface;
	
	// TODO?: bSetScreenPalette?
}


// @#$
extern "C" void TW_DebugSurface(TW_Surface* s, const char* szFilename)
{
	static int n = 0;
	if (n == 10) return;
	++n;
	
	Qt_Surface* pSurface = static_cast<Qt_Surface*>(s);
	QString sNFilename = QString::number(n) + szFilename;
	pSurface->GetImage().save(sNFilename);
	// pSurface->GetImage().createAlphaMask().save(sNFilename);
}
// $#@


extern "C" uint32_t TW_GetTicks(void)
{
/*
	static QTime time;
	static bool bStarted = false;
	if (!bStarted)
	{
		time.start();
		bStarted = true;
	}
	return time.elapsed();
*/

	static const time_t t0 = time(0);
	timeb timeBuf;
	ftime(&timeBuf);
	return  (timeBuf.time - t0) * 1000  +  timeBuf.millitm;
}


extern "C" void TW_Delay(uint32_t nMS)
{
/*
	static QWaitCondition waitCond;
	static QMutex mutex;
	mutex.lock();
	waitCond.wait(&mutex, nMS);
	mutex.unlock();
*/

#ifdef WIN32
	Sleep(nMS);
#else
	usleep(nMS * 1000);
#endif
}
