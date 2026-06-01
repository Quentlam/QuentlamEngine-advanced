#include "qlpch.h"
#include "OpenGLContext.h"
#include <Glad/glad.h>
#include <GLFW/glfw3.h>
namespace Quentlam
{
	
	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		:m_WindowHandle(windowHandle)
	{
		QL_CORE_ASSERT(windowHandle, "window handle is null!");
	}

	void OpenGLContext::Init()
	{
		QL_PROFILE_FUNCTION();

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		QL_CORE_ASSERT(status, "Faile to initialized Glad!");
		QL_CORE_INFO("OpenGL Info:");
		QL_CORE_INFO("OpenGL Vendor: {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
		QL_CORE_INFO("OpenGL Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
		QL_CORE_INFO("OpenGL Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	


#ifdef QL_ENABLE_ASSERT
		int versionMajor;
		int versionMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
		glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

		QL_CORE_ASSERT(versionMajor < 4 || (versionMajor == 4 && versionMinor >= 5),"Quentlam Engine requires at least OpenGL version 4.5!")
#endif

	}
	
	void OpenGLContext::SwapBuffers()
	{
		QL_PROFILE_FUNCTION();

		glfwSwapBuffers(m_WindowHandle);
	}

}