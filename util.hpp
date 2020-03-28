#pragma once

#ifdef _WIN32
#include<iostream>
#include<fstream>
#include<vector>
#include<stdint.h>
#include<sstream>
#include<WS2tcpip.h>
#include<Iphlpapi.h>//��ȡ������Ϣ�ӿڵ�ͷ�ļ�
#pragma comment(lib,"Iphlpapi.lib")//��ȡ������Ϣ�ӿڵĿ��ļ�
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
		tmp >> res;//���ݺ���������͵Ĳ�ͬ������������ݽ�������ת��
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
		fopen_s(&fp, name.c_str(), "wb");//�Զ����ƴ��ļ�

		if (fp == NULL)
		{
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}

		fseek(fp, offset, SEEK_SET);//SEEK_SETƫ����
		int ret = fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size())
		{
			std::cerr << "�ļ�д������ʧ��\n";
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
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}

		int ret = fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize)
		{
			std::cerr << "�ļ���ȡʧ��\n";
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
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);//��ת��ָ��λ��

		int ret = fread(&(*body)[0], 1, len, fp);//��ȡָ��λ�����ݵ�body��
		if (ret != len)
		{
			std::cerr << "�ļ���ȡʧ��\n";
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
	uint32_t _ip_addr;  //�����ϵ�IP��ַ
	uint32_t _mask_addr;//�����ϵ���������
};

class AdpaterUtil
{
#ifdef _WIN32
public:
	static bool GetAllAdapter(std::vector<Adapter>* list)//��ȡ������Ϣ���������� 
	{
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();
		//GetAdaptersInfo win�»�ȡ������Ϣ�Ľӿڣ�������Ϣ�����кܶ����һ�δ���һ��ָ��
		//��ȡ������Ϣ���ܻ�ʧ�ܣ���Ϊ�ռ䲻�㣬�����һ������Ͳ������������û����ص����ж��ٸ�������Ϣ
		//����ȫ��������Ϣ�Ĵ�С
		uint64_t all_adapter_size = sizeof(IP_ADAPTER_INFO);

		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);//��ȡ������Ϣ�ĺ���
		if (ret = ERROR_BUFFER_OVERFLOW)//��ʾ������������ռ䲻��
		{
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];//��������ռ�
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);//���»�ȡ������Ϣ
		}
		while (p_adapters)//
		{
			Adapter adapter;
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);//��һ���ַ������ʮ���Ƶ�IP��ַת��Ϊ�����ֽ���IP��ַ
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0)//��Ϊ��Щ����û�����ã�������Ϣ����Ϊ0
			{
				list->push_back(adapter);
				/*std::cout << "��������:" << p_adapters->AdapterName << std::endl;
				std::cout << "��������:" << p_adapters->Description << std::endl;
				std::cout << "IP��ַ:" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				std::cout << "��������:" << p_adapters->IpAddressList.IpMask.String << std::endl;*/
				//std::cout << std::endl;
			}
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}
#else
	//static bool GetAllAdapter(std::vector<Adpater>* list) linux�½ӿ�
#endif
};