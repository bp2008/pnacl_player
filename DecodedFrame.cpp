#include "DecodedFrame.h"
#include "Decoder.h"

namespace PnaclPlayer
{
	void DecodedFrame::RecyclePicture()
	{
		if (recycled || rendering)
			return;
		recycled = true;
		const PP_VideoPicture& pRef = picture;
		decoder->RecyclePicture(pRef);
	}
}