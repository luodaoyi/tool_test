#pragma once
class CNonCopyable
{
public:
	CNonCopyable() {}
	~CNonCopyable() {}

	CNonCopyable(const CNonCopyable &) = delete;
	void operator=(const CNonCopyable &) = delete;
};

