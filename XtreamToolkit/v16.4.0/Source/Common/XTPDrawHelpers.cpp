// XTPDrawHelpers.cpp: implementation of the CXTPDrawHelpers class.
//
// This file is a part of the XTREME TOOLKIT PRO MFC class library.
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
#include "resource.h"

#include "XTPSystemHelpers.h"
#include "XTPColorManager.h"
#include "XTPDrawHelpers.h"

#include "XTPResourceManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

HHOOK CXTPMouseMonitor::m_hHookMouse = 0;
CWnd* CXTPMouseMonitor::m_pWndMonitor = 0;

typedef BOOL (WINAPI *PFNSETLAYEREDWINDOWATTRIBUTES) (HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

#ifndef LWA_ALPHA
#define LWA_ALPHA               0x00000002
#endif

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
#endif


//////////////////////////////////////////////////////////////////////////
// CXTPMouseMonitor
//////////////////////////////////////////////////////////////////////////

void CXTPMouseMonitor::SetupHook(CWnd* pWndMonitor)
{
	if (pWndMonitor && m_hHookMouse == 0)
	{
		m_hHookMouse = SetWindowsHookEx(WH_MOUSE, MouseProc, 0, GetCurrentThreadId ());
	}
	if (!pWndMonitor && m_hHookMouse)
	{
		UnhookWindowsHookEx(m_hHookMouse);
		m_hHookMouse = 0;
	}
	m_pWndMonitor = pWndMonitor;
}

BOOL CXTPMouseMonitor::IsMouseHooked()
{
	return m_pWndMonitor != NULL;
}

LRESULT CALLBACK CXTPMouseMonitor::MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode != HC_ACTION || !m_pWndMonitor)
		return CallNextHookEx(m_hHookMouse, nCode, wParam, lParam);

	CXTPWindowRect rc(m_pWndMonitor);

	if (!rc.PtInRect(((PMOUSEHOOKSTRUCT)lParam)->pt))
	{
		switch (wParam)
		{
		case WM_LBUTTONDOWN:
		case WM_NCLBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_NCMBUTTONDOWN:
			m_pWndMonitor->GetOwner()->SetFocus();
			return TRUE;
		}
	}

	return CallNextHookEx(m_hHookMouse, nCode, wParam, lParam);
}


//===========================================================================
// CXTPClientCursorPos class
//===========================================================================

CXTPTransparentBitmap::CXTPTransparentBitmap(HBITMAP hBitmap)
: m_hBitmap(hBitmap)
{
}

// Not foolproof, but works 99% of the time :).  Assumes the top
// left pixel is the transparent color.

COLORREF CXTPTransparentBitmap::GetTransparentColor() const
{
	CBitmap* pBitmap = CBitmap::FromHandle(m_hBitmap);
	if (pBitmap != NULL)
	{
		CXTPCompatibleDC dc(NULL, pBitmap);
		return dc.GetPixel(0, 0);
	}
	return (COLORREF)-1;
}


HICON CXTPTransparentBitmap::ConvertToIcon() const
{
	if (m_hBitmap == NULL)
		return NULL;

	COLORREF crTransparent = GetTransparentColor();

	BITMAP bmp;
	if (!::GetObject(m_hBitmap, sizeof(BITMAP), &bmp))
		return NULL;

	if (bmp.bmHeight == 0 || bmp.bmWidth == 0)
		return NULL;

	CImageList il;
	il.Create(bmp.bmWidth, bmp.bmHeight, ILC_COLOR24 | ILC_MASK, 0, 1);
	il.Add(CBitmap::FromHandle(m_hBitmap), crTransparent);

	ASSERT(il.GetImageCount() == 1);

	return il.ExtractIcon(0);
}

//===========================================================================
// CXTPClientCursorPos class
//===========================================================================

CXTPClientCursorPos::CXTPClientCursorPos(CWnd* pWnd)
{
	GetCursorPos(this);
	pWnd->ScreenToClient(this);
}

//===========================================================================
// CXTPEmptySize class
//===========================================================================

CXTPEmptySize::CXTPEmptySize()
{
	SetSizeEmpty();
}

void CXTPEmptySize::SetSizeEmpty()
{
	cx = 0;
	cy = 0;
}

const SIZE& CXTPEmptySize::operator=(const SIZE& srcSize)
{
	cx = srcSize.cx;
	cy = srcSize.cy;
	return *this;
}

//===========================================================================
// CXTPEmptyRect class
//===========================================================================

CXTPEmptyRect::CXTPEmptyRect()
{
	SetRectEmpty();
}

//===========================================================================
// CXTPWindowRect class
//===========================================================================

CXTPWindowRect::CXTPWindowRect(HWND hWnd)
{
	if (::IsWindow(hWnd))
		::GetWindowRect(hWnd, this);
	else
		SetRectEmpty();
}

CXTPWindowRect::CXTPWindowRect(const CWnd* pWnd)
{
	if (::IsWindow(pWnd->GetSafeHwnd()))
		::GetWindowRect(pWnd->GetSafeHwnd(), this);
	else
		SetRectEmpty();
}

//===========================================================================
// CXTPClientRect class
//===========================================================================

CXTPClientRect::CXTPClientRect(HWND hWnd)
{
	if (::IsWindow(hWnd))
		::GetClientRect(hWnd, this);
	else
		SetRectEmpty();
}

CXTPClientRect::CXTPClientRect(const CWnd* pWnd)
{
	if (::IsWindow(pWnd->GetSafeHwnd()))
		::GetClientRect(pWnd->GetSafeHwnd(), this);
	else
		SetRectEmpty();
}

//===========================================================================
// CXTPBufferDC class
//===========================================================================

CXTPBufferDC::CXTPBufferDC(HDC hDestDC, const CRect& rcPaint)
	: m_hDestDC (hDestDC)
{
	m_rect = rcPaint;
	Attach (::CreateCompatibleDC (m_hDestDC));
	if (!m_hDC)
		return;

	m_bitmap.Attach (::CreateCompatibleBitmap(
		m_hDestDC, m_rect.right, m_rect.bottom));
	m_hOldBitmap = ::SelectObject (m_hDC, m_bitmap);
}

CXTPBufferDC::CXTPBufferDC(HDC hDestDC, const CRect& rcPaint, const CXTPPaintManagerColorGradient& clrBack, const BOOL bHorz /*=FALSE*/)
	: m_hDestDC (hDestDC)
{
	m_rect = rcPaint;
	Attach (::CreateCompatibleDC (m_hDestDC));
	if (!m_hDC)
		return;

	m_bitmap.Attach (::CreateCompatibleBitmap(
		m_hDestDC, m_rect.right, m_rect.bottom));
	m_hOldBitmap = ::SelectObject (m_hDC, m_bitmap);

	if (!clrBack.IsNull())
	{
		XTPDrawHelpers()->GradientFill(this, m_rect, clrBack, bHorz);
	}
}

CXTPBufferDC::CXTPBufferDC(CPaintDC& paintDC)
{
	m_hDestDC = paintDC.GetSafeHdc();
	m_rect = paintDC.m_ps.rcPaint;

	Attach (::CreateCompatibleDC (m_hDestDC));
	if (!m_hDC)
		return;

	m_bitmap.Attach (::CreateCompatibleBitmap(
		m_hDestDC, max(1, m_rect.right), max(1, m_rect.bottom)));
	m_hOldBitmap = ::SelectObject (m_hDC, m_bitmap);

	CRgn rgn;
	rgn.CreateRectRgnIndirect(&m_rect);

	SelectClipRgn(&rgn);
}


CXTPBufferDC::~CXTPBufferDC()
{
	if (!m_hDC)
		return;

	if (m_hDestDC)
	{
		::BitBlt (m_hDestDC, m_rect.left, m_rect.top, m_rect.Width(),
			m_rect.Height(), m_hDC, m_rect.left, m_rect.top, SRCCOPY);
	}
	::SelectObject (m_hDC, m_hOldBitmap);
}
void CXTPBufferDC::Discard()
{
	m_hDestDC = 0;
}

CDC* CXTPBufferDC::GetDestDC()
{
	return CDC::FromHandle(m_hDestDC);
}

void CXTPBufferDC::TakeSnapshot()
{
	::BitBlt (m_hDC, m_rect.left, m_rect.top, m_rect.Width(),
		m_rect.Height(), m_hDestDC, m_rect.left, m_rect.top, SRCCOPY);
}

//===========================================================================
// CXTPBufferDC class
//===========================================================================

CXTPBufferDCEx::CXTPBufferDCEx(HDC hDestDC, const CRect rcPaint) : m_hDestDC (hDestDC)
{
	m_rect = rcPaint;
	Attach (::CreateCompatibleDC (m_hDestDC));
	m_bitmap = ::CreateCompatibleBitmap(
		m_hDestDC, m_rect.Width(), m_rect.Height());

	m_hOldBitmap = ::SelectObject (m_hDC, m_bitmap);

	SetViewportOrg(-rcPaint.left, -rcPaint.top);
}

CXTPBufferDCEx::~CXTPBufferDCEx()
{
	SetViewportOrg(0, 0);

	::BitBlt (m_hDestDC, m_rect.left, m_rect.top, m_rect.Width(),
		m_rect.Height(), m_hDC, 0, 0, SRCCOPY);
	::SelectObject (m_hDC, m_hOldBitmap);
	::DeleteObject(m_bitmap);
}

//===========================================================================
// CXTPBitmapDC class
//===========================================================================

CXTPBitmapDC::CXTPBitmapDC()
	: m_hOldBitmap(NULL)
{
}

CXTPBitmapDC::CXTPBitmapDC(CDC *pDC, CBitmap *pBitmap)
{
	m_hDC = pDC->GetSafeHdc();
	SetBitmap(HBITMAP(pBitmap->GetSafeHandle()));
}


CXTPBitmapDC::CXTPBitmapDC(CDC* pDC, HBITMAP hBitmap)
{
	m_hDC = pDC->GetSafeHdc();
	SetBitmap(hBitmap);
}

CXTPBitmapDC::~CXTPBitmapDC()
{
	SelectOld();
}

void CXTPBitmapDC::SetBitmap(HBITMAP hBitmap)
{
	SelectOld();
	m_hOldBitmap = HBITMAP(::SelectObject(m_hDC, hBitmap));
}

void CXTPBitmapDC::SelectOld()
{
	if (NULL != m_hOldBitmap)
	{
		::SelectObject(GetSafeHdc(), m_hOldBitmap);
	}
}

//===========================================================================
// CXTPFontDC class
//===========================================================================

CXTPFontDC::CXTPFontDC(CDC* pDC, CFont* pFont)
{
	ASSERT(pDC);

	m_pDC = pDC;
	m_pOldFont = NULL;
	m_clrOldTextColor = COLORREF_NULL;

	if (pFont)
	{
		SetFont(pFont);
	}
}

CXTPFontDC::CXTPFontDC(CDC* pDC, CFont* pFont, COLORREF clrTextColor)
{
	ASSERT(pDC);
	ASSERT(clrTextColor != COLORREF_NULL);

	m_pDC = pDC;
	m_pOldFont = NULL;
	m_clrOldTextColor = COLORREF_NULL;


	if (pFont)
	{
		SetFont(pFont);
	}

	SetColor(clrTextColor);
}

CXTPFontDC::~CXTPFontDC()
{
	ReleaseFont();
	ReleaseColor();
}

void CXTPFontDC::SetFont(CFont* pFont)
{
	if (m_pDC && pFont)
	{
		CFont* pFontPrev = m_pDC->SelectObject(pFont);

		if (!m_pOldFont && pFontPrev)
		{
			m_pOldFont = pFontPrev;
		}
	}
}

void CXTPFontDC::SetColor(COLORREF clrTextColor)
{
	ASSERT(clrTextColor != COLORREF_NULL);
	ASSERT(m_pDC);

	if (m_pDC && clrTextColor != COLORREF_NULL)
	{
		COLORREF clrTextColorPrev= m_pDC->SetTextColor(clrTextColor);

		if (m_clrOldTextColor == COLORREF_NULL)
		{
			m_clrOldTextColor = clrTextColorPrev;
		}
	}
}

void CXTPFontDC::SetFontColor(CFont* pFont, COLORREF clrTextColor)
{
	SetFont(pFont);
	SetColor(clrTextColor);
}

void CXTPFontDC::ReleaseFont()
{
	ASSERT(m_pDC);
	if (m_pDC && m_pOldFont)
	{
		m_pDC->SelectObject(m_pOldFont);
		m_pOldFont = NULL;
	}
}

void CXTPFontDC::ReleaseColor()
{
	ASSERT(m_pDC);
	if (m_pDC && m_clrOldTextColor != COLORREF_NULL)
	{
		m_pDC->SetTextColor(m_clrOldTextColor);
		m_clrOldTextColor = COLORREF_NULL;
	}
}

//===========================================================================
// CXTPPenDC class
//===========================================================================

CXTPPenDC::CXTPPenDC(CDC* pDC, CPen* pPen)
: m_hDC(pDC->GetSafeHdc())
{
	m_hOldPen = (HPEN)::SelectObject(m_hDC, pPen->GetSafeHandle());
}

CXTPPenDC::CXTPPenDC(HDC hDC, COLORREF crColor)
: m_hDC (hDC)
{
	VERIFY(m_pen.CreatePen (PS_SOLID, 1, crColor));
	m_hOldPen = (HPEN)::SelectObject (m_hDC, m_pen);
}

CXTPPenDC::~CXTPPenDC ()
{
	::SelectObject (m_hDC, m_hOldPen);
}

void CXTPPenDC::Color(COLORREF crColor)
{
	::SelectObject (m_hDC, m_hOldPen);
	VERIFY(m_pen.DeleteObject());
	VERIFY(m_pen.CreatePen (PS_SOLID, 1, crColor));
	m_hOldPen = (HPEN)::SelectObject (m_hDC, m_pen);
}

COLORREF CXTPPenDC::Color()
{
	LOGPEN logPen;
	m_pen.GetLogPen(&logPen);
	return logPen.lopnColor;
}

//===========================================================================
// CXTPBrushDC class
//===========================================================================

CXTPBrushDC::CXTPBrushDC(HDC hDC, COLORREF crColor)
: m_hDC (hDC)
{
	VERIFY(m_brush.CreateSolidBrush (crColor));
	m_hOldBrush = (HBRUSH)::SelectObject (m_hDC, m_brush);
}

CXTPBrushDC::~CXTPBrushDC()
{
	::SelectObject(m_hDC, m_hOldBrush);
}

void CXTPBrushDC::Color(COLORREF crColor)
{
	::SelectObject(m_hDC, m_hOldBrush);
	VERIFY(m_brush.DeleteObject());
	VERIFY(m_brush.CreateSolidBrush(crColor));
	m_hOldBrush = (HBRUSH)::SelectObject (m_hDC, m_brush);
}

//===========================================================================
// CXTPCompatibleDC class
//===========================================================================

CXTPCompatibleDC::CXTPCompatibleDC(CDC* pDC, CBitmap* pBitmap)
{
	CreateCompatibleDC(pDC);
	m_hOldBitmap = (HBITMAP)::SelectObject(GetSafeHdc(), pBitmap->GetSafeHandle());
}

CXTPCompatibleDC::CXTPCompatibleDC(CDC* pDC, HBITMAP hBitmap)
{
	CreateCompatibleDC(pDC);
	m_hOldBitmap = (HBITMAP)::SelectObject(GetSafeHdc(), hBitmap);
}

CXTPCompatibleDC::~CXTPCompatibleDC()
{
	::SelectObject(GetSafeHdc(), m_hOldBitmap);
	DeleteDC();
}



//===========================================================================
// CXTPSplitterTracker class
//===========================================================================
CXTPSplitterTracker::CXTPSplitterTracker(BOOL bSolid /*= FALSE*/, BOOL bDesktopDC /*= TRUE*/)
{
	m_bSolid = bSolid;
	m_rcBoundRect.SetRectEmpty();
	m_pDC = 0;
	m_bDesktopDC = bDesktopDC;
	m_pWnd = NULL;
	m_pSplitterWnd = NULL;

	m_pfnSetLayeredWindowAttributes = NULL;

	HMODULE hLib = GetModuleHandle(_T("USER32"));
	if (hLib)
	{
		m_pfnSetLayeredWindowAttributes = (PVOID) ::GetProcAddress(hLib, "SetLayeredWindowAttributes");
	}
}


void CXTPSplitterTracker::OnInvertTracker(CRect rect)
{
	ASSERT(!rect.IsRectEmpty());

	if (!m_bDesktopDC)
	{
		m_pWnd->ScreenToClient(rect);
	}

	if (m_pSplitterWnd)
	{
		m_pSplitterWnd->SetWindowPos(0, rect.left, rect.top, rect.Width(), rect.Height(),
			SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
		return;
	}

	if (!m_pDC)
		return;

	if (m_bSolid)
	{
		m_pDC->InvertRect(rect);
	}
	else
	{
		CBrush* pDitherBrush = CDC::GetHalftoneBrush();
		CBrush* pBrush = (CBrush*)m_pDC->SelectObject(pDitherBrush);

		m_pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);
		m_pDC->SelectObject(pBrush);
	}
}

BOOL CXTPSplitterTracker::Track(CWnd* pTrackWnd, CRect rcAvail, CRect& rectTracker, CPoint point, BOOL bHoriz)
{
	pTrackWnd->SetCapture();
	m_pDC = 0;
	m_pSplitterWnd = NULL;

	if (m_rcBoundRect.IsRectEmpty() && m_bDesktopDC && XTPSystemVersion()->IsWinVistaOrGreater() &&
		m_pfnSetLayeredWindowAttributes && !XTPColorManager()->IsLowResolution())
	{
		m_pSplitterWnd = new CWnd();
		m_pSplitterWnd->CreateEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
			AfxRegisterWndClass(0, AfxGetApp()->LoadStandardCursor(IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH)), 0, WS_POPUP, CRect(0, 0, 0, 0), NULL, 0);

		((PFNSETLAYEREDWINDOWATTRIBUTES)m_pfnSetLayeredWindowAttributes)(m_pSplitterWnd->m_hWnd, 0, 100, LWA_ALPHA);
	}
	else
	{
		if (m_bDesktopDC)
			m_pWnd = CWnd::GetDesktopWindow();
		else
			m_pWnd = pTrackWnd;

		if (m_pWnd->LockWindowUpdate())
			m_pDC = m_pWnd->GetDCEx(NULL, DCX_WINDOW | DCX_CACHE | DCX_LOCKWINDOWUPDATE);
		else
			m_pDC = m_pWnd->GetDCEx(NULL, DCX_WINDOW | DCX_CACHE);
		ASSERT(m_pDC != NULL);
	}

	CPoint ptOffset = bHoriz ? CPoint(rectTracker.left - point.x, 0) :
		CPoint(0, rectTracker.top - point.y);

	OnInvertTracker(rectTracker);

	if (!m_rcBoundRect.IsRectEmpty())
		OnInvertTracker(m_rcBoundRect);

	BOOL bAccept = FALSE;
	while (CWnd::GetCapture() == pTrackWnd)
	{
		MSG msg;
		if (!GetMessage(&msg, NULL, 0, 0))
			break;

		if (msg.message == WM_MOUSEMOVE)
		{
			point = CPoint(msg.lParam);
			pTrackWnd->ClientToScreen(&point);
			point += ptOffset;

			point.x = max(min(point.x, rcAvail.right), rcAvail.left);
			point.y = max(min(point.y, rcAvail.bottom), rcAvail.top);

			if (bHoriz)
			{
				if (rectTracker.left != point.x)
				{
					OnInvertTracker(rectTracker);
					rectTracker.OffsetRect(point.x - rectTracker.left, 0);
					OnInvertTracker(rectTracker);
				}

			}
			else
			{
				if (rectTracker.top != point.y)
				{
					OnInvertTracker(rectTracker);
					rectTracker.OffsetRect(0, point.y - rectTracker.top);
					OnInvertTracker(rectTracker);
				}
			}
		}
		else if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) break;
		else if (msg.message == WM_LBUTTONUP)
		{
			bAccept = TRUE;
			break;
		}
		else  ::DispatchMessage(&msg);
	}

	if (!m_rcBoundRect.IsRectEmpty())
		OnInvertTracker(m_rcBoundRect);

	OnInvertTracker(rectTracker);

	if (CWnd::GetCapture() == pTrackWnd) ReleaseCapture();

	if (m_pSplitterWnd)
	{
		m_pSplitterWnd->DestroyWindow();

		delete m_pSplitterWnd;
		m_pSplitterWnd = NULL;
	}
	else
	{
		m_pWnd->UnlockWindowUpdate();
		if (m_pDC != NULL)
		{
			m_pWnd->ReleaseDC(m_pDC);
			m_pDC = NULL;
		}
	}

	return bAccept;
}

//===========================================================================
// CXTPDrawHelpers class
//===========================================================================

CXTPDrawHelpers::CXTPDrawHelpers()
{
	m_pfnFastGradientFill = 0;
	m_pfnAlphaBlend = 0;
	m_pfnTransparentBlt = 0;

	// Don't use CXTPModuleHandle to reduce dependence between common source
	m_hMsImgDll = ::LoadLibrary(_T("msimg32.dll"));

	if (m_hMsImgDll)
	{
		m_pfnFastGradientFill = (PFNGRADIENTFILL)GetProcAddress(m_hMsImgDll, "GradientFill");
		m_pfnAlphaBlend = (PFNALPHABLEND)::GetProcAddress(m_hMsImgDll, "AlphaBlend");
		m_pfnTransparentBlt = (PFNTRANSPARENTBLT)::GetProcAddress(m_hMsImgDll, "TransparentBlt");
	}
}

CXTPDrawHelpers* AFX_CDECL XTPDrawHelpers()
{
	static CXTPDrawHelpers s_instance; // singleton
	return &s_instance;
}

CXTPDrawHelpers::~CXTPDrawHelpers()
{
	if (m_hMsImgDll != NULL)
	{
		//::FreeLibrary(m_hMsImgDll); Dangerous to call FreeLibrary in destructor of static object.
	}
}

void CXTPDrawHelpers::SolidRectangle(CDC *pDC, CRect rc, COLORREF clrRect, COLORREF clrFill)
{
	pDC->Draw3dRect(rc, clrRect, clrRect);
	rc.DeflateRect(1, 1, 1, 1);
	pDC->FillSolidRect(rc, clrFill);
}

BOOL CXTPDrawHelpers::GradientFill(HDC hdc, PTRIVERTEX pVertex, ULONG dwNumVertex, PVOID pMesh, ULONG dwNumMesh, ULONG dwMode)
{
	if (m_pfnFastGradientFill)
	{
		return (*m_pfnFastGradientFill)(hdc, pVertex, dwNumVertex, pMesh, dwNumMesh, dwMode);
	}

	return FALSE;
}

void CXTPDrawHelpers::GradientFillSlow(CDC* pDC, LPCRECT lpRect, COLORREF crFrom, COLORREF crTo, BOOL bHorz)
{
	int cx = max(1, lpRect->right - lpRect->left);
	int cy = max(1, lpRect->bottom - lpRect->top);

	CRect rc;
	pDC->GetClipBox(&rc);

	if (rc.IsRectEmpty())
		rc = *lpRect;
	else
		rc.IntersectRect(rc, lpRect);

	if (bHorz)
	{
		for (int nX = rc.left; nX < rc.right; nX++)
		{
			pDC->FillSolidRect(nX, rc.top, 1, rc.Height(), BlendColors(
				crFrom, crTo, (double)(1.0 - ((nX - lpRect->left) / (double)cx))));
		}
	}
	else
	{
		for (int nY = rc.top; nY < rc.bottom; nY++)
		{
			pDC->FillSolidRect(rc.left, nY, rc.Width(), 1, BlendColors(
				crFrom, crTo, (double)(1.0 - ((nY - lpRect->top)) / (double)cy)));
		}
	}
}

void CXTPDrawHelpers::GradientFillFast(CDC* pDC, LPCRECT lpRect, COLORREF crFrom, COLORREF crTo, BOOL bHorz)
{
	TRIVERTEX vert[2];
	vert[0].x = lpRect->left;
	vert[0].y = lpRect->top;
	vert[0].Red = (COLOR16)(GetRValue(crFrom) << 8);
	vert[0].Green = (COLOR16)(GetGValue(crFrom) << 8);
	vert[0].Blue = (COLOR16)(GetBValue(crFrom) << 8);
	vert[0].Alpha = 0x0000;

	vert[1].x = lpRect->right;
	vert[1].y = lpRect->bottom;
	vert[1].Red = (COLOR16)(GetRValue(crTo) << 8);
	vert[1].Green = (COLOR16)(GetGValue(crTo) << 8);
	vert[1].Blue = (COLOR16)(GetBValue(crTo) << 8);
	vert[1].Alpha = 0x0000;

	GRADIENT_RECT gRect = { 0, 1 };

	GradientFill(*pDC, vert, 2, &gRect, 1, bHorz ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V);
}

void CXTPDrawHelpers::GradientFill(CDC* pDC, LPCRECT lpRect, COLORREF crFrom, COLORREF crTo, BOOL bHorz)
{
	if (!lpRect)
		return;

	if (::IsRectEmpty(lpRect))
		return;

	if (IsLowResolution(pDC->GetSafeHdc()))
	{
		pDC->FillSolidRect(lpRect, crFrom);
	}
	else if (crFrom == crTo)
	{
		pDC->FillSolidRect(lpRect, crFrom);
	}
	else if ((m_pfnFastGradientFill == NULL) || (IsContextRTL(pDC) && XTPSystemVersion()->IsWin9x()))
	{
		GradientFillSlow(pDC, lpRect, crFrom, crTo, bHorz);
	}
	else
	{
		GradientFillFast(pDC, lpRect, crFrom, crTo, bHorz);
	}
}

void CXTPDrawHelpers::GradientFill(CDC* pDC, LPCRECT lpRect, COLORREF crFrom, COLORREF crTo, BOOL bHorz, LPCRECT lpRectClip)
{
	CRect rc(lpRect);

	if (lpRectClip == NULL)
	{
		GradientFill(pDC, lpRect, crFrom, crTo, bHorz);
		return;
	}

	COLORREF crFrom1 = crFrom;

	if (bHorz)
	{
		if (rc.top < lpRectClip->top)
		{
			rc.top = lpRectClip->top;
		}

		if (rc.bottom > lpRectClip->bottom)
		{
			rc.bottom = lpRectClip->bottom;
		}

		if ((rc.left > lpRectClip->right) || (rc.right < lpRectClip->left))
			return;

		if (rc.left < lpRectClip->left)
		{
			rc.left = lpRectClip->left;

			crFrom = BlendColors(crFrom, crTo,
				(float)(lpRect->right - lpRectClip->left) / (float)(lpRect->right - lpRect->left));
		}

		if (rc.right > lpRectClip->right)
		{
			rc.right = lpRectClip->right;

			crTo = BlendColors(crFrom1, crTo,
				(float)(lpRect->right - lpRectClip->right) / (float)(lpRect->right - lpRect->left));

		}

		GradientFill(pDC, rc, crFrom, crTo, bHorz);

	}
	else
	{
		if (rc.left < lpRectClip->left)
		{
			rc.left = lpRectClip->left;
		}

		if (rc.right > lpRectClip->right)
		{
			rc.right = lpRectClip->right;
		}

		if ((rc.top > lpRectClip->bottom) || (rc.bottom < lpRectClip->top))
			return;

		if (rc.top < lpRectClip->top)
		{
			rc.top = lpRectClip->top;

			crFrom = BlendColors(crFrom, crTo,
				(float)(lpRect->bottom - lpRectClip->top) / (float)(lpRect->bottom - lpRect->top));
		}

		if (rc.bottom > lpRectClip->bottom)
		{
			rc.bottom = lpRectClip->bottom;

			crTo = BlendColors(crFrom1, crTo,
				(float)(lpRect->bottom - lpRectClip->bottom) / (float)(lpRect->bottom - lpRect->top));

		}

		GradientFill(pDC, rc, crFrom, crTo, bHorz);
	}
}

void CXTPDrawHelpers::GradientFill(CDC* pDC, LPCRECT lpRect, const CXTPPaintManagerColorGradient& grc, BOOL bHorz, LPCRECT lpRectClip)
{
	// using gradient factor color gradient fill
	if (grc.fGradientFactor != 0.5f)
	{
		COLORREF clrMid = BlendColors(grc.clrLight, grc.clrDark, grc.fGradientFactor);

		if (bHorz)
		{
			CRect rcLeft(lpRect);
			rcLeft.right -= rcLeft.Width()/2;
			GradientFill(pDC, &rcLeft, grc.clrLight, clrMid, bHorz, lpRectClip);

			CRect rcRight(lpRect);
			rcRight.left = rcLeft.right;
			GradientFill(pDC, &rcRight, clrMid, grc.clrDark, bHorz, lpRectClip);
		}
		else
		{
			CRect rcTop(lpRect);
			rcTop.bottom -= rcTop.Height()/2;
			GradientFill(pDC, &rcTop, grc.clrLight, clrMid, bHorz, lpRectClip);

			CRect rcBottom(lpRect);
			rcBottom.top = rcTop.bottom;
			GradientFill(pDC, &rcBottom, clrMid, grc.clrDark, bHorz, lpRectClip);
		}
	}
	// using 2 color gradient fill
	else
	{
		GradientFill(pDC, lpRect, grc.clrLight, grc.clrDark, bHorz, lpRectClip);
	}
}

void CXTPDrawHelpers::ExcludeCorners(CDC* pDC, CRect rc, BOOL bTopCornersOnly /*= FALSE*/)
{
	pDC->ExcludeClipRect(rc.left, rc.top, rc.left + 1, rc.top + 1);
	pDC->ExcludeClipRect(rc.right - 1, rc.top, rc.right, rc.top + 1);

	if (bTopCornersOnly == FALSE)
	{
		pDC->ExcludeClipRect(rc.left, rc.bottom - 1, rc.left + 1, rc.bottom);
		pDC->ExcludeClipRect(rc.right - 1, rc.bottom - 1, rc.right, rc.bottom);
	}
}

void CXTPDrawHelpers::StripMnemonics(CString& strClear)
{
	for (int i = 0; i < strClear.GetLength(); i++)
	{
		if (strClear[i] == _T('&')) // Converts "&&" to "&" and "&&&&" to "&&"
		{
			strClear.Delete(i);
		}
	}
}

void CXTPDrawHelpers::StripMnemonics(LPTSTR lpszClear)
{
	if (lpszClear == NULL || lpszClear == LPSTR_TEXTCALLBACK)
		return;

	LPTSTR lpszResult = lpszClear;

	while (*lpszClear)
	{
		if (*lpszClear == _T('&') && *(lpszClear + 1) != _T('&'))
		{
			lpszClear++;
			continue;
		}

		*lpszResult++ = *lpszClear++;
	}

	*lpszResult = 0;
}

void CXTPDrawHelpers::BlurPoints(CDC* pDC, LPPOINT pts, int nCount)
{
	for (int i = 0; i < nCount; i += 2)
	{
		CPoint ptBlur = pts[i];
		CPoint ptDirection(pts[i].x + pts[i + 1].x, pts[i].y + pts[i + 1].y);

		COLORREF clrBlur = pDC->GetPixel(ptDirection);
		COLORREF clrDirection = pDC->GetPixel(ptBlur);

		pDC->SetPixel(ptBlur, RGB(
			(GetRValue(clrBlur) + GetRValue(clrDirection)) / 2,
			(GetGValue(clrBlur) + GetGValue(clrDirection)) / 2,
			(GetBValue(clrBlur) + GetBValue(clrDirection)) / 2));
	}
}

COLORREF CXTPDrawHelpers::BlendColors(COLORREF crA, COLORREF crB, double fAmountA)
{
	double fAmountB = (1.0 - fAmountA);
	int btR = (int)(GetRValue(crA) * fAmountA + GetRValue(crB) * fAmountB);
	int btG = (int)(GetGValue(crA) * fAmountA + GetGValue(crB) * fAmountB);
	int btB = (int)(GetBValue(crA) * fAmountA + GetBValue(crB) * fAmountB);

	return RGB(min(255, btR), (BYTE)min(255, btG), (BYTE)min(255, btB));
}

COLORREF CXTPDrawHelpers::DarkenColor(long lScale, COLORREF lColor)
{
	long red   = MulDiv(GetRValue(lColor), (255 - lScale), 255);
	long green = MulDiv(GetGValue(lColor), (255 - lScale), 255);
	long blue  = MulDiv(GetBValue(lColor), (255 - lScale), 255);

	return RGB(red, green, blue);
}

COLORREF CXTPDrawHelpers::LightenColor(long lScale, COLORREF lColor)
{
	long R = MulDiv(255 - GetRValue(lColor), lScale, 255) + GetRValue(lColor);
	long G = MulDiv(255 - GetGValue(lColor), lScale, 255) + GetGValue(lColor);
	long B = MulDiv(255 - GetBValue(lColor), lScale, 255) + GetBValue(lColor);

	return RGB(R, G, B);
}

CPoint CXTPDrawHelpers::Dlu2Pix(int dluX, int dluY)
{
	CPoint baseXY(::GetDialogBaseUnits());
	CPoint pixXY(0,0);
	pixXY.x = ::MulDiv(dluX, baseXY.x, 4);
	pixXY.y = ::MulDiv(dluY, baseXY.y, 8);
	return pixXY;
}

COLORREF CXTPDrawHelpers::RGBtoHSL(COLORREF rgb)
{
	int delta, sum;
	int nH, nS, nL;
	int r = GetRValue(rgb);
	int g = GetGValue(rgb);
	int b = GetBValue(rgb);
	int cmax = ((r) >= (g) ? ((r) >= (b) ? (r) : (b)) : (g) >= (b) ? (g) : (b));
	int cmin = ((r) <= (g) ? ((r) <= (b) ? (r) : (b)) : (g) <= (b) ? (g) : (b));

	nL = (cmax + cmin + 1) / 2;
	if (cmax == cmin)
	{
		nH = 255; // H is really undefined
		nS = 0;
	}
	else
	{
		delta = cmax - cmin;
		sum = cmax + cmin;
		if (nL < 127)
			nS = ((delta + 1) * 256) / sum;
		else
			nS = (delta * 256) / ((2 * 256) - sum);
		if (r == cmax)
			nH = ((g - b) * 256) / delta;
		else if (g == cmax)
			nH = (2 * 256) + ((b - r) * 256) / delta;
		else
			nH = (4 * 256) + ((r - g) * 256) / delta;
		nH /= 6;
		if (nH < 0)
			nH += 256;
	}
	nH = nH * 239 / 255;
	nS = nS * 240 / 255;
	nL = nL * 240 / 255;

	return RGB((BYTE)min(nH, 239), (BYTE)min(nS, 240), (BYTE)min(nL, 240));
}

void CXTPDrawHelpers::RGBtoHSL(COLORREF clr, double& h, double& s, double& l)
{
	double r = (double)GetRValue(clr)/255.0;
	double g = (double)GetGValue(clr)/255.0;
	double b = (double)GetBValue(clr)/255.0;

	double maxcolor = __max(r, __max(g, b));
	double mincolor = __min(r, __min(g, b));

	l = (maxcolor + mincolor)/2;

	if (maxcolor == mincolor)
	{
		h = 0;
		s = 0;
	}
	else
	{
		if (l < 0.5)
			s = (maxcolor-mincolor)/(maxcolor + mincolor);
		else
			s = (maxcolor-mincolor)/(2.0-maxcolor-mincolor);

		if (r == maxcolor)
			h = (g-b)/(maxcolor-mincolor);
		else if (g == maxcolor)
			h = 2.0+(b-r)/(maxcolor-mincolor);
		else
			h = 4.0+(r-g)/(maxcolor-mincolor);

		h /= 6.0;
		if (h < 0.0) h += 1;
	}
}

double CXTPDrawHelpers::HueToRGB(double temp1, double temp2, double temp3)
{
	if (temp3 < 0)
		temp3 = temp3 + 1.0;
	if (temp3 > 1)
		temp3 = temp3-1.0;

	if (6.0*temp3 < 1)
		return (temp1+(temp2-temp1)*temp3 * 6.0);

	else if (2.0*temp3 < 1)
		return temp2;

	else if (3.0*temp3 < 2.0)
		return (temp1+(temp2-temp1)*((2.0/3.0)-temp3)*6.0);

	return temp1;
}

int CXTPDrawHelpers::HueToRGB(int m1, int m2, int h)
{
	if (h < 0)
		h += 255;

	if (h > 255)
		h -= 255;

	if ((6 * h) < 255)
		return ((m1 + ((m2 - m1) / 255 * h * 6)) / 255);

	if ((2 * h) < 255)
		return m2 / 255;

	if ((3 * h) < (2 * 255))
		return ((m1 + (m2 - m1) / 255 * ((255 * 2 / 3) - h) * 6) / 255);

	return (m1 / 255);
}

COLORREF CXTPDrawHelpers::HSLtoRGB(COLORREF hsl)
{
	int r, g, b;
	int m1, m2;
	int nH = GetRValue(hsl) * 255 / 239;
	int nS = GetGValue(hsl) * 255 / 240;
	int nL = GetBValue(hsl) * 255 / 240;

	if (nS == 0)
		r = g = b = nL;
	else
	{
		if (nL <= 127)
			m2 = nL * (255 + nS);
		else
			m2 = (nL + nS - ((nL * nS) / 255)) * 255;
		m1 = (2 * 255 * nL) - m2;
		r = HueToRGB(m1, m2, nH + (255 / 3));
		g = HueToRGB(m1, m2, nH);
		b = HueToRGB(m1, m2, nH - (255 / 3));
	}
	return RGB((BYTE)min(r, 255), (BYTE)min(g, 255), (BYTE)min(b, 255));
}

COLORREF CXTPDrawHelpers::HSLtoRGB(double h, double s, double l)
{
	double r, g, b;
	double temp1, temp2;

	if (s == 0)
	{
		r = g = b = l;
	}
	else
	{
		if (l < 0.5)
			temp2 = l*(1.0 + s);
		else
			temp2 = l + s-l*s;

		temp1 = 2.0 * l-temp2;

		r = HueToRGB(temp1, temp2, h + 1.0/3.0);
		g = HueToRGB(temp1, temp2, h);
		b = HueToRGB(temp1, temp2, h - 1.0/3.0);
	}

	return RGB((BYTE)(r * 255), (BYTE)(g * 255), (BYTE)(b * 255));
}

static int CALLBACK XTPEnumFontFamExProc(ENUMLOGFONTEX* pelf, NEWTEXTMETRICEX* /*lpntm*/, int /*FontType*/, LPVOID pThis)
{
	LPCTSTR strFontName = (LPCTSTR)pThis;
	CString strFaceName = pelf->elfLogFont.lfFaceName;

	if (strFaceName.CompareNoCase(strFontName) == 0)
		return 0;

	return 1;
}

BOOL CXTPDrawHelpers::FontExists(LPCTSTR strFaceName)
{
	// Enumerate all styles and charsets of all fonts:
	LOGFONT lfEnum;
	::ZeroMemory(&lfEnum, sizeof(LOGFONT));

	lfEnum.lfFaceName[ 0 ] = 0;
	lfEnum.lfCharSet = DEFAULT_CHARSET;

	CWindowDC dc(NULL);

	return  ::EnumFontFamiliesEx(dc.m_hDC, &lfEnum, (FONTENUMPROC)
		XTPEnumFontFamExProc, (LPARAM)strFaceName, 0) == 0;
}

CString CXTPDrawHelpers::GetDefaultFontName()
{
	LOGFONT lfFont;
	ZeroMemory(&lfFont, sizeof(LOGFONT));
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lfFont);

	return CString(lfFont.lfFaceName);
}

CString AFX_CDECL CXTPDrawHelpers::GetVerticalFontName(BOOL bUseOfficeFont)
{
	LOGFONT lfFont;
	ZeroMemory(&lfFont, sizeof(LOGFONT));
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lfFont);
	bool bUseSystemFont = lfFont.lfCharSet > SYMBOL_CHARSET;

	if (bUseSystemFont && !XTPSystemVersion()->IsWin2KOrGreater())
		bUseSystemFont = FALSE;

	if (bUseSystemFont && (_tcsicmp(lfFont.lfFaceName, _T("MS Shell Dlg")) == 0))
		bUseSystemFont = FALSE; // Can draw it vertically in Windows 2000.

	CString strVerticalFaceName = _T("Arial");
	LPCTSTR strOfficeFont = _T("Tahoma");

	if (bUseSystemFont || !FontExists(strVerticalFaceName))
	{
		strVerticalFaceName = lfFont.lfFaceName;
	}
	else if (bUseOfficeFont && !bUseSystemFont && FontExists(strOfficeFont))
	{
		strVerticalFaceName = strOfficeFont;
	}

	return strVerticalFaceName;
}

DWORD CXTPDrawHelpers::GetLayout(CDC *pDC)
{
	DWORD dwLayout = 0;

	if (NULL != pDC)
	{
		dwLayout = GetLayout(pDC->GetSafeHdc());
	}

	return dwLayout;
}

DWORD CXTPDrawHelpers::GetLayout(HDC hDC)
{
	typedef DWORD (CALLBACK* PFNGDIGETLAYOUTPROC)(HDC);
	static PFNGDIGETLAYOUTPROC s_pfnGetLayout = NULL;

	if (NULL == s_pfnGetLayout)
	{
		HINSTANCE hInstance = ::GetModuleHandle(_T("GDI32.DLL"));
		s_pfnGetLayout = (PFNGDIGETLAYOUTPROC)::GetProcAddress(hInstance, "GetLayout");
		ASSERT(NULL != s_pfnGetLayout); // No entry point for GetLayout
	}

	DWORD dwLayout = 0;

	if (NULL != s_pfnGetLayout)
	{
		dwLayout = s_pfnGetLayout(hDC);
	}

	return dwLayout;
}

BOOL CXTPDrawHelpers::IsContextRTL(CDC *pDC)
{
	BOOL bIsContextRTL = FALSE;

	if (NULL != pDC)
	{
		bIsContextRTL = IsContextRTL(pDC->GetSafeHdc());
	}

	return bIsContextRTL;
}

BOOL CXTPDrawHelpers::IsContextRTL(HDC hDC)
{
	DWORD dwLayout = GetLayout(hDC);
	return dwLayout;
}

void CXTPDrawHelpers::SetContextRTL(CDC* pDC, BOOL bLayoutRTL)
{
	if (pDC) SetContextRTL(pDC->GetSafeHdc(), bLayoutRTL);
}

void CXTPDrawHelpers::SetContextRTL(HDC hDC, BOOL bLayoutRTL)
{
	typedef DWORD (CALLBACK* PFNGDISETLAYOUTPROC)(HDC, DWORD);
	static PFNGDISETLAYOUTPROC s_pfn = (PFNGDISETLAYOUTPROC)-1;

	if (s_pfn == (PFNGDISETLAYOUTPROC)-1)
	{
		HINSTANCE hInst = ::GetModuleHandleA("GDI32.DLL");

		s_pfn = hInst ? (PFNGDISETLAYOUTPROC)GetProcAddress(hInst, "SetLayout") : NULL;
	}

	if (s_pfn != NULL)
	{
		(*s_pfn)(hDC, bLayoutRTL);
	}
}


void CXTPDrawHelpers::KeyToLayout(CWnd* pWnd, UINT& nChar)
{
	ASSERT(pWnd);
	if (!pWnd || !pWnd->GetSafeHwnd())
		return;

	if (nChar == VK_LEFT && pWnd->GetExStyle() & WS_EX_LAYOUTRTL)
		nChar = VK_RIGHT;
	else if (nChar == VK_RIGHT && pWnd->GetExStyle() & WS_EX_LAYOUTRTL)
		nChar = VK_LEFT;

}

void CXTPDrawHelpers::Triangle(CDC* pDC, CPoint pt0, CPoint pt1, CPoint pt2, COLORREF clr)
{
	CXTPPenDC pen (*pDC, clr);
	CXTPBrushDC brush (*pDC, clr);

	Triangle(pDC, pt0, pt1, pt2);
}

BOOL CXTPDrawHelpers::DrawLine(CDC *pDC, int x1, int y1, int x2, int y2, COLORREF crLine)
{
	if (pDC->GetSafeHdc())
	{
		CXTPPenDC penDC(*pDC, crLine);
		pDC->MoveTo(x1, y1);
		pDC->LineTo(x2, y2);
		return TRUE;
	}
	return FALSE;
}

DWORD CXTPDrawHelpers::GetComCtlVersion()
{
	return XTPSystemVersion()->GetComCtlVersion();
}

BOOL CXTPDrawHelpers::IsLowResolution(HDC hDC/* = 0*/)
{
	return XTPColorManager()->IsLowResolution(hDC);
}

CRect CXTPDrawHelpers::GetWorkArea(LPCRECT rect)
{
	return XTPMultiMonitor()->GetWorkArea(rect);
}

CRect CXTPDrawHelpers::GetWorkArea(const CWnd* pWnd)
{
	return XTPMultiMonitor()->GetWorkArea(pWnd);
}

CRect CXTPDrawHelpers::GetScreenArea(const CWnd* pWnd)
{
	return XTPMultiMonitor()->GetScreenArea(pWnd);
}

CRect CXTPDrawHelpers::GetWorkArea(const POINT& point)
{
	return XTPMultiMonitor()->GetWorkArea(point);
}

CRect CXTPDrawHelpers::GetWorkArea()
{
	return XTPMultiMonitor()->GetWorkArea();
}

BOOL CXTPDrawHelpers::TakeSnapShot(CWnd* pWnd, CBitmap& bmpSnapshot)
{
	if (!::IsWindow(pWnd->GetSafeHwnd()))
		return FALSE;

	CWnd *pWndParent = pWnd->GetParent();
	if (::IsWindow(pWndParent->GetSafeHwnd()))
	{
		if (bmpSnapshot.GetSafeHandle() != NULL)
			bmpSnapshot.DeleteObject();

		//convert our coordinates to our parent coordinates.
		CXTPWindowRect rc(pWnd);
		pWndParent->ScreenToClient(&rc);

		//copy what's on the parents background at this point
		CDC *pDC = pWndParent->GetDC();

		CDC memDC;
		memDC.CreateCompatibleDC(pDC);
		bmpSnapshot.CreateCompatibleBitmap(pDC, rc.Width(), rc.Height());

		CXTPBitmapDC bitmapDC(&memDC, &bmpSnapshot);
		memDC.BitBlt(0, 0, rc.Width(), rc.Height(), pDC, rc.left, rc.top, SRCCOPY);

		pWndParent->ReleaseDC(pDC);

		return TRUE;
	}

	return FALSE;
}

BOOL CXTPDrawHelpers::DrawTransparentBack(CDC* pDC, CWnd* pWnd, CBitmap& bmpSnapshot)
{
	if (!::IsWindow(pWnd->GetSafeHwnd()))
		return FALSE;

	if (::GetWindowLong(pWnd->GetSafeHwnd(), GWL_EXSTYLE) & WS_EX_TRANSPARENT)
	{
		// Get background.
		if (!TakeSnapShot(pWnd, bmpSnapshot))
			return FALSE;

		CXTPClientRect rc(pWnd);

		CDC memDC;
		memDC.CreateCompatibleDC(pDC);

		CXTPBitmapDC bitmapDC(&memDC, &bmpSnapshot);
		pDC->BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);

		return TRUE;
	}

	return FALSE;
}

BOOL CXTPDrawHelpers::IsTopParentActive(HWND hWnd)
{
	HWND hwndForeground = ::GetForegroundWindow();

	HWND hWndT;
	while ((hWndT = AfxGetParentOwner(hWnd)) != NULL)
	{
#ifdef _XTP_ACTIVEX
		if (hwndForeground == hWndT)
			return TRUE;
#endif
		hWnd = hWndT;
	}

	HWND hwndActivePopup = ::GetLastActivePopup(hWnd);

	if (hwndForeground == hwndActivePopup)
		return TRUE;

#ifdef _XTP_ACTIVEX
	if (hwndActivePopup && ::GetWindowLong(hwndForeground, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
	{
		if (::GetParent(hwndForeground) == hwndActivePopup)
			return TRUE;
	}
#endif

	return FALSE;
}
void CXTPDrawHelpers::ScreenToWindow(CWnd* pWnd, LPPOINT lpPoint)
{
	RECT rc;
	::GetWindowRect(pWnd->GetSafeHwnd(), &rc);

	lpPoint->y -= rc.top;

	if (GetWindowLong(pWnd->GetSafeHwnd(), GWL_EXSTYLE) & WS_EX_LAYOUTRTL )
	{
		lpPoint->x = rc.right - lpPoint->x;
	}
	else
	{
		lpPoint->x -= rc.left;
	}
}

BOOL AFX_CDECL CXTPDrawHelpers::RegisterWndClass(HINSTANCE hInstance, LPCTSTR lpszClassName, UINT style, HICON hIcon, HBRUSH hbrBackground)
{
	WNDCLASS wndcls;
	ZeroMemory(&wndcls, sizeof(wndcls));

	wndcls.style = style;
	wndcls.lpfnWndProc = ::DefWindowProc;
	wndcls.hInstance = hInstance ? hInstance : XTPGetInstanceHandle();
	wndcls.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
	wndcls.lpszClassName = lpszClassName;
	wndcls.hIcon = hIcon;
	wndcls.hbrBackground = hbrBackground;

	return AfxRegisterClass(&wndcls);
}

void CXTPDrawHelpers::GetWindowCaption(HWND hWnd, CString& strWindowText)
{
#ifdef _UNICODE
	int nLen = (int)::DefWindowProc(hWnd, WM_GETTEXTLENGTH, 0, 0);
	::DefWindowProc(hWnd, WM_GETTEXT, nLen + 1, (WPARAM)(LPCTSTR)strWindowText.GetBuffer(nLen));
	strWindowText.ReleaseBuffer();
#else
	int nLen = ::GetWindowTextLength(hWnd);
	::GetWindowText(hWnd, strWindowText.GetBufferSetLength(nLen), nLen + 1);
	strWindowText.ReleaseBuffer();
#endif
}

BOOL AFX_CDECL XTPTrackMouseEvent(HWND hWndTrack, DWORD dwFlags /*= TME_LEAVE*/, DWORD dwHoverTime /*= HOVER_DEFAULT*/)
{
	ASSERT(::IsWindow(hWndTrack));

	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = dwFlags;
	tme.dwHoverTime = dwHoverTime;
	tme.hwndTrack = hWndTrack;

	return ::_TrackMouseEvent(&tme);
}

BOOL AFX_CDECL XTPGetPrinterDeviceDefaults(HGLOBAL& ref_hDevMode, HGLOBAL& ref_hDevNames)
{
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		PRINTDLG _pd;
		::ZeroMemory(&_pd, sizeof(_pd));
		if (pApp->GetPrinterDeviceDefaults(&_pd))
		{
			ref_hDevMode = _pd.hDevMode;
			ref_hDevNames = _pd.hDevNames;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CXTPDrawHelpers::GetIconLogFont(LOGFONT* plf)
{
	VERIFY(::SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), plf, 0));
	plf->lfCharSet = XTPResourceManager()->GetFontCharset();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CXTPPrinterInfo, CCmdTarget)

CXTPPrinterInfo::CXTPPrinterInfo()
{

#ifdef _XTP_ACTIVEX
	EnableAutomation();
	EnableTypeLib();
#endif
}

CXTPPrinterInfo::~CXTPPrinterInfo()
{

}

CString CXTPPrinterInfo::GetName(XTPEnumDeviceName eNameID)
{
	CString strName;
	HGLOBAL hDevMode = NULL, hDevNames = NULL;

	if (XTPGetPrinterDeviceDefaults(hDevMode, hDevNames) && hDevNames != NULL)
	{
		LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(hDevNames);
		if (lpDev)
		{
			strName = (LPCTSTR)lpDev + _GetNameOffset(lpDev, eNameID);
		}
		::GlobalUnlock(hDevNames);
	}
	return strName;
}

WORD CXTPPrinterInfo::_GetNameOffset(LPDEVNAMES pDevNames, XTPEnumDeviceName eNameID)
{
	ASSERT(pDevNames);
	if (!pDevNames)
		return 0;

	switch (eNameID)
	{
	case xtpDevName_Driver:
		return pDevNames->wDriverOffset;
	case xtpDevName_Device:
		return pDevNames->wDeviceOffset;
	case xtpDevName_Port:
		return pDevNames->wOutputOffset;
	};

	ASSERT(FALSE);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CXTPPrintPageHeaderFooter, CCmdTarget)

CXTPPrintPageHeaderFooter::CXTPPrintPageHeaderFooter(CXTPPrintOptions* pOwner, BOOL bHeader)
{
	ASSERT(pOwner);

	m_bHeader = bHeader;
	m_pOwner = pOwner;

	VERIFY(CXTPDrawHelpers::GetIconLogFont(&m_lfFont));
	m_clrColor = RGB(0, 0, 0);

#ifdef _XTP_ACTIVEX
	EnableAutomation();
	EnableTypeLib();
#endif
}

CXTPPrintPageHeaderFooter::~CXTPPrintPageHeaderFooter()
{
}

void CXTPPrintPageHeaderFooter::Set(const CXTPPrintPageHeaderFooter* pSrc)
{
	ASSERT(pSrc);
	if (!pSrc)
		return;

	m_lfFont = pSrc->m_lfFont;
	m_clrColor = pSrc->m_clrColor;

	m_strFormatString = pSrc->m_strFormatString;

	m_strLeft = pSrc->m_strLeft;
	m_strCenter = pSrc->m_strCenter;
	m_strRight = pSrc->m_strRight;
}

void CXTPPrintPageHeaderFooter::Clear()
{
	VERIFY(CXTPDrawHelpers::GetIconLogFont(&m_lfFont));
	m_clrColor = RGB(0, 0, 0);
	m_strFormatString.Empty();

	m_strLeft.Empty();
	m_strCenter.Empty();
	m_strRight.Empty();
}

BOOL CXTPPrintPageHeaderFooter::IsEmpty()
{
	return m_strLeft.IsEmpty() && m_strCenter.IsEmpty() && m_strRight.IsEmpty();
}

//===========================================================================
//  &w      Window title
//
//  &d      Date in short format (as specified by Regional Settings in Control Panel)
//  &D      Date in long format (as specified by Regional Settings in Control Panel)
//
//  &t      Time in the format specified by Regional Settings in Control Panel
//  &T      Time in 24-hour format
//
//  &p      Current page number
//  &P      Total number of pages
//  &#      Ordinal page number (even for WYSIWYG mode)
//
//  &b      The text immediately following these characters as centered.
//  &b&b    The text immediately following the first "&b" as centered, and the text following the second "&b" as right-justified.
//
//  &&      A single ampersand (&)
//===========================================================================
void CXTPPrintPageHeaderFooter::FormatTexts(CPrintInfo* pInfo, LPCTSTR pcszWndTitle, int nVirtualPage)
{
	ASSERT(pInfo);
	if (m_strFormatString.IsEmpty() || !pInfo)
		return;

	LCID lcidLocale = m_pOwner ? m_pOwner->GetActiveLCID() : LOCALE_USER_DEFAULT;

	CString strFormat = m_strFormatString;
	strFormat.Replace(_T("&&"), _T("\1"));

	CString str_p, str_P = _T("*");
	if (nVirtualPage > 0)
		str_p.Format(_T("%d-%d"), pInfo->m_nCurPage, nVirtualPage);
	else
		str_p.Format(_T("%d"), pInfo->m_nCurPage);

	if (pInfo->GetMaxPage() != 65535)
		str_P.Format(_T("%d"), pInfo->GetMaxPage());

	strFormat.Replace(_T("&p"), str_p);
	strFormat.Replace(_T("&P"), str_P);

	strFormat.Replace(_T("\\n"), _T("\n"));

	_SplitFormatLCR(strFormat);

	_FormatDateTime(m_strLeft, lcidLocale);
	_FormatDateTime(m_strCenter, lcidLocale);
	_FormatDateTime(m_strRight, lcidLocale);

	if (pcszWndTitle)
	{
		m_strLeft.Replace(_T("&w"), pcszWndTitle);
		m_strCenter.Replace(_T("&w"), pcszWndTitle);
		m_strRight.Replace(_T("&w"), pcszWndTitle);
	}

	m_strLeft.Replace( _T("\1"), _T("&"));
	m_strCenter.Replace(_T("\1"), _T("&"));
	m_strRight.Replace(_T("\1"), _T("&"));
}

CString CXTPPrintPageHeaderFooter::GetParentFrameTitle(CWnd* pWnd)
{
	CString strTitle;

	ASSERT(pWnd);
	if (!pWnd)
		return strTitle;

	pWnd->GetWindowText(strTitle);
	if (strTitle.IsEmpty() || !pWnd->IsFrameWnd())
	{
		if (pWnd->GetParentFrame())
			pWnd->GetParentFrame()->GetWindowText(strTitle);

		if (strTitle.IsEmpty())
		{
			if (pWnd->GetParentOwner())
				pWnd->GetParentOwner()->GetWindowText(strTitle);
		}
	}
	return strTitle;
}

void CXTPPrintPageHeaderFooter::_FormatDateTime(CString& strFormat, LCID lcidLocale)
{
	SYSTEMTIME stNow;
	::GetLocalTime(&stNow);

	const int cnBufferSize = 200;
	TCHAR szDateFormatShort[cnBufferSize] = {0};
	TCHAR szDateFormatLong[cnBufferSize] = {0};

	TCHAR szTimeFormat[cnBufferSize] = {0};
	TCHAR szTimeFormat24[cnBufferSize] = {0};

	::GetDateFormat(lcidLocale, DATE_SHORTDATE, &stNow, NULL, szDateFormatShort, cnBufferSize);
	::GetDateFormat(lcidLocale, DATE_LONGDATE, &stNow, NULL, szDateFormatLong, cnBufferSize);

	::GetTimeFormat(lcidLocale, 0, &stNow, NULL, szTimeFormat, cnBufferSize);
	DWORD dwFlags24h = TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER;
	::GetTimeFormat(lcidLocale, dwFlags24h, &stNow, NULL, szTimeFormat24, cnBufferSize);

	strFormat.Replace(_T("&d"), szDateFormatShort); // &d   Date in short format (as specified by Regional Settings in Control Panel)
	strFormat.Replace(_T("&D"), szDateFormatLong);  // &D   Date in long format (as specified by Regional Settings in Control Panel)

	strFormat.Replace(_T("&t"), szTimeFormat);   // &t  Time in the format specified by Regional Settings in Control Panel
	strFormat.Replace(_T("&T"), szTimeFormat24); // &T  Time in 24-hour format
}

void CXTPPrintPageHeaderFooter::_SplitFormatLCR(CString strFormat)
{
	m_strLeft.Empty();
	m_strCenter.Empty();
	m_strRight.Empty();

	int nAmpBidx = strFormat.Find(_T("&b"));
	if (nAmpBidx < 0)
	{
		m_strLeft = strFormat;
	}
	else if (nAmpBidx >= 0)
	{
		if (nAmpBidx > 0)
			m_strLeft = strFormat.Left(nAmpBidx);

		strFormat.Delete(0, nAmpBidx + 2);

		nAmpBidx = strFormat.Find(_T("&b"));
		if (nAmpBidx < 0)
		{
			m_strCenter = strFormat;
		}
		else if (nAmpBidx >= 0)
		{
			if (nAmpBidx > 0)
				m_strCenter = strFormat.Left(nAmpBidx);
			strFormat.Delete(0, nAmpBidx + 2);

			m_strRight = strFormat;
		}
	}
}

int CXTPPrintPageHeaderFooter::fnYi(int Xi, int Wi)
{
	int nYi = Wi / Xi;
	int nYi_ = Wi % Xi;
	return nYi + (nYi_ ? 1 : 0);
}


int CXTPPrintPageHeaderFooter::_Calc3ColSizesIfSingleCol(CDC* pDC, int nW,
		const CString& str1, const CString& str2, const CString& str3,
		CSize& rsz1, CSize& rsz2, CSize& rsz3)
{
	CString str1trim = str1;
	CString str2trim = str2;
	CString str3trim = str3;

	str1trim.TrimLeft();
	str2trim.TrimLeft();
	str3trim.TrimLeft();

	UINT uFlags = DT_TOP | DT_WORDBREAK | DT_CALCRECT;
	rsz1 = rsz2 = rsz3 = CSize(0, 0);

	if (!str1trim.IsEmpty() && str2trim.IsEmpty() && str3trim.IsEmpty())
	{
		CRect rcText1(0, 0, nW, 32000);
		rsz1.cx = nW;
		return rsz1.cy = pDC->DrawText(str1, &rcText1, uFlags | DT_LEFT);

	}

	if (str1trim.IsEmpty() && !str2trim.IsEmpty() && str3trim.IsEmpty())
	{
		CRect rcText2(0, 0, nW, 32000);
		rsz2.cx = nW;
		return rsz2.cy = pDC->DrawText(str2, &rcText2, uFlags | DT_CENTER);
	}

	if (str1trim.IsEmpty() && str2trim.IsEmpty() && !str3trim.IsEmpty())
	{
		CRect rcText3(0, 0, nW, 32000);
		rsz3.cx = nW;
		return rsz3.cy = pDC->DrawText(str3, &rcText3, uFlags | DT_RIGHT);
	}

	return -1;
}

int CXTPPrintPageHeaderFooter::Calc3ColSizes(CDC* pDC, int nW,
		const CString& str1, const CString& str2, const CString& str3,
		CSize& rsz1, CSize& rsz2, CSize& rsz3)
{
	ASSERT(pDC);
	if (!pDC)
		return 0;

	int nIfSingleCol = _Calc3ColSizesIfSingleCol(pDC, nW, str1, str2, str3, rsz1, rsz2, rsz3);
	if (nIfSingleCol >= 0)
		return nIfSingleCol;

	int nW1 = pDC->GetTextExtent(str1).cx;
	int nW2 = pDC->GetTextExtent(str2).cx;
	int nW3 = pDC->GetTextExtent(str3).cx;

	if (!nW || !nW1 && !nW2 && !nW3)
		return 0;

	nW1 = max(nW1, 1);
	nW2 = max(nW2, 1);
	nW3 = max(nW3, 1);

	int nYmin = INT_MAX;
	int nYminMetric = INT_MAX;
	int nXforYmin[4] = {0, 1, 1, 1};

	int nX[4] = {0, 0, 0, 0};
	int nX_step = 15; //nW/50;

	int nX_start = nW/9;
	int nX_stop = nW/2 - nW/9;
	for (nX[1] = nX_start; nX[1] <= nX_stop; nX[1] += nX_step)
	{
		int nY[4];
		//************************
		nY[1] = fnYi(nX[1], nW1);

		int nX2_stop = nW - nX[1] - nW/9;
		for (nX[2] = nX_start; nX[2] <= nX2_stop; nX[2] += nX_step)
		{
			nX[3] = nW - nX[1] - nX[2];

			//************************
			nY[2] = fnYi(nX[2], nW2);
			if (nY[2] > nYmin)
				continue;

			nY[3] = fnYi(nX[3], nW3);
			if (nY[3] > nYmin)
				break;

			int nY123 = max( max(nY[1], nY[2]), max(nY[1], nY[3]) );

			int nM = nW / 3;
			int nY123Metric = abs(nM - nX[1]) + abs(nM - nX[2]) + abs(nM - nX[3]);

			if (nY123 < nYmin || nY123 == nYmin && nY123Metric < nYminMetric)
			{
				//TRACE(_T("-- =*=   %d <= %d, metrics(cur: %d, min: %d) \n"), nY123, nYmin, nY123Metric, nYminMetric);
				//TRACE(_T("-- prev (%d, %d, %d) \n"), nXforYmin[1], nXforYmin[2], nXforYmin[3]);
				//TRACE(_T("-- new  (%d, %d, %d) \n"), nX[1], nX[2], nX[3]);

				nYmin = nY123;
				nYminMetric = nY123Metric;

				nXforYmin[1] = nX[1];
				nXforYmin[2] = nX[2];
				nXforYmin[3] = nX[3];
			}
		}
	}

	ASSERT(nXforYmin[1] + nXforYmin[2] + nXforYmin[3] <= nW);

	UINT uFlags = DT_TOP | DT_WORDBREAK | DT_CALCRECT;
	CRect rcText1(0, 0, nXforYmin[1], 32000);

	rsz1.cx = nXforYmin[1];
	rsz1.cy = pDC->DrawText(str1, &rcText1, uFlags | DT_LEFT);

	CRect rcText2(0, 0, nXforYmin[2], 32000);
	rsz2.cx = nXforYmin[2];
	rsz2.cy = pDC->DrawText(str2, &rcText2, uFlags | DT_CENTER);

	CRect rcText3(0, 0, nXforYmin[3], 32000);
	rsz3.cx = nXforYmin[3];
	rsz3.cy = pDC->DrawText(str3, &rcText3, uFlags | DT_RIGHT);

	return max( max(rsz1.cy, rsz2.cy), max(rsz1.cy, rsz3.cy) );
}

void CXTPPrintPageHeaderFooter::Draw(CDC* pDC, CRect& rcRect, BOOL bCalculateOnly)
{
	CFont fntHF;
	VERIFY(fntHF.CreateFontIndirect(&m_lfFont));
	CXTPFontDC autoFont(pDC, &fntHF, m_clrColor);

	int nWidth = rcRect.Width();

	CSize szLeft2, szCenter2, szRight2;

	int nHeight = Calc3ColSizes(pDC, nWidth, m_strLeft, m_strCenter, m_strRight,
								szLeft2, szCenter2, szRight2);

	int nBorderH = nHeight ? 3 : 0;
	if (m_bHeader)
	{
		rcRect.bottom = rcRect.top + nHeight + nBorderH;
	}
	else
	{
		rcRect.top = rcRect.bottom - nHeight - nBorderH;
	}

	if (bCalculateOnly)
	{
		return;
	}

	int nBkModePrev = pDC->SetBkMode(TRANSPARENT);

	UINT uFlags = DT_TOP | DT_WORDBREAK | DT_NOPREFIX;

	CRect rcLeft = rcRect;
	rcLeft.right = rcLeft.left + szLeft2.cx;
	pDC->DrawText(m_strLeft, &rcLeft, uFlags | DT_LEFT);

	CRect rcCenter = rcRect;
	rcCenter.left = rcLeft.right;
	rcCenter.right = rcCenter.left + szCenter2.cx;
	pDC->DrawText(m_strCenter, &rcCenter, uFlags | DT_CENTER);

	CRect rcRight = rcRect;
	rcRight.left = rcRight.right - szRight2.cx;
	pDC->DrawText(m_strRight, &rcRight, uFlags | DT_RIGHT);

	pDC->SetBkMode(nBkModePrev);
}

//////////////////////////////////////////////////////////////////////////
// CXTPDpi
//////////////////////////////////////////////////////////////////////////

int CXTPDpi::m_iDefaultDpi = 96; // static

CXTPDpi* XTPDpiHelper()
{
	static CXTPDpi s_instance; // singleton
	return &s_instance;
}

//////////////////////////////////////////////////////////////////////////

//*************************************************************
IMPLEMENT_DYNAMIC(CXTPPrintOptions, CCmdTarget)

LCID CXTPPrintOptions::GetActiveLCID()
{
	return LOCALE_USER_DEFAULT;
}

BOOL CXTPPrintOptions::IsMarginsMeasureInches()
{
	TCHAR szResult[5];

	int nResult = ::GetLocaleInfo(GetActiveLCID(), LOCALE_IMEASURE, szResult, 4);
	ASSERT(nResult == 2);
	long nIsInches = _ttoi(szResult);

	return nIsInches != 0;
}

CXTPPrintOptions::CXTPPrintOptions()
{
	BOOL bIsInches = IsMarginsMeasureInches();
	int nMargin = bIsInches ? (1000 * 1/2  ) : (10 * 100);
	m_rcMargins.SetRect(nMargin, nMargin, nMargin, nMargin);

	m_pPageHeader = new CXTPPrintPageHeaderFooter(this, TRUE);
	m_pPageFooter = new CXTPPrintPageHeaderFooter(this, FALSE);

	m_bBlackWhitePrinting = FALSE;
	m_nBlackWhiteContrast = 0;

//#ifdef _XTP_ACTIVEX
//  EnableAutomation();
//  EnableTypeLib();
//#endif
}

CXTPPrintOptions::~CXTPPrintOptions()
{
	CMDTARGET_RELEASE(m_pPageHeader);
	CMDTARGET_RELEASE(m_pPageFooter);
}

void CXTPPrintOptions::Set(const CXTPPrintOptions* pSrc)
{
	ASSERT(pSrc);
	if (!pSrc)
		return;

	m_rcMargins = pSrc->m_rcMargins;

	if (m_pPageHeader)
		m_pPageHeader->Set(pSrc->m_pPageHeader);

	if (m_pPageFooter)
		m_pPageFooter->Set(pSrc->m_pPageFooter);

	m_bBlackWhitePrinting = pSrc->m_bBlackWhitePrinting;
	m_nBlackWhiteContrast = pSrc->m_nBlackWhiteContrast;
}

CRect CXTPPrintOptions::GetMarginsHimetric()
{
	BOOL bIsInches = IsMarginsMeasureInches();

	if (!bIsInches)
	{
		return m_rcMargins;
	}

	int nMul_i2m = 254;
	int nDiv_i2m = 100;

	CRect rcMarginsHimetric;
	rcMarginsHimetric.left = m_rcMargins.left * nMul_i2m / nDiv_i2m;
	rcMarginsHimetric.top = m_rcMargins.top * nMul_i2m / nDiv_i2m;
	rcMarginsHimetric.right = m_rcMargins.right * nMul_i2m / nDiv_i2m;
	rcMarginsHimetric.bottom = m_rcMargins.bottom * nMul_i2m / nDiv_i2m;

	return rcMarginsHimetric;
}

CRect CXTPPrintOptions::GetMarginsLP(CDC* pDC)
{
	ASSERT(pDC);
	if (!pDC)
		return CRect(0, 0, 0, 0);

	CRect rcMarginsHI = GetMarginsHimetric();

	CSize szLT(rcMarginsHI.left, rcMarginsHI.top);
	CSize szRB(rcMarginsHI.right, rcMarginsHI.bottom);

	pDC->HIMETRICtoLP(&szLT);
	pDC->HIMETRICtoLP(&szRB);

	CRect rcLP(szLT.cx, szLT.cy, szRB.cx, szRB.cy);

	return rcLP;
}

CXTPPrintPageHeaderFooter* CXTPPrintOptions::GetPageHeader()
{
	return m_pPageHeader;
}

CXTPPrintPageHeaderFooter* CXTPPrintOptions::GetPageFooter()
{
	return m_pPageFooter;
}


/////////////////////////////////////////////////////////////////////////////
// Printing Dialog


class XTP_PRINT_STATE : public CNoTrackObject
{
public:
	// printing abort
	BOOL m_bUserAbort;
};

PROCESS_LOCAL(XTP_PRINT_STATE, g_xtpPrintState)

BOOL CALLBACK _XTPAbortProc(HDC, int)
{
	XTP_PRINT_STATE* pState = g_xtpPrintState;
	MSG msg;
	while (!pState->m_bUserAbort &&
		::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE))
	{
		if (!XTPGetThread()->PumpMessage())
			return FALSE;   // terminate if WM_QUIT received
	}
	return !pState->m_bUserAbort;
}

CXTPPrintingDialog::CXTPPrintingDialog(CWnd* pParent)
{
	Create(CXTPPrintingDialog::IDD, pParent);      // modeless !
	g_xtpPrintState->m_bUserAbort = FALSE;
}

BOOL CXTPPrintingDialog::OnInitDialog()
{
	//SetWindowText(AfxGetAppName());
	CenterWindow();
	return CDialog::OnInitDialog();
}

void CXTPPrintingDialog::OnCancel()
{
	g_xtpPrintState->m_bUserAbort = TRUE;  // flag that user aborted print
	CDialog::OnCancel();
}

void CXTPPrintPageHeaderFooter::DoInsertHFFormatSpecifierViaMenu(
								CWnd* pParent, CEdit* pEdit, CButton* pButton)
{
	CMap<UINT, UINT, CString, CString&> menuData;
	menuData[XTP_ID_HF_FORMAT_D1]   = _T("&d");
	menuData[XTP_ID_HF_FORMAT_D2]   = _T("&D");
	menuData[XTP_ID_HF_FORMAT_T1]   = _T("&t");
	menuData[XTP_ID_HF_FORMAT_T2]   = _T("&T");
	menuData[XTP_ID_HF_FORMAT_P1]   = _T("&p");
	menuData[XTP_ID_HF_FORMAT_P2]   = _T("&P");
	menuData[XTP_ID_HF_FORMAT_B]    = _T("&b");
	menuData[XTP_ID_HF_FORMAT_W]    = _T("&w");
	menuData[XTP_ID_HF_FORMAT_UMP]  = _T("&&");
	menuData[XTP_ID_HF_FORMAT_N]    = _T("\\n");

	if (!pParent || !pEdit || !pButton)
	{
		ASSERT(FALSE);
		return;
	}
	CRect rcButton;
	pButton->GetWindowRect(&rcButton);

	CPoint ptMenu(rcButton.left, rcButton.bottom + 1);

	CMenu menu;
	CXTPResourceManager::AssertValid(XTPResourceManager()->LoadMenu(&menu, XTP_IDR_MENU_HEADERFOOTER_FORMATS));

	// track menu
	int nMenuResult = menu.GetSubMenu(0)->TrackPopupMenu(
			TPM_RETURNCMD | TPM_LEFTALIGN |TPM_RIGHTBUTTON,
			ptMenu.x, ptMenu.y, pParent, NULL);

	pEdit->SetFocus();

	CString strItem = menuData[nMenuResult];
	if (!strItem.IsEmpty())
	{
		pEdit->ReplaceSel(strItem, TRUE);
	}

}

//////////////////////////////////////////////////////////////////////////
#ifdef _XTP_ACTIVEX

// {D0CD337C-ED8F-4ce1-9E36-403876BEE557}
static const GUID IID_IXtremePrinterInfo =
{ 0xd0cd337c, 0xed8f, 0x4ce1, { 0x9e, 0x36, 0x40, 0x38, 0x76, 0xbe, 0xe5, 0x57 } };

BEGIN_INTERFACE_MAP(CXTPPrinterInfo, CCmdTarget)
	INTERFACE_PART(CXTPPrinterInfo, IID_IXtremePrinterInfo, Dispatch)
END_INTERFACE_MAP()

IMPLEMENT_OLETYPELIB_EX(CXTPPrinterInfo, IID_IXtremePrinterInfo)

BEGIN_DISPATCH_MAP(CXTPPrinterInfo, CCmdTarget)
	DISP_FUNCTION_ID(CXTPPrinterInfo, "DeviceName", 1, OleGetDeviceName, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CXTPPrinterInfo, "DriverName", 2, OleGetDriverName, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CXTPPrinterInfo, "PortName",   3, OleGetPortName,   VT_BSTR, VTS_NONE)
END_DISPATCH_MAP()

BSTR CXTPPrinterInfo::OleGetDeviceName()
{
	CString strName = GetName(xtpDevName_Device);
	return strName.AllocSysString();
}

BSTR CXTPPrinterInfo::OleGetDriverName()
{
	CString strName = GetName(xtpDevName_Driver);
	return strName.AllocSysString();
}

BSTR CXTPPrinterInfo::OleGetPortName()
{
	CString strName = GetName(xtpDevName_Port);
	return strName.AllocSysString();
}

//===========================================================================
// {8CABF54D-E75C-4a50-A908-F36D35F098D2}
static const GUID IID_IPrintPageHeaderFooter =
{ 0x8cabf54d, 0xe75c, 0x4a50, { 0xa9, 0x8, 0xf3, 0x6d, 0x35, 0xf0, 0x98, 0xd2 } };

BEGIN_INTERFACE_MAP(CXTPPrintPageHeaderFooter, CCmdTarget)
	INTERFACE_PART(CXTPPrintPageHeaderFooter, IID_IPrintPageHeaderFooter, Dispatch)
END_INTERFACE_MAP()

IMPLEMENT_OLETYPELIB_EX(CXTPPrintPageHeaderFooter, IID_IPrintPageHeaderFooter)

BEGIN_DISPATCH_MAP(CXTPPrintPageHeaderFooter, CCmdTarget)

	DISP_PROPERTY_EX_ID(CXTPPrintPageHeaderFooter, "Font", DISPID_FONT, OleGetFont, OleSetFont, VT_FONT)

	DISP_PROPERTY_ID(CXTPPrintPageHeaderFooter, "FormatString",   1, m_strFormatString,   VT_BSTR)

	DISP_PROPERTY_ID(CXTPPrintPageHeaderFooter, "TextLeft",   2, m_strLeft,   VT_BSTR)
	DISP_PROPERTY_ID(CXTPPrintPageHeaderFooter, "TextCenter", 3, m_strCenter, VT_BSTR)
	DISP_PROPERTY_ID(CXTPPrintPageHeaderFooter, "TextRight",  4, m_strRight,  VT_BSTR)

	DISP_FUNCTION_ID(CXTPPrintPageHeaderFooter, "Clear", 50, OleClear, VT_EMPTY, VTS_NONE)

END_DISPATCH_MAP()

LPFONTDISP CXTPPrintPageHeaderFooter::OleGetFont()
{
	return AxCreateOleFont(m_lfFont, this, (LPFNFONTCHANGED)&CXTPPrintPageHeaderFooter::OleSetFont);
}

void CXTPPrintPageHeaderFooter::OleSetFont(LPFONTDISP pFontDisp)
{
	VERIFY(AxGetLogFont(m_lfFont, pFontDisp));
}

void CXTPPrintPageHeaderFooter::OleClear()
{
	Clear();
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_DISPATCH_MAP(CXTPPrintOptions, CCmdTarget)

	DISP_PROPERTY_ID(CXTPPrintOptions, "MarginLeft",   201, m_rcMargins.left, VT_I4)
	DISP_PROPERTY_ID(CXTPPrintOptions, "MarginRight",  202, m_rcMargins.right, VT_I4)
	DISP_PROPERTY_ID(CXTPPrintOptions, "MarginTop",    203, m_rcMargins.top, VT_I4)
	DISP_PROPERTY_ID(CXTPPrintOptions, "MarginBottom", 204, m_rcMargins.bottom, VT_I4)

	DISP_PROPERTY_ID(CXTPPrintOptions, "BlackWhitePrinting", 205, m_bBlackWhitePrinting, VT_BOOL)
	DISP_PROPERTY_ID(CXTPPrintOptions, "BlackWhiteContrast", 206, m_nBlackWhiteContrast, VT_I4)

	DISP_PROPERTY_EX_ID(CXTPPrintOptions, "Landscape", 207, OleGetLandscape, OleSetLandscape, VT_BOOL)

	DISP_FUNCTION_ID(CXTPPrintOptions, "MarginsMeasureMetrics", 250, OleGetMarginsMeasureMetrics, VT_BOOL, VTS_NONE)

	DISP_FUNCTION_ID(CXTPPrintOptions, "Header", 251, OleGetHeader, VT_DISPATCH, VTS_NONE)
	DISP_FUNCTION_ID(CXTPPrintOptions, "Footer", 252, OleGetFooter, VT_DISPATCH, VTS_NONE)

	DISP_FUNCTION_ID(CXTPPrintOptions, "PrinterInfo", 253, OleGetPrinterInfo, VT_DISPATCH, VTS_NONE)
END_DISPATCH_MAP()

LPDISPATCH CXTPPrintOptions::OleGetHeader()
{
	return m_pPageHeader ? m_pPageHeader->GetIDispatch(TRUE) : NULL;
}

LPDISPATCH CXTPPrintOptions::OleGetFooter()
{
	return m_pPageFooter ? m_pPageFooter->GetIDispatch(TRUE) : NULL;
}

LPDISPATCH CXTPPrintOptions::OleGetPrinterInfo()
{
	CXTPPrinterInfo* pPrnIf = new CXTPPrinterInfo();
	if (!pPrnIf)
	{
		AfxThrowOleException(E_OUTOFMEMORY);
	}
	return pPrnIf->GetIDispatch(FALSE);
}

BOOL CXTPPrintOptions::OleGetMarginsMeasureMetrics()
{
	return !IsMarginsMeasureInches();
}

BOOL CXTPPrintOptions::OleGetLandscape()
{
	HGLOBAL hDevMode = NULL, hDevNames = NULL;

	if (!XTPGetPrinterDeviceDefaults(hDevMode, hDevNames) || !hDevMode)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	LPDEVMODE pDevMode = (LPDEVMODE )::GlobalLock(hDevMode);
	ASSERT(pDevMode);
	if (!pDevMode)
		return FALSE;

	int nOrientation = pDevMode->dmOrientation;

	::GlobalUnlock(hDevMode);
	pDevMode = NULL;

	ASSERT(nOrientation == DMORIENT_LANDSCAPE || nOrientation == DMORIENT_PORTRAIT);

	return (nOrientation == DMORIENT_LANDSCAPE);
}

void CXTPPrintOptions::OleSetLandscape(BOOL bLandscape)
{
	HGLOBAL hDevMode = NULL, hDevNames = NULL;

	if (!XTPGetPrinterDeviceDefaults(hDevMode, hDevNames) || !hDevMode)
	{
		ASSERT(FALSE);
		return;
	}

	LPDEVMODE pDevMode = (LPDEVMODE )::GlobalLock(hDevMode);
	ASSERT(pDevMode);
	if (!pDevMode)
		return;

	ASSERT(pDevMode->dmOrientation == DMORIENT_LANDSCAPE || pDevMode->dmOrientation == DMORIENT_PORTRAIT);

	pDevMode->dmOrientation = bLandscape ? DMORIENT_LANDSCAPE : DMORIENT_PORTRAIT;

	::GlobalUnlock(hDevMode);
	pDevMode = NULL;
}


#endif
