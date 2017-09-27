#include "pnacl_player.h"

// Assert |context_| isn't holding any GL Errors.  Done as a macro instead of a
// function to preserve line number information in the failure message.
#define assertNoGLError() assert(!gles2_if_->GetError(context_->pp_resource()));

namespace PnaclPlayer
{

	pnacl_player::pnacl_player(PP_Instance instance, pp::Module* module) : pp::Instance(instance), pp::Graphics3DClient(this), is_painting_(false), hwaccel_(0), num_frames_rendered_(0), first_frame_delivered_ticks_(-1), last_swap_request_ticks_(-1), swap_ticks_(0), callback_factory_(this), context_(NULL), nextFrameTimestamp(0)
	{
		console_if_ = static_cast<const PPB_Console*>(pp::Module::Get()->GetBrowserInterface(PPB_CONSOLE_INTERFACE));
		core_if_ = static_cast<const PPB_Core*>(pp::Module::Get()->GetBrowserInterface(PPB_CORE_INTERFACE));
		gles2_if_ = static_cast<const PPB_OpenGLES2*>(pp::Module::Get()->GetBrowserInterface(PPB_OPENGLES2_INTERFACE));
	}

	pnacl_player::~pnacl_player()
	{
		if (!context_)
			return;

		PP_Resource graphics_3d = context_->pp_resource();
		if (shader_2d_.program)
			gles2_if_->DeleteProgram(graphics_3d, shader_2d_.program);
		if (shader_rectangle_arb_.program)
			gles2_if_->DeleteProgram(graphics_3d, shader_rectangle_arb_.program);
		if (shader_external_oes_.program)
			gles2_if_->DeleteProgram(graphics_3d, shader_external_oes_.program);

		if (video_decoder_)
			delete video_decoder_;

		delete context_;
	}

	bool pnacl_player::Init(uint32_t argc, const char * argn[], const char * argv[])
	{
		for (uint32_t i = 0; i < argc; i++)
		{
			if (strncmp(argn[i], "hwaccel", 256) == 0)
			{
				if (strncmp(argv[i], "1", 256) == 0)
					hwaccel_ = 1;
				else if (strncmp(argv[i], "2", 256) == 0)
					hwaccel_ = 2;
				else
					hwaccel_ = 0;
			}
		}
		return true;
	}

	void pnacl_player::DidChangeView(const pp::Rect& position, const pp::Rect& clip_ignored)
	{
		if (position.width() == 0 || position.height() == 0)
			return;
		if (plugin_size_.width() > 0)
		{
			//plugin_size_ = position.size();
		}
		else
		{
			PostString("initializing");
			plugin_size_ = position.size();

			// Initialize graphics.
			InitGL();
			InitializeDecoders();
		}
	}

	void pnacl_player::InitializeDecoders()
	{
		assert(!video_decoder_);
		video_decoder_ = new Decoder(this, 0, *context_, hwaccel_);
	}

	void pnacl_player::ReceiveDecodedPicture(DecodedFrame frame)
	{
		// TODO: Figure out how long we should wait before painting this frame.
		core_if_->CallOnMainThread(0, callback_factory_.NewCallback(&pnacl_player::DelayedPaint, frame).pp_completion_callback(), 0);
	}

	void pnacl_player::DelayedPaint(int32_t result, DecodedFrame frame)
	{
		// If we are still working on the same video stream, paint the picture.
		//if (streamNum == currentStreamNum)
		PaintPicture(frame);
		// TODO: Else Recycle the picture now, if it has not already been recycled.
		// TODO: We should loop through and recycle pending pictures during decoder reset
		// That means keeping a reference to the DecodedFrame objects until they get recycled.  A queue should handle that nicely.

	}

	void pnacl_player::PaintPicture(DecodedFrame frame)
	{
		if (first_frame_delivered_ticks_ == -1)
			assert((first_frame_delivered_ticks_ = core_if_->GetTimeTicks()) != -1);

		if (plugin_size_.width() != frame.picture.texture_size.width || plugin_size_.height() != frame.picture.texture_size.height)
		{
			// This frame size is different from the last one.
			plugin_size_.SetSize(frame.picture.texture_size.width, frame.picture.texture_size.height);
			std::stringstream sstm;
			sstm << "vr {"
				<< "\"w\":" << plugin_size_.width()
				<< ",\"h\":" << plugin_size_.height()
				<< " }";
			PostString(sstm.str());
			context_->ResizeBuffers(plugin_size_.width(), plugin_size_.height());
			return;
		}

		/// Enqueue up to N pictures for painting.  If we get more than N pictures behind, frames will be dropped.
		if (pending_pictures_.size() < 15)
			pending_pictures_.push(frame);

		if (!is_painting_)
			PaintNextPicture();
	}

	void pnacl_player::PaintNextPicture()
	{
		assert(!is_painting_);

		DecodedFrame& next = pending_pictures_.front();

		// A frame may have already been recycled during a decoder reset, so we should check that here.
		if (next.recycled)
		{
			pending_pictures_.pop();
			if (!pending_pictures_.empty())
				PaintNextPicture();
		}
		is_painting_ = true;
		next.rendering = true;

		const PP_VideoPicture& picture = next.picture;

		int x = 0;
		int y = 0;
		int w = plugin_size_.width();
		int h = plugin_size_.height();

		PP_Resource graphics_3d = context_->pp_resource();
		if (picture.texture_target == GL_TEXTURE_2D)
		{
			Create2DProgramOnce();
			gles2_if_->UseProgram(graphics_3d, shader_2d_.program);
			gles2_if_->Uniform2f(graphics_3d, shader_2d_.texcoord_scale_location, 1.0, 1.0);
		}
		else if (picture.texture_target == GL_TEXTURE_RECTANGLE_ARB)
		{
			CreateRectangleARBProgramOnce();
			gles2_if_->UseProgram(graphics_3d, shader_rectangle_arb_.program);
			gles2_if_->Uniform2f(graphics_3d, shader_rectangle_arb_.texcoord_scale_location, picture.texture_size.width, picture.texture_size.height);
		}
		else
		{
			assert(picture.texture_target == GL_TEXTURE_EXTERNAL_OES);
			CreateExternalOESProgramOnce();
			gles2_if_->UseProgram(graphics_3d, shader_external_oes_.program);
			gles2_if_->Uniform2f(graphics_3d, shader_external_oes_.texcoord_scale_location, 1.0, 1.0);
		}

		gles2_if_->Viewport(graphics_3d, x, y, w, h);
		gles2_if_->ActiveTexture(graphics_3d, GL_TEXTURE0);
		gles2_if_->BindTexture(graphics_3d, picture.texture_target, picture.texture_id);
		gles2_if_->DrawArrays(graphics_3d, GL_TRIANGLE_STRIP, 0, 4);

		gles2_if_->UseProgram(graphics_3d, 0);

		last_swap_request_ticks_ = core_if_->GetTimeTicks();
		context_->SwapBuffers(callback_factory_.NewCallback(&pnacl_player::PaintFinished));
	}

	void pnacl_player::PaintFinished(int32_t result)
	{
		assert(result == PP_OK);
		swap_ticks_ += core_if_->GetTimeTicks() - last_swap_request_ticks_;
		is_painting_ = false;
		++num_frames_rendered_;
		DecodedFrame& last = pending_pictures_.front();
		last.rendering = false;
		{
			std::stringstream sstm;
			sstm << "rf {"
				<< "\"w\":" << plugin_size_.width()
				<< ",\"h\":" << plugin_size_.height()
				<< ",\"t\":" << last.timestamp
				<< " }";
			PostString(sstm.str());
		}
		if (num_frames_rendered_ % 50 == 0)
		{
			double elapsed = core_if_->GetTimeTicks() - first_frame_delivered_ticks_;
			double fps = (elapsed > 0) ? num_frames_rendered_ / elapsed : 1000;
			double ms_per_swap = (swap_ticks_ * 1e3) / num_frames_rendered_;
			double secs_average_latency = video_decoder_->GetAverageLatency();
			double ms_average_latency = 1000 * secs_average_latency;
			std::stringstream sstm;
			sstm << "RenderInfo { \"NumFrames\": " << num_frames_rendered_
				<< ", \"fps\": " << fps
				<< ", \"avg_swap_ms\": " << ms_per_swap
				<< ", \"avg_latency_ms\": " << ms_average_latency
				<< " }";
			PostString(sstm.str());
		}

		// If the decoders were reset, this will be empty.
		if (pending_pictures_.empty())
			return;

		last.RecyclePicture();
		pending_pictures_.pop();

		// Keep painting as long as we have pictures.
		if (!pending_pictures_.empty())
			PaintNextPicture();
	}

	/// Handler for messages coming in from the browser via postMessage().  The
	/// @a var_message can contain be any pp:Var type; for example int, string
	/// Array or Dictinary. Please see the pp:Var documentation for more details.
	/// @param[in] var_message The message posted by the browser.
	void pnacl_player::HandleMessage(const pp::Var& var_message)
	{
		if (var_message.is_string())
		{
			std::string message = var_message.AsString();
			if (message == "reset")
			{
				if (video_decoder_)
				{
					video_decoder_->Reset();
					PostString("Reset");
				}
				else
					PostString("not yet ready!");
			}
			else if (message.find("f ") == 0)
				nextFrameTimestamp = strtoll(message.substr(2).c_str(), NULL, 10);
		}
		else if (var_message.is_array_buffer())
		{
			if (video_decoder_)
			{
				pp::VarArrayBuffer buffer(var_message);
				std::stringstream sstr;
				sstr << "Received frame " << nextFrameTimestamp;
				PostString(sstr.str());
				video_decoder_->ReceiveFrame(buffer, nextFrameTimestamp);
			}
			else
				PostString("not yet ready!");
		}
	}

	void pnacl_player::PostString(std::string message)
	{
		pp::Var var_message = pp::Var(message);
		PostMessage(var_message);
	}

#pragma region Low-Level Rendering
	void pnacl_player::InitGL()
	{
		assert(plugin_size_.width() && plugin_size_.height());
		is_painting_ = false;

		assert(!context_);
		int32_t context_attributes[] = {
			PP_GRAPHICS3DATTRIB_ALPHA_SIZE,     8,
			PP_GRAPHICS3DATTRIB_BLUE_SIZE,      8,
			PP_GRAPHICS3DATTRIB_GREEN_SIZE,     8,
			PP_GRAPHICS3DATTRIB_RED_SIZE,       8,
			PP_GRAPHICS3DATTRIB_DEPTH_SIZE,     0,
			PP_GRAPHICS3DATTRIB_STENCIL_SIZE,   0,
			PP_GRAPHICS3DATTRIB_SAMPLES,        0,
			PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS, 0,
			PP_GRAPHICS3DATTRIB_WIDTH,          plugin_size_.width(),
			PP_GRAPHICS3DATTRIB_HEIGHT,         plugin_size_.height(),
			PP_GRAPHICS3DATTRIB_NONE,
		};
		context_ = new pp::Graphics3D(this, context_attributes);
		assert(!context_->is_null());
		assert(BindGraphics(*context_));

		// Clear color bit.
		gles2_if_->ClearColor(context_->pp_resource(), 1, 0, 0, 1);
		gles2_if_->Clear(context_->pp_resource(), GL_COLOR_BUFFER_BIT);

		assertNoGLError();

		CreateGLObjects();
	}

	void pnacl_player::CreateGLObjects()
	{
		// Assign vertex positions and texture coordinates to buffers for use in
		// shader program.
		static const float kVertices[] = {
			-1, -1, -1, 1, 1, -1, 1, 1,  // Position coordinates.
			0,  1,  0,  0, 1, 1,  1, 0,  // Texture coordinates.
		};

		GLuint buffer;
		gles2_if_->GenBuffers(context_->pp_resource(), 1, &buffer);
		gles2_if_->BindBuffer(context_->pp_resource(), GL_ARRAY_BUFFER, buffer);

		gles2_if_->BufferData(context_->pp_resource(),
			GL_ARRAY_BUFFER,
			sizeof(kVertices),
			kVertices,
			GL_STATIC_DRAW);
		assertNoGLError();
	}
#pragma region Shader Stuff
	static const char kVertexShader[] =
		"varying vec2 v_texCoord;            \n"
		"attribute vec4 a_position;          \n"
		"attribute vec2 a_texCoord;          \n"
		"uniform vec2 v_scale;               \n"
		"void main()                         \n"
		"{                                   \n"
		"    v_texCoord = v_scale * a_texCoord; \n"
		"    gl_Position = a_position;       \n"
		"}";

	void pnacl_player::Create2DProgramOnce()
	{
		if (shader_2d_.program)
			return;
		static const char kFragmentShader2D[] =
			"precision mediump float;            \n"
			"varying vec2 v_texCoord;            \n"
			"uniform sampler2D s_texture;        \n"
			"void main()                         \n"
			"{"
			"    gl_FragColor = texture2D(s_texture, v_texCoord); \n"
			"}";
		shader_2d_ = CreateProgram(kVertexShader, kFragmentShader2D);
		assertNoGLError();
	}

	void pnacl_player::CreateRectangleARBProgramOnce()
	{
		if (shader_rectangle_arb_.program)
			return;
		static const char kFragmentShaderRectangle[] =
			"#extension GL_ARB_texture_rectangle : require\n"
			"precision mediump float;            \n"
			"varying vec2 v_texCoord;            \n"
			"uniform sampler2DRect s_texture;    \n"
			"void main()                         \n"
			"{"
			"    gl_FragColor = texture2DRect(s_texture, v_texCoord).rgba; \n"
			"}";
		shader_rectangle_arb_ =
			CreateProgram(kVertexShader, kFragmentShaderRectangle);
		assertNoGLError();
	}

	void pnacl_player::CreateExternalOESProgramOnce()
	{
		if (shader_external_oes_.program)
			return;
		static const char kFragmentShaderExternal[] =
			"#extension GL_OES_EGL_image_external : require\n"
			"precision mediump float;            \n"
			"varying vec2 v_texCoord;            \n"
			"uniform samplerExternalOES s_texture; \n"
			"void main()                         \n"
			"{"
			"    gl_FragColor = texture2D(s_texture, v_texCoord); \n"
			"}";
		shader_external_oes_ = CreateProgram(kVertexShader, kFragmentShaderExternal);
		assertNoGLError();
	}
	Shader pnacl_player::CreateProgram(const char* vertex_shader, const char* fragment_shader)
	{
		Shader shader;

		// Create shader program.
		shader.program = gles2_if_->CreateProgram(context_->pp_resource());
		CreateShader(shader.program, GL_VERTEX_SHADER, vertex_shader, strlen(vertex_shader));
		CreateShader(shader.program, GL_FRAGMENT_SHADER, fragment_shader, strlen(fragment_shader));
		gles2_if_->LinkProgram(context_->pp_resource(), shader.program);
		gles2_if_->UseProgram(context_->pp_resource(), shader.program);
		gles2_if_->Uniform1i(context_->pp_resource(), gles2_if_->GetUniformLocation(context_->pp_resource(), shader.program, "s_texture"), 0);
		assertNoGLError();

		shader.texcoord_scale_location = gles2_if_->GetUniformLocation(context_->pp_resource(), shader.program, "v_scale");

		GLint pos_location = gles2_if_->GetAttribLocation(context_->pp_resource(), shader.program, "a_position");
		GLint tc_location = gles2_if_->GetAttribLocation(context_->pp_resource(), shader.program, "a_texCoord");
		assertNoGLError();

		gles2_if_->EnableVertexAttribArray(context_->pp_resource(), pos_location);
		gles2_if_->VertexAttribPointer(context_->pp_resource(), pos_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
		gles2_if_->EnableVertexAttribArray(context_->pp_resource(), tc_location);
		gles2_if_->VertexAttribPointer(context_->pp_resource(), tc_location, 2, GL_FLOAT, GL_FALSE, 0, static_cast<float*>(0) + 8);  // Skip position coordinates.

		gles2_if_->UseProgram(context_->pp_resource(), 0);
		assertNoGLError();
		return shader;
	}

	void pnacl_player::CreateShader(GLuint program, GLenum type, const char* source, int size)
	{
		GLuint shader = gles2_if_->CreateShader(context_->pp_resource(), type);
		gles2_if_->ShaderSource(context_->pp_resource(), shader, 1, &source, &size);
		gles2_if_->CompileShader(context_->pp_resource(), shader);
		gles2_if_->AttachShader(context_->pp_resource(), program, shader);
		gles2_if_->DeleteShader(context_->pp_resource(), shader);
	}
#pragma endregion

#pragma endregion
}