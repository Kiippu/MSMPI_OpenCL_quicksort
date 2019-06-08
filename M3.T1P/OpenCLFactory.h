#pragma once
#include <map>
#include <vector>
#include "CL/cl.h"

class OpenCLFactory
{
public:
	/// singleton implementation
	static OpenCLFactory&getInstance()
	{
		static OpenCLFactory instance;
		return instance;
	}
	/// no copy
	OpenCLFactory(OpenCLFactory&temp) = delete;
	void operator=(OpenCLFactory const&temp) = delete;
	~OpenCLFactory() {};


	void buildTask(std::string& kernelName, std::string& pathKey);


private:
	OpenCLFactory();

	std::map<std::string, cl_command_queue> m_commandQueueList;
	std::map<std::string, cl_program> m_programList;
	std::map<std::string, cl_device_id> m_deviceIdList;
	std::map<std::string, cl_context> m_contextList;
	std::map<std::string, cl_kernel> m_kernelList;

	std::map<std::string, std::string> m_kernalCodePath;
	std::vector<std::string> m_programNames;

};

