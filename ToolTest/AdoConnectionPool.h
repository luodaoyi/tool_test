#pragma once
#include <functional>
#include <mutex>
#include <vector>
#include "NonCopyable.h"
#include <string>
#import "C:\Program Files\Common Files\System\ado\msado15.dll" rename("EOF", "adoEOF") rename("BOF", "adoBOF")


class AdoConnectionPool : public CNonCopyable
{
public:
	static AdoConnectionPool & GetInstance();
	static void SetPoolSize(int n);
public:
	AdoConnectionPool(int pool_size = 1);
	bool ConnectDataBase(const std::wstring & ip, const std::wstring & user, const std::wstring & password, const std::wstring & Catalog);
	void Execute(std::function<void(ADODB::_ConnectionPtr & conntion)> Func);
private:
	struct AdoConnection
	{
		AdoConnection();
		~AdoConnection();
		std::mutex mutex;
		ADODB::_ConnectionPtr ado_connection;
		void ConnectDb(const std::wstring & conntion_str);
	};
	AdoConnection &  GetConnection();
	std::mutex pool_mutex_;
	std::vector<std::unique_ptr< AdoConnection> >::size_type index_ = 0;
	std::vector<std::unique_ptr< AdoConnection> > connection_pool_;
	std::wstring connection_string_;
private:
	static int pool_size;
};

