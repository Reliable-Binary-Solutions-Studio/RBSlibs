#include "Network.h"

namespace net = RbsLib::Network;

void RbsLib::Network::init_network()
{
#ifdef WIN32
	if (!net::network_inited)
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			throw net::NetworkException("Can not init winsock");
		else net::network_inited = true;
	}
#endif
}


RbsLib::Network::TCP::TCPServer::TCPServer()
{
	net::init_network();
	this->server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->server_socket == INVALID_SOCKET)
		throw net::NetworkException("Allocate socket failed");
	this->reference_counter = new int;
	*this->reference_counter = 1;
}

RbsLib::Network::TCP::TCPServer::TCPServer(int port, const std::string& address)
{
	net::init_network();
	this->server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->server_socket == INVALID_SOCKET)
		throw net::NetworkException("Allocate socket failed");
	this->Bind(port, address);
	this->reference_counter = new int;
	*this->reference_counter = 1;
}

RbsLib::Network::TCP::TCPServer::TCPServer(const TCPServer& server) noexcept
{
	this->server_socket = server.server_socket;
	this->is_bind = server.is_bind;
	this->is_listen = server.is_listen;
	this->reference_counter = server.reference_counter;
	if (reference_counter) ++*this->reference_counter;
}

RbsLib::Network::TCP::TCPServer::~TCPServer(void) noexcept
{
	this->Close();
}

const RbsLib::Network::TCP::TCPServer& RbsLib::Network::TCP::TCPServer::operator=(const TCPServer& server) noexcept
{
	if (this == &server) return *this;
	this->Close();
	this->server_socket = server.server_socket;
	this->is_bind = server.is_bind;
	this->is_listen = server.is_listen;
	this->reference_counter = server.reference_counter;
	if (reference_counter) ++*this->reference_counter;
	return *this;
}

void RbsLib::Network::TCP::TCPServer::Bind(int port, const std::string& address)
{
	struct sockaddr_in s_sin;
	if (is_bind) throw net::NetworkException("This object is already bind");
	if (port < 0 || port>65535) throw net::NetworkException("Port mast be in range 0-65535");
	s_sin.sin_family = AF_INET;
	s_sin.sin_port = htons(port);
	s_sin.sin_addr.s_addr = htonl(inet_addr(address.c_str()));


	if (bind(this->server_socket, (struct sockaddr*)&s_sin, sizeof(s_sin)) == SOCKET_ERROR)
		throw net::NetworkException("Bind failed");
	this->is_bind = true;
	
}

RbsLib::Network::TCP::TCPConnection RbsLib::Network::TCP::TCPServer::Accept(void)
{
	struct sockaddr_in info;
	SOCKET sock;
	socklen_t info_len = sizeof(info);
	if (!this->is_listen)
	{
		if (listen(this->server_socket, 5) == SOCKET_ERROR)
		{
			throw net::NetworkException("Start listening mode failed");
		}
		else this->is_listen = true;
	}
	if ((sock = accept(this->server_socket, (struct sockaddr*)&info, &info_len)) == INVALID_SOCKET)
	{
		throw net::NetworkException("Accept connection failed");
	}
	return RbsLib::Network::TCP::TCPConnection(sock, info, info_len);
}

void RbsLib::Network::TCP::TCPServer::Close(void) noexcept
{
	if (this->reference_counter)
	{
		--*this->reference_counter;
		if (*this->reference_counter == 0)
		{
#ifdef WIN32
			closesocket(this->server_socket);
#endif //Windows
#ifdef linux
			close(this->server_socket);
#endif // linux
			delete this->reference_counter;
			this->reference_counter = nullptr;
		}
	}
}


RbsLib::Network::TCP::TCPConnection::TCPConnection(SOCKET sock,const struct sockaddr_in& connection_info,int info_len) noexcept
{
	this->sock = sock;
	this->connection_info = connection_info;
	this->info_len = info_len;
	this->reference_counter = new int;
	*this->reference_counter = 1;
}

RbsLib::Network::TCP::TCPConnection::TCPConnection(const TCPConnection& connection) noexcept
{
	this->sock = connection.sock;
	this->reference_counter = connection.reference_counter;
	if (this->reference_counter) ++*this->reference_counter;
}

RbsLib::Network::TCP::TCPConnection::TCPConnection(TCPConnection&& connection) noexcept
{
	this->sock = connection.sock;
	this->reference_counter = connection.reference_counter;
	connection.reference_counter = nullptr;
}

RbsLib::Network::TCP::TCPConnection::~TCPConnection(void) noexcept
{
	this->Close();
}

const RbsLib::Network::TCP::TCPConnection& RbsLib::Network::TCP::TCPConnection::operator=(const TCPConnection& connection) noexcept
{
	if (this == &connection) return *this;
	this->Close();
	this->sock = connection.sock;
	this->reference_counter = connection.reference_counter;
	*this->reference_counter += 1;
	return *this;
}

const RbsLib::Network::TCP::TCPConnection& RbsLib::Network::TCP::TCPConnection::operator=(TCPConnection&& connection) noexcept
{
	if (this == &connection) return *this;
	this->Close();
	this->sock = connection.sock;
	this->reference_counter = connection.reference_counter;
	connection.reference_counter = nullptr;
	return *this;
}

void RbsLib::Network::TCP::TCPConnection::Send(const RbsLib::IBuffer& buffer, int flag) const
{
	if (buffer.GetLength() > 0)
	{
		if (send(this->sock, (const char*)buffer.Data(), buffer.GetLength(), flag) <= 0)
		{
			throw net::NetworkException("Send failed");
		}
	}
}

int RbsLib::Network::TCP::TCPConnection::Send(const void* data, int len, int flag) const noexcept
{
	return send(this->sock, (const char*)data, len, flag);
}

RbsLib::Buffer RbsLib::Network::TCP::TCPConnection::Recv(int len, int flag) const
{
	char* data = new char[len];
	int s;

	if ((s = recv(this->sock, data, len, flag)) <= 0)
	{
		delete[]data;
		throw net::NetworkException("Recv failed");
	}
	RbsLib::Buffer buffer(data, s);
	delete data;
	return buffer;
}

int RbsLib::Network::TCP::TCPConnection::Recv(RbsLib::Buffer& buffer, int flag) const
{
	char* data = new char[buffer.GetSize()];
	int s;
	if ((s = recv(this->sock, data, buffer.GetSize(), flag)) <= 0)
	{
		delete[]data;
		throw net::NetworkException("Recv failed");
	}
	buffer.SetData(data, s);
	delete[]data;
	return s;
}

int RbsLib::Network::TCP::TCPConnection::Recv(void* data, int len, int flag) const noexcept
{
	return recv(this->sock, (char*)data, len, flag);
}

void RbsLib::Network::TCP::TCPConnection::Close(void)
{
	if (this->reference_counter)
	{
		*this->reference_counter -= 1;
		if (*this->reference_counter <= 0)
		{
#ifdef WIN32
			closesocket(this->sock);
#endif // WIN32
#ifdef linux
			close(this->sock);
#endif // linux
			delete this->reference_counter;
			this->reference_counter = nullptr;
		}
		else
		{
			this->reference_counter = nullptr;
		}
	}
}

RbsLib::Network::TCP::TCPConnection RbsLib::Network::TCP::TCPClient::Connect(std::string ip, int port)
{
	net::init_network();
	SOCKET c_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == c_Socket) throw net::NetworkException("Allocate socket failed");
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	//如果变量ip中是域名则转换成IP地址
	if (isalpha(ip[0]))
	{
		struct hostent* host = gethostbyname(ip.c_str());
		if (host == nullptr) throw net::NetworkException("Get host by name failed");
		memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);
	}
	else server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	
	server_addr.sin_port = htons(port);
	if (SOCKET_ERROR == connect(c_Socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
		throw net::NetworkException("Connect server failed");
	return net::TCP::TCPConnection(c_Socket, server_addr, sizeof(server_addr));
}
