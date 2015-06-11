// XTPSkinObjectFrame.cpp: implementation of the XTPSkinObjectFrame class.
//
// This file is a part of the XTREME SKINFRAMEWORK MFC class library.
// (c)1998-2013 Codejock Software, All Rights Reserved.
//
// THIS SOURCE FILE IS THE PROPERTY OF CODEJOCK SOFTWARE AND IS NOT TO BE
// RE-DISTRIBUTED BY ANY MEANS WHATSOEVER WITHOUT THE EXPRESSED WRITTEN
// CONSENT OF CODEJOCK SOFTWARE.
//
// THIS SOURCE CODE CAN ONLY BE USED UNDER THE TERMS AND CONDITIONS OUTLINED
// IN THE XTREME TOOLKIT PRO LICENSE AGREEMENT. CODEJOCK SOFTWARE GRANTS TO
// YOU (ONE SOFTWARE DEVELOPER) THE LIMITED RIGHT TO USE THIS SOFTWARE ON A
// SINGLE COMPUTER.
//
// CONTACT INFORMATION:
// support@codejock.com
// http://www.codejock.com
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <Common/Uxtheme.h>
#include <Common/Tmschema.h>
#include <Common/XTPColorManager.h>
#include <Common/XTPDrawHelpers.h>
#include <Common/ScrollBar/XTPScrollBase.h>
#include <Common/XTPSystemHelpers.h>
#include <Common/XTPSynchro.h>

#include "XTPSkinObject.h"
#include "XTPSkinDrawTools.h"
#include "XTPSkinManager.h"
#include "XTPSkinManagerSchema.h"
#include "XTPSkinImage.h"
#include "XTPSkinObjectFrame.h"
#include "XTPSkinManagerApiHook.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CXTPSkinObjectFrame, CWnd)

CXTPSkinObjectFrame::CCaptionButton::CCaptionButton(int nCommand, CXTPSkinObjectFrame *pFrame, UINT nHTCode, int nClassPart)
	: m_pFrame    (pFrame)
	, m_nHTCode   (nHTCode)
	, m_nClassPart(nClassPart)
	, m_nCommand  (nCommand)
	, m_bEnabled  (TRUE)
{
	m_rcButton.SetRectEmpty();
}

CXTPSkinObjectFrame::CCaptionButton::~CCaptionButton()
{
	m_pFrame  = NULL;
	m_nHTCode = UINT(HTERROR);
}

BOOL CXTPSkinObjectFrame::CCaptionButton::IsPressed() const
{
	return m_pFrame->m_pButtonPressed == this;
}

BOOL CXTPSkinObjectFrame::CCaptionButton::IsHighlighted() const
{
	return m_pFrame->m_pButtonHot == this;
}


void CXTPSkinObjectFrame::CCaptionButton::Draw(CDC* pDC, BOOL bFrameActive)
{
	BOOL bHot = IsHighlighted() && (IsPressed() || !m_pFrame->m_pButtonPressed);
	BOOL pRessed = bHot && IsPressed();

	int nState = !m_bEnabled ? SBS_DISABLED : pRessed? SBS_PUSHED: bHot? SBS_HOT: SBS_NORMAL;
	if (!bFrameActive) nState += 4;

	CXTPSkinManagerClass* pClassWindow = m_pFrame->GetSkinManager()->GetSkinClass(m_pFrame, _T("WINDOW"));
	pClassWindow->DrawThemeBackground(pDC, m_nClassPart, nState, m_rcButton);
}

/////////////////////////////////////////////////////////////////////////////
// CXTPSkinObjectFrame


CXTPThreadLocal<BOOL> CXTPSkinObjectFrame::m_bMenuStatus;

CXTPSkinObjectFrame::CXTPSkinObjectFrame()
	: m_bInUpdateRegion    (FALSE)
	, m_bInWindowPosChanged(FALSE)
{
	*m_bMenuStatus = FALSE;

	m_rcBorders.SetRectEmpty();
	m_pSBTrack = NULL;

	m_strClassName = _T("WINDOW");
	m_nCtlColorMessage = WM_CTLCOLORDLG;

	m_bMDIClient = FALSE;

	ZeroMemory(&m_spi[SB_VERT], sizeof(XTP_SKINSCROLLBAR_POSINFO));
	ZeroMemory(&m_spi[SB_HORZ], sizeof(XTP_SKINSCROLLBAR_POSINFO));

	m_spi[SB_VERT].fVert = TRUE;
	m_spi[SB_VERT].fSizebox = FALSE;
	m_spi[SB_VERT].nBar= SB_VERT;

	m_spi[SB_HORZ].fVert = FALSE;
	m_spi[SB_HORZ].fSizebox = FALSE;
	m_spi[SB_HORZ].nBar = SB_HORZ;

	m_szFrameRegion = CSize(0, 0);

	m_pButtonHot = NULL;
	m_pButtonPressed = NULL;
	m_bActive = FALSE;

	m_dwExStyle = (DWORD)-1;
	m_dwStyle = (DWORD)-1;

	m_bActiveX = FALSE;

	m_bRegionChanged = FALSE;
	m_bLockFrameDraw = FALSE;


	m_dwDialogTexture = ETDT_DISABLE;

	m_pMDIClient = NULL;
}

CXTPSkinObjectFrame::~CXTPSkinObjectFrame()
{
	RemoveButtons();
}

void CXTPSkinObjectFrame::ScreenToFrame(LPPOINT lpPoint)
{
	CXTPDrawHelpers::ScreenToWindow(this, lpPoint);
}

void CXTPSkinObjectFrame::ClientToFrame(LPPOINT lpPoint)
{
	ClientToScreen(lpPoint);
	ScreenToFrame(lpPoint);
}


void CXTPSkinObjectFrame::AdjustFrame(CRect& rc)
{
	rc.DeflateRect(m_rcBorders);
}



BEGIN_MESSAGE_MAP(CXTPSkinObjectFrame, CCmdTarget)
	//{{AFX_MSG_MAP(CXTPSkinObjectFrame)
	ON_WM_NCPAINT()
	ON_WM_NCCALCSIZE()
	ON_WM_NCMOUSEMOVE()
	ON_WM_NCHITTEST_EX()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCLBUTTONDBLCLK()
	ON_WM_NCRBUTTONUP()
	ON_MESSAGE_VOID(WM_NCMOUSELEAVE, OnNcMouseLeave)
	ON_WM_NCACTIVATE()
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	ON_WM_STYLECHANGED()
	ON_WM_CREATE()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_WINDOWPOSCHANGED()
	ON_MESSAGE(WM_PRINT, OnPrint)
	ON_WM_GETMINMAXINFO()
	ON_WM_SYSCOMMAND()
	ON_WM_ERASEBKGND()

	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_KEYUP()
	ON_MESSAGE(WM_SETICON, OnSetIcon)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXTPSkinObjectFrame message handlers


BOOL CXTPSkinObjectFrame::OnEraseBkgnd(CDC* pDC)
{
	HBRUSH hbr = (HBRUSH)(DWORD_PTR)GetClassLongPtr(m_hWnd, GCLP_HBRBACKGROUND);
	BOOL bChanged = FALSE;

	if (hbr > 0 && (ULONG_PTR)hbr < (ULONG_PTR)XTP_SKINMETRICS_COLORTABLESIZE)
	{
		HBRUSH hbrTheme = GetMetrics()->m_brTheme[(ULONG_PTR)hbr - 1];
		SetClassLongPtr(m_hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)hbrTheme);
		bChanged = TRUE;
	}

	BOOL bResult =  CXTPSkinObject::OnEraseBkgnd(pDC);

	if (bChanged)
	{
		SetClassLongPtr(m_hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)hbr);
	}
	return bResult;
}

void CXTPSkinObjectFrame::OnNcPaint()
{
#if 0
	static _int64 nPerfomanceEnd;
	_int64 nPerfomanceStart;
	static _int64 nPerfomanceSum = 0;

	QueryPerformanceCounter((LARGE_INTEGER*)&nPerfomanceStart);
	if (nPerfomanceStart - nPerfomanceEnd  > 100000)
	{
		TRACE(_T("Reset Timer \n"));
		nPerfomanceSum = 0;
	}
#endif

	if (IsFlatScrollBarInitialized())
	{
		Default();
	}
	else
	{
		UpdateButtons();

		CWindowDC dc(this);
		DrawFrame(&dc);
	}

#if 0
	QueryPerformanceCounter((LARGE_INTEGER*)&nPerfomanceEnd);
	nPerfomanceSum += nPerfomanceEnd - nPerfomanceStart;
	TRACE(_T("TotalCounter = %i \n"), int(nPerfomanceSum));
#endif
}

void CXTPSkinObjectFrame::ResizeFrame()
{
	if (HasCaption() && m_pManager->IsEnabled())
	{
		CXTPWindowRect rc(this);

		DWORD dwStyle = GetStyle();
		DWORD dwExStyle = GetExStyle();

		if ((m_pManager->GetApplyOptions() & xtpSkinApplyFrame) && ((dwStyle & WS_CHILD) == 0))
		{
			CRect rcWindow(rc);
			AdjustWindowRectEx(rcWindow, dwStyle, FALSE, dwExStyle);

			CRect rcSysWindow(rc);
			CXTPSkinManagerApiHook::AdjustWindowRectExOrig(rcSysWindow, dwStyle, FALSE, dwExStyle);

			rc.bottom -= rcSysWindow.Height() - rcWindow.Height();
			rc.right -= rcSysWindow.Width() - rcWindow.Width();

			MoveWindow(rc, FALSE);
		}

		UpdateFrameRegion(rc.Size());
	}
}

void CXTPSkinObjectFrame::OnHookAttached(LPCREATESTRUCT lpcs, BOOL bAuto)
{
	CXTPSkinObject::OnHookAttached(lpcs, bAuto);

	if (!bAuto)
	{
		ResizeFrame();
	}
}

void CXTPSkinObjectFrame::RefreshFrameStyle()
{
	if ((GetSkinManager()->GetApplyOptions() & xtpSkinApplyFrame) == 0)
		return;

	m_bLockFrameDraw++;
	DWORD dwStyle = GetStyle();
	DWORD dwStyleRemove = (WS_DLGFRAME | WS_VSCROLL | WS_HSCROLL);

	if (dwStyle & dwStyleRemove)
	{
		SetWindowLong(m_hWnd, GWL_STYLE, dwStyle & ~dwStyleRemove);

		RECT rc = CXTPWindowRect(this);
		SendMessage(WM_NCCALCSIZE, FALSE, (LPARAM)&rc);

		SetWindowLong(m_hWnd, GWL_STYLE, dwStyle);
	}
	m_bLockFrameDraw--;
}

void CXTPSkinObjectFrame::OnSkinChanged(BOOL bPrevState, BOOL bNewState)
{
	if (bNewState)
	{
		RefreshFrameStyle();
	}

	if (bNewState && !bPrevState)
	{
		m_bActive = SendMessage(0x0035) != 0;
	}

	SetWindowPos(0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);

	RefreshMetrics();

	CXTPWindowRect rc(this);
	m_szFrameRegion = CSize(0);
	UpdateFrameRegion(rc.Size());

	Invalidate();
}

void CXTPSkinObjectFrame::OnHookDetached(BOOL bAuto)
{
	if (m_bRegionChanged && !bAuto && ::IsWindow(m_hWnd))
	{
		SetWindowRgn(NULL, TRUE);
		m_bRegionChanged = FALSE;
		SetWindowPos(0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
	}

	CXTPSkinObject::OnHookDetached(bAuto);
}

BOOL CXTPSkinObjectFrame::IsFrameScrollBars()
{
	HWND hWnd = m_hWnd;
	CXTPWindowRect rcChild(hWnd);

	int cxEdge = GetMetrics()->m_cxEdge;
	int cyEdge = GetMetrics()->m_cyEdge;

	while (hWnd)
	{
		if ((GetWindowLong(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION)
		{
			CPoint ptParent = CXTPClientRect(hWnd).BottomRight();
			::ClientToScreen(hWnd, &ptParent);

			if ((rcChild.right + cxEdge < ptParent.x) ||
				(rcChild.bottom + cyEdge < ptParent.y))
			{
				return FALSE;
			}

			return hWnd ? TRUE : FALSE;
		}

		if (((GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD) == 0) || ((GetWindowLong(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION))
			break;

		hWnd = ::GetParent(hWnd);
	}

	return FALSE;
}

BOOL CXTPSkinObjectFrame::IsSizeBox()
{
	HWND hWnd = m_hWnd;
	CXTPWindowRect rcChild(hWnd);

	int cxEdge = GetMetrics()->m_cxEdge;
	int cyEdge = GetMetrics()->m_cyEdge;

	while (hWnd)
	{
		if (GetWindowLong(hWnd, GWL_STYLE) & WS_SIZEBOX)
		{
			if ((GetWindowLong(hWnd, GWL_STYLE) & WS_MAXIMIZE))
				return FALSE;

			CPoint ptParent = CXTPClientRect(hWnd).BottomRight();
			::ClientToScreen(hWnd, &ptParent);

			if ((rcChild.right + cxEdge < ptParent.x) ||
				(rcChild.bottom + cyEdge < ptParent.y))
			{
				return FALSE;
			}

			return hWnd ? TRUE : FALSE;
		}

		if (((GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD) == 0) || ((GetWindowLong(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION))
			break;

		hWnd = ::GetParent(hWnd);
	}

	return FALSE;
}

void CXTPSkinObjectFrame::RedrawFrame()
{
	if (m_pManager->GetApplyOptions() & xtpSkinApplyFrame)
	{
		CWindowDC dc(this);
		DrawFrame(&dc);
	}
	else
	{
		SendMessage(WM_NCPAINT);
	}
}

void CXTPSkinObjectFrame::DrawFrame(CDC* pDC)
{
	if (m_bLockFrameDraw)
		return;

	CXTPWindowRect rc(this);
	rc.OffsetRect(-rc.TopLeft());

	GetSchema()->DrawThemeFrame(pDC, this);

	if (m_spi[SB_VERT].fVisible || m_spi[SB_HORZ].fVisible)
	{
		if (m_spi[SB_VERT].fVisible)
		{
			if (!m_pSBTrack || !m_pSBTrack->bTrackThumb)
				SetupScrollInfo(&m_spi[SB_VERT]);

			CXTPBufferDCEx dcMem(*pDC, m_spi[SB_VERT].rc);
			DrawScrollBar(&dcMem, &m_spi[SB_VERT]);
		}

		if (m_spi[SB_HORZ].fVisible)
		{
			if (!m_pSBTrack || !m_pSBTrack->bTrackThumb)
				SetupScrollInfo(&m_spi[SB_HORZ]);

			CXTPBufferDCEx dcMem(*pDC, m_spi[SB_HORZ].rc);
			DrawScrollBar(&dcMem, &m_spi[SB_HORZ]);
		}

		if (m_spi[SB_HORZ].fVisible && m_spi[SB_VERT].fVisible)
		{
			CRect rcSizeGripper(m_spi[SB_HORZ].rc.right, m_spi[SB_VERT].rc.bottom,
				m_spi[SB_VERT].rc.right, m_spi[SB_HORZ].rc.bottom);

			if (GetExStyle() & WS_EX_LEFTSCROLLBAR)
			{
				rcSizeGripper.left = m_spi[SB_VERT].rc.left;
				rcSizeGripper.right = m_spi[SB_HORZ].rc.left;
			}

			pDC->FillSolidRect(rcSizeGripper, GetSchema()->GetScrollBarSizeBoxColor(this));

			if (IsSizeBox())
			{
				CXTPSkinManagerClass* pClassScrollBar = GetSkinManager()->GetSkinClass(this, _T("SCROLLBAR"));
				pClassScrollBar->DrawThemeBackground(pDC, SBP_SIZEBOX, SZB_RIGHTALIGN, rcSizeGripper);
			}
		}
	}
}

BOOL CXTPSkinObjectFrame::IsFlatScrollBarInitialized() const
{
	INT nStyle = 0;
	if (FlatSB_GetScrollProp(m_hWnd, WSB_PROP_VSTYLE, &nStyle))
		return TRUE;

	return FALSE;
}

void CXTPSkinObjectFrame::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp)
{
	if (IsFlatScrollBarInitialized())
	{
		m_spi[SB_VERT].fVisible = FALSE;
		m_spi[SB_HORZ].fVisible = FALSE;
		CXTPSkinObject::OnNcCalcSize(bCalcValidRects, lpncsp);
		return;
	}

	m_rcBorders = GetSchema()->CalcFrameBorders(this);

	lpncsp[0].rgrc->top += m_rcBorders.top;
	lpncsp[0].rgrc->left += m_rcBorders.left;
	lpncsp[0].rgrc->right -= m_rcBorders.right;
	lpncsp[0].rgrc->bottom -= m_rcBorders.bottom;

	DWORD dwStyle = GetStyle();

	m_spi[SB_VERT].fVisible = dwStyle & WS_VSCROLL;
	m_spi[SB_HORZ].fVisible = dwStyle & WS_HSCROLL;

	if (m_spi[SB_VERT].fVisible)
	{
		if (GetExStyle() & (WS_EX_LAYOUTRTL | WS_EX_LEFTSCROLLBAR))
			lpncsp[0].rgrc->left += GetMetrics()->m_cxVScroll;
		else
			lpncsp[0].rgrc->right -= GetMetrics()->m_cxVScroll;
	}

	if (m_spi[SB_HORZ].fVisible)
	{
		lpncsp[0].rgrc->bottom -= GetMetrics()->m_cyHScroll;
	}

	if (lpncsp[0].rgrc->bottom < lpncsp[0].rgrc->top)
		lpncsp[0].rgrc->bottom = lpncsp[0].rgrc->top;

	if (lpncsp[0].rgrc->right < lpncsp[0].rgrc->left)
		lpncsp[0].rgrc->right = lpncsp[0].rgrc->left;
}

LRESULT CXTPSkinObjectFrame::HandleNcHitTest(CPoint point)
{
	ScreenToFrame(&point);

	DWORD dwStyle = GetStyle();

	if ((dwStyle & WS_VSCROLL) && m_spi[SB_VERT].fVisible && ::PtInRect(&m_spi[SB_VERT].rc, point))
		return (LRESULT)HTVSCROLL;

	if ((dwStyle & WS_HSCROLL) &&m_spi[SB_HORZ].fVisible && ::PtInRect(&m_spi[SB_HORZ].rc, point))
		return (LRESULT)HTHSCROLL;

	if ((dwStyle & WS_VSCROLL) && m_spi[SB_VERT].fVisible && (dwStyle & WS_HSCROLL) && m_spi[SB_HORZ].fVisible && IsSizeBox())
	{
		CRect rcSizeGripper(m_spi[SB_HORZ].rc.right, m_spi[SB_VERT].rc.bottom,
			m_spi[SB_VERT].rc.right, m_spi[SB_HORZ].rc.bottom);

		if (rcSizeGripper.PtInRect(point))
			return GetExStyle() & WS_EX_LAYOUTRTL ? HTBOTTOMLEFT : HTBOTTOMRIGHT;
	}

	CCaptionButton* pButtonHot = HitTestButton(point);

	if (pButtonHot)
	{
		return pButtonHot->m_nHTCode;
	}

	return (LRESULT)HTNOWHERE;
}

LRESULT CXTPSkinObjectFrame::OnNcHitTest(CPoint point)
{
	LRESULT nHitCode = (LRESULT)CXTPSkinObject::OnNcHitTest(point);

	LRESULT nHitCode2 = HandleNcHitTest(point);
	if (nHitCode2 != HTNOWHERE)
		return nHitCode2;

	if (nHitCode == HTCLOSE || nHitCode == HTMAXBUTTON || nHitCode == HTMINBUTTON || nHitCode == HTHELP || nHitCode == HTMENU)
		return (LRESULT)HTCAPTION;

	if ((nHitCode == HTVSCROLL || nHitCode == HTHSCROLL) && !IsFlatScrollBarInitialized())
		return (LRESULT)HTCLIENT;

	return nHitCode;
}

void CXTPSkinObjectFrame::CancelMouseLeaveTracking()
{
	KillTimer(XTP_TID_MOUSELEAVE);
}

void CXTPSkinObjectFrame::HandleTimer(UINT_PTR uTimerID)
{
	if (uTimerID == XTP_TID_REFRESHFRAME)
	{
		RefreshFrameStyle();
		KillTimer(XTP_TID_REFRESHFRAME);
		return;
	}

	if (uTimerID == XTP_TID_MOUSELEAVE)
	{
		RECT rect;
		POINT pt;

		GetWindowRect(&rect);
		::GetCursorPos (&pt);

		if (::GetCapture() != NULL)
			return;

		LRESULT lHitTest = HandleNcHitTest(MAKELPARAM(pt.x, pt.y));

		if ((lHitTest == HTCLIENT || lHitTest == HTNOWHERE || lHitTest == HTBOTTOMRIGHT || lHitTest == HTBOTTOMLEFT) || !::PtInRect (&rect, pt))
		{
			HandleMouseMove(CPoint(-1, -1));
			CancelMouseLeaveTracking();
		}
		return;
	}
}

void CALLBACK CXTPSkinObjectFrame::OnTimerInternal(HWND hWnd, UINT nMsg, UINT_PTR uTimerID, DWORD /*dwTime*/)
{
	if (nMsg != WM_TIMER)
		return;

	XTP_SKINFRAMEWORK_MANAGE_STATE();
	XTP_GUARD_SHARED(CXTPSkinManager, XTPSkinManager())
	{
		CXTPSkinObjectFrame* pFrame = (CXTPSkinObjectFrame*)XTPSkinManager()->Lookup(hWnd);
		if (!pFrame)
		{
			::KillTimer(hWnd, uTimerID);
			return;
		}

		pFrame->HandleTimer(uTimerID);
	}
}


BOOL CXTPSkinObjectFrame::HandleMouseMove(CPoint point)
{
	CPoint ptClient(point);
	ScreenToFrame(&ptClient);

	CCaptionButton* pButton = HitTestButton(ptClient);

	if (m_pButtonHot != pButton)
	{
		if (m_pButtonHot && !pButton)
			CancelMouseLeaveTracking();

		m_pButtonHot = pButton;
		InvalidateButtons();

		if (m_pButtonHot)
		{
			SetTimer(XTP_TID_MOUSELEAVE, 50, &OnTimerInternal);
			return  TRUE;
		}
	}

	if (m_pSBTrack)
		return TRUE;

	BOOL bResult = FALSE;
	for (int i = 0; i < 2; i++)
	{
		XTP_SKINSCROLLBAR_POSINFO* pSBInfo = &m_spi[i];
		int ht = HitTestScrollBar(pSBInfo, ptClient);

		if (ht != pSBInfo->ht && pSBInfo->fVisible)
		{
			if (pSBInfo->ht != HTNOWHERE && ht == HTNOWHERE && !bResult)
			{
				CancelMouseLeaveTracking();
			}

			pSBInfo->ht = ht;
			RedrawScrollBar(pSBInfo);
		}

		if (ht != HTNOWHERE)
		{
			SetTimer (XTP_TID_MOUSELEAVE, 50, &OnTimerInternal);
			bResult = TRUE;
		}
	}

	return bResult;
}

void CXTPSkinObjectFrame::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	if (HandleMouseMove(point))
		return;

	CXTPSkinObject::OnNcMouseMove(nHitTest, point);
}

void CXTPSkinObjectFrame::OnNcMouseLeave()
{
	if (HandleMouseMove(CPoint(-1, -1)))
		return;

	Default();
}

void CXTPSkinObjectFrame::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
	OnNcLButtonDown(nHitTest, point);
}

void CXTPSkinObjectFrame::OnNcRButtonUp(UINT nHitTest, CPoint point)
{
	CPoint ptClient(point);
	ScreenToFrame(&ptClient);

	for (int i = 0; i < 2; i++)
	{
		XTP_SKINSCROLLBAR_POSINFO* pSBInfo = &m_spi[i];
		int ht = HitTestScrollBar(pSBInfo, ptClient);

		if (ht != HTNOWHERE)
		{
			m_bLockFrameDraw++;
			RECT rc = CXTPWindowRect(this);
			SendMessage(WM_NCCALCSIZE, FALSE, (LPARAM)&rc);
			m_bLockFrameDraw--;

			SendMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, MAKELPARAM(point.x, point.y));

			RefreshFrameStyle();
			return;
		}
	}

	CXTPSkinObject::OnNcRButtonDown(nHitTest, point);
}


void CXTPSkinObjectFrame::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	if (m_pButtonHot)
	{
		TrackCaptionButton();
	}

	CPoint ptClient(point);
	ScreenToFrame(&ptClient);

	for (int i = 0; i < 2; i++)
	{
		XTP_SKINSCROLLBAR_POSINFO* pSBInfo = &m_spi[i];
		int ht = HitTestScrollBar(pSBInfo, ptClient);

		if (ht != HTNOWHERE)
		{
			TrackInit(ptClient, pSBInfo, (GetKeyState(VK_SHIFT) < 0) ? TRUE : FALSE);
			return;
		}
	}

	if (nHitTest == HTZOOM || nHitTest == HTREDUCE || nHitTest == HTCLOSE || nHitTest == HTHELP)
		return;

	CXTPSkinObject::OnNcLButtonDown(nHitTest, point);
}

BOOL CXTPSkinObjectFrame::OnHookDefWindowProc(UINT nMessage, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)
{
	if (WM_CTLCOLORBTN     == nMessage ||
		 WM_CTLCOLORDLG     == nMessage ||
		 WM_CTLCOLORSTATIC  == nMessage ||
		 WM_CTLCOLOREDIT    == nMessage ||
		 WM_CTLCOLORLISTBOX == nMessage)
	{
		if (m_strClassName == _T("COMBOBOX"))
			return FALSE;

		if (nMessage == WM_CTLCOLOREDIT || nMessage == WM_CTLCOLORLISTBOX)
		{
			::SetBkColor((HDC)wParam, GetColor(COLOR_WINDOW));
			::SetTextColor((HDC)wParam, GetColor(COLOR_WINDOWTEXT));
			lResult = (LRESULT)GetMetrics()->m_brushWindow;
			return TRUE;
		}

		if (CWnd::GrayCtlColor((HDC)wParam, (HWND)lParam,
			(UINT)(nMessage - WM_CTLCOLORMSGBOX),
			GetMetrics()->m_brushDialog, 0))
		{
			lResult = (LRESULT)GetClientBrush((HDC)wParam, (HWND)lParam, (UINT)(nMessage - WM_CTLCOLORMSGBOX));
			return TRUE;
		}
	}

	return CXTPSkinObject::OnHookDefWindowProc(nMessage, wParam, lParam, lResult);
}

BOOL CXTPSkinObjectFrame::IsDefWindowProcAvail(int nMessage) const
{
	if (!CXTPSkinObject::IsDefWindowProcAvail(nMessage))
		return FALSE;

	if (m_bActiveX && m_bSetWindowProc && nMessage == WM_GETMINMAXINFO)
		return FALSE;

	return TRUE;
}

BOOL CXTPSkinObjectFrame::PreHookMessage(UINT nMessage)
{
	if ((nMessage == WM_ENTERIDLE) && (m_pManager->GetApplyOptions() & xtpSkinApplyFrame) &&
		((GetStyle() & (WS_CAPTION | WS_MINIMIZE | WS_VISIBLE)) == (WS_CAPTION | WS_MINIMIZE | WS_VISIBLE)))
	{
		RedrawFrame();
	}

	if (nMessage >= WM_LBUTTONDOWN && nMessage <= WM_MBUTTONDBLCLK)
		*m_bMenuStatus = FALSE;

	if (m_bLockFrameDraw && (nMessage == WM_NCCALCSIZE || nMessage == WM_STYLECHANGED || nMessage == WM_STYLECHANGING))
		return TRUE;

	if ((m_pManager->GetApplyOptions() & xtpSkinApplyFrame) == 0)
	{
		if (nMessage == WM_NCPAINT || nMessage == WM_NCCALCSIZE ||
			nMessage == WM_NCHITTEST || nMessage == WM_NCLBUTTONDOWN ||
			nMessage == WM_NCLBUTTONDBLCLK ||
			nMessage == WM_WINDOWPOSCHANGED || nMessage == WM_NCMOUSEMOVE ||
			nMessage == WM_NCMOUSELEAVE || nMessage == WM_NCACTIVATE ||
			nMessage == WM_WINDOWPOSCHANGING || nMessage == WM_PRINT)
		return TRUE;
	}

	return CXTPSkinObject::PreHookMessage(nMessage);
}

BOOL CXTPSkinObjectFrame::OnHookMessage(UINT nMessage, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)
{
	if (nMessage == WM_CTLCOLORBTN || nMessage == WM_CTLCOLORDLG ||
		nMessage == WM_CTLCOLORSTATIC || nMessage == WM_CTLCOLORLISTBOX  || nMessage == WM_CTLCOLOREDIT)
	{
		if (m_strClassName == _T("COMBOBOX"))
			return FALSE;

		if (IsDefWindowProcAvail(nMessage) &&
			(m_dwDialogTexture != ETDT_ENABLETAB || !m_bActiveX))
		{
			return FALSE;
		}

		if (nMessage == WM_CTLCOLORLISTBOX || nMessage == WM_CTLCOLOREDIT)
		{
			::SetBkColor((HDC)wParam, GetColor(COLOR_WINDOW));
			::SetTextColor((HDC)wParam, GetColor(COLOR_WINDOWTEXT));
			lResult = (LRESULT)GetMetrics()->m_brushWindow;
			return TRUE;
		}

	// handle standard gray backgrounds if enabled
		if (CWnd::GrayCtlColor((HDC)wParam, (HWND)lParam,
			(UINT)(nMessage - WM_CTLCOLORMSGBOX),
			GetMetrics()->m_brushDialog, 0))
		{
			lResult = (LRESULT)GetClientBrush((HDC)wParam, (HWND)lParam, (UINT)(nMessage - WM_CTLCOLORMSGBOX));
			return TRUE;
		}
	}

#ifdef _XTP_ACTIVEX
	if (nMessage == WM_PAINT && m_dwDialogTexture == ETDT_ENABLETAB && m_bActiveX)
	{
		CPaintDC dc(this);
		::SetBrushOrgEx(dc, 0, 0, NULL);
		FillRect(dc, CXTPClientRect(this), GetClientBrush(dc, m_hWnd, CTLCOLOR_DLG));
		return TRUE;
	}
#endif
	if (nMessage == WM_ERASEBKGND && m_dwDialogTexture == ETDT_ENABLETAB)
	{
		::SetBrushOrgEx((HDC)wParam, 0, 0, NULL);
		FillRect((HDC)wParam, CXTPClientRect(this), GetClientBrush((HDC)wParam, m_hWnd, CTLCOLOR_DLG));
		return TRUE;
	}

	return CXTPSkinObject::OnHookMessage(nMessage, wParam, lParam, lResult);
}

//////////////////////////////////////////////////////////////////////////
// CXTPSkinObjectFrame;

void CXTPSkinObjectFrame::RemoveButtons()
{
	for (int i = 0; i < m_arrButtons.GetSize(); i++)
	{
		delete m_arrButtons[i];
	}

	m_arrButtons.RemoveAll();

	ASSERT(NULL == m_pButtonHot);     // Invalid after remove
	ASSERT(NULL == m_pButtonPressed); // Invalid after remove
}


void CXTPSkinObjectFrame::UpdateButton(int nCommand, BOOL bVisible, BOOL bEnabled, UINT htCode, int nClassPart)
{
	if (bVisible)
	{
		CCaptionButton* pButton = new CCaptionButton(nCommand, this, htCode, nClassPart);
		pButton->m_bEnabled = bEnabled;

		m_arrButtons.Add(pButton);
	}
}

void CXTPSkinObjectFrame::UpdateButtons()
{
	DWORD dwExStyle = GetExStyle();
	DWORD dwStyle = GetStyle();

	if (m_dwStyle == dwStyle && m_dwExStyle == dwExStyle)
		return;

	m_dwExStyle = dwExStyle;
	m_dwStyle = dwStyle;

	m_pButtonHot     = NULL;
	m_pButtonPressed = NULL;

	RemoveButtons();

	if (HasCaption())
	{
		BOOL bToolWindow = (dwExStyle & WS_EX_TOOLWINDOW) == WS_EX_TOOLWINDOW;
		BOOL bMaximized = (dwStyle & WS_MAXIMIZE) == WS_MAXIMIZE;
		BOOL bMinimized = (dwStyle & WS_MINIMIZE) == WS_MINIMIZE;

		BOOL bSysMenu = (dwStyle & WS_SYSMENU);
		BOOL bDialogFrame = (dwStyle & WS_DLGFRAME || dwExStyle & WS_EX_DLGMODALFRAME);

		BOOL bEnableClose = TRUE;
		BOOL bEnabledMaximize = ((dwStyle & WS_MAXIMIZEBOX) == WS_MAXIMIZEBOX);
		BOOL bEnabledMinimize = ((dwStyle & WS_MINIMIZEBOX) == WS_MINIMIZEBOX);
		BOOL bShowMinMaxButtons = !bToolWindow && bSysMenu && (bEnabledMaximize || bEnabledMinimize);

		if (bSysMenu && !bToolWindow && ((m_dwExStyle & WS_EX_MDICHILD) == 0))
		{
			HMENU hMenu = ::GetSystemMenu(m_hWnd, FALSE);
			if (::GetMenuState(hMenu, SC_CLOSE, MF_BYCOMMAND) & (MF_DISABLED | MF_GRAYED)) bEnableClose = FALSE;
		}

		UpdateButton(SC_CLOSE, !bDialogFrame || bSysMenu,
			bEnableClose, HTCLOSE, bToolWindow? WP_SMALLCLOSEBUTTON: WP_CLOSEBUTTON);

		UpdateButton(SC_MAXIMIZE, !bMaximized && bShowMinMaxButtons,
			bEnabledMaximize, HTMAXBUTTON, WP_MAXBUTTON);

		if (bMinimized)
		{
			UpdateButton(SC_RESTORE, bShowMinMaxButtons,
				bEnabledMinimize, HTMINBUTTON, WP_RESTOREBUTTON);
		}
		else
		{
			UpdateButton(SC_RESTORE,  bMaximized && bShowMinMaxButtons,
				bEnabledMaximize, HTMAXBUTTON, WP_RESTOREBUTTON);

			UpdateButton(SC_MINIMIZE, bShowMinMaxButtons,
				bEnabledMinimize, HTMINBUTTON, WP_MINBUTTON);
		}

		UpdateButton(SC_CONTEXTHELP, ((dwExStyle & WS_EX_CONTEXTHELP) == WS_EX_CONTEXTHELP) && !bToolWindow && bSysMenu,
			TRUE, HTHELP, WP_HELPBUTTON);
	}

}


CXTPSkinObjectFrame::CCaptionButton* CXTPSkinObjectFrame::HitTestButton(CPoint pt)
{
	DWORD dwStyle = GetStyle();

	for (int i = 0; i < m_arrButtons.GetSize(); i++)
	{
		CCaptionButton* pButton = m_arrButtons[i];
		CRect rcButton(pButton->m_rcButton);

		if ((dwStyle & (WS_MAXIMIZE | WS_CHILD)) == WS_MAXIMIZE)
			rcButton.InflateRect(0, 2, i == 0 ? 2 : 0, 0);

		if (rcButton.PtInRect(pt) && pButton->m_bEnabled)
			return pButton;
	}
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CXTPSkinObjectFrame message handlers

LRESULT CXTPSkinObjectFrame::OnSetText(WPARAM wParam, LPARAM lParam)
{
	if (((GetStyle() & WS_CAPTION) == WS_CAPTION) && (GetSkinManager()->GetApplyOptions() & xtpSkinApplyFrame))
	{
		LRESULT lRet = DefWindowProc(WM_SETTEXT, wParam, lParam);

		RedrawFrame();

		if ((GetExStyle() & WS_EX_MDICHILD) && (GetStyle() & (WS_MAXIMIZE | WS_CHILD)) == (WS_MAXIMIZE | WS_CHILD))
		{
			CWnd* pWnd = GetTopLevelParent();
			pWnd->SendMessage(WM_NCPAINT, 0, 0);
		}

		return lRet;
	}

	return Default();
}

HWND CXTPSkinObjectFrame::FindMDIClient()
{
	return m_pMDIClient ? m_pMDIClient->m_hWnd : NULL;
}

#if (_MSC_VER <= 1200) && !defined(_WIN64)
#define GetWindowLongPtrW GetWindowLongW
#define GetWindowLongPtrA GetWindowLongA
#endif

#ifndef DWLP_DLGPROC
#define DWLP_DLGPROC 4
#endif


LRESULT CXTPSkinObjectFrame::CallDefDlgProc(UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	if (m_dwDialogTexture == ETDT_ENABLE) // Dialog ?
	{
		int nNotifyFormat = (int)SendMessage(WM_NOTIFYFORMAT, 0, NF_QUERY);

		WNDPROC pDlgWndProc = (WNDPROC) (nNotifyFormat == NFR_UNICODE ?
			GetWindowLongPtrW(m_hWnd, DWLP_DLGPROC) : GetWindowLongPtrA(m_hWnd, DWLP_DLGPROC));

		if (pDlgWndProc && (HIWORD((ULONG)(ULONG_PTR)pDlgWndProc) != 0xFFFF))
		{
			return (pDlgWndProc)(m_hWnd, nMessage, wParam, lParam);
		}
	}
	return 0;
}

BOOL CXTPSkinObjectFrame::OnNcActivate(BOOL bActive)
{
	if (HasCaption())
	{
		CallDefDlgProc(WM_NCACTIVATE, (WPARAM)bActive, 0);

		if (!bActive)
		{
			CWnd* pWnd = CWnd::FromHandlePermanent(m_hWnd);
			// Mimic MFC kludge to stay active if WF_STAYACTIVE bit is on
			//
			if (pWnd && pWnd->m_nFlags & WF_STAYACTIVE)
				bActive = TRUE;

			if (!IsWindowEnabled())
				bActive = FALSE;
		}


		HWND hWndClient = FindMDIClient();
		HWND hWndActive = hWndClient ? (HWND)::SendMessage(hWndClient, WM_MDIGETACTIVE, 0, 0) : NULL;

		if (hWndActive != m_hWnd && hWndActive)
		{
			::SendMessage(hWndActive, WM_NCACTIVATE, bActive, 0);
			::SendMessage(hWndActive, WM_NCPAINT, 0, 0);
		}

		m_bActive = bActive;            // update state

		RedrawFrame();
		return TRUE;
	}

	return CXTPSkinObject::OnNcActivate(bActive);
}


int CXTPSkinObjectFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CXTPSkinObject::OnCreate(lpCreateStruct) == -1)
		return -1;

	ResizeFrame();

	return 0;
}

BOOL CXTPSkinObjectFrame::HasCaption() const
{
	return (GetStyle() & WS_CAPTION) == WS_CAPTION;
}


void CXTPSkinObjectFrame::UpdateFrameRegion(CSize szFrameRegion)
{
	if (m_bInUpdateRegion)
		return;

	m_bInUpdateRegion = TRUE;

	if (m_bRegionChanged && !HasCaption())
	{
		SetWindowRgn(NULL, TRUE);
		m_bRegionChanged = FALSE;
	}
//  DWORD dwStyle = GetStyle();

	if (HasCaption() && (m_szFrameRegion != szFrameRegion))
	{
		if (GetSkinManager()->IsEnabled() && GetSkinManager()->GetApplyOptions() & xtpSkinApplyFrame)
		{
			HRGN hRgn = GetSchema()->CalcFrameRegion(this, szFrameRegion);

			SetWindowRgn(hRgn, TRUE);
			m_bRegionChanged = TRUE;
		}
		else if (m_bRegionChanged)
		{
			SetWindowRgn(NULL, TRUE);
			m_bRegionChanged = FALSE;
		}

		m_szFrameRegion = szFrameRegion;
	}

	m_bInUpdateRegion = FALSE;
}

void CXTPSkinObjectFrame::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos)
{
	CXTPSkinObject::OnWindowPosChanging(lpwndpos);

	if ((m_pManager->HasApplyOptions(xtpSkinApplyMetrics | xtpSkinApplyFrame)) &&
		((GetExStyle() & WS_EX_MDICHILD) == WS_EX_MDICHILD) &&
		((GetStyle() & WS_MAXIMIZE) == WS_MAXIMIZE))
	{

		RECT rc;
		GetParent()->GetClientRect(&rc);

		int cx = rc.right - rc.left;
		int cy = rc.bottom - rc.top;

		if (lpwndpos->cx > cx + m_rcBorders.left + m_rcBorders.right)
		{
			lpwndpos->cx = cx + m_rcBorders.left + m_rcBorders.right;
			lpwndpos->cy = cy + m_rcBorders.top + m_rcBorders.bottom;
			lpwndpos->x = -m_rcBorders.left;
			lpwndpos->y = -m_rcBorders.top;
		}
	}

	CSize szFrameRegion(lpwndpos->cx, lpwndpos->cy);

	if (((lpwndpos->flags & SWP_NOSIZE) ==  0) && (m_szFrameRegion != szFrameRegion))
	{
		if ((GetExStyle() & WS_EX_LAYOUTRTL) == 0)
			UpdateFrameRegion(szFrameRegion);
	}
}

void CXTPSkinObjectFrame::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos)
{
	if (m_bInWindowPosChanged)
	{
		m_bInWindowPosChanged = TRUE;
	}

	m_bInWindowPosChanged = TRUE;

	CXTPSkinObject::OnWindowPosChanged(lpwndpos);

	CSize szFrameRegion(lpwndpos->cx, lpwndpos->cy);

	if (((lpwndpos->flags & SWP_NOSIZE) ==  0) && (m_szFrameRegion != szFrameRegion))
	{
		UpdateFrameRegion(szFrameRegion);
	}

	m_bInWindowPosChanged = FALSE;
}


void CXTPSkinObjectFrame::RefreshMetrics()
{
	CXTPSkinObject::RefreshMetrics();

	m_szFrameRegion = CSize(0, 0);

	PostMessage(WM_SYSCOLORCHANGE);
}



void CXTPSkinObjectFrame::InvalidateButtons()
{
	RedrawFrame();
}


void CXTPSkinObjectFrame::TrackCaptionButton()
{
	SetCapture();
	BOOL bAccept = FALSE;
	m_pButtonPressed = m_pButtonHot;
	CCaptionButton* pButtonPressed = m_pButtonHot;
	ASSERT(pButtonPressed);

	RedrawFrame();

	while (::GetCapture() == m_hWnd)
	{
		MSG msg;

		if (!::GetMessage(&msg, NULL, 0, 0))
		{
			AfxPostQuitMessage((int)msg.wParam);
			break;
		}

		if (msg.message == WM_LBUTTONUP)
		{
			bAccept = m_pButtonPressed == pButtonPressed;
			break;
		}
		else if (msg.message == WM_NCMOUSELEAVE)
		{

		}
		else if (msg.message == WM_MOUSEMOVE)
		{
			POINT point = msg.pt;
			ScreenToFrame(&point);

			CCaptionButton* pButton = HitTestButton(point) == pButtonPressed ? pButtonPressed : NULL;

			if (pButton != m_pButtonPressed)
			{
				m_pButtonPressed = pButton;
				RedrawFrame();
			}
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	m_pButtonPressed = NULL;
	m_pButtonHot = NULL;
	ReleaseCapture();
	RedrawFrame();

	if (bAccept)
	{
		SendMessage(WM_SYSCOMMAND, pButtonPressed->m_nCommand);
	}
}

LRESULT CXTPSkinObjectFrame::OnPrint(WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = TRUE;

	if (lParam != PRF_NONCLIENT)
	{
		m_bLockFrameDraw++;
		lResult = DefWindowProc(WM_PRINT, wParam, lParam);
		m_bLockFrameDraw--;
	}

	if (lParam & PRF_NONCLIENT)
	{
		if (m_spi[SB_VERT].fVisible) m_spi[SB_VERT].fVisible = GetStyle() & WS_VSCROLL;
		if (m_spi[SB_HORZ].fVisible) m_spi[SB_HORZ].fVisible = GetStyle() & WS_HSCROLL;

		CDC* pDC = CDC::FromHandle((HDC)wParam);

		if ((GetExStyle() & WS_EX_LAYOUTRTL) && !XTPDrawHelpers()->IsContextRTL(pDC))
			XTPDrawHelpers()->SetContextRTL(pDC, 1);

		DrawFrame(pDC);
	}

	return lResult;
}

BOOL CXTPSkinObjectFrame::HandleSysCommand(UINT nID, LPARAM lParam)
{
	if ((GetSkinManager()->GetApplyOptions() & xtpSkinApplyMenus) == 0)
		return FALSE;

	UINT nCmd = (nID & 0xFFF0);

	if (nCmd == SC_MOUSEMENU)
	{
		return TRUE;
	}

	if (nCmd == SC_KEYMENU)
	{
		if ((GetStyle() & WS_CHILD )== 0)
			return TRUE;

		HWND hWnd = m_hWnd;

		while (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
		{
			if (GetWindowLong(hWnd, GWL_STYLE) & WS_SYSMENU)
				break;

			hWnd = ::GetParent(hWnd);
		}

		if (hWnd != m_hWnd)
		{
			if (::GetMenu(hWnd) || (GetWindowLong(hWnd, GWL_STYLE) & WS_SYSMENU))
			{
				CXTPSkinObjectFrame* pFrame = (CXTPSkinObjectFrame*)GetSkinManager()->Lookup(hWnd);
				if (pFrame)
				{
					pFrame->HandleSysCommand(nID, lParam);
				}
			}

			return TRUE;
		}
	}

	return FALSE;
}


void CXTPSkinObjectFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	UINT nCmd = (nID & 0xFFF0);

	if ((nCmd == SC_MAXIMIZE) && m_pManager->HasApplyOptions(xtpSkinApplyMetrics | xtpSkinApplyFrame) &&
		((GetExStyle() & WS_EX_MDICHILD) == WS_EX_MDICHILD) &&
		((GetStyle() & WS_MAXIMIZE) == 0))
	{
		CXTPSkinObject::OnSysCommand(nID, lParam);

		RECT rc;
		GetParent()->GetClientRect(&rc);
		int cx = rc.right - rc.left;
		int cy = rc.bottom - rc.top;

		rc.left = -m_rcBorders.left;
		rc.top = -m_rcBorders.top;
		rc.right = cx + m_rcBorders.right;
		rc.bottom = cy + m_rcBorders.bottom;

		MoveWindow(rc.left, rc.top,
			rc.right - rc.left, rc.bottom - rc.top, TRUE);
		return;
	}

	if (HandleSysCommand(nID, lParam))
		return;

	if ((nCmd == SC_MOVE) && (GetStyle() & WS_MINIMIZE) && (GetSkinManager()->GetApplyOptions() & xtpSkinApplyFrame))
	{
		DoDefWindowProc(WM_NCPAINT, 0, 0);
		RedrawFrame();
	}


	CXTPSkinObject::OnSysCommand(nID, lParam);

	if ((nCmd == SC_MINIMIZE) && (GetSkinManager()->GetApplyOptions() & xtpSkinApplyFrame))
	{
		RedrawFrame();
	}
}

HWND CXTPSkinObjectFrame::GetTopLevelWindow() const
{
	HWND hWnd = m_hWnd;
	while (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
		hWnd = ::GetParent(hWnd);
	return hWnd;

}

void CXTPSkinObjectFrame::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_MENU && ((nFlags & KF_REPEAT) == 0))
	{
		HWND hWnd = GetTopLevelWindow();

		HMENU hMenu = ::GetMenu(hWnd);
		if (hMenu)
		{
			*m_bMenuStatus = TRUE;
			return;
		}
	}

	CXTPSkinObject::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

void CXTPSkinObjectFrame::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_MENU && *m_bMenuStatus)
	{
		*m_bMenuStatus = FALSE;

		HWND hWnd = GetTopLevelWindow();
		::SendMessage(hWnd, WM_SYSCOMMAND, SC_KEYMENU, 0);
		return;
	}

	CXTPSkinObject::OnSysKeyUp(nChar, nRepCnt, nFlags);
}

void CXTPSkinObjectFrame::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CXTPSkinObject::OnKeyUp(nChar, nRepCnt, nFlags);
}


void CXTPSkinObjectFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	CXTPSkinObject::OnGetMinMaxInfo(lpMMI);

	if (m_pManager->HasApplyOptions(xtpSkinApplyFrame) && HasCaption())
	{
		int yMin = m_rcBorders.top + m_rcBorders.bottom;
		int xMin = (int)m_arrButtons.GetSize() * m_rcBorders.top;

		xMin += GetSystemMetrics(SM_CYSIZE) + 2 * GetSystemMetrics(SM_CXEDGE);

		lpMMI->ptMinTrackSize.x = max(lpMMI->ptMinTrackSize.x, xMin);
		lpMMI->ptMinTrackSize.y = max(lpMMI->ptMinTrackSize.y, yMin);
	}

	if (m_pManager->HasApplyOptions(xtpSkinApplyFrame | xtpSkinApplyMetrics) && ((GetExStyle() & WS_EX_MDICHILD) == WS_EX_MDICHILD))
	{

		int nDelta = m_rcBorders.top + lpMMI->ptMaxPosition.y;
		lpMMI->ptMaxPosition.y -= nDelta;
		lpMMI->ptMaxSize.y += nDelta;
	}
}

void CXTPSkinObjectFrame::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	CXTPSkinObject::OnStyleChanged(nStyleType, lpStyleStruct);

	m_rcBorders = GetSchema()->CalcFrameBorders(this);

	if (((lpStyleStruct->styleNew ^ lpStyleStruct->styleOld) & (WS_EX_TOOLWINDOW | WS_BORDER | WS_DLGFRAME | WS_THICKFRAME)))
	{
		CXTPWindowRect rc(this);
		m_szFrameRegion = CSize(0);
		UpdateFrameRegion(rc.Size());

	}
	if (HasCaption()) PostMessage(WM_NCPAINT);

}

int CXTPSkinObjectFrame::GetClientBrushMessage()
{
	return m_nCtlColorMessage;
}

HBRUSH CXTPSkinObjectFrame::GetClientBrush(CDC* pDC)
{
	return GetFillBackgroundBrush(pDC, GetClientBrushMessage());
}

HBRUSH CXTPSkinObjectFrame::GetFillBackgroundBrush(CDC* pDC, int nMessage)
{
	if (GetParent() == NULL)
		return GetMetrics()->m_brTheme[COLOR_3DFACE];

	HBRUSH hBrush = (HBRUSH)::SendMessage(GetParent()->GetSafeHwnd(), nMessage, (WPARAM)pDC->GetSafeHdc(), (LPARAM)m_hWnd);
	if (hBrush)
	{
		if ((DWORD_PTR)hBrush < XTP_SKINMETRICS_COLORTABLESIZE)
		{
			hBrush = GetMetrics()->m_brTheme[(DWORD_PTR)hBrush - 1];
		}
		return hBrush;
	}

	return nMessage == WM_CTLCOLOREDIT || nMessage == WM_CTLCOLORLISTBOX ?
		GetMetrics()->m_brushWindow : GetMetrics()->m_brTheme[COLOR_3DFACE];
}

HBRUSH CXTPSkinObjectFrame::FillBackground(CDC* pDC, LPCRECT lprc, int nMessage /*= WM_CTLCOLORSTATIC*/)
{
	HBRUSH hBrush = ::GetParent(m_hWnd) ? (HBRUSH)::SendMessage(::GetParent(m_hWnd), nMessage, (WPARAM)pDC->GetSafeHdc(), (LRESULT)m_hWnd) : NULL;
	if (hBrush)
	{
		if ((DWORD_PTR)hBrush < XTP_SKINMETRICS_COLORTABLESIZE)
		{
			hBrush = GetMetrics()->m_brTheme[(DWORD_PTR)hBrush - 1];
		}
		::FillRect(pDC->GetSafeHdc(), lprc, hBrush);
		return hBrush;
	}

	pDC->FillSolidRect(lprc, GetColor(COLOR_3DFACE));
	return GetMetrics()->m_brTheme[COLOR_3DFACE];
}

BOOL CXTPSkinObjectFrame::DrawMenuBar()
{
	m_dwStyle = m_dwExStyle = (DWORD)-1;

	//UpdateButtons();

	UpdateMenuBar();

	RedrawFrame();

	return TRUE;
}

LRESULT CXTPSkinObjectFrame::OnSetIcon(WPARAM, LPARAM)
{
	LRESULT lResult = Default();

	RedrawFrame();

	return lResult;
}



//////////////////////////////////////////////////////////////////////////
// CXTPSkinObjectUser32Control

CXTPSkinObjectUser32Control::CXTPSkinObjectUser32Control()
{
	ZeroMemory(&m_si, sizeof(m_si));
}

void CXTPSkinObjectUser32Control::CheckScrollBarsDraw()
{
	if (m_pManager->IsComCtlV6())
		return;

	if (!IsWindow(m_hWnd))
		return;

	for (int i = 0; i < 2; i++)
	{
		if (m_spi[i].fVisible)
		{
			SCROLLINFO si;
			si.cbSize = sizeof(SCROLLINFO);

			GetScrollInfo(i, &si, SIF_POS | SIF_RANGE);

			if (si.nPos != m_si[i].nPos || si.nMax != m_si[i].nMax || si.nMin != m_si[i].nMin)
			{
				m_si[i] = si;
				RedrawScrollBar(&m_spi[i]);
			}
		}
	}
}


BEGIN_MESSAGE_MAP(CXTPSkinObjectUser32Control, CXTPSkinObjectFrame)
	//{{AFX_MSG_MAP(CXTPSkinObjectUser32Control)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CXTPSkinObjectUser32Control::OnEraseBkgnd(CDC* pDC)
{
	BOOL bResult = CXTPSkinObjectFrame::OnEraseBkgnd(pDC);

	CheckScrollBarsDraw();

	return bResult;

}

void CXTPSkinObjectUser32Control::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CXTPSkinObjectFrame::OnKeyDown(nChar, nRepCnt, nFlags);
	CheckScrollBarsDraw();
}

void CXTPSkinObjectUser32Control::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CXTPSkinObjectFrame::OnChar(nChar, nRepCnt, nFlags);
	CheckScrollBarsDraw();
}

void CXTPSkinObjectUser32Control::OnHScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/)
{
	Default();
	CheckScrollBarsDraw();
}

void CXTPSkinObjectUser32Control::OnVScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/)
{
	Default();
	CheckScrollBarsDraw();
}

BOOL CXTPSkinObjectUser32Control::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	BOOL bResult = CXTPSkinObjectFrame::OnWndMsg(message, wParam, lParam, pResult);

	if (message == WM_MOUSEWHEEL)
	{
		CheckScrollBarsDraw();
	}

	return bResult;
}

void CXTPSkinObjectUser32Control::OnWindowPosChanged(WINDOWPOS FAR* /*lpwndpos*/)
{
	Default();
	CheckScrollBarsDraw();
}

//////////////////////////////////////////////////////////////////////////
// CXTPSkinObjectComCtl32Control

CXTPSkinObjectComCtl32Control::CXTPSkinObjectComCtl32Control()
{
}

void CXTPSkinObjectComCtl32Control::CheckScrollBarsDraw()
{
	if (!m_pManager->IsWin9x())
		return;

	CXTPSkinObjectUser32Control::CheckScrollBarsDraw();
}


//////////////////////////////////////////////////////////////////////////
// CXTPSkinObjectSolidFilled

