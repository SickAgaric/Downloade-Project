#include"client.hpp"

void helloworld(const httplib::Request &req,httplib::Response &rsp)
{
	
	rsp.set_content("<html><h1>helloworld<h1></html>", "text/html");
	rsp.status = 200;
}

void Scandir()//浏览目录
{
	//boost::filesystem::exists(real_path)判断文件是否存在
	const char *ptr = "./";
	boost::filesystem::directory_iterator begin(ptr);//目录迭代器对象
	boost::filesystem::directory_iterator end;
	for(; begin != end; ++begin)
	{
		//begin->status获取当前文件的状态信息
		//判断当前文件是否是一个目录
		if (boost::filesystem::is_directory(begin->status()))
		{
			//获取当前迭代器的文件名
			std::cout << begin->path().string() << "它是一个目录\n";
		}
		else
		{
			std::cout << begin->path().string() << "它是一个文件\n";
			//只要文件名不要路径
			std::cout << "文件名" << begin->path().filename().string() << std::endl;
		}
	}

}
void Test()
{
	//Scandir();
	//httplib::Server srv;

	//srv.Get("/", helloworld);//
	//srv.listen("0.0.0.0", 9000);
	/*std::vector<Adapter> list;
	AdpaterUtil::GetAllAdapter(&list);*/
}

void Instantiate()
{
	Client cli;
	cli.Start();
}

int main(int argc, char* argv[])
{
	//创建一个线程运行客户端模块，主线程运行服务端模块
	std::thread tr_client(Instantiate);

	Server srv;
	srv.Start();

	return 0;
}