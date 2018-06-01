#pragma once
#include "ppapi/cpp/video_decoder.h"
namespace PnaclPlayer
{
	class Decoder;
	struct DecodedFrame
	{
		DecodedFrame() : decoder(NULL), streamNum(0), timestamp(0) {}
		DecodedFrame(Decoder* decoder, const PP_VideoPicture& picture, int32_t streamNum, int64_t timestamp) : decoder(decoder), picture(picture), streamNum(streamNum), timestamp(timestamp), expectedInterframe(0), recycled(false), rendering(false) {}
		~DecodedFrame() {}
		Decoder* decoder;
		PP_VideoPicture picture;
		int32_t streamNum;
		int64_t timestamp;
		int32_t expectedInterframe;

		bool recycled;
		void RecyclePicture();

		bool rendering;

		bool operator<(DecodedFrame const &other) { return timestamp < other.timestamp; }
	};
}