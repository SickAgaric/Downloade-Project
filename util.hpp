#pragma once

#ifdef _WIN32
#include<iostream>
#include<fstream>
#include<vector>
#include<stdint.h>
#include<sstream>
#include<WS2tcpip.h>
#include<Iphlpapi.h>//获取网卡信息接口的头文件
#pragma comment(lib,"Iphlpapi.lib")//获取网卡信息接口的库文件
#pragma comment(lib,"ws2_32.lib")
#include"boost/filesystem.hpp"
#else
	//linux
#endif

class StringUtil
{
public:

	static int64_t StrNum(const std::string &num)
	{
		std::stringstream tmp;
		tmp << num;
		int64_t res;
		tmp >> res;//根据后边数据类型的不同，对输入的数据进行类型转换
		return res;

	}
};

class File
{
public:
	static int64_t GetFileSize(const std::string &name)
	{
		return boost::filesystem::file_size(name);
	}
	static bool Write(const std::string &name, const std::string &body, int64_t offset = 0)
	{
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "wb");//以二进制打开文件

		if (fp == NULL)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}

		fseek(fp, offset, SEEK_SET);//SEEK_SET偏移量
		int ret = fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size())
		{
			std::cerr << "文件写入数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}

	static bool Read(const std::string &name, std::string *body)
	{
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);

		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");

		if (fp == NULL)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}

		int ret = fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize)
		{
			std::cerr << "文件读取失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
	static bool RangeRead(const std::string &name, std::string *body,int64_t len, int64_t offset)
	{
		body->resize(len);

		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");

		if (fp == NULL)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);//跳转到指定位置

		int ret = fread(&(*body)[0], 1, len, fp);//读取指定位置数据到body中
		if (ret != len)
		{
			std::cerr << "文件读取失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
};

class Adapter
{
public:
	uint32_t _ip_addr;  //网卡上的IP地址
	uint32_t _mask_addr;//网卡上的子网掩码
};

class AdpaterUtil
{
#ifdef _WIN32
public:
	static bool GetAllAdapter(std::vector<Adapter>* list)//获取网卡信息，返回数组 
	{
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();
		//GetAdaptersInfo win下获取网卡信息的接口，网卡信息可能有很多个，一次传入一个指针
		//获取网卡信息可能会失败，因为空间不足，因此有一个输出型参数，用于向用户返回到底有多少个网卡信息
		//计算全部网卡信息的大小
		uint64_t all_adapter_size = sizeof(IP_ADAPTER_INFO);

		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);//获取网卡信息的函数
		if (ret = ERROR_BUFFER_OVERFLOW)//表示缓冲区溢出，空间不足
		{
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];//重新申请空间
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);//重新获取网卡信息
		}
		while (p_adapters)//
		{
			Adapter adapter;
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);//讲一个字符串点分十进制的IP地址转换为网络字节序IP地址
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0)//因为有些网卡没有启用，网卡信息可能为0
			{
				list->push_back(adapter);
				/*std::cout << "网卡名称:" << p_adapters->AdapterName << std::endl;
				std::cout << "网卡描述:" << p_adapters->Description << std::endl;
				std::cout << "IP地址:" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				std::cout << "子网掩码:" << p_adapters->IpAddressList.IpMask.String << std::endl;*/
				//std::cout << std::endl;
			}
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}
#else
	//static bool GetAllAdapter(std::vector<Adpater>* list) linux下接口
#endif
};