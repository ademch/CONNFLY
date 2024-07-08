#ifndef CSTATIC_PLOT_H
#define CSTATIC_PLOT_H

// Asynch command state
enum plotState
{	
	plotStateInit,
	plotStateSun,
	plotStateShade
};


class CStaticPlot : public CStatic
{
	DECLARE_DYNAMIC(CStaticPlot)

public:
	CStaticPlot();
	virtual ~CStaticPlot();

	// The current angle in step driver local coordinate system
	float m_AngleStepDriverCurrentDeg;

	// Angle in step driver coordinate systems pointing to Zero deg in global coordinate system
	float m_AngleStepDriverZeroDeg;

	int m_iLeftLimitDeg;
	int m_iRightLimitDeg;

	// Global coordinates are th ecoordinates where 0deg is at the top (like on the plot)
	float AngleGlobalCoordinatesDeg()
	{
		return m_AngleStepDriverCurrentDeg - m_AngleStepDriverZeroDeg;
	}

	plotState plotState;

protected:
	DECLARE_MESSAGE_MAP()
public:

	afx_msg void OnPaint();
};

#endif