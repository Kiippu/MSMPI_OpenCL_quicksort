#pragma once
#include "CL/cl.h"
#include <string>

class OpenCLPrograms
{
public:
	OpenCLPrograms(cl_command_queue& cq, cl_program& p, cl_device_id& di, cl_context& c, cl_kernel& k)
		:m_commands(cq), m_program(p), m_device_id(di), m_context(c), m_kernel(k)
	{}
	
	virtual void runProgram() = 0;

	cl_command_queue& getCommands() { return m_commands; };
	cl_program& getProgram() { return m_program; };
	cl_device_id& getDeviceId() { return m_device_id; };
	cl_context& getContext() { return m_context; };
	cl_kernel& getKernel() { return m_kernel; };

	virtual const std::string& Name(std::string name) { m_name = name; return m_name; };

	virtual int* Array(int* arr) { arr = m_array; return m_array; };

private:
///Open CL vars
	cl_command_queue& m_commands;
	cl_program& m_program;
	cl_device_id& m_device_id;
	cl_context& m_context;
	cl_kernel& m_kernel;

	std::string m_name;
	int * m_array;
};


class LeftOfPivot : public OpenCLPrograms
{
public:
	LeftOfPivot(cl_command_queue& cq, cl_program& p, cl_device_id& di, cl_context& c, cl_kernel& k)
		: OpenCLPrograms(cq, p, di, c, k)
	{ 
		Name("leftOfPivot"); 
	};
	~LeftOfPivot() {};

private:

	virtual void runProgram();

};
