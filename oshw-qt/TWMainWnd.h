/* Copyright (C) 2001-2010 by Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef TWMAINWND_H
#define TWMAINWND_H


#include "ui_TWMainWnd.h"

#include "CCMetaData.h"

#include "../generic/generic.h"

#include "../gen.h"
#include "../defs.h"
#include "../state.h"
#include "../series.h"
#include "../oshw.h"

#include <QMainWindow>

#include <QLocale>

#if OLD_TIMER_IMPL
#include <QBasicTimer>
#endif

class QSortFilterProxyModel;


class TileWorldMainWnd : public QMainWindow, protected Ui::TWMainWnd
{
	Q_OBJECT
	
public:
	enum Page
	{
		PAGE_GAME,
		PAGE_TABLE,
		PAGE_TEXT
	};

	TileWorldMainWnd(QWidget* pParent = 0, Qt::WindowFlags flags = 0);
	~TileWorldMainWnd();

	virtual bool eventFilter(QObject* pObject, QEvent* pEvent);
	virtual void closeEvent(QCloseEvent* pCloseEvent);

#if OLD_TIMER_IMPL
	virtual void timerEvent(QTimerEvent* pTimerEvent);
	void SetTimer(int action);
	void SetTimerSecond(int nMS);
	int GetTickCount();
	bool WaitForTick();
	int AdvanceTick();
#endif

	bool SetKeyboardRepeat(bool bEnable);
	uint8_t* GetKeyState(int* pnNumKeys);
	int GetReplaySecondsToSkip() const;
	
	bool CreateGameDisplay();
	void ClearDisplay();
	bool DisplayGame(const gamestate* pState, int nTimeLeft, int nBestTime);
	bool SetDisplayMsg(const char* szMsg, int nMSecs, int nBoldMSecs);
	int DisplayEndMessage(int nBaseScore, int nTimeScore, long lTotalScore, int nCompleted);
	int DisplayList(const char* szTitle, const tablespec* pTableSpec, int* pnIndex,
			DisplayListType eListType, int (*pfnInputCallback)(int*));
	int DisplayInputPrompt(const char* szPrompt, char* pInput, int nMaxLen,
			InputPromptType eInputType, int (*pfnInputCallback)());

	void SetSubtitle(const char* szSubtitle);
	
	void ReadExtensions(gameseries* pSeries);
	void Narrate(CCX::Text CCX::Level::*pmTxt, bool bForce = false);
	
	
private slots:
	void OnListItemActivated(const QModelIndex& index);
	void OnFindTextChanged(const QString& sText);
	void OnFindReturnPressed();
	void OnPlayback();
	void OnSpeedValueChanged(int nValue);
	void OnSpeedSliderReleased();	
	void OnSeekPosChanged(int nValue);
	void OnTextNext();
	void OnTextPrev();
	void OnTextReturn();
	void OnCopyText();
	void OnMenuActionTriggered(QAction* pAction);
	
private:
	bool HandleEvent(QObject* pObject, QEvent* pEvent);
	void SetCurrentPage(Page ePage);
	void CheckForProblems(const gamestate* pState);
	void DisplayMapView(const gamestate* pState);
	void DisplayShutter();
	void SetSpeed(int nValue);
	void PulseKey(int nTWKey);
	int GetTWKeyForAction(QAction* pAction) const;
	
	bool m_bSetupUi;
	bool m_bWindowClosed;
	
	Qt_Surface* m_pSurface;
	Qt_Surface* m_pInvSurface;
	TW_Rect m_disploc;
	
	uint8_t m_nKeyState[TWK_LAST];
	
#if OLD_TIMER_IMPL
	QBasicTimer m_timer;
	int m_nMSPerSecond;
	bool m_bTimerStarted;
	int m_nTickCount;
#endif

	uint32_t m_nMsgUntil, m_nMsgBoldUntil;
	
	bool m_bKbdRepeatEnabled;
	
	int m_nNextCompletionMsg, m_nNextTimeoutMsg, m_nNextDeathMsg;
	
	int m_nRuleset;
	int m_nLevelNum;
	bool m_bProblematic;
	bool m_bOFNT;
	int m_nBestTime;
	bool m_bHintShown;
	int m_nTimeLeft;
	bool m_bReplay;
	
	QSortFilterProxyModel* m_pSortFilterProxyModel;
	QLocale m_locale;
	
	CCX::Levelset m_ccxLevelset;
	
	QString m_sTextToCopy;
};


#endif
