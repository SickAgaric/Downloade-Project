#include"client.hpp"

void helloworld(const httplib::Request &req,httplib::Response &rsp)
{
	
	rsp.set_content("<html><h1>helloworld<h1></html>", "text/html");
	rsp.status = 200;
}

void Scandir()//���Ŀ¼
{
	//boost::filesystem::exists(real_path)�ж��ļ��Ƿ����
	const char *ptr = "./";
	boost::filesystem::directory_iterator begin(ptr);//Ŀ¼����������
	boost::filesystem::directory_iterator end;
	for(; begin != end; ++begin)
	{
		//begin->status��ȡ��ǰ�ļ���״̬��Ϣ
		//�жϵ�ǰ�ļ��Ƿ���һ��Ŀ¼
		if (boost::filesystem::is_directory(begin->status()))
		{
			//��ȡ��ǰ���������ļ���
			std::cout << begin->path().string() << "����һ��Ŀ¼\n";
		}
		else
		{
			std::cout << begin->path().string() << "����һ���ļ�\n";
			//ֻҪ�ļ�����Ҫ·��
			std::cout << "�ļ���" << begin->path().filename().string() << std::endl;
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
	//����һ���߳����пͻ���ģ�飬���߳����з����ģ��
	std::thread tr_client(Instantiate);

	Server srv;
	srv.Start();

	return 0;
}