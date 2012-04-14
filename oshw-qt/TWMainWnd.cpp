/* Copyright (C) 2001-2010 by Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include "TWMainWnd.h"
#include "TWApp.h"

#include "../generic/generic.h"

#include "../gen.h"
#include "../defs.h"
#include "../state.h"
#include "../play.h"
#include "../oshw.h"
#include "../err.h"
extern int pedanticmode;

#include <QApplication>
#include <QClipboard>

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QShortcut>

#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>

#include <QTextDocument>

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <QStyle>
// #include <QMotifStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItemV2>

#include <QPainter>
#include <QPalette>
#include <QBrush>
#include <QLinearGradient>

#include <QDir>
#include <QFileInfo>

#include <QString>
#include <QTextStream>

#include <vector>

#include <string.h>
#include <ctype.h>

using namespace std;


class TWStyledItemDelegate : public QStyledItemDelegate
{
public:
	TWStyledItemDelegate(QObject* pParent = 0)
		: QStyledItemDelegate(pParent) {}
		
	virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& option,
		const QModelIndex& index) const;
};


void TWStyledItemDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& _option,
	const QModelIndex& index) const
{
	QStyleOptionViewItemV2 option = _option;
	option.state &= ~QStyle::State_HasFocus;
	QStyledItemDelegate::paint(pPainter, option, index);
}

// ... All this just to remove a silly little dotted focus rectangle


class TWTableModel : public QAbstractTableModel
{
public:
	TWTableModel(QObject* pParent = 0);
	void SetTableSpec(const tablespec* pSpec);
	
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	
protected:
	struct ItemInfo
	{
		QString sText;
		Qt::Alignment align;
		ItemInfo() : align(Qt::AlignCenter) {}
	};
	
	int m_nRows, m_nCols;
	vector<ItemInfo> m_vecItems;
	
	QVariant GetData(int row, int col, int role) const;
};


TWTableModel::TWTableModel(QObject* pParent)
	:
	QAbstractTableModel(pParent),
	m_nRows(0), m_nCols(0)
{
}

void TWTableModel::SetTableSpec(const tablespec* pSpec)
{
	m_nRows = pSpec->rows;
	m_nCols = pSpec->cols;
	int n = m_nRows * m_nCols;
	
	m_vecItems.clear();
	m_vecItems.reserve(n);
	
	ItemInfo dummyItemInfo;
	
	const char* const * pp = pSpec->items;
	for (int i = 0; i < n; ++pp)
	{
		const char* p = *pp;
		ItemInfo ii;

		ii.sText = p + 2;
		
		char c = p[1];
		Qt::Alignment ha = (c=='+' ? Qt::AlignRight : c=='.' ? Qt::AlignHCenter : Qt::AlignLeft);
		ii.align = (ha | Qt::AlignVCenter);
		
		m_vecItems.push_back(ii);

		int d = p[0] - '0';
		for (int j = 1; j < d; ++j)
		{
			m_vecItems.push_back(dummyItemInfo);
		}

		i += d;
	}
	
	reset();
}

int TWTableModel::rowCount(const QModelIndex& parent) const
{
	return m_nRows-1;
}

int TWTableModel::columnCount(const QModelIndex& parent) const
{
	return m_nCols;
}

QVariant TWTableModel::GetData(int row, int col, int role) const
{
	int i = row*m_nCols + col;
	const ItemInfo& ii = m_vecItems[i];
	
	switch (role)
	{
		case Qt::DisplayRole:
			return ii.sText;
			
		case Qt::TextAlignmentRole:
			return int(ii.align);
		
		default:
			return QVariant();
	}
}

QVariant TWTableModel::data(const QModelIndex& index, int role) const
{
	return GetData(1+index.row(), index.column(), role);
}

QVariant TWTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
		return GetData(0, section, role);
	else
		return QVariant();
}


TileWorldMainWnd::TileWorldMainWnd(QWidget* pParent, Qt::WindowFlags flags)
	:
	QMainWindow(pParent, flags/*|Qt::FramelessWindowHint*/),
	m_bSetupUi(false),
	m_bWindowClosed(false),
	m_pSurface(0),
	m_pInvSurface(0),
#if OLD_TIMER_IMPL
	m_nMSPerSecond(1000),
	m_bTimerStarted(false),
	m_nTickCount(0),
#endif
	m_nMsgUntil(0), m_nMsgBoldUntil(0),
	m_bKbdRepeatEnabled(true),
	m_nNextCompletionMsg(0), m_nNextTimeoutMsg(0), m_nNextDeathMsg(0),
	m_nRuleset(Ruleset_None),
	m_nLevelNum(0),
	m_bProblematic(false),
	m_bOFNT(false),
	m_nBestTime(TIME_NIL),
	m_bHintShown(false),
	m_nTimeLeft(TIME_NIL),
	m_bReplay(false),
	m_pSortFilterProxyModel(0)
{
	memset(m_nKeyState, 0, TWK_LAST*sizeof(uint8_t));
	
	setupUi(this);
	m_bSetupUi = true;
	
	QLayout* pGameLayout = m_pGamePage->layout();
	if (pGameLayout != 0)
	{
		pGameLayout->setAlignment(m_pGameFrame, Qt::AlignCenter);
		pGameLayout->setAlignment(m_pCountersFrame, Qt::AlignCenter);
		pGameLayout->setAlignment(m_pObjectsFrame, Qt::AlignCenter);
		pGameLayout->setAlignment(m_pMessagesFrame, Qt::AlignHCenter);
	}
	
	QPalette pal = m_pMainWidget->palette();
	QLinearGradient gradient(0, 0, 1, 1);
	gradient.setCoordinateMode(QGradient::StretchToDeviceMode);
	QColor color = pal.window().color();
	gradient.setColorAt(0, color.lighter(125));
	gradient.setColorAt(1, color.darker(125));
	pal.setBrush(QPalette::Window, QBrush(gradient));
	m_pMainWidget->setPalette(pal);

	m_pTblList->setItemDelegate( new TWStyledItemDelegate(m_pTblList) );
	
	// m_pGameWidget->setAttribute(Qt::WA_PaintOnScreen);
	// m_pObjectsWidget->setAttribute(Qt::WA_PaintOnScreen);
	
	// m_pPrgTime->setStyleSheet("QProgressBar::chunk {margin: 0px;}");
	// m_pPrgTime->setStyle(new QMotifStyle());
	
	g_pApp->installEventFilter(this);
	
	connect( m_pTblList, SIGNAL(activated(const QModelIndex&)), this, SLOT(OnListItemActivated(const QModelIndex&)) );
	connect( m_pTxtFind, SIGNAL(textChanged(const QString&)), this, SLOT(OnFindTextChanged(const QString&)) );
	connect( m_pTxtFind, SIGNAL(returnPressed()), this, SLOT(OnFindReturnPressed()) );
	connect( m_pBtnPlay, SIGNAL(clicked()), this, SLOT(OnPlayback()) );
	connect( m_pSldSpeed, SIGNAL(valueChanged(int)), this, SLOT(OnSpeedValueChanged(int)) );
	connect( m_pSldSpeed, SIGNAL(sliderReleased()), this, SLOT(OnSpeedSliderReleased()) );
	connect( m_pSldSeek, SIGNAL(valueChanged(int)), this, SLOT(OnSeekPosChanged(int)) );
	connect( m_pBtnTextNext, SIGNAL(clicked()), this, SLOT(OnTextNext()) );
	connect( m_pBtnTextPrev, SIGNAL(clicked()), this, SLOT(OnTextPrev()) );
	connect( m_pBtnTextReturn, SIGNAL(clicked()), this, SLOT(OnTextReturn()) );
	
	connect( new QShortcut(Qt::Key_Escape, m_pTextPage), SIGNAL(activated()), this, SLOT(OnTextReturn()) );
	connect( new QShortcut(Qt::CTRL+Qt::Key_R, m_pTextPage), SIGNAL(activated()), this, SLOT(OnTextReturn()) );
	connect( new QShortcut(Qt::CTRL+Qt::Key_N, m_pTextPage), SIGNAL(activated()), this, SLOT(OnTextNext()) );
	connect( new QShortcut(Qt::CTRL+Qt::Key_P, m_pTextPage), SIGNAL(activated()), this, SLOT(OnTextPrev()) );
	connect( new QShortcut(Qt::Key_N, m_pTextPage), SIGNAL(activated()), this, SLOT(OnTextNext()) );
	connect( new QShortcut(Qt::Key_P, m_pTextPage), SIGNAL(activated()), this, SLOT(OnTextPrev()) );
	
	connect( m_pMenuBar, SIGNAL(triggered(QAction*)), this, SLOT(OnMenuActionTriggered(QAction*)) );
}


TileWorldMainWnd::~TileWorldMainWnd()
{
	g_pApp->removeEventFilter(this);

	TW_FreeSurface(m_pInvSurface);
	TW_FreeSurface(m_pSurface);
}


void TileWorldMainWnd::closeEvent(QCloseEvent* pCloseEvent)
{
	QMainWindow::closeEvent(pCloseEvent);
	// pCloseEvent->ignore();
	m_bWindowClosed = true;

	/*
	switch (m_pMainWidget->currentIndex())
	{
		case PAGE_GAME:
			keyeventcallback(TWK_LALT, false);
			keyeventcallback(TWK_LCTRL, false);
			keyeventcallback(TWK_LSHIFT, true);
			keyeventcallback('q', true);
		break;
	
		case PAGE_TABLE:
			g_pApp->exit(CmdQuitLevel);
		break;
		
		case PAGE_TEXT:
			g_pApp->exit(CmdQuit);
		break;
	}
	*/
	
	if (m_pMainWidget->currentIndex() == PAGE_GAME)
		g_pApp->ExitTWorld();
	else
		g_pApp->quit();
}


bool TileWorldMainWnd::eventFilter(QObject* pObject, QEvent* pEvent)
{
	if (HandleEvent(pObject, pEvent))
		return true;
	return QMainWindow::eventFilter(pObject, pEvent);
}

bool TileWorldMainWnd::HandleEvent(QObject* pObject, QEvent* pEvent)
{
	if (!m_bSetupUi) return false;
	
	QEvent::Type eType = pEvent->type();
	// if (eType == QEvent::LayoutRequest) puts("QEvent::LayoutRequest");

	switch (eType)
	{
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
		{
			QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);

			int nQtKey = pKeyEvent->key();
			if (nQtKey == Qt::Key_Backtab)
				nQtKey = Qt::Key_Tab;

			int nTWKey = -1;
			if (nQtKey >= 0  &&  nQtKey <= 0xFF)
				nTWKey = tolower(nQtKey);
			else if (nQtKey >= 0x01000000  &&  nQtKey <= 0x01000060)
				nTWKey = TWK_FUDGE(nQtKey);
			else
				return false;
			// Completely ignore multimedia keys, etc. and don't consume them
				
			bool bPress = (eType == QEvent::KeyPress);
			m_nKeyState[nTWKey] = bPress;
			// Always record the application key state
				
			bool bConsume = (m_pMainWidget->currentIndex() == PAGE_GAME) &&
							(QApplication::activeModalWidget() == 0);
			// Only consume keys for the game, not for the tables or the message boxes
			//  with the exception of a few keys for the table
			if (pObject == m_pTblList  &&  bPress  &&  pKeyEvent->modifiers() == Qt::NoModifier)
			{
				bConsume = true;
				if (nQtKey == Qt::Key_Home)
					m_pTblList->selectRow(0);
				else if (nQtKey == Qt::Key_End)
					m_pTblList->selectRow(m_pSortFilterProxyModel->rowCount()-1);
				else if (nQtKey == Qt::Key_Escape)
					g_pApp->exit(CmdQuitLevel);
				else
					bConsume = false;
			}

			if (m_bKbdRepeatEnabled || !pKeyEvent->isAutoRepeat())
			{
				// if (bConsume)
				// Always pass the key events so that the game's internal key state is in sync
				//  otherwise, Ctrl/Shift remain "pressed" after Ctrl+S / Shift+Tab / etc.
				{
					// printf("nTWKey=0x%03X bPress=%d bConsume=%d\n", nTWKey, int(bPress), int(bConsume));
					keyeventcallback(nTWKey, bPress);
				}
			}
			return bConsume;
		}
		break;
		
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		{
			if (pObject != m_pGameWidget)
				return false;
			QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
			mouseeventcallback(pMouseEvent->x(), pMouseEvent->y(), pMouseEvent->button(),
				(eType == QEvent::MouseButtonPress));
			return true;
		}
		break;
		
		case QEvent::Wheel:
		{
			if (pObject != m_pGameWidget)
				return false;
			QWheelEvent* pWheelEvent = static_cast<QWheelEvent*>(pEvent);
			mouseeventcallback(pWheelEvent->x(), pWheelEvent->y(),
				(pWheelEvent->delta() > 0 ? TW_BUTTON_WHEELUP : TW_BUTTON_WHEELDOWN),
				true);
			return true;
		}
		break;

		default:
			break;
	}
	
	return false;
}


extern "C" uint8_t* TW_GetKeyState(int* pnNumKeys)
{
	return g_pMainWnd->GetKeyState(pnNumKeys);
}

uint8_t* TileWorldMainWnd::GetKeyState(int* pnNumKeys)
{
	if (pnNumKeys != 0)
		*pnNumKeys = TWK_LAST;
	return m_nKeyState;
}


void TileWorldMainWnd::PulseKey(int nTWKey)
{
	keyeventcallback(nTWKey, true);
	keyeventcallback(nTWKey, false);
}


void TileWorldMainWnd::OnPlayback()
{
	int nTWKey = m_bReplay ? TWC_PAUSEGAME : TWC_PLAYBACK;
	PulseKey(nTWKey);
}


#if OLD_TIMER_IMPL

/*
 * Timer functions.
 */

void TileWorldMainWnd::timerEvent(QTimerEvent* pTimerEvent)
{
	if (!m_bTimerStarted)
		return;
		
	++m_nTickCount;
	m_timer.start(m_nMSPerSecond/TICKS_PER_SECOND, this);
}


/* Control the timer depending on the value of action. A negative
 * value turns off the timer if it is running and resets the counter
 * to zero. A zero value turns off the timer but does not reset the
 * counter. A positive value starts (or resumes) the timer.
 */
void settimer(int action)
{
	g_pMainWnd->SetTimer(action);
}

void TileWorldMainWnd::SetTimer(int action)
{
	if (action > 0)
	{
		m_bTimerStarted = true;
		m_timer.start(m_nMSPerSecond/TICKS_PER_SECOND, this);
	}
	else
	{
		m_bTimerStarted = false;
		m_timer.stop();
		if (action < 0)
			m_nTickCount = 0;
	}
}


/* Set the length (in real time) of a second of game time. A value of
 * zero selects the default of 1000 milliseconds.
 */
void settimersecond(int ms)
{
	g_pMainWnd->SetTimerSecond(ms);
}

void TileWorldMainWnd::SetTimerSecond(int nMS)
{
	if (nMS <= 0)
		m_nMSPerSecond = 1000;
	else
		m_nMSPerSecond = nMS;
}


/* Return the number of ticks since the timer was last reset.
 */
int gettickcount(void)
{
	return g_pMainWnd->GetTickCount();
}

int TileWorldMainWnd::GetTickCount()
{
	return m_nTickCount;
}


/* Put the program to sleep until the next timer tick.
 */
int waitfortick(void)
{
	return g_pMainWnd->WaitForTick();
}

bool TileWorldMainWnd::WaitForTick()
{
	if (!m_bTimerStarted) return false;
	
	int nTickCount = m_nTickCount;
	do
	{
		QApplication::processEvents(QEventLoop::WaitForMoreEvents|QEventLoop::ExcludeUserInputEvents, 5);
	} while (m_nTickCount == nTickCount);
	return true;
}


/* Force the timer to advance to the next tick.
 */
int advancetick(void)
{
	return g_pMainWnd->AdvanceTick();
}

int TileWorldMainWnd::AdvanceTick()
{
	return ++m_nTickCount;
}

#endif


/*
 * Keyboard input functions.
 */
 
/* Turn keyboard repeat on or off. If enable is TRUE, the keys other
 * than the direction keys will repeat at the standard rate.
 */
int setkeyboardrepeat(int enable)
{
	return g_pMainWnd->SetKeyboardRepeat(enable);
}

bool TileWorldMainWnd::SetKeyboardRepeat(bool bEnable)
{
	m_bKbdRepeatEnabled = bEnable;
	return true;
}


/*
 * Video output functions.
 */

/* Create a display surface appropriate to the requirements of the
 * game (e.g., sized according to the tiles and the font). FALSE is
 * returned on error.
 */
int creategamedisplay(void)
{
	return g_pMainWnd->CreateGameDisplay();
}

bool TileWorldMainWnd::CreateGameDisplay()
{
	TW_FreeSurface(m_pSurface);
	TW_FreeSurface(m_pInvSurface);
	
	int w = NXTILES*geng.wtile, h = NYTILES*geng.htile;
	m_pSurface = static_cast<Qt_Surface*>(TW_NewSurface(w, h, false));
	m_pInvSurface = static_cast<Qt_Surface*>(TW_NewSurface(4*geng.wtile, 2*geng.htile, false));

	m_pGameWidget->setPixmap(m_pSurface->GetPixmap());
	m_pObjectsWidget->setPixmap(m_pInvSurface->GetPixmap());

	m_pGameWidget->setFixedSize(m_pSurface->GetPixmap().size());
	m_pObjectsWidget->setFixedSize(m_pInvSurface->GetPixmap().size());

	geng.screen = m_pSurface;
	m_disploc = TW_Rect(0, 0, w, h);
	geng.maploc = m_pGameWidget->geometry();
	
	SetCurrentPage(PAGE_GAME);

	m_pControlsFrame->setVisible(true);
	// this->adjustSize();
	// this->resize(minimumSizeHint());
	// TODO: not working!

	return true;
}


void TileWorldMainWnd::SetCurrentPage(Page ePage)
{
	m_pMainWidget->setCurrentIndex(ePage);
	m_pMenuBar->setVisible(ePage == PAGE_GAME);
}



/* Select the colors used for drawing the display background, normal
 * text, bold (highlighted) text, and dim (grayed) text. The color
 * values are of the form 0x00RRGGBB.
 */
void setcolors(long bkgnd, long text, long bold, long dim)
{
	// N/A?
}


/* Fill the display with the background color.
 */
void cleardisplay(void)
{
	g_pMainWnd->ClearDisplay();
}

void TileWorldMainWnd::ClearDisplay()
{
	// TODO?
	geng.mapvieworigin = -1;
}


/* Display the current game state. timeleft and besttime provide the
 * current time on the clock and the best time recorded for the level,
 * measured in seconds. All other data comes from the gamestate
 * structure.
 */
int displaygame(gamestate const *state, int timeleft, int besttime)
{
	return g_pMainWnd->DisplayGame(state, timeleft, besttime);
}

bool TileWorldMainWnd::DisplayGame(const gamestate* pState, int nTimeLeft, int nBestTime)
{
	m_nTimeLeft = nTimeLeft;

	bool bInit = (pState->currenttime == -1);
	if (bInit)
	{
		m_nRuleset = pState->ruleset;
		m_nLevelNum = pState->game->number;
		m_bProblematic = false;
		m_nBestTime = nBestTime;
		m_bReplay = false;	// IMPORTANT for OnSpeedValueChanged
		SetSpeed(0);	// IMPORTANT
		
		m_pGameWidget->setCursor(m_nRuleset==Ruleset_MS ? Qt::CrossCursor : Qt::ArrowCursor);
		
		/*
		QString sNumber = QString::number(pState->game->number);
		m_pLblNumber->setText(sNumber);
		*/
		m_pLCDNumber->display(pState->game->number);
		// m_pLCDNumber->setVisible(true);

		QString sTitle = pState->game->name;
		m_pLblTitle->setText(sTitle);
		Qt::AlignmentFlag halign = (m_pLblTitle->sizeHint().width() <= m_pLblTitle->width()) ? Qt::AlignHCenter : Qt::AlignLeft;
		m_pLblTitle->setAlignment(halign | Qt::AlignVCenter);
		
		m_pLblPassword->setText(pState->game->passwd);
		m_pLblPassword->setVisible(false);	//
		
		m_bOFNT = (sTitle.toUpper() == "YOU CAN'T TEACH AN OLD FROG NEW TRICKS");
			
		m_pSldSeek->setValue(0);
		bool bHasSolution = hassolution(pState->game);
		m_pControlsFrame->setVisible(bHasSolution);
		
		menu_Game->setEnabled(true);
		menu_Solution->setEnabled(bHasSolution);

		m_pPrgTime->setPar(-1);
		
		bool bParBad = (pState->game->sgflags & SGF_REPLACEABLE) != 0;
		m_pPrgTime->setParBad(bParBad);
		const char* a = bParBad ? " *" : "";
		
		if (nTimeLeft == TIME_NIL)
		{
			// m_pPrgTime->setTextVisible(false);
			if (nBestTime == TIME_NIL)
				m_pPrgTime->setFormat("---");
			else
			{
				m_pPrgTime->setFormat("(" + QString::number(nBestTime) + a + ") / ---");
				m_pSldSeek->setMaximum(999-nBestTime);
			}
			m_pPrgTime->setMaximum(999);
			m_pPrgTime->setValue(999);
		}
		else
		{
			if (nBestTime == TIME_NIL)
			{
				m_pPrgTime->setFormat("%v");
			}
			else
			{
				m_pPrgTime->setFormat(QString::number(nBestTime) + a + " / %v");
				m_pPrgTime->setPar(nBestTime);
				m_pSldSeek->setMaximum(nTimeLeft-nBestTime);
			}
			m_pPrgTime->setMaximum(pState->game->time);
			m_pPrgTime->setValue(nTimeLeft);
			// m_pPrgTime->setTextVisible(true);
		}
		
		m_pLblHint->clear();
		m_bHintShown = false;
		
		CheckForProblems(pState);
		
		Narrate(&CCX::Level::txtPrologue);
	}
	else
	{
		// m_pLCDNumber->setVisible(false);
		m_pLblPassword->setVisible(false);
		m_bReplay = (pState->replay >= 0);
		m_pControlsFrame->setVisible(m_bReplay);
		if (m_bProblematic)
		{
			m_pLblHint->clear();
			m_bProblematic = false;
		}

		menu_Game->setEnabled(false);
		menu_Solution->setEnabled(false);
	}

	if (pState->statusflags & SF_SHUTTERED)
	{
		DisplayShutter();
	}
	else
	{
		DisplayMapView(pState);
	}
	
	for (int i = 0; i < 4; ++i)
	{
		drawfulltileid(m_pInvSurface, i*geng.wtile, 0,
			(pState->keys[i] ? Key_Red+i : Empty));
		drawfulltileid(m_pInvSurface, i*geng.wtile, geng.htile,
			(pState->boots[i] ? Boots_Ice+i : Empty));
	}
	m_pObjectsWidget->setPixmap(m_pInvSurface->GetPixmap());

	m_pLCDChipsLeft->display(pState->chipsneeded);
	
	if (nTimeLeft != TIME_NIL)
	{
		m_pPrgTime->setValue(nTimeLeft);
		if (nBestTime != TIME_NIL  &&  !bInit)
		{
			// QString sFormat = QString::number(nBestTime) + " / %v";
			QString sFormat;
			sFormat.sprintf("%%v (%+d)", nTimeLeft-nBestTime);
			m_pPrgTime->setFormat(sFormat);
		}
	}
	
	if (m_bReplay && !m_pSldSeek->isSliderDown())
	{
		m_pSldSeek->blockSignals(true);
		m_pSldSeek->setValue(pState->currenttime / TICKS_PER_SECOND);
		m_pSldSeek->blockSignals(false);
	}

	if (!m_bProblematic)
	{
		bool bShowHint = (pState->statusflags & SF_SHOWHINT) != 0;
		if (bShowHint != m_bHintShown)
		{
			// Call setText / clear only when really required
			// See comments about QLabel in TWDisplayWidget.h
			if (bShowHint)
				m_pLblHint->setText(pState->hinttext);
			else
				m_pLblHint->clear();
			m_bHintShown = bShowHint;
		}
	}

	if (!m_pLblShortMsg->text().isEmpty())
	{
		uint32_t nCurTime = TW_GetTicks();
		if (nCurTime > m_nMsgUntil)
			m_pLblShortMsg->clear();
		else if (nCurTime > m_nMsgBoldUntil)
			m_pLblShortMsg->setForegroundRole(QPalette::Text);
	}

	return true;
}

void TileWorldMainWnd::CheckForProblems(const gamestate* pState)
{
	QString s;

	if (pState->statusflags & SF_INVALID)
	{
		s = "This level cannot be played.";
	}
	else if (pState->game->unsolvable)
	{
		s = "This level is reported to be unsolvable";
		if (*pState->game->unsolvable)
			s += ": " + QString(pState->game->unsolvable);
		s += ".";
	}
	else
	{
		CCX::RulesetCompatibility ruleCompat = m_ccxLevelset.vecLevels[m_nLevelNum].ruleCompat;
		CCX::Compatibility compat = CCX::COMPAT_UNKNOWN;
		if (m_nRuleset == Ruleset_Lynx)
		{
			if (pedanticmode)
				compat = ruleCompat.ePedantic;
			else
				compat = ruleCompat.eLynx;
		}
		else if (m_nRuleset == Ruleset_MS)
		{
			compat = ruleCompat.eMS;
		}
		if (compat == CCX::COMPAT_NO)
		{
			s = "This level is flagged as being incompatible with the current ruleset.";
		}
	}

	m_bProblematic = !s.isEmpty();
	if (m_bProblematic)
	{
		m_pLblHint->setText(s);
	}
}

void TileWorldMainWnd::DisplayMapView(const gamestate* pState)
{
	short xviewpos = pState->xviewpos;
	short yviewpos = pState->yviewpos;
	bool bFrogShow = (m_bOFNT  &&  m_bReplay  &&
	                  xviewpos/8 == 14  &&  yviewpos/8 == 9);
	if (bFrogShow)
	{
		int x = xviewpos, y = yviewpos;
		if (m_nRuleset == Ruleset_MS)
		{
			for (int pos = 0; pos < CXGRID*CYGRID; ++pos)
			{
				int id = pState->map[pos].top.id;
				if ( ! (id >= Teeth && id < Teeth+4) )
					continue;
				x = (pos % CXGRID) * 8;
				y = (pos / CXGRID) * 8;
				break;
			}
		}
		else
		{
			for (const creature* p = pState->creatures; p->id != 0; ++p)
			{
				if ( ! (p->id >= Teeth && p->id < Teeth+4) )
					continue;
				x = (p->pos % CXGRID) * 8;
				y = (p->pos / CXGRID) * 8;
				if (p->moving > 0)
				{
					switch (p->dir)
					{
					  case NORTH:	y += p->moving;	break;
					  case WEST:	x += p->moving;	break;
					  case SOUTH:	y -= p->moving;	break;
					  case EAST:	x -= p->moving;	break;
					}
				}
				break;
			}
		}
		const_cast<gamestate*>(pState)->xviewpos = x;
		const_cast<gamestate*>(pState)->yviewpos = y;
	}

	displaymapview(pState, m_disploc);
	m_pGameWidget->setPixmap(m_pSurface->GetPixmap());
	
	if (bFrogShow)
	{
		const_cast<gamestate*>(pState)->xviewpos = xviewpos;
		const_cast<gamestate*>(pState)->yviewpos = yviewpos;
	}
}

void TileWorldMainWnd::DisplayShutter()
{
	QPixmap pixmap(m_pGameWidget->size());
	pixmap.fill(Qt::black);

	QPainter painter(&pixmap);
	painter.setPen(Qt::red);
	QFont font;
	font.setPixelSize(geng.htile);
	painter.setFont(font);
	painter.drawText(pixmap.rect(), Qt::AlignCenter, "Paused");
	painter.end();

	m_pGameWidget->setPixmap(pixmap);
}


void TileWorldMainWnd::OnSpeedValueChanged(int nValue)
{
	// IMPORTANT!
	if (!m_bReplay) return;
	// Even though the replay controls are hidden when play begins,
	//  the slider could be manipulated before making the first move
	
	SetSpeed(nValue);
}
	
void TileWorldMainWnd::SetSpeed(int nValue)
{
	int nMS = (m_nRuleset == Ruleset_MS) ? 1100 : 1000;
	if (nValue >= 0)
		settimersecond(nMS >> nValue);
	else
		settimersecond(nMS << (-nValue/2));
}

void TileWorldMainWnd::OnSpeedSliderReleased()
{
	m_pSldSpeed->setValue(0);
}


/* Get number of seconds to skip at start of playback.
 */
int getreplaysecondstoskip(void)
{
	return g_pMainWnd->GetReplaySecondsToSkip();
}

int TileWorldMainWnd::GetReplaySecondsToSkip() const
{
	return m_pSldSeek->value();
}


void TileWorldMainWnd::OnSeekPosChanged(int nValue)
{
	if (!m_bReplay) return;
	PulseKey(TWC_SEEK);
}


/* Display a short message appropriate to the end of a level's game
 * play. If the level was completed successfully, completed is TRUE,
 * and the other three arguments define the base score and time bonus
 * for the level, and the user's total score for the series; these
 * scores will be displayed to the user.
 */
int displayendmessage(int basescore, int timescore, long totalscore,
			     int completed)
{
	return g_pMainWnd->DisplayEndMessage(basescore, timescore, totalscore, completed);
}

struct TWMessage
{
	enum Type {TW_INFO=0, TW_QUESTION, TW_WARNING, TW_ERROR};

	Type eType;
	const char* szText;
	
	// static const QMessageBox::Icon s_eIcon[];
	static const QStyle::StandardPixmap s_eStdIcon[];
	static const char* const s_szTitle[];
};

/*
const QMessageBox::Icon TWMessage::s_eIcon[] =
	{QMessageBox::Information, QMessageBox::Question, QMessageBox::Warning, QMessageBox::Critical};
*/

const QStyle::StandardPixmap TWMessage::s_eStdIcon[] =
	{QStyle::SP_MessageBoxInformation, QStyle::SP_MessageBoxQuestion, QStyle::SP_MessageBoxWarning, QStyle::SP_MessageBoxCritical};

const char* const TWMessage::s_szTitle[] =
	{"Information", "Question", "Warning", "Error"};
	
#define COUNTOF(a) (sizeof(a) / sizeof(a[0]))
	
static const TWMessage g_msgDeath[] =
{
	{TWMessage::TW_INFO,     "Whoops... Let's try again."},
	{TWMessage::TW_QUESTION, "Why don't ya watch where you're going?"},
	{TWMessage::TW_WARNING,  "Getting killed can be injurious to Chip's health!"},
	{TWMessage::TW_ERROR,    "Uh-oh: Chip performed a fatal operation and was terminated"},
	{TWMessage::TW_WARNING,  "Great, now look what you did!"},
	{TWMessage::TW_QUESTION, "Hey, are you doing that on purpose?"}
};

static const TWMessage g_msgTimeout[] =
{
	{TWMessage::TW_INFO,     "Well, that was an untimely demise."},
	{TWMessage::TW_QUESTION, "You do know there's a time limit on this level, right?"},
	{TWMessage::TW_WARNING,  "Look, we don't have all the time in the world!"},
	{TWMessage::TW_ERROR,    "Alert: The system has determined that you are either moving or thinking too slowly"}
};

static const char* const g_szMsgCompletion[] =
{
	"Congratulations!",
	"Well done!",
	"Good work!"
};

int TileWorldMainWnd::DisplayEndMessage(int nBaseScore, int nTimeScore, long lTotalScore, int nCompleted)
{
	if (nCompleted == 0)
		return CmdNone;
		
	if (nCompleted == -2)	// abandoned
		return CmdNone;
		
	QMessageBox msgBox(this);
	// msgBox.setFont(m_pLblTitle->font());
	//? msgBox.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	//? msgBox.setMinimumWidth(320);
	
	if (nCompleted > 0)	// Success
	{
		QString sTitle = m_pLblTitle->text();
		QString sAuthor = m_ccxLevelset.vecLevels[m_nLevelNum].sAuthor;
		const char* szMsg = 0;
		if (m_bReplay)
			szMsg = "Alright!";
		else
			szMsg = g_szMsgCompletion[m_nNextCompletionMsg++ % COUNTOF(g_szMsgCompletion)];
		
		QString sText;
		QTextStream strm(&sText);
		strm.setLocale(m_locale);
		strm
			<< "<table>"
			// << "<tr><td><hr></td></tr>"
			<< "<tr><td><big><b>" << sTitle << "</b></big></td></tr>"
			// << "<tr><td><hr></td></tr>"
			;
		if (!sAuthor.isEmpty())
			strm << "<tr><td>by " << sAuthor << "</td></tr>";
		strm
			<< "<tr><td><hr></td></tr>"
			<< "<tr><td>&nbsp;</td></tr>"
			<< "<tr><td><big><b>" << szMsg << "</b></big>" << "</td></tr>"
			;

		if (!m_bReplay)
		{
		  if (m_nTimeLeft != TIME_NIL  &&  m_nBestTime != TIME_NIL)
		  {
			strm << "<tr><td>";
			if (m_nTimeLeft > m_nBestTime)
			{
				strm << "You made it " << (m_nTimeLeft - m_nBestTime) << " second(s) faster this time!";
			}
			else if (m_nTimeLeft == m_nBestTime)
			{
				strm << "You scored " << m_nBestTime << " yet again.";
			}
			else
			{
				strm << "But not as quick as your previous score of " << m_nBestTime << "...";
			}
			strm << "</td></tr>";
		  }

		  strm
			<< "<tr><td>&nbsp;</td></tr>"
			<< "<tr><td><table>"
			  << "<tr><td>Time Bonus:</td><td align='right'>"  << nTimeScore << "</td></tr>"
			  << "<tr><td>Level Bonus:</td><td align='right'>" << nBaseScore << "</td></tr>"
			  << "<tr><td>Level Score:</td><td align='right'>" << (nTimeScore + nBaseScore) << "</td></tr>"
			  << "<tr><td colspan='2'><hr></td></tr>"
			  << "<tr><td>Total Score:</td><td align='right'>" << lTotalScore << "</td></tr>"
			<< "</table></td></tr>"
			// << "<tr><td><pre>                                </pre></td></tr>"	// spacer
			<< "</table>"
			;
		}

		msgBox.setTextFormat(Qt::RichText);
		msgBox.setText(sText);

		Qt_Surface* pSurface = static_cast<Qt_Surface*>(TW_NewSurface(geng.wtile, geng.htile, false));
		drawfulltileid(pSurface, 0, 0, Exited_Chip);
		msgBox.setIconPixmap(pSurface->GetPixmap());
		TW_FreeSurface(pSurface);
		
		msgBox.setWindowTitle(m_bReplay ? "Replay Completed" : "Level Completed");
		
		m_sTextToCopy = "#" + QString::number(m_nLevelNum)
			+ " (" + sTitle + "): "
			+ (m_nTimeLeft == TIME_NIL ? "---" : QString::number(m_nTimeLeft))
			+ "\n";
		
		msgBox.addButton("&Onward!", QMessageBox::AcceptRole);
		QPushButton* pBtnRestart = msgBox.addButton("&Restart", QMessageBox::AcceptRole);
		QPushButton* pBtnCopyScore = msgBox.addButton("&Copy Score", QMessageBox::ActionRole);
		connect( pBtnCopyScore, SIGNAL(clicked()), this, SLOT(OnCopyText()) );
		
		msgBox.exec();
		if (msgBox.clickedButton() == pBtnRestart)
			return CmdSameLevel;
			
		// if (!m_bReplay)
			Narrate(&CCX::Level::txtEpilogue);
	}
	else	// Failure
	{
		bool bTimeout = (m_nTimeLeft != TIME_NIL  &&  m_nTimeLeft <= 0);
		if (m_bReplay)
		{
			QString sMsg = "Whoa!  Chip ";
			if (bTimeout)
				sMsg += "ran out of time";
			else
				sMsg += "ran into some trouble";
			// TODO: What about when Chip just doesn't reach the exit or reaches the exit too early?
			sMsg += " there.\nIt looks like the level has changed after that solution was recorded.";
			msgBox.setText(sMsg);
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setWindowTitle("Replay Failed");
		}
		else
		{
			const TWMessage* pMsg = 0;
			if (bTimeout)
				pMsg = &g_msgTimeout[m_nNextTimeoutMsg++ % COUNTOF(g_msgTimeout)];
			else
				pMsg = &g_msgDeath[m_nNextDeathMsg++  % COUNTOF(g_msgDeath)];
			
			msgBox.setText(pMsg->szText);
			// msgBox.setIcon(TWMessage::s_eIcon[pMsg->eType]);
			// setIcon also causes the corresponding system sound to play
			// setIconPixmap does not
			QStyle* pStyle = QApplication::style();
			if (pStyle != 0)
			{
				QIcon icon = pStyle->standardIcon(TWMessage::s_eStdIcon[pMsg->eType]);
				msgBox.setIconPixmap(icon.pixmap(48));
			}
			msgBox.setWindowTitle(TWMessage::s_szTitle[pMsg->eType]);
		}
		msgBox.exec();
	}
	
	return CmdProceed;
}


/* Display a (very short) message for the given number of
 * milliseconds. bold indicates the number of milliseconds the
 * message is with highlighting. After that (if the message is
 * still visible) it is rendered as normal text.
 */
int setdisplaymsg(char const *msg, int msecs, int bold)
{
	return g_pMainWnd->SetDisplayMsg(msg, msecs, bold);
}

bool TileWorldMainWnd::SetDisplayMsg(const char* szMsg, int nMSecs, int nBoldMSecs)
{
	uint32_t nCurTime = TW_GetTicks();
	m_nMsgUntil = nCurTime + nMSecs;
	m_nMsgBoldUntil = nCurTime + nBoldMSecs;
	
	m_pLblShortMsg->setForegroundRole(QPalette::BrightText);
	m_pLblShortMsg->setText(szMsg);
	
	return true;
}


/* Display a scrollable table. title provides a title to display. The
 * table's first row provides a set of column headers which will not
 * scroll. index points to the index of the item to be initially
 * selected; upon return, the value will hold the current selection.
 * Either listtype or inputcallback must be used to tailor the UI.
 * listtype specifies the type of list being displayed.
 * inputcallback points to a function that is called to retrieve
 * input. The function is passed a pointer to an integer. If the
 * callback returns TRUE, this integer should be set to either a new
 * index value or one of the following enum values. This value will
 * then cause the selection to be changed, whereupon the display will
 * be updated before the callback is called again. If the callback
 * returns FALSE, the table is removed from the display, and the value
 * stored in the integer will become displaylist()'s return value.
 */
int displaylist(char const *title, tablespec const *table, int *index,
		       DisplayListType listtype, int (*inputcallback)(int*))
{
	return g_pMainWnd->DisplayList(title, table, index, listtype, inputcallback);
}

int TileWorldMainWnd::DisplayList(const char* szTitle, const tablespec* pTableSpec, int* pnIndex,
		DisplayListType eListType, int (*pfnInputCallback)(int*))
{
  int nCmd = 0;
  
  // dummy scope to force model destructors before ExitTWorld
  {
	TWTableModel model;
	model.SetTableSpec(pTableSpec);
	QSortFilterProxyModel proxyModel;
	m_pSortFilterProxyModel = &proxyModel;
	proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
	proxyModel.setFilterKeyColumn(-1);
	proxyModel.setSourceModel(&model);
	m_pTblList->setModel(&proxyModel);
	
	QModelIndex index = proxyModel.mapFromSource(model.index(*pnIndex, 0));
	m_pTblList->setCurrentIndex(index);
	m_pTblList->resizeColumnsToContents();
	m_pTblList->resizeRowsToContents();
	m_pTxtFind->clear();
	SetCurrentPage(PAGE_TABLE);
	m_pTblList->setFocus();
	
	nCmd = g_pApp->exec();
	// if (m_bWindowClosed) g_pApp->ExitTWorld();
	*pnIndex = proxyModel.mapToSource(m_pTblList->currentIndex()).row();

	SetCurrentPage(PAGE_GAME);
	m_pTblList->setModel(0);
	m_pSortFilterProxyModel = 0;
  }
	
  if (m_bWindowClosed) g_pApp->ExitTWorld();

  return nCmd;
}

void TileWorldMainWnd::OnListItemActivated(const QModelIndex& index)
{
	g_pApp->exit(CmdProceed);
}

void TileWorldMainWnd::OnFindTextChanged(const QString& sText)
{
	if (!m_pSortFilterProxyModel) return;
	
	QString sWildcard;
	if (sText.isEmpty())
		sWildcard = "*";
	else
		sWildcard = '*' + sText + '*';
	m_pSortFilterProxyModel->setFilterWildcard(sWildcard);
}

void TileWorldMainWnd::OnFindReturnPressed()
{
	if (!m_pSortFilterProxyModel) return;

	int n = m_pSortFilterProxyModel->rowCount();
	if (n == 0)
	{
		ding();
		return;
	}

	m_pTblList->setFocus();

	if (!m_pTblList->currentIndex().isValid())
		m_pTblList->selectRow(0);

	if (n == 1)
		g_pApp->exit(CmdProceed);
}


/* Display an input prompt to the user. prompt supplies the prompt to
 * display, and input points to a buffer to hold the user's input.
 * maxlen sets a maximum length to the input that will be accepted.
 * Either inputtype or inputcallback must be used to validate input.
 * inputtype indicates the type of input desired.
 * The supplied callback function is called repeatedly to obtain
 * input. If the callback function returns a printable ASCII
 * character, the function will automatically append it to the string
 * stored in input. If '\b' is returned, the function will erase the
 * last character in input, if any. If '\f' is returned the function
 * will set input to "". If '\n' is returned, the input prompt is
 * erased and displayinputprompt() returns TRUE. If a negative value
 * is returned, the input prompt is erased and displayinputprompt()
 * returns FALSE. All other return values from the callback are
 * ignored.
 */
int displayinputprompt(char const *prompt, char *input, int maxlen,
			      InputPromptType inputtype, int (*inputcallback)(void))
{
	return g_pMainWnd->DisplayInputPrompt(prompt, input, maxlen, inputtype, inputcallback);
}

int TileWorldMainWnd::DisplayInputPrompt(const char* szPrompt, char* pInput, int nMaxLen,
		InputPromptType eInputType, int (*pfnInputCallback)())
{
	switch (eInputType)
	{
		case INPUT_YESNO:
		{
			QMessageBox::StandardButton eBtn = QMessageBox::question(
				this, TileWorldApp::s_szTitle, szPrompt, QMessageBox::Yes|QMessageBox::No);
			pInput[0] = (eBtn==QMessageBox::Yes) ? 'Y' : 'N';
			pInput[1] = '\0';
			return true;
		}
		
		case INPUT_ALPHA:
		default:
		{
			// TODO: proper validation, maybe embedded prompt
			QString sText = QInputDialog::getText(this, TileWorldApp::s_szTitle, szPrompt);
			if (sText.isEmpty())
				return false;
			sText.truncate(nMaxLen);
			if (eInputType == INPUT_ALPHA)
				sText = sText.toUpper();
			strcpy(pInput, sText.toAscii().constData());
			return true;
		}
	}
}


#if DUMMY_SFX_IMPL

/*
 * Sound functions.
 */

/* Activate or deactivate the sound system. The return value is TRUE
 * if the sound system is (or already was) active.
 */
int setaudiosystem(int active)
{
	return false;
}


/* Specify the sounds effects to be played at this time. sfx is the
 * bitwise-or of any number of sound effects. If a non-continuous
 * sound effect in sfx is already playing, it will be restarted. Any
 * continuous sound effects that are currently playing that are not
 * set in sfx will stop playing.
 */
void playsoundeffects(unsigned long sfx)
{
}


/* Control sound-effect production depending on the value of action.
 * A negative value turns off all sound effects that are playing. A
 * zero value temporarily suspends the playing of sound effects. A
 * positive value continues the sound effects at the point at which
 * they were suspended.
 */
void setsoundeffects(int action)
{
}


/* Set the current volume level. Volume ranges from 0 (silence) to 10
 * (the default). Setting the sound to zero causes sound effects to be
 * displayed as textual onomatopoeia. If display is TRUE, the new
 * volume level will be displayed to the user. FALSE is returned if
 * the sound system is not currently active.
 */
int setvolume(int volume, int display)
{
	return false;
}

/* Alters the current volume level by delta.
 */
int changevolume(int delta, int display)
{
	return true;
}

#endif


/*
 * Miscellaneous functions.
 */

/* Ring the bell.
 */
void ding(void)
{
	QApplication::beep();
}


/* Set the program's subtitle. A NULL subtitle is equivalent to the
 * empty string. The subtitle is displayed in the window dressing (if
 * any).
 */
void setsubtitle(char const *subtitle)
{
	g_pMainWnd->SetSubtitle(subtitle);
}

void TileWorldMainWnd::SetSubtitle(const char* szSubtitle)
{
	QString sTitle = TileWorldApp::s_szTitle;
	if (szSubtitle && *szSubtitle)
		sTitle += " - " + QString(szSubtitle);
	setWindowTitle(sTitle);
}


/* Display a message to the user. cfile and lineno can be NULL and 0
 * respectively; otherwise, they identify the source code location
 * where this function was called from. prefix is an optional string
 * that is displayed before and/or apart from the body of the message.
 * fmt and args define the formatted text of the message body. action
 * indicates how the message should be presented. NOTIFY_LOG causes
 * the message to be displayed in a way that does not interfere with
 * the program's other activities. NOTIFY_ERR presents the message as
 * an error condition. NOTIFY_DIE should indicate to the user that the
 * program is about to shut down.
 */
void usermessage(int action, char const *prefix,
                 char const *cfile, unsigned long lineno,
                 char const *fmt, va_list args)
{
	fprintf(stderr, "%s: ", action == NOTIFY_DIE ? "FATAL" :
	                        action == NOTIFY_ERR ? "error" : "warning");
	if (prefix)
		fprintf(stderr, "%s: ", prefix);
	if (fmt)
		vfprintf(stderr, fmt, args);
	if (cfile)
		fprintf(stderr, " [%s:%lu] ", cfile, lineno);
    fputc('\n', stderr);
    fflush(stderr);
}


/* Displays a screenful of (hopefully) helpful information which
 * includes tile images. title provides the title of the display. rows
 * points to an array of tiletablerow structures. count specifies the
 * size of this array. The text of each row is displayed alongside one
 * or two tile images. completed controls the prompt that the user
 * sees at the bottom of the display. A positive value will indicate
 * that more text follows. A negative value will indicate that leaving
 * this screen will return to the prior display. A value of zero will
 * indicate that the current display is the end of a sequence.
 */
int displaytiletable(char const *title, tiletablerow const *rows,
			    int count, int completed)
{
	// TODO
	return true;
}


/* Displays a screenful of (hopefully) helpful information. title
 * provides the title of the display. table points to a table that
 * contains the body of the text. completed controls the prompt that
 * the user sees at the bottom of the display; see the description of
 * displaytiletable() for details.
 */
int displaytable(char const *title, tablespec const *table,
			int completed)
{
	// TODO
	return true;
}


/* Read any additional data for the series.
 */
void readextensions(gameseries *series)
{
	if (g_pMainWnd == 0) return;	// happens during batch verify, etc.
	g_pMainWnd->ReadExtensions(series);
}

void TileWorldMainWnd::ReadExtensions(gameseries* pSeries)
{
	QDir dataDir(seriesdatdir);
	QString sSetName = QFileInfo(pSeries->mapfilename).completeBaseName();
	QString sFilePath = dataDir.filePath(sSetName + ".ccx");
	
	m_ccxLevelset.Clear();
	if (!m_ccxLevelset.ReadFile(sFilePath, pSeries->count))
		warn("%s: failed to read file", sFilePath.toAscii().constData());
		
	for (int i = 1; i <= pSeries->count; ++i)
	{
		CCX::Level& rCCXLevel = m_ccxLevelset.vecLevels[i];
		rCCXLevel.txtPrologue.bSeen = false;	// @#$ (pSeries->games[i-1].sgflags & SGF_HASPASSWD) != 0;
		rCCXLevel.txtEpilogue.bSeen = false;
	}
}


void TileWorldMainWnd::Narrate(CCX::Text CCX::Level::*pmTxt, bool bForce)
{
	CCX::Text& rText = m_ccxLevelset.vecLevels[m_nLevelNum].*pmTxt;
	if (rText.bSeen && !bForce)
		return;
	rText.bSeen = true;

	if (rText.vecPages.empty())
		return;
	int n = rText.vecPages.size();

	QString sWindowTitle = this->windowTitle();
	SetSubtitle("");	// TODO: set name
	SetCurrentPage(PAGE_TEXT);
	m_pBtnTextNext->setFocus();
	
	int d = +1;
	for (int nPage = 0; nPage < n; nPage += d)
	{
		m_pBtnTextPrev->setVisible(nPage > 0);
		
		CCX::Page& rPage = rText.vecPages[nPage];
		
		m_pTextBrowser->setAlignment(rPage.pageProps.align | rPage.pageProps.valign);
		// TODO: not working!
		
		// m_pTextBrowser->setTextBackgroundColor(rPage.pageProps.bgcolor);
		// m_pTextBrowser->setTextColor(rPage.pageProps.color);
		QPalette pal = m_pTextBrowser->palette();
		pal.setColor(QPalette::Base, rPage.pageProps.bgcolor);
		pal.setColor(QPalette::Text, rPage.pageProps.color);
		m_pTextBrowser->setPalette(pal);

		QTextDocument* pDoc = m_pTextBrowser->document();
		if (pDoc != 0)
		{
			if (!m_ccxLevelset.sStyleSheet.isEmpty())
				pDoc->setDefaultStyleSheet(m_ccxLevelset.sStyleSheet);
			pDoc->setDocumentMargin(16);
		}

		QString sText = rPage.sText;
		if (rPage.pageProps.eFormat == CCX::TEXT_PLAIN)
		{
			/*
			sText.replace("&", "&amp;");
			sText.replace("<", "&lt;");
			sText.replace(">", "&gt;");
			sText.replace("\n", "<br>");
			m_pTextBrowser->setHtml(sText);
			*/
			m_pTextBrowser->setPlainText(sText);
		}
		else
		{
			m_pTextBrowser->setHtml(sText);
		}
		
		d = g_pApp->exec();
		if (m_bWindowClosed) g_pApp->ExitTWorld();
		if (d == 0)	// Return
			break;
		if (nPage+d < 0)
			d = 0;
	}

	SetCurrentPage(PAGE_GAME);
	setWindowTitle(sWindowTitle);
}

void TileWorldMainWnd::OnTextNext()
{
	g_pApp->exit(+1);
}

void TileWorldMainWnd::OnTextPrev()
{
	g_pApp->exit(-1);
}

void TileWorldMainWnd::OnTextReturn()
{
	g_pApp->exit(0);
}


void TileWorldMainWnd::OnCopyText()
{
	QClipboard* pClipboard = QApplication::clipboard();
	if (pClipboard == 0) return;
	pClipboard->setText(m_sTextToCopy);
}


void TileWorldMainWnd::OnMenuActionTriggered(QAction* pAction)
{
	int nTWKey = GetTWKeyForAction(pAction);
	if (nTWKey == TWK_dummy) return;
	PulseKey(nTWKey);
}

int TileWorldMainWnd::GetTWKeyForAction(QAction* pAction) const
{
    if (pAction == action_Scores) return TWC_SEESCORES;
    if (pAction == action_SolutionFiles) return TWC_SEESOLUTIONFILES;
    if (pAction == action_Levelsets) return TWC_QUITLEVEL;
    if (pAction == action_Exit) return TWC_QUIT;

    if (pAction == action_Begin) return TWC_PROCEED;
    if (pAction == action_Pause) return TWC_PAUSEGAME;
    if (pAction == action_Restart) return TWC_SAMELEVEL;
    if (pAction == action_Next) return TWC_NEXTLEVEL;
    if (pAction == action_Previous) return TWC_PREVLEVEL;
    if (pAction == action_GoTo) return TWC_GOTOLEVEL;

    if (pAction == action_Playback) return TWC_PLAYBACK;
    if (pAction == action_Verify) return TWC_CHECKSOLUTION;
    if (pAction == action_Replace) return TWC_REPLSOLUTION;
    if (pAction == action_Delete) return TWC_KILLSOLUTION;
    
    return TWK_dummy;
}
