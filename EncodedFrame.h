#pragma once
#include "ppapi/cpp/var_array_buffer.h"
namespace PnaclPlayer
{
	struct EncodedFrame
	{
		EncodedFrame() : buffer(pp::VarArrayBuffer()), timestamp(0) {}
		EncodedFrame(pp::VarArrayBuffer& buffer, long long timestamp) : buffer(buffer), timestamp(timestamp) {}
		~EncodedFrame() {}
		pp::VarArrayBuffer buffer;
		long long timestamp;
	};
}