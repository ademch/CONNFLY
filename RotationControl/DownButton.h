
#ifndef CDOWNBUTTON_H
#define CDOWNBUTTON_H

class CDownButton : public CButton
{
	DECLARE_DYNAMIC(CDownButton)

public:
	CDownButton();
	virtual ~CDownButton();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
};

class CButtonSpecial : public CButton
{
	DECLARE_DYNAMIC(CButtonSpecial)

public:
	CButtonSpecial();
	virtual ~CButtonSpecial();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSetFocus(CWnd* wnd);
};

class CStaticSpecial : public CStatic
{
	DECLARE_DYNAMIC(CStaticSpecial)

public:
	CStaticSpecial();
	virtual ~CStaticSpecial();

	void SetTextColor(COLORREF clr);

	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);

protected:
	DECLARE_MESSAGE_MAP()

private:
	COLORREF m_clrForeground;
};

#endif