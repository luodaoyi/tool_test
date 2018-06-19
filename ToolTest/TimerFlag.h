#pragma once
class CTimerFlag
{
public:
	CTimerFlag();
	~CTimerFlag();
	bool Set(bool b);//返回是否首次置为true
	bool GetTimeTrue(time_t true_time = 0) const;
	bool GetIsTrue() const { return m_flag; }
	void Reset();
	time_t GetTrueTime() const;
private:
	bool m_flag = false;
	time_t m_true_start_time = 0;
};

