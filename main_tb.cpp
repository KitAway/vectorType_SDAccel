/*
======================================================
 Copyright 2016 Liang Ma

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
======================================================
*
* Author:   Liang Ma (liang-ma@polito.it)
*
* Host code defining all the parameters and launching the kernel.
*
* ML_cl.h is the OpenCL library file <CL/cl.hpp>. Currently the version shipped with SDAccel is buggy.
*
* Exception handling is enabled (__CL_ENABLE_EXCEPTIONS) to make host code simpler.
*
* The global and local size are set to 1 since the kernel is written in C/C++ instead of OpenCL.
*
* Several input parameters for the simulation are defined in namespace Params
* and can be changed by using command line options. Only the kernel name must
* be defined.
*
* S0:		 stock price at time 0
* K:		  strike price
* rate:	interest rate
* volatility:	 volatility of stock
* T:		 time period of the option
*
*
* callR:	-c reference value for call price
* putR:		-p reference value for put price
* binary_name:  -a the .xclbin binary name
* kernel_name:  -n the kernel name
* num_sims:					-s number of simulations
*----------------------------------------------------------------------------
*/

#define __CL_ENABLE_EXCEPTIONS

// This should be used when cl.hpp from SDAccel works.
#ifdef CL_HEADER_BUG_FIXED
#include <CL/cl.hpp>
#else
#include "ML_cl.h"
#endif

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <unistd.h>
using namespace std;

namespace Params
{
	char *kernel_name=NULL;     // -n
	char *binary_name=NULL;     // -a
	int   global_size=1024;     // -a
}
void usage(char* name)
{
    cout<<"Usage: "<<name
        <<" -a opencl_binary_name"
        <<" -n kernel_name"
        <<" -s global_size"
        <<endl;
}
int main(int argc, char** argv)
{
	int opt;
	bool flaga=false,flags=false,flagn=false;
	while((opt=getopt(argc,argv,"n:a:c:p:s:"))!=-1){
		switch(opt){
			case 'n':
				Params::kernel_name=optarg;
				flagn=true;
				break;
			case 'a':
				Params::binary_name=optarg;
				flaga=true;
				break;
			case 's':
				Params::global_size=atoi(optarg);
				flags=true;
				break;
			default:
				usage(argv[0]);
				return -1;
		}
	}
	// Check the mandatory argument.
	if(!flaga || !flagn) {
		usage(argv[0]);
		return -1;
	}
	ifstream ifstr(Params::binary_name);
	const string programString(istreambuf_iterator<char>(ifstr),
		(istreambuf_iterator<char>()));
	vector<cl_float4> h_a(Params::global_size);
	vector<float> h_b(Params::global_size);
	vector<float> h_c(Params::global_size);

	for(int i = 0;i<Params::global_size;i++){
		cl_float4 tmp4;
		float tmp, pro=1.0f;
		for(int j=0;j<4;j++){
			tmp = (rand()%100)/100.0f;
			tmp4.s[j]=tmp;//((float)(rand()%100))/100.0f;
				pro*=tmp;
			}
			h_c[i] =pro;				
			h_a[i] =tmp4;
		}

	try
	{
		vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);

		cl::Context context(CL_DEVICE_TYPE_ACCELERATOR);
		vector<cl::Device> devices=context.getInfo<CL_CONTEXT_DEVICES>();

		cl::Program::Binaries binaries(1, make_pair(programString.c_str(), programString.length()));
		cl::Program program(context,devices,binaries);
		try
		{
			program.build(devices);
		}
		catch (cl::Error err)
		{
			if (err.err() == CL_BUILD_PROGRAM_FAILURE)
			{
				string info;
				program.getBuildInfo(devices[0],CL_PROGRAM_BUILD_LOG, &info);
				cout << info << endl;
				return EXIT_FAILURE;
			}
			else throw err;
		}

		cl::CommandQueue commandQueue(context, devices[0]);

		typedef cl::make_kernel<cl::Buffer,cl::Buffer> kernelType;
		kernelType kernelFunctor = kernelType(program, Params::kernel_name);

		cl::Buffer d_a = cl::Buffer(context, h_a.begin(), h_a.end(), true);
		cl::Buffer d_b = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * h_b.size());

		cl::Event event = kernelFunctor(
				cl::EnqueueArgs(
					commandQueue,
					cl::NDRange(Params::global_size),
					cl::NDRange(1)),
				d_a,d_b);

		commandQueue.finish();
		event.wait();

		cl::copy(commandQueue, d_b, h_b.begin(), h_b.end());

		//double ratio = TB::K / TB::S0;
		for (vector<float>::iterator it = h_b.begin(), com=h_c.begin(); it != h_b.end(); it++,com++)
		{
			if(*it != *com)
				cout<<"error happens at location:"<<it-h_b.begin()<<'\t'
					<<"cpu="<<*com<<'\t'
					<<"gpu="<<*it<<endl;

		}
	}
	catch (cl::Error err)
	{
		cerr
			<< "Error:\t"
			<< err.what()
			<< endl;

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
