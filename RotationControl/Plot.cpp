#include "stdafx.h"
#include "Plot.h"
#include <Math.h>

int round(float x)
{
	if (x < 0.0f)
		return (int)(x-0.5f);
	else
		return (int)(x+0.5f);
}

float deg2rad(float fAngle) {
	return fAngle * float(M_PI) / 180.0f;
}

float deg2rad(int iAngle) {
	return float(iAngle) * float(M_PI) / 180.0f;
}



IMPLEMENT_DYNAMIC(CStaticPlot, CStatic)

CStaticPlot::CStaticPlot()
{
	m_AngleStepDriverCurrentDeg = 0;
	m_AngleStepDriverZeroDeg    = 0;

	m_iLeftLimitDeg             = -110;
	m_iRightLimitDeg            =  110;

	plotState = plotStateShade;
}

CStaticPlot::~CStaticPlot() {
}


BEGIN_MESSAGE_MAP(CStaticPlot, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()


void CStaticPlot::OnPaint()
{
	CStatic::OnPaint();	// handles BeginPaint/EndPaint, should be called first

	CClientDC dc(this); 

	CRect rc;
	GetClientRect(&rc);
	

	switch (plotState)
	{
	case plotStateInit:	
		{
			CRgn rgn;
			rgn.CreateEllipticRgn(24,24,rc.right-23,rc.right-23);

			CBrush brushGrey(RGB(224,223,237));
			dc.SelectObject(&brushGrey);
			dc.PaintRgn(&rgn);
		}
		break;
	case plotStateSun:
		{
			CRgn rgn;
			rgn.CreateEllipticRgn(24,24,rc.right-23,rc.right-23);

			CBrush brushGrey(RGB(224,223,227));
			dc.SelectObject(&brushGrey);
			dc.PaintRgn(&rgn);

			CBrush brushYellow(RGB(255,255,64));
			dc.SelectObject(&brushYellow);
			dc.BeginPath();
				dc.MoveTo(rc.right/2, rc.right/2);
				//          centerX     centerY     radius           startAngle                  SweepAngle
				dc.AngleArc(rc.right/2, rc.right/2, (rc.right-48)/2, float(90-m_iRightLimitDeg), float(m_iRightLimitDeg - m_iLeftLimitDeg) );
				dc.LineTo(rc.right/2, rc.right/2);
			dc.EndPath();
			dc.FillPath();
		}
		break;
	case plotStateShade:
		{
			CRgn rgn;
			rgn.CreateEllipticRgn(24,24,rc.right-24,rc.right-24);

			CBrush brushGrey(RGB(224,223,227));
			dc.SelectObject(&brushGrey);
			dc.PaintRgn(&rgn);

			CBrush brushYellow(RGB(200,200,0));
			dc.SelectObject(&brushYellow);
			dc.BeginPath();
				dc.MoveTo(rc.right/2, rc.right/2);
				//          centerX     centerY     radius           startAngle                  SweepAngle
				dc.AngleArc(rc.right/2, rc.right/2, (rc.right-40)/2, float(90-m_iRightLimitDeg), float(m_iRightLimitDeg - m_iLeftLimitDeg) );
				dc.LineTo(rc.right/2, rc.right/2);
			dc.EndPath();
			dc.FillPath();
		}
		break;
	}
	

	int iCircleRadius = (rc.right-40)/2;
	
	CPen bluePen(PS_SOLID, 5, RGB(0,0,255));
	dc.SelectObject(&bluePen);
	dc.SelectStockObject(NULL_BRUSH);
	dc.Ellipse(20,20,rc.right-20,rc.right-20);

	CPen blackPen(PS_SOLID, 1, RGB(0,0,0));
	dc.SelectObject(&blackPen);

	// vertical line
	dc.MoveTo(rc.right/2, 10);
	dc.LineTo(rc.right/2, rc.right-10);
	// horizontal line
	dc.MoveTo(10,          rc.right/2);
	dc.LineTo(rc.right-10, rc.right/2);

	// 10 deg ticks
	for (int i=0; i<360; i+=5)
	{
		dc.MoveTo(rc.right/2 - round(sinf(deg2rad(i))*(iCircleRadius+3)),      
				  rc.right/2 - round(cosf(deg2rad(i))*(iCircleRadius+3)));
		dc.LineTo(rc.right/2 - round(sinf(deg2rad(i))*(iCircleRadius-6)),
				  rc.right/2 - round(cosf(deg2rad(i))*(iCircleRadius-6)));
	}

	// 45 deg ticks
	for (int i=45; i<360; i+=45)
	{
		dc.MoveTo(rc.right/2 - round(sinf(deg2rad(i))*(iCircleRadius+6)),      
				  rc.right/2 - round(cosf(deg2rad(i))*(iCircleRadius+6)));
		dc.LineTo(rc.right/2 - round(sinf(deg2rad(i))*(iCircleRadius-14)),
				  rc.right/2 - round(cosf(deg2rad(i))*(iCircleRadius-14)));
	}

	float aArrow[16] = {// boom
		                0.0f, float(-iCircleRadius +8),
	                    0.0f, float( iCircleRadius -10),
						// left arrow side
					    0.0f, float(-iCircleRadius +8),
					  -10.0f, float(-iCircleRadius +40),
						// right arrow side
					    0.0f, float(-iCircleRadius +8),
				   	   10.0f, float(-iCircleRadius +40),
						// bottom part
					  -10.0f, float( iCircleRadius -10),
					   10.0f, float( iCircleRadius -10)};
	
	CPen blackPenThick(PS_SOLID, 3, RGB(0,0,0));
	dc.SelectObject(&blackPenThick);

	float m_AngleCurrentRad = deg2rad(AngleGlobalCoordinatesDeg());

	float aArrowTStartX, aArrowTStartY;
	float aArrowTEndX,   aArrowTEndY;
	for (int i=0; i<8; i+=2)
	{
		aArrowTStartX = -aArrow[i*2]*cosf(m_AngleCurrentRad) - aArrow[i*2+1]*sinf(m_AngleCurrentRad);
		aArrowTStartY = -aArrow[i*2]*sinf(m_AngleCurrentRad) + aArrow[i*2+1]*cosf(m_AngleCurrentRad);

		aArrowTEndX   = -aArrow[(i+1)*2]*cosf(m_AngleCurrentRad) - aArrow[(i+1)*2+1]*sinf(m_AngleCurrentRad);
		aArrowTEndY   = -aArrow[(i+1)*2]*sinf(m_AngleCurrentRad) + aArrow[(i+1)*2+1]*cosf(m_AngleCurrentRad);

		dc.MoveTo( round(rc.right/2+ aArrowTStartX), round(rc.right/2+ aArrowTStartY) );
		dc.LineTo( round(rc.right/2+ aArrowTEndX),   round(rc.right/2+ aArrowTEndY) );
	}
}
