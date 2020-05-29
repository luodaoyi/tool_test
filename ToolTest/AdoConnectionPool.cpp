#include "AdoConnectionPool.h"
#include "StringTool.h"
int AdoConnectionPool::pool_size = 1;


AdoConnectionPool::AdoConnection::AdoConnection()
{
	ado_connection = NULL;
}

AdoConnectionPool::AdoConnection::~AdoConnection()
{
	ado_connection = NULL;
}
void AdoConnectionPool::AdoConnection::ConnectDb(const std::wstring & conntion_str)
{
	ado_connection = NULL;
	HRESULT hr = ado_connection.CreateInstance(__uuidof(ADODB::Connection));
	if (hr != S_OK)
		throw std::logic_error("CreateInstance Connection Failed" + string_tool::WideToChar(conntion_str) );
	if (ado_connection->Open(conntion_str.c_str(), "", "", ADODB::adModeUnknown) != S_OK)
		throw std::logic_error("Connect Db failed" + string_tool::WideToChar(conntion_str));
}

AdoConnectionPool::AdoConnectionPool(int pool_size )
{
	connection_pool_.resize(pool_size);
	for (decltype(connection_pool_.size()) i = 0; i < connection_pool_.size(); i++) 
		connection_pool_[i] = std::make_unique<AdoConnection>();
}
void AdoConnectionPool::Execute(std::function<void(ADODB::_ConnectionPtr & conntion)> Func)
{
	AdoConnectionPool::AdoConnection & connection = GetConnection();
	std::lock_guard<std::mutex> lk(connection.mutex);
	Func(connection.ado_connection);
}


AdoConnectionPool::AdoConnection & AdoConnectionPool::GetConnection()
{
	std::lock_guard<std::mutex> lk(pool_mutex_);
	AdoConnectionPool::AdoConnection & conntion = *connection_pool_[index_];
	++index_;
	if (index_ >= connection_pool_.size())
		index_ = 0;
	return conntion;
}

bool AdoConnectionPool::ConnectDataBase(const std::wstring & ip, const std::wstring & user, const std::wstring & password,const std::wstring & Catalog)
{
	wchar_t szConnectionBuffer[2048] = { 0 };
	swprintf_s(szConnectionBuffer, L"Provider=SQLOLEDB.1;Password=%s;Persist Security Info=True; \
								 User ID=%s;Initial Catalog=%s;Data Source=%s", password.c_str(), user.c_str(), Catalog.c_str(), ip.c_str());
	connection_string_ = szConnectionBuffer;
	for (decltype(connection_pool_.size()) i = 0; i < connection_pool_.size(); i++)
		connection_pool_[i]->ConnectDb(connection_string_.c_str());
	return true;
}

AdoConnectionPool & AdoConnectionPool::GetInstance()
{
	static AdoConnectionPool s(AdoConnectionPool::pool_size);
	return s;
}
void AdoConnectionPool::SetPoolSize(int n)
{
	AdoConnectionPool::pool_size = n;
}