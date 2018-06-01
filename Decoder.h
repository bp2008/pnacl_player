#pragma once
#include "EncodedFrame.h"
#include "DecodedFrame.h"

#include <queue>
#include <map>

#include "ppapi/cpp/graphics_3d.h"
#include "ppapi/cpp/video_decoder.h"
#include "ppapi/utility/completion_callback_factory.h"

namespace PnaclPlayer
{
	class pnacl_player;
	class Decoder
	{
	public:
		Decoder(pnacl_player* instance, int id, const pp::Graphics3D& graphics_3d, int hwaccel);
		~Decoder();

		int id() const { return id_; }
		/// <summary>
		/// Returns true if the Decoder is currently performaing an asynchronous flush operation.
		/// </summary>
		bool flushing() const { return flushing_; }
		/// <summary>
		/// Returns true if the Decoder is currently performaing an asynchronous reset operation.
		/// </summary>
		bool resetting() const { return resetting_; }

		/// <summary>
		/// The queue of frames that have not yet been decoded.
		/// </summary>
		std::queue<EncodedFrame> encodedFrameQueue;
		/// <summary>
		/// The queue of frames that have not yet been decoded.
		/// </summary>
		std::map<int32_t, int64_t> timestampMap;

		/// <summary>
		/// The current stream number.  Incremented with each Reset() call.
		/// </summary>
		int32_t currentStreamNum;

		/// <summary>
		/// Clears the queue of frames that have not yet been decoded, and begins asynchronously resetting the decoder.  It is safe to begin sending new frames to the decoder immediately after this method returns.
		/// </summary>
		void Reset();
		/// <summary>
		/// Call this when finished with PP_VideoPicture, to allow the decoder to continue decoding frames.
		/// </summary>
		void RecyclePicture(const PP_VideoPicture& picture);
		/// <summary>
		/// Call this when the browser sends an ArrayBuffer containing video data.  The decoder is responsible for deleting the EncodedFrame when it is no longer needed.
		/// </summary>
		void ReceiveFrame(EncodedFrame frame);
	private:
		void InitializeDone(int32_t result);
		void Start();
		void DecodeNextFrame();
		void DecodeDone(int32_t result);
		void PictureReady(int32_t result, PP_VideoPicture picture);
		void FlushDone(int32_t result);
		void ResetDone(int32_t result);

		pnacl_player* instance_;
		int id_;

		pp::VideoDecoder* ppDecoder;
		pp::CompletionCallbackFactory<Decoder> callback_factory_;

		int next_picture_id_;
		bool flushing_;
		bool resetting_;
		bool initializing_;
		bool decode_looping_;
	};
}