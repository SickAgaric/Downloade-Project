#pragma once

#include<thread>
#include"util.hpp"
#include"httplib.h"
#include"boost/filesystem.hpp"


#define P2P_PORT 9000
#define IP_BUF 16
#define MAX_RANGE (100 * 1024 * 1024) //100M��Ϊ�ֿ�ı�׼
#define SHARED_PATH "./Shared/"
#define DOWNLOAD_PATH "./Dwonload/"

class Host
{
public:
	uint32_t _ip_addr;//����IP��ַ
	bool _pair_ret;//��Խ��
};

class Client
{
public:
	bool Start()
	{
		//�ͻ��˵ĳ���ѭ������
		//ѭ������ÿ�������ļ��󣬶���ȥ����һ��ƥ��
		while (1)
		{
			GetOnlineHost();
		}
		return true;
	}
	//�����������߳���ں���
	void HostPair(Host *host)
	{
		//1��֯httpЭ���ʽ����������
		//2�һ��tcp�Ŀͻ��ˣ������ݷ���
		//3�ȴ��������Ļظ��������н���
		host->_pair_ret = false;
		char buf[IP_BUF] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, 16);//�������ֽ���ת��Ϊ�ַ������ʮ����
		httplib::Client cli(buf, P2P_PORT);//ʵ�����ͻ��˶���
		auto rsp = cli.Get("/hostpair");//�����˷�����Դλ/hostpair��Get����
		//�����ӽ���ʧ�ܣ�rsp����Ϊ��
		if (rsp && rsp->status == 200)//ƥ��ɹ�
		{
			host->_pair_ret = true;//����������Խ��
		}
		return;
	}
	//��ȡ��������
	bool GetOnlineHost()
	{
		char ch = 'Y';//Ĭ��Ϊ��
		if (!_online_host.empty())
		{
			std::cout << "�Ƿ����²鿴��������(Y/N):";
			fflush(stdout);
			std::cin >> ch;
		}
		if (ch == 'Y')
		{
			//��ȡ������Ϣ�������õ�������IP��ַ�б�
			std::vector<Adapter> list;
			AdpaterUtil::GetAllAdapter(&list);
			std::vector<Host> host_list;
			for (int i = 0; i < list.size(); ++i)//ѭ����������
			{
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;
				//���������
				uint32_t net = (ntohl(ip & mask));
				//�������������
				uint32_t max_host = (~ntohl(mask));

				std::vector<bool> ret_list;
				for (int j = 1; j < (int32_t)max_host; ++j)
				{
					uint32_t host_ip = net + j;//����õ���ip��ַ
					//���̣߳������IP��ַ�б��е����������������
					//tr_list[i] = new std::thread(&Client::HostPair, this, host_ip);
					Host host;
					host._ip_addr = htonl(host_ip);//����������ֽ���ת��Ϊ�����ֽ���
					host._pair_ret = false;
					host_list.push_back(host);
				}
			}

				//�����IP��ַ�б��е����������������
				std::vector<std::thread*> tr_list(host_list.size());//�����߳���
				for (int i = 0; i < host_list.size(); ++i)
				{
					tr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
				}

				std::cout << "����ƥ���У����Ժ�\n ";
				//��_pair_ret���Ϊ���Host�࣬�������������б���
				for (int i = 0; i < host_list.size(); ++i)
				{
					//3����Գɹ�������ӦIP������ӵ�_online_host�б���
					tr_list[i]->join();
					if (host_list[i]._pair_ret == true)
					{
						_online_host.push_back(host_list[i]);
					}
					delete tr_list[i];
				}
			
		}
		//��ӡ���������б�ʹ�û�ѡ��

		for (int i = 0; i < _online_host.size(); ++i)
		{
			char buf[IP_BUF] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, IP_BUF);
			std::cout << "\t" << buf << std::endl;
		}
		std::cout << "��ѡ���������";
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		GetShareList(select_ip);//�û����������ļ��б�ӿ� 
		return true;
	}
	//��ȡ�ļ��б�
	bool GetShareList(const std::string &host_ip)
	{
		httplib::Client cli(host_ip.c_str(), P2P_PORT);//ʵ�����ͻ��˶���
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "��ȡ�ļ��б����\n";
			return false;
		}

		std::cout << rsp->body << std::endl;
		std::cout << "ѡ�������ļ�";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		RangeDownload(host_ip, filename);
		return true;
	}
	//�����ļ�
	bool DownloadFile(const std::string &host_ip, const std::string &filename)
	{
		//�����˷����ļ���������
		//
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "�ļ�����ʧ��\n";
			return false;
		}

		if (!boost::filesystem::exists(DOWNLOAD_PATH))//�ж��ļ��Ƿ����
		{
			boost::filesystem::create_directory(DOWNLOAD_PATH);//����Ŀ¼
		}
		std::string real_path = DOWNLOAD_PATH + filename;

		if (File::Write(real_path, rsp->body) == false)
		{
			std::cerr << "�ļ�����ʧ��\n";
			return false;
		}
		std::cout << "�ļ����سɹ�\n";
		return true;
	}
	bool rangedown(const std::string &host_ip, const std::string &name, int64_t s, int64_t e){ 
    std::string req_path = "/download/" + name;
    std::string realpath = DOWNLOAD_PATH + name;
    //httplib::Headers header;
    //header = httplib::make_range_header({{s, e}});
    //header.insert(httplib::make_range_header({ {s, e} }));//����һ��range����
    httplib::Client cli(host_ip.c_str(), P2P_PORT);
    auto rsp = cli.Get(req_path.c_str(), {httplib::make_range_header(s, e)});
    if (rsp == NULL || rsp->status != 206) {
        if (rsp == NULL) { std::cout << "client rsp status NULL\n"; }
        else { std::cout << "��Ӧ״̬��" << rsp->status << "\n"; }
        std::cout << "range download failed\n";
        return false;
    }   
    std::cout << "client range write [" << rsp->body << "]\n";
    FileUtil::Write(realpath, rsp->body, s); 
    std::cout << "client range write success\n";
    return true;
}
int64_t getfilesize(const std::string &host_ip, const std::string &req_path) {
    //1. ����HEAD����ͨ����Ӧ�е�Content-Length��ȡ�ļ���С
    httplib::Client cli(host_ip.c_str(), P2P_PORT);
    auto rsp = cli.Head(req_path.c_str());
    if (rsp == NULL || rsp->status != 200) {
        std::cout << "��ȡ�ļ���С��Ϣʧ��\n";
        return false;
    }
    std::string clen = rsp->get_header_value("Content-Length");
    int64_t filesize = StringUtil::Str2Dig(clen);
    return filesize;
}
bool RangeDownload(const std::string &host_ip, const std::string &name) {
    std::string req_path = "/download/" + name;
    int64_t filesize = getfilesize(host_ip, req_path);
    //2. �����ļ���С���зֿ�
    //int range_count = filesize / MAX_RANGE;
    //a. ���ļ���СС�ڿ��С����ֱ�������ļ�
    if (filesize < MAX_RANGE) {
        std::cout << "�ļ���С,ֱ�������ļ�\n";
        return DonwloadFile(host_ip, name);
    }
    //����ֿ����
    //b. ���ļ���С�����������С����ֿ����λ�ļ���С���Էֿ��СȻ��+1
    //c. ���ļ���С�պ��������С����ֿ���������ļ���С���Էֿ��С
    std::cout << "too max, range download\n";
    int range_count = 0;
    if ((filesize % MAX_RANGE) == 0) {
        range_count = filesize / MAX_RANGE;
    }else {
        range_count = (filesize / MAX_RANGE) + 1;
    }
    // 136   100    0~99  100~135
    int64_t range_start = 0, range_end = 0;
    for (int i = 0; i < range_count; i++) {
        range_start = i * MAX_RANGE;
        if (i == (range_count - 1)) {
            range_end = filesize - 1;
        }else {
            range_end = ((i + 1) * MAX_RANGE) - 1;
        }
        std::cout << "client range req:" << range_start << "-" << range_end << std::endl;
        //3. ��һ����ֿ��������ݣ��õ���Ӧ֮��д���ļ���ָ��λ��
        rangedown(host_ip, name, range_start, range_end);
    }
    std::cout << "Download Success\n";
    return true;
}
    
	bool RangeDownload(const std::string &host_ip, const std::string &name)
	{
		std::string req_path = "/download/" + name;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "��ȡ�ļ���Сʧ��\n";
		}
		//����head���󣬵õ���С
		std::string len = rsp->get_header_value("Content-Length");
		int64_t filesize = StringUtil::StrNum(len);
		
		//�����ļ���С���зֿ�
		if (filesize < MAX_RANGE)//�����ļ�С�ڷֿ��С
		{
			std::cout << "�ļ�С������ֿ鴫��\n";
			return DownloadFile(host_ip, name);
		}

		
		int range_count = 0;
		if ((filesize % MAX_RANGE) == 0)
			range_count = filesize / MAX_RANGE;
		else
			range_count = (filesize / MAX_RANGE) + 1;

		int64_t start = 0, end = 0;
		for (int i = 0; i < range_count; ++i)
		{
			start = i * MAX_RANGE;
			if (i == (range_count - 1))
				end = filesize - 1;
			else
				end = ((i + 1) * MAX_RANGE) - 1;
			
			std::cout << "�ļ����󣬽��зֿ鴫��:" << start << "-" << end << std::endl;
			httplib::Headers header;
			//����һ��range����
			header.insert(httplib::make_range_header({ { start,end } }));
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
			auto rsp = cli.Get(req_path.c_str(), header);

			if (rsp == NULL || rsp->status != 206)
			{
				std::cout << "���������ļ�ʧ��\n";
				return false;
			}
			File::Write(name, rsp->body, start);
		}

		std::cout << "�ļ����سɹ�\n";
		return true;
	}
private:
	std::vector<Host> _online_host;
};


class Server
{
public:
	bool Start()
	{
		_srv.Get("/hostpair",HostPair);
		_srv.Get("/list", ShareList);
		//������ʽ,�������ַ���ָ���ĸ�ʽ������ʾĳ�ֹؼ�����������
		//.ƥ�������ַ���*��ʶƥ���ַ������
		_srv.Get("/download.*", Download);
		_srv.listen("0.0.0.0", P2P_PORT);//������һ��������������һ��������tcp������
		return true;
	}
private:
	static void HostPair(const httplib::Request &req, httplib::Response &rsp)
	{
		rsp.status = 200;
		return;
	}
	//��ȡ�����ļ��б�
	static void ShareList(const httplib::Request &req, httplib::Response &rsp)
	{
		if (!boost::filesystem::exists(SHARED_PATH))//�ж��ļ��Ƿ����
		{
			boost::filesystem::create_directory(SHARED_PATH);//����Ŀ¼
		}
		boost::filesystem::directory_iterator begin(SHARED_PATH);//ʵ����Ŀ¼������
		boost::filesystem::directory_iterator end;
		//��ʼ����Ŀ¼
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{
				//����һ��Ŀ¼
				continue;
			}
			std::string name = begin->path().filename().string();//��ȡ��ǰ�ļ������sting
			rsp.body += name + "\r\n";
		}
		rsp.status = 200;
		return;
	}
	static void Download(const httplib::Request &req, httplib::Response &rsp)
	{
		boost::filesystem::path req_path(req.path);
		std::string name = req_path.filename().string();//ֻ��ȡ�ļ�����
		std::string real_path = SHARED_PATH + name;//ʵ���ļ���·��
		if (!boost::filesystem::exists(real_path) || boost::filesystem::is_directory(real_path))//�ж��ļ��Ƿ����
		{
			rsp.status = 404;
			return;
		}

		//GET����
		if (req.method == "GET")
		{
			//�ж��Ƿ��Ƿֿ鴫������ݣ�������������Ƿ���rangeͷ���ֶ�
			if (req.has_header("Range"))//�ж�����ͷ�����Ƿ���range�ֶ�
			{
				//����һ���ֿ鴫��
				std::string range_str = req.get_header_value("Range");//byte=start-end
				httplib::Ranges ranges;//vector<Range> Range std::pair<start,end>
				httplib::detail::parse_range_header(range_str, ranges);//�����ͻ��˵�range����
				int64_t start = ranges[0].first;
				int64_t end = ranges[0].second;
				int64_t range_len = end - start - 1;//�õ����䳤��
				std::cout << "range:" << start <<  "-" << end << std::endl;
				File::RangeRead(real_path, &rsp.body, range_len, start);
				rsp.status = 206;
				std::cout << "�������Ӧ�����������\n";
			}
			else
			{
				//û��rangeͷ��������һ���������ļ�����
				if (File::Read(real_path, &rsp.body) == false)
				{
					rsp.status = 500;//�ڲ�����
					return;
				}
				rsp.status = 200;
			}
			
		}
		else
		{
			int64_t filesize = File::GetFileSize(real_path);
			rsp.set_header("Content-Length",std::to_string(filesize).c_str());//
			rsp.status = 200;
		}
		return;
	}

private:
	httplib::Server _srv;
};