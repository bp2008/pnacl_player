#include "RenderScheduler.h"
#include "pnacl_player.h"

namespace PnaclPlayer
{
	void RenderScheduler::PrintSchedulerStatus(DecodedFrame* frame, std::string message)
	{
#ifdef DebugLogging
		std::stringstream sstm;
		sstm << "{ ";
		sstm << "playbackClockStart: " << playbackClockStart << ", ";
		sstm << "playbackClockOffset: " << playbackClockOffset << ", ";
		sstm << "numFramesAccepted: " << numFramesAccepted << ", ";
		sstm << "lastFrameTS: " << lastFrameTS << ", ";
		sstm << "timeoutHelper: " << timeoutHelper << ", ";
		sstm << "lastRenderDuration: " << lastRenderDuration << ", ";
		sstm << "frameQueue: " << frameQueue.size() << " / " << maxQueuedFrames << ", ";
		sstm << "frame: ";
		if (frame)
			sstm << frame->timestamp;
		else
			sstm << "NULL";
		sstm << ",    " << message << " ";
		sstm << "}";
		instance_->PostString(sstm.str());
#endif
	}
	/// <summary>
	/// Returns the time in milliseconds similar to performance.now() in the browser, but related to no particular epoch.
	/// </summary>
	int64_t RenderScheduler::perfNow()
	{
		return instance_->perfNow();
	}
	/// <summary>To be called by the owner of this RenderScheduler when a frame is decoded and should be scheduled for rendering.</summary>
	void RenderScheduler::AddFrame(DecodedFrame* frame)
	{
		PrintSchedulerStatus(frame, "start AddFrame()");
		if (numFramesAccepted == 0)
			playbackClockStart = perfNow();
		numFramesAccepted++;
		frame->expectedInterframe = frame->timestamp - lastFrameTS;
		lastFrameTS = frame->timestamp;
		frameQueue.push_back(frame);
		// Sort the frame queue by timestamp, as this can improve playback if frames come in out-of-order.
		// This is also the entire reason we are using a vector instead of a queue to hold frames.
		std::sort(frameQueue.begin(), frameQueue.end(), comparePtrToDecodedFrame);

		if (frameQueue.size() > maxQueuedFrames)
		{
			// Frame queue is overfull.
			// Adjust the playback clock to match the oldest queued frame.
			// When we MaintainSchedule later, this will cause at least one frame to be rendered immediately.
			int64_t timeRemaining = GetTimeUntilRenderOldest();
			std::stringstream sstm;
			sstm << "Jumping clock ahead " << timeRemaining;
			PrintSchedulerStatus(NULL, sstm.str());
			if (timeRemaining > 0)
				OffsetPlaybackClock(timeRemaining); // Jump the clock ahead because we are getting too many frames queued.
		}
		MaintainSchedule();
		PrintSchedulerStatus(frame, "end AddFrame()");
	}
	/// <summary>To be called by the owner of this RenderScheduler when changing streams.  Any queued frames will be dropped.</summary>
	void RenderScheduler::Reset()
	{
		timeoutHelper++; // this invalidates the previous callback watching timeoutHelper
		lastFrameTS = 0;
		numFramesAccepted = 0;
		playbackClockOffset = 0;
		playbackClockStart = perfNow();
		while (frameQueue.size() > 0)
			instance_->frameDropFunc(DequeueOldest(), false);
	}
	void RenderScheduler::DelayedPaint(int32_t result)
	{
		if(frameQueue.empty() && timeoutHelper == result)
			instance_->PostString("RenderScheduler::DelayedPaint() found empty frameQueue");
		if (timeoutHelper == result && !frameQueue.empty())
		{
			std::stringstream sstm;
			sstm << "DelayedPaint(" << result << ")";
			PrintSchedulerStatus(NULL, sstm.str());
			instance_->frameRenderFunc(DequeueOldest());
		}
	}
	/// <summary>To be called by the owner of this RenderScheduler when a frame is finished rendering.</summary>
	void RenderScheduler::RenderComplete()
	{
		PrintSchedulerStatus(NULL, "RenderComplete()");
		MaintainSchedule();
	}
	void RenderScheduler::MaintainSchedule()
	{
		timeoutHelper++; // this invalidates the previous callback watching timeoutHelper
		if (frameQueue.size() > 0)
		{
			int64_t timeToWait = GetTimeUntilRenderOldest();
			if (timeToWait <= 0)
			{
				if (timeToWait < 0)
					OffsetPlaybackClock(timeToWait); // Roll the clock back because frames are coming in late.
				std::stringstream sstm;
				sstm << "MaintainSchedule > " << timeToWait << " > frameRenderFunc()";
				PrintSchedulerStatus(NULL, sstm.str());
				instance_->frameRenderFunc(DequeueOldest());
			}
			else
			{
				std::stringstream sstm;
				sstm << "MaintainSchedule < " << timeToWait << " < frameRenderFunc()";
				PrintSchedulerStatus(NULL, sstm.str());
				instance_->CallDelayedPaintAfterDelay((int32_t)timeToWait, timeoutHelper);
			}
		}
	}
}