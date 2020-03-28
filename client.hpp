#pragma once

#include<thread>
#include"util.hpp"
#include"httplib.h"
#include"boost/filesystem.hpp"


#define P2P_PORT 9000
#define IP_BUF 16
#define MAX_RANGE (100 * 1024 * 1024) //100M作为分块的标准
#define SHARED_PATH "./Shared/"
#define DOWNLOAD_PATH "./Dwonload/"

class Host
{
public:
	uint32_t _ip_addr;//主机IP地址
	bool _pair_ret;//配对结果
};

class Client
{
public:
	bool Start()
	{
		//客户端的程序循环运行
		//循环运行每次下载文件后，都会去进行一次匹配
		while (1)
		{
			GetOnlineHost();
		}
		return true;
	}
	//主机配对你的线程入口函数
	void HostPair(Host *host)
	{
		//1组织http协议格式的请求数据
		//2搭建一个tcp的客户端，将数据发送
		//3等待服务器的回复，并进行解析
		host->_pair_ret = false;
		char buf[IP_BUF] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, 16);//将网络字节序转换为字符串点分十进制
		httplib::Client cli(buf, P2P_PORT);//实例化客户端对象
		auto rsp = cli.Get("/hostpair");//想服务端发送资源位/hostpair的Get请求
		//若连接建立失败，rsp返回为空
		if (rsp && rsp->status == 200)//匹配成功
		{
			host->_pair_ret = true;//重置主机配对结果
		}
		return;
	}
	//获取在线主机
	bool GetOnlineHost()
	{
		char ch = 'Y';//默认为空
		if (!_online_host.empty())
		{
			std::cout << "是否重新查看在线主机(Y/N):";
			fflush(stdout);
			std::cin >> ch;
		}
		if (ch == 'Y')
		{
			//获取网卡信息，进而得到局域网IP地址列表
			std::vector<Adapter> list;
			AdpaterUtil::GetAllAdapter(&list);
			std::vector<Host> host_list;
			for (int i = 0; i < list.size(); ++i)//循环所有网卡
			{
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;
				//计算网络号
				uint32_t net = (ntohl(ip & mask));
				//计算最大主机号
				uint32_t max_host = (~ntohl(mask));

				std::vector<bool> ret_list;
				for (int j = 1; j < (int32_t)max_host; ++j)
				{
					uint32_t host_ip = net + j;//计算得到的ip地址
					//用线程，逐个对IP地址列表中的主机发送配对请求
					//tr_list[i] = new std::thread(&Client::HostPair, this, host_ip);
					Host host;
					host._ip_addr = htonl(host_ip);//将这个主机字节序，转换为网络字节序
					host._pair_ret = false;
					host_list.push_back(host);
				}
			}

				//逐个对IP地址列表中的主机发送配对请求
				std::vector<std::thread*> tr_list(host_list.size());//创建线程组
				for (int i = 0; i < host_list.size(); ++i)
				{
					tr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
				}

				std::cout << "正在匹配中，请稍后\n ";
				//将_pair_ret标记为真的Host类，加入在线主机列表中
				for (int i = 0; i < host_list.size(); ++i)
				{
					//3若配对成功，将对应IP地主添加到_online_host列表中
					tr_list[i]->join();
					if (host_list[i]._pair_ret == true)
					{
						_online_host.push_back(host_list[i]);
					}
					delete tr_list[i];
				}
			
		}
		//打印在线主机列表，使用户选择

		for (int i = 0; i < _online_host.size(); ++i)
		{
			char buf[IP_BUF] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, IP_BUF);
			std::cout << "\t" << buf << std::endl;
		}
		std::cout << "请选择配对主机";
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		GetShareList(select_ip);//用户调用主机文件列表接口 
		return true;
	}
	//获取文件列表
	bool GetShareList(const std::string &host_ip)
	{
		httplib::Client cli(host_ip.c_str(), P2P_PORT);//实例化客户端对象
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "获取文件列表错误\n";
			return false;
		}

		std::cout << rsp->body << std::endl;
		std::cout << "选择下载文件";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		RangeDownload(host_ip, filename);
		return true;
	}
	//下载文件
	bool DownloadFile(const std::string &host_ip, const std::string &filename)
	{
		//向服务端发送文件下载请求
		//
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "文件下载失败\n";
			return false;
		}

		if (!boost::filesystem::exists(DOWNLOAD_PATH))//判断文件是否存在
		{
			boost::filesystem::create_directory(DOWNLOAD_PATH);//创建目录
		}
		std::string real_path = DOWNLOAD_PATH + filename;

		if (File::Write(real_path, rsp->body) == false)
		{
			std::cerr << "文件下载失败\n";
			return false;
		}
		std::cout << "文件下载成功\n";
		return true;
	}
	bool rangedown(const std::string &host_ip, const std::string &name, int64_t s, int64_t e){ 
    std::string req_path = "/download/" + name;
    std::string realpath = DOWNLOAD_PATH + name;
    //httplib::Headers header;
    //header = httplib::make_range_header({{s, e}});
    //header.insert(httplib::make_range_header({ {s, e} }));//设置一个range区间
    httplib::Client cli(host_ip.c_str(), P2P_PORT);
    auto rsp = cli.Get(req_path.c_str(), {httplib::make_range_header(s, e)});
    if (rsp == NULL || rsp->status != 206) {
        if (rsp == NULL) { std::cout << "client rsp status NULL\n"; }
        else { std::cout << "响应状态码" << rsp->status << "\n"; }
        std::cout << "range download failed\n";
        return false;
    }   
    std::cout << "client range write [" << rsp->body << "]\n";
    FileUtil::Write(realpath, rsp->body, s); 
    std::cout << "client range write success\n";
    return true;
}
int64_t getfilesize(const std::string &host_ip, const std::string &req_path) {
    //1. 发送HEAD请求，通过响应中的Content-Length获取文件大小
    httplib::Client cli(host_ip.c_str(), P2P_PORT);
    auto rsp = cli.Head(req_path.c_str());
    if (rsp == NULL || rsp->status != 200) {
        std::cout << "获取文件大小信息失败\n";
        return false;
    }
    std::string clen = rsp->get_header_value("Content-Length");
    int64_t filesize = StringUtil::Str2Dig(clen);
    return filesize;
}
bool RangeDownload(const std::string &host_ip, const std::string &name) {
    std::string req_path = "/download/" + name;
    int64_t filesize = getfilesize(host_ip, req_path);
    //2. 根据文件大小进行分块
    //int range_count = filesize / MAX_RANGE;
    //a. 若文件大小小于块大小，则直接下载文件
    if (filesize < MAX_RANGE) {
        std::cout << "文件较小,直接下载文件\n";
        return DonwloadFile(host_ip, name);
    }
    //计算分块个数
    //b. 若文件大小不能整除块大小，则分块个数位文件大小除以分块大小然后+1
    //c. 若文件大小刚好整除块大小，则分块个数就是文件大小除以分块大小
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
        //3. 逐一请求分块区间数据，得到响应之后写入文件的指定位置
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
			std::cerr << "获取文件大小失败\n";
		}
		//发送head请求，得到大小
		std::string len = rsp->get_header_value("Content-Length");
		int64_t filesize = StringUtil::StrNum(len);
		
		//根据文件大小进行分块
		if (filesize < MAX_RANGE)//传输文件小于分块大小
		{
			std::cout << "文件小，不许分块传输\n";
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
			
			std::cout << "文件过大，进行分块传输:" << start << "-" << end << std::endl;
			httplib::Headers header;
			//设置一个range区间
			header.insert(httplib::make_range_header({ { start,end } }));
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
			auto rsp = cli.Get(req_path.c_str(), header);

			if (rsp == NULL || rsp->status != 206)
			{
				std::cout << "区间下载文件失败\n";
				return false;
			}
			File::Write(name, rsp->body, start);
		}

		std::cout << "文件下载成功\n";
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
		//正则表达式,将特殊字符以指定的格式，来表示某种关键特征的数据
		//.匹配任意字符，*标识匹配字符任意次
		_srv.Get("/download.*", Download);
		_srv.listen("0.0.0.0", P2P_PORT);//建立了一个监听本机任意一块网卡的tcp服务器
		return true;
	}
private:
	static void HostPair(const httplib::Request &req, httplib::Response &rsp)
	{
		rsp.status = 200;
		return;
	}
	//获取共享文件列表
	static void ShareList(const httplib::Request &req, httplib::Response &rsp)
	{
		if (!boost::filesystem::exists(SHARED_PATH))//判断文件是否存在
		{
			boost::filesystem::create_directory(SHARED_PATH);//创建目录
		}
		boost::filesystem::directory_iterator begin(SHARED_PATH);//实例化目录迭代器
		boost::filesystem::directory_iterator end;
		//开始迭代目录
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{
				//这是一个目录
				continue;
			}
			std::string name = begin->path().filename().string();//获取当前文件对象的sting
			rsp.body += name + "\r\n";
		}
		rsp.status = 200;
		return;
	}
	static void Download(const httplib::Request &req, httplib::Response &rsp)
	{
		boost::filesystem::path req_path(req.path);
		std::string name = req_path.filename().string();//只获取文件名称
		std::string real_path = SHARED_PATH + name;//实际文件的路径
		if (!boost::filesystem::exists(real_path) || boost::filesystem::is_directory(real_path))//判断文件是否存在
		{
			rsp.status = 404;
			return;
		}

		//GET方法
		if (req.method == "GET")
		{
			//判断是否是分块传输的依据，就是这次请求是否有range头部字段
			if (req.has_header("Range"))//判断请求头部中是否有range字段
			{
				//这是一个分块传输
				std::string range_str = req.get_header_value("Range");//byte=start-end
				httplib::Ranges ranges;//vector<Range> Range std::pair<start,end>
				httplib::detail::parse_range_header(range_str, ranges);//解析客户端的range数据
				int64_t start = ranges[0].first;
				int64_t end = ranges[0].second;
				int64_t range_len = end - start - 1;//得到区间长度
				std::cout << "range:" << start <<  "-" << end << std::endl;
				File::RangeRead(real_path, &rsp.body, range_len, start);
				rsp.status = 206;
				std::cout << "服务端响应区间数据完毕\n";
			}
			else
			{
				//没有range头部，则是一个完整的文件下载
				if (File::Read(real_path, &rsp.body) == false)
				{
					rsp.status = 500;//内部错误
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