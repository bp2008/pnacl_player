// This project is originally based on the video_decode example from the Google Native Client SDK which is:
// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.

// Refactored and extended since 2017 by bp2008
// https://github.com/bp2008

#include "pnacl_player.h"

#pragma region PPAPI Boilerplate

namespace
{
	/// <summary>
	/// This object is the global object representing this plugin library as long as it is loaded.
	/// </summary>
	class MyModule : public pp::Module
	{
	public:
		MyModule() : pp::Module() {}
		virtual ~MyModule() {}

		virtual pp::Instance* CreateInstance(PP_Instance instance)
		{
			return new PnaclPlayer::pnacl_player(instance, this);
		}
	};
}  // anonymous namespace

namespace pp
{
	/// <summary>
	/// Factory function for your specialization of the Module object.
	/// </summary>
	Module* CreateModule()
	{
		return new MyModule();
	}
}  // namespace pp

#pragma endregion