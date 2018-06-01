#pragma once
#include "DecodedFrame.h"
#include <queue>
#include <sstream>
#include "ppapi/utility/completion_callback_factory.h"
namespace PnaclPlayer
{
	class pnacl_player;
	class RenderScheduler
	{
	public:
		RenderScheduler(pnacl_player* instance) : lastRenderStarted(0), lastRenderDuration(0), instance_(instance), maxQueuedFrames(2), playbackClockStart(0), playbackClockOffset(0), numFramesAccepted(0), lastFrameTS(0), timeoutHelper(0) {}
		~RenderScheduler() {}
		/// <summary>To be called by the owner of this RenderScheduler when a frame is decoded and should be scheduled for rendering.</summary>
		void AddFrame(DecodedFrame* frame);
		/// <summary>To be called by the owner of this RenderScheduler when a frame is finished rendering.</summary>
		void RenderComplete();
		/// <summary>To be called by the owner of this RenderScheduler when changing streams.  Any queued frames will be dropped.</summary>
		void Reset();
		void DelayedPaint(int32_t result);

		int64_t lastRenderStarted;
		int32_t lastRenderDuration;
	private:
		pnacl_player * instance_;

		int32_t maxQueuedFrames;
		int64_t playbackClockStart;
		int64_t playbackClockOffset;
		int64_t numFramesAccepted;
		int64_t lastFrameTS;
		int32_t timeoutHelper;

		// Pictures go into this vector when they are received from the decoder.
		std::vector<DecodedFrame*> frameQueue;
		static bool comparePtrToDecodedFrame(DecodedFrame* a, DecodedFrame* b) { return (a->timestamp < b->timestamp); }

		int64_t ReadPlaybackClock()
		{
			return (perfNow() - playbackClockStart) + playbackClockOffset;
		}
		void OffsetPlaybackClock(int64_t offset)
		{
			playbackClockOffset += offset;
		}
		int64_t GetTimeUntilRenderOldest()
		{
			return (frameQueue.front()->timestamp - ReadPlaybackClock()) - lastRenderDuration;
		}
		DecodedFrame* DequeueOldest()
		{
			/// <summary>Removes and returns a reference to the first item in the queue. Do not call if the queue is empty.</summary>
			DecodedFrame* oldest = frameQueue.front();
			frameQueue.erase(frameQueue.begin());
			return oldest;
		}

		/// <summary>
		/// Returns the time in milliseconds similar to performance.now() in the browser, but related to no particular epoch.
		/// </summary>
		int64_t perfNow();
		void MaintainSchedule();
		void PrintSchedulerStatus(DecodedFrame* frame, std::string message);
	};
}