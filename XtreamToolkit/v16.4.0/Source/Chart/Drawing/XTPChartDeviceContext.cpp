// XTPChartDeviceContext.cpp
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

#include "Common/XTPResourceManager.h"
#include "Common/XTPIntel80Helpers.h"

#include "GraphicLibrary/GdiPlus/GdiPlus.h"
#include "GraphicLibrary/XTGraphicUtils.h"

#include "../Types/XTPChartTypes.h"

#include "XTPChartDeviceContext.h"
#include "XTPChartDeviceCommand.h"

#pragma comment(lib, "GdiPlus.lib")
#pragma comment(lib, "GLU32.lib")


using namespace Gdiplus;
using namespace Gdiplus::DllExports;

#pragma warning(disable:4510)
#pragma warning(disable:4610)



//////////////////////////////////////////////////////////////////////////
// CXTPChartDeviceContext

class CXTPChartDeviceContext::CGdiPlus
{
public:
	CGdiPlus()
	{
		m_hModule = NULL;
		m_nGdiplusToken = NULL;
		m_nCount = 0;
	}

public:
	void Register(BOOL bInit);

public:
	HMODULE m_hModule;
	ULONG_PTR m_nGdiplusToken;
	int m_nCount;
};

CXTPChartDeviceContext::CGdiPlus* CXTPChartDeviceContext::GetGdiPlus()
{
	static CGdiPlus GdiPlus;
	return &GdiPlus;
}

void CXTPChartDeviceContext::CGdiPlus::Register(BOOL bInit)
{
	static GdiplusStartupOutput gdiplusStartupOutput;
	if (bInit)
	{
		m_nCount++;

		if (m_nCount > 1)
			return;

		ASSERT(m_nGdiplusToken == 0 && m_hModule == 0);


		m_hModule = LoadLibrary(_T("GdiPlus.dll"));

		if (m_hModule)
		{
			GdiplusStartupInput gdiplusStartupInput;
			gdiplusStartupInput.SuppressBackgroundThread = TRUE;
			GdiplusStartup(&m_nGdiplusToken, &gdiplusStartupInput, &gdiplusStartupOutput);
			gdiplusStartupOutput.NotificationHook(&m_nGdiplusToken);
		}
	}
	else
	{
		m_nCount--;

		if (m_nCount != 0)
			return;

		if (m_hModule)
		{
			// Termination of background thread, which is causing the shutdown problem.
			gdiplusStartupOutput.NotificationUnhook(m_nGdiplusToken);
			GdiplusShutdown(m_nGdiplusToken);
			FreeLibrary(m_hModule);
		}

		m_hModule = NULL;
		m_nGdiplusToken = 0;
	}
}

void CXTPChartDeviceContext::Register(BOOL bInit)
{
	GetGdiPlus()->Register(bInit);
}


CXTPChartDeviceContext::CXTPChartDeviceContext(CXTPChartContainer* pContainer, HDC hDC)
{
	m_pContainer = pContainer;
	m_hDC = hDC;
	GdipCreateFromHDC(hDC, &m_pGpGraphics);

	// Adjust world scalling to output DPI.
	GdipSetPageUnit(m_pGpGraphics, UnitDocument);
	REAL dpix;
	REAL dpiy;
	GdipGetDpiX(m_pGpGraphics, &dpix);
	GdipGetDpiY(m_pGpGraphics, &dpiy);
	GdipScaleWorldTransform(m_pGpGraphics, 300.f / dpix, 300.f / dpiy, MatrixOrderAppend);

	// Additional device configuration.
	GdipSetSmoothingMode(m_pGpGraphics, SmoothingModeHighSpeed);
}

CXTPChartDeviceContext::CXTPChartDeviceContext(CXTPChartContainer* pContainer, Graphics* pGraphics, HDC hDC)
{
	USES_PROTECTED_ACCESS(CXTPChartDeviceContext, Graphics, GpGraphics, nativeGraphics);

	m_pContainer = pContainer;
	m_hDC = hDC;
	m_pGpGraphics = PROTECTED_ACCESS(Graphics, pGraphics, nativeGraphics);

	GdipSetPageUnit(m_pGpGraphics, UnitPixel);
	GdipSetSmoothingMode(m_pGpGraphics, SmoothingModeHighSpeed);
}


CXTPChartDeviceContext::CXTPChartDeviceContext(CXTPChartContainer* pContainer)
{
	m_pContainer = pContainer;
	m_hDC = NULL;
	m_pGpGraphics = NULL;
}


CXTPChartDeviceContext::~CXTPChartDeviceContext()
{
	if (m_pGpGraphics)
	{
		GdipDeleteGraphics(m_pGpGraphics);
	}
}


void CXTPChartDeviceContext::Execute(CXTPChartDeviceCommand* pCommand)
{
	pCommand->Execute(this);
}


CXTPChartSizeF CXTPChartDeviceContext::MeasureString(const CXTPChartString* pText, CXTPChartFont* pFont)
{
	GpFont* pGpFont = NULL;
	GdipCreateFontFromLogfont(m_hDC, &pFont->m_lf, &pGpFont);

	RectF boundingBox;
	RectF layoutRect(0, 0, 0.0f, 0.0f);

	GdipMeasureString(m_pGpGraphics, XTP_CT2CW(*pText), -1, pGpFont, &layoutRect, 0, &boundingBox, 0, 0);

	CXTPChartSizeF sz;
	boundingBox.GetSize((SizeF*)&sz);

	GdipDeleteFont(pGpFont);

	return sz;
}

void CXTPChartDeviceContext::SaveToFile(HBITMAP hBitmap, LPCTSTR lpszFileName)
{
	CXTPGraphicUtils::SaveBitmapToFile(hBitmap, lpszFileName);
}
