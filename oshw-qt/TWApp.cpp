/* Copyright (C) 2001-2010 by Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include "TWApp.h"
#include "TWMainWnd.h"

#include "../generic/generic.h"
#include "../oshw-sdl/sdlsfx.h"

#include "../gen.h"
#include "../defs.h"
#include "../oshw.h"

#ifdef WIN32
#include <QWindowsStyle>
#endif

#include <string.h>
#include <stdlib.h>


TileWorldApp* g_pApp = 0;
TileWorldMainWnd* g_pMainWnd = 0;


const char TileWorldApp::s_szTitle[] = "Tile World";


TileWorldApp::TileWorldApp(int& argc, char** argv)
	:
	QApplication(argc, argv),
	m_bSilence(false),
	m_bShowHistogram(false),
	m_bFullScreen(false)
{
	g_pApp = this;
}


TileWorldApp::~TileWorldApp()
{
	delete g_pMainWnd;
	g_pMainWnd = 0;

	g_pApp = 0;
}


/* Process all pending events. If wait is TRUE and no events are
 * currently pending, the function blocks until an event arrives.
 */
static void _eventupdate(int wait)
{
	QApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents : QEventLoop::AllEvents);
}


/* Initialize the OS/hardware interface. This function must be called
 * before any others in the oshw library. If silence is TRUE, the
 * sound system will be disabled, as if no soundcard was present. If
 * showhistogram is TRUE, then during shutdown the timer module will
 * send a histogram to stdout describing the amount of time the
 * program explicitly yielded to other processes. (This feature is for
 * debugging purposes.) soundbufsize is a number between 0 and 3 which
 * is used to scale the size of the sound buffer. A larger number is
 * more efficient, but pushes the sound effects farther out of
 * synchronization with the video.
 */
int oshwinitialize(int silence, int soundbufsize,
                   int showhistogram, int fullscreen)
{
	return g_pApp->Initialize(silence, soundbufsize, showhistogram, fullscreen);
}

bool TileWorldApp::Initialize(bool bSilence, int nSoundBufSize,
                              bool bShowHistogram, bool bFullScreen)
{
    geng.eventupdatefunc = _eventupdate;

	m_bSilence = bSilence;
	m_bShowHistogram = bShowHistogram;
	m_bFullScreen = bFullScreen;
	
	g_pMainWnd = new TileWorldMainWnd;
	g_pMainWnd->setWindowTitle(s_szTitle);

	if ( ! (
		_generictimerinitialize(bShowHistogram) &&
		_generictileinitialize() &&
		_genericinputinitialize() &&
		_sdlsfxinitialize(bSilence, nSoundBufSize)
	   ) )
		return false;
	
	if (bFullScreen)
	{
		g_pMainWnd->showFullScreen();
	}
	else
	{
		g_pMainWnd->adjustSize();
		g_pMainWnd->show();
	}
		
	return true;
}


/*
 * Resource-loading functions.
 */

/* Extract the font stored in the given file and make it the current
 * font. FALSE is returned if the attempt was unsuccessful. If
 * complain is FALSE, no error messages will be displayed.
 */
int loadfontfromfile(char const *filename, int complain)
{
	// N/A
	return true;
}

/* Free all memory associated with the current font.
 */
void freefont(void)
{
	// N/A
}


#if DUMMY_SFX_IMPL

/* Load a wave file into memory. index indicates which sound effect to
 * associate the sound with. FALSE is returned if an error occurs.
 */
int loadsfxfromfile(int index, char const *filename)
{
	return true;
}

/* Release all memory used for the given sound effect's wave data.
 */
void freesfx(int index)
{
}

#endif


int TileWorldApp::RunTWorld()
{
	if (!setjmp(m_jmpBuf))	// try
	{
		return tworld(argc(), argv());
	}
	else	// catch (int)
	{
		return 0;
	}
}


void TileWorldApp::ExitTWorld()
{
	// Attempt to gracefully destroy application objects
	
	// throw 1;
	// Can't throw C++ exceptions through C code
	
	// longjmp(m_jmpBuf, 1);
	// Works, but needs to be cleaner

	::exit(0);
	// Live with this for now...
}


/* The real main().
 */
int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		const char* szArg = argv[i];
		if (strlen(szArg) == 2  &&  szArg[0] == '-'  &&  strchr("lstbhdvV", szArg[1]) != 0)
			return tworld(argc, argv);
	}
	
	TileWorldApp app(argc, argv);
#ifdef WIN32
	QApplication::setStyle(new QWindowsStyle());	// Vista / XP styles may mess up colors
#endif

	return app.RunTWorld();
}
