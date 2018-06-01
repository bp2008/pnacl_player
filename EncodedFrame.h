#pragma once
#include "ppapi/cpp/var_array_buffer.h"
namespace PnaclPlayer
{
	struct EncodedFrame
	{
		EncodedFrame() : buffer(pp::VarArrayBuffer()), timestamp(0) {}
		EncodedFrame(pp::VarArrayBuffer& buffer, int64_t timestamp) : buffer(buffer), timestamp(timestamp), id(0) {}
		~EncodedFrame() {}
		pp::VarArrayBuffer buffer;
		int64_t timestamp;
		int32_t id;
	};
}