#include "stdafx.h"
#include "DownButton.h"

IMPLEMENT_DYNAMIC(CDownButton, CButton)

CDownButton::CDownButton()
{
}

CDownButton::~CDownButton()
{
}

BEGIN_MESSAGE_MAP(CDownButton, CButton)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

void CDownButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	GetParent()->SendMessage(WM_COMMAND, GetDlgCtrlID() | 1 << 13, (LPARAM)GetSafeHwnd());
	CButton::OnLButtonDown(nFlags, point);
}

void CDownButton::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	GetParent()->SendMessage(WM_COMMAND, GetDlgCtrlID() | 1 << 13, (LPARAM)GetSafeHwnd());
	CButton::OnLButtonDown(nFlags, point);
}

void CDownButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	GetParent()->SendMessage(WM_COMMAND, GetDlgCtrlID()| 1 << 14, (LPARAM)GetSafeHwnd());
	CButton::OnLButtonUp(nFlags, point);
}

IMPLEMENT_DYNAMIC(CButtonSpecial, CButton)

CButtonSpecial::CButtonSpecial()
{
}

CButtonSpecial::~CButtonSpecial()
{
}

BEGIN_MESSAGE_MAP(CButtonSpecial, CButton)
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

void CButtonSpecial::OnSetFocus(CWnd* wnd)
{
	// Handler preventing default Windows radio button behavior
	// OnWmSetFocus BN_CLICKED is sent, now it is not
	Sleep(20);
}


IMPLEMENT_DYNAMIC(CStaticSpecial, CStatic)

CStaticSpecial::CStaticSpecial()
{
	m_clrForeground = RGB(0,0,0);
}

CStaticSpecial::~CStaticSpecial() {
}

BEGIN_MESSAGE_MAP(CStaticSpecial, CStatic)
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()

HBRUSH CStaticSpecial::CtlColor(CDC* pDC, UINT nCtlColor)
{
	pDC->SetTextColor(m_clrForeground);
	pDC->SetBkColor(GetSysColor(COLOR_BTNFACE));

	return GetSysColorBrush(COLOR_BTNFACE);	// system brush
}

void CStaticSpecial::SetTextColor(COLORREF clr)
{
	m_clrForeground = clr;
	Invalidate();
}
