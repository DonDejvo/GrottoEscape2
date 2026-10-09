#include "gles_backend.h"

#include <assert.h>
#include <GLES3/gl3.h>
#include "engine.h"
#include <iostream>

namespace gles {

	const GLuint CAMERA_BINDING_POINT = 0;

	Buffers::Buffers()
		: layers(BUFFERS_COUNT)
	{
	}

	GLESBackend::GLESBackend(Window* window)
		: Backend(window),
		mTextureCache(MAX_TEXTURES),
		mNextTexID(0)
	{
	}

	void GLESBackend::Init()
	{
		glfwMakeContextCurrent(mWindow->getGLFWWindow());

		InitBuffers();
		InitShaderPrograms();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(mShaderCache.sprite);

		GLint samplerLoc = glGetUniformLocation(mShaderCache.sprite, "spriteTexture");
		glUniform1i(samplerLoc, 0);

		GLuint blockIndex = glGetUniformBlockIndex(mShaderCache.sprite, "Camera");
		glUniformBlockBinding(mShaderCache.sprite, blockIndex, CAMERA_BINDING_POINT);
	}

	void GLESBackend::EndFrame()
	{
		glfwSwapBuffers(mWindow->getGLFWWindow());
	}

	void GLESBackend::Destroy()
	{
		glDeleteBuffers(1, &mBuffers.index);
		glDeleteBuffers(1, &mBuffers.camera);
		glDeleteBuffers(TILE_LAYER_COUNT, &mBuffers.layers[0]);
		glDeleteTextures(mNextTexID, &mTextureCache[0]);
		glDeleteProgram(mShaderCache.sprite);
	}

	int GLESBackend::CreateTexture(unsigned char* pixels, int w, int h)
	{
		assert(mNextTexID < MAX_TEXTURES);

		GLuint handle;
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_2D, handle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		int texID = mNextTexID++;
		mTextureCache[texID] = handle;

		return texID;
	}

	void GLESBackend::UploadLayerBuffer(LayerBuffer idx, const float* data, uint32_t size)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers.layers[idx]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size * sizeof(float), data);
	}

	void GLESBackend::UploadCameraBuffer(const float* data)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mBuffers.camera);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, CAMERA_BUFFER_SIZE * CAMERA_COUNT * sizeof(float), data);
	}

	void GLESBackend::BeginPass()
	{
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBuffers.index);
	}

	void GLESBackend::SetViewport(float x, float y, float w, float h)
	{
		glViewport(x, y, w, h);
	}

	void GLESBackend::SetLayerBuffer(LayerBuffer idx)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers.layers[idx]);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, SPRITE_VERTEX_STRIDE * sizeof(float), 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, SPRITE_VERTEX_STRIDE * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
		glEnableVertexAttribArray(1);
	}

	void GLESBackend::SetCamera(CameraBuffer idx)
	{
		glBindBufferRange(
			GL_UNIFORM_BUFFER,
			CAMERA_BINDING_POINT,
			mBuffers.camera,
			static_cast<GLintptr>(idx) * CAMERA_BUFFER_SIZE * sizeof(float),
			CAMERA_BUFFER_SIZE * sizeof(float)
		);
	}

	void GLESBackend::SetTexture(int id)
	{
		glBindTexture(GL_TEXTURE_2D, mTextureCache[id]);
	}

	void GLESBackend::Draw(uint32_t count, uint32_t offset)
	{
		glDrawElements(GL_TRIANGLES, count * 6, GL_UNSIGNED_INT, reinterpret_cast<void*>(offset * 6 * sizeof(uint32_t)));
	}


	void GLESBackend::EndPass()
	{
	}

	void GLESBackend::InitBuffers()
	{
		glGenBuffers(1, &mBuffers.index);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBuffers.index);

		std::vector<uint32_t> indexData = genIndexData();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, LAYER_MAX_TILES * TILE_LAYER_COUNT * 6 * sizeof(uint32_t), indexData.data(), GL_STATIC_DRAW);

		glGenBuffers(BUFFERS_COUNT, &mBuffers.layers[0]);

		glBindBuffer(GL_ARRAY_BUFFER, mBuffers.layers[LayerBuffer::BUFFER_STATIC_TILES]);
		glBufferData(GL_ARRAY_BUFFER, LAYER_MAX_TILES * TILE_LAYER_COUNT * SPRITE_STRIDE * sizeof(float), nullptr, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, mBuffers.layers[LayerBuffer::BUFFER_DYNAMIC_TILES]);
		glBufferData(GL_ARRAY_BUFFER, LAYER_MAX_DYNAMIC_TILES * TILE_LAYER_COUNT * SPRITE_STRIDE * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, mBuffers.layers[LayerBuffer::BUFFER_ENTITIES]);
		glBufferData(GL_ARRAY_BUFFER, MAX_SPRITES * SPRITE_STRIDE * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, mBuffers.layers[LayerBuffer::BUFFER_UI]);
		glBufferData(GL_ARRAY_BUFFER, MAX_UI_SPRITES * SPRITE_STRIDE * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	
		glGenBuffers(1, &mBuffers.camera);
		glBindBuffer(GL_UNIFORM_BUFFER, mBuffers.camera);
		glBufferData(GL_UNIFORM_BUFFER, CAMERA_BUFFER_SIZE * CAMERA_COUNT * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	}

	void GLESBackend::InitShaderPrograms()
	{
		mShaderCache.sprite = CreateShaderProgram("shaders/sprite");
	}

	GLuint GLESBackend::CreateShaderProgram(const std::string& path)
	{
		GLuint program = glCreateProgram();

		GLuint vertexShader = CompileShader(Engine::GetAssetManager().LoadToString(path + ".vert"), GL_VERTEX_SHADER);
		GLuint fragmentShader = CompileShader(Engine::GetAssetManager().LoadToString(path + ".frag"), GL_FRAGMENT_SHADER);

		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);

		glLinkProgram(program);

		glDetachShader(program, vertexShader);
		glDetachShader(program, fragmentShader);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		GLint linked = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (!linked) {
			GLint logLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

			std::string log(logLength, '\0');
			if (logLength > 0) {
				glGetProgramInfoLog(program, logLength, nullptr, log.data());
			}

			std::cout << "Shader program link error (" << path << "): " << log << "\n";

			glDeleteProgram(program);
			program = 0;
		}

		return program;
	}

	GLuint GLESBackend::CompileShader(const std::string& source, GLenum type)
	{
		GLuint shader = glCreateShader(type);

		const char* src = source.c_str();
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		GLint compiled = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint logLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

			std::string log(logLength, '\0');
			if (logLength > 0) {
				glGetShaderInfoLog(shader, logLength, nullptr, log.data());
			}

			std::cout << "Shader compile error (" << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << "): " << log << "\n";

			glDeleteShader(shader);
			return 0;
		}

		return shader;
	}

}