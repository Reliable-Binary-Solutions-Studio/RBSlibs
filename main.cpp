#include "Network.h"//网络相关
#include "Buffer.h"//缓冲区相关
#include "Storage.h"//存储相关
#include "FileIO.h"//文件IO相关
#include "Streams.h"//流相关
using namespace std;
//样例程序
int main()
{
	//遍历C盘下的Windows目录
	RbsLib::Storage::StorageFile storageFile("C:\\Windows");//创建文件存储对象
	if (storageFile.GetFileType() != RbsLib::Storage::FileType::Dir)//判断是否为目录
	{
		cout << "不是目录" << endl;
		return 1;
	}
	for (auto file : storageFile)
	{
		//遍历目录下的文件
		cout << file.Path() << endl;//输出文件路径
	}

	//读取C盘下的a.txt文件
	RbsLib::Storage::StorageFile file("C:\\a.txt");//创建文件存储对象
	if (file.IsExist() == false)//判断文件是否存在
	{
		cout << "文件不存在" << endl;
		return 1;
	}
	RbsLib::Storage::FileIO::File fp(file.Path(), RbsLib::Storage::FileIO::OpenMode::Read);//创建打开的文件对象
	RbsLib::Buffer buffer = fp.Read(1024);//从文件读取1024字节并存储在缓冲区中
	cout << buffer.Data() << endl;//输出缓冲区中的数据，需要自行确保缓冲区中的字符串以'\0'结尾

	//创建一个TCP客户端并发送缓冲区中的数据
	RbsLib::Network::TCP::TCPClient client;//创建TCP客户端
	auto connection = client.Connect("127.0.0.1", 8080);//连接到指定的服务器
	connection.Send(buffer);//发送缓冲区中的数据

	//所有已打开的对象会在对象析构时自动关闭
	//其他更多功能请查看文档
	return 0;
}