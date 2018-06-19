#include "stdafx.h"
#include "TimerFlag.h"
#include <time.h>

CTimerFlag::CTimerFlag()
{
}


CTimerFlag::~CTimerFlag()
{
}




bool CTimerFlag::Set(bool cur_val)
{
	if (cur_val != m_flag)
	{
		//新的标志
		m_flag = cur_val;
		if (m_flag)
			m_true_start_time = time(NULL);
		else
			m_true_start_time = 0;
		return true;
	}
	else
		return false;
}

bool CTimerFlag::GetTimeTrue(time_t true_time ) const
{
	return m_flag && time(NULL) - m_true_start_time >= true_time;
}
void CTimerFlag::Reset()
{
	bool m_flag = false;
	time_t m_true_start_time = 0;
}
time_t CTimerFlag::GetTrueTime() const
{
	return m_flag ? time(NULL) - m_true_start_time : 0;
}