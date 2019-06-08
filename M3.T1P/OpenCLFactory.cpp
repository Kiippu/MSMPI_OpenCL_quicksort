#include "OpenCLFactory.h"


OpenCLFactory::OpenCLFactory()
{
	m_programNames.push_back("quicksort");
	m_kernalCodePath.insert(std::make_pair("quicksort","D:\\University\\Uni 2019\\Programming Paradigms\\assignments\\M3.T2C\\MSMPI_OpenCL_quicksort\\M3.T1P\\quicksort.cl"));

	for (auto& it : m_kernalCodePath)
	{
		for (auto& name : m_programNames)
		{
			buildTask(name, m_kernalCodePath[it.first]);
		}
	}
}


void OpenCLFactory::buildTask(std::string& kernelName, std::string& pathKey)
{
	auto it = m_kernalCodePath.find(pathKey);
	if (it == m_kernalCodePath.end()) {
		printf("[ERROR] - PathKey(%s) does not exist within m_kernelPath map.\n", pathKey.c_str());
		return;
	}
	else
		printf("[info] - Building \"%s\" from with file key \"%s\" on path: \n%s\n",kernelName.c_str(), pathKey.c_str(),m_kernalCodePath[pathKey].c_str());
	int errMsg;
	cl_uint dev_cnt = 1;
	clGetPlatformIDs(0, 0, &dev_cnt);
	m_deviceIdList[kernelName] = cl_device_id();

	cl_platform_id platform_ids[100];
	clGetPlatformIDs(dev_cnt, platform_ids, NULL);

	/// Connectcompute device and check for error
	int gpu = 1;
	errMsg = clGetDeviceIDs(platform_ids[0], gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &m_deviceIdList[kernelName], NULL);
	if (errMsg != CL_SUCCESS)
	{
		printf("Error -  Failed to create a device\n");
		return;
	}

	/// Create a context 
	m_contextList[kernelName] = clCreateContext(0, 1, &m_deviceIdList[kernelName], NULL, NULL, &errMsg);
	if (!m_contextList[kernelName])
	{
		printf("Error -  Failed to create a context\n");
		return;
	}

	/// Create a command with properties
	m_commandQueueList[kernelName] = clCreateCommandQueueWithProperties(m_contextList[kernelName], m_deviceIdList[kernelName], 0, &errMsg);
	if (!m_commandQueueList[kernelName])
	{
		printf("Error - Failed to create a command with properties\n");
		return;
	}

	///Load the file which containing the kernel code, .cl file
	//char string[256];
	FILE *fp;
	// my lovely personal path - worked as reletive path did not...?
	auto temp = it->second.c_str();
	//char fileName[] = "b";
	char *source_str;
	size_t source_size;

	fp = fopen(it->second.c_str(), "r");
	if (!fp) {

		printf("Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(0x100000);
	source_size = fread(source_str, 1, 0x100000, fp);
	fclose(fp);

	/// Create Kernel Program from file
	m_programList[kernelName] = clCreateProgramWithSource(m_contextList[kernelName], 1, (const char **)&source_str, (const size_t *)&source_size, &errMsg);
	if (errMsg != CL_SUCCESS) {
		printf("Error - Failed to create OpenCL program from file %d\n", (int)errMsg);
		exit(1);
	}

	/// Build the executable frome the kernel file
	errMsg = clBuildProgram(m_programList[kernelName], 1, &m_deviceIdList[kernelName], NULL, NULL, NULL);
	if (errMsg != CL_SUCCESS)
	{
		size_t len;
		char buffer[2048];
		printf("Error - Failed to build executable!\n");
		clGetProgramBuildInfo(m_programList[kernelName], m_deviceIdList[kernelName], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		exit(1);
	}

	/// Create the compute kernel in the program we wish to run
	m_kernelList[kernelName] = clCreateKernel(m_programList[kernelName], kernelName.c_str(), &errMsg);
	if (!m_kernelList[kernelName] || errMsg != CL_SUCCESS)
	{
		printf("Error - Failed to create kernel!\n");
		exit(1);
	}

}
