#include <glad\glad.h>
#include <glfw\glfw3.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ERROR(format, ...)	printf(format, __VA_ARGS__)
#define INFO(format, ...)	printf(format, __VA_ARGS__)

static int sWidth		= 1280;
static int sHeight		= 720;

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
}

void error_callback(int error, const char* description)
{
	ERROR("GL code: %d error: %s \n", error, description);
}

static bool shader_compileStatus(const std::string& tag, uint32_t shader)
{
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		char log[1024];
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		ERROR("%s %s \n", tag.data(), log);
		return false;
	}
	return true;
}

static bool shader_linkStatus(uint32_t program)
{
	GLint isLinked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		char log[1024];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		ERROR("%s \n", log);
		return false;
	}
	return true;
}

static uint32_t shader_create(const std::string&tag,  std::string& src, GLenum type)
{
	const GLchar* Source = src.c_str();
	uint32_t shader = glCreateShader(type);
	glShaderSource(shader, 1, &Source, nullptr);
	glCompileShader(shader);
	if (!shader_compileStatus(tag, shader))
		return 0;
	return shader;
}

uint32_t createShaderProgram(std::string& vert, std::string& frag)
{
    uint32_t program = 0;
	uint32_t shader1 = shader_create("Vertex", vert, GL_VERTEX_SHADER);
	uint32_t shader2 = shader_create("Fragment", frag, GL_FRAGMENT_SHADER);	
    if (shader1 != 0 && shader2 != 0) 
    {
        program = glCreateProgram();
        glAttachShader(program, shader1);
        glAttachShader(program, shader2);
        glLinkProgram(program);
        if (!shader_linkStatus(program))
        {
            glDeleteProgram(program);
        }
    }

    if (shader1 != 0) glDeleteShader(shader1);
    if (shader2 != 0) glDeleteShader(shader2);
	return program;
}

// 使用单项设置
void shaderUseUniform()
{
	enum LocationType
	{
		LOC_PROJECT,
		LOC_VIEW,
		LOC_MODEL,
		LOC_COUNT
	};
	
	static bool inited = false;
	static uint32_t program = 0;
	static int32_t  locations[LOC_COUNT] = {};
	if (!inited)
	{
		std::string vertex = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;

			uniform mat4 project;
			uniform mat4 view;
            uniform mat4 model;

            void main()
            {
                gl_Position = project * view * model * vec4(aPos, 1.0);
            }
		)";
		std::string fragment = R"(
            #version 330 core
            
            out vec4 FragColor;

            void main()
            {
                 FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
            }
		)";
		program = createShaderProgram(vertex, fragment);
		if (program != 0)
		{
			locations[LOC_PROJECT] = glGetUniformLocation(program, "project");
			locations[LOC_VIEW] = glGetUniformLocation(program, "view");
			locations[LOC_MODEL] = glGetUniformLocation(program, "model");
			for (uint16_t i = 0; i < LOC_COUNT; ++i)
			{
				if (locations[i] < 0)
					ERROR("location [%d] invalid\n", i);
			}
		}
		inited = true;
	}
	if (program != 0)
	{
		glm::mat4 project 	= glm::perspective((float)glm::radians(90.0), (float)sWidth / (float)sHeight, 0.1f, 100.f);
		glm::mat4 view 		= glm::lookAt(glm::vec3(0.0, 0.0, 5.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 model 	= glm::mat4(1.0);
		model 				= glm::translate(model, glm::vec3(-2.5f, 2.5f, 0.0));

		glUseProgram(program);
		glUniformMatrix4fv(locations[LOC_PROJECT], 1, GL_FALSE, glm::value_ptr(project));
		glUniformMatrix4fv(locations[LOC_VIEW], 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(locations[LOC_MODEL], 1, GL_FALSE, glm::value_ptr(model));
	}
}

const uint16_t CountProgram = 3;
GLint BlockAlienment = 0;
// 使用批量设置
void shaderUseUniformBlock(uint32_t index)
{
	struct MatrixGroup
	{
		glm::vec4 testAlign0;
		glm::mat4 project;
		glm::vec4 testAlign1;
		glm::mat4 view;
		glm::mat4 model;
		glm::vec4 testAlign2;
	};

	struct LightData
	{
		// 这里用vec4数组同步，但shader中使用vec3取出，展示了同样连续存储的字节对齐效果
		glm::vec4 colors[CountProgram];
	};

	static bool inited = false;
	
	static MatrixGroup dataGroup;
	static LightData dataLight;
	static uint32_t programs[CountProgram];
	static uint32_t buffer = 0;
	static uint32_t offset = 0;

	if (!inited)
	{
		// 这里为了演示
		const uint32_t BufferIndexMatrices 	= 0;
		const uint32_t BufferIndexLight 	= 1;
		const uint32_t SizeBlock0	= sizeof(MatrixGroup); // 块0大小
		const uint32_t SizeBlock1	= sizeof(LightData); // 块1大小
		const uint32_t SizeLeft		= SizeBlock0 % BlockAlienment; // 块0对齐的填充量
		const uint32_t OffsetBlock0 = sizeof(MatrixGroup) + (BlockAlienment - SizeLeft); // 块0产生的偏移量
		const uint32_t SizeTotal 	= OffsetBlock0 + SizeBlock1; // 总大小	

		// 初始数据
		dataGroup.testAlign0 = glm::vec4(1.f);
		dataGroup.testAlign1 = glm::vec4(1.f);
		dataGroup.testAlign2 = glm::vec4(1.f);
		dataGroup.project 	 = glm::perspective((float)glm::radians(90.0), (float)sWidth / (float)sHeight, 0.1f, 100.f);
		dataGroup.view		 = glm::lookAt(glm::vec3(0.0, 0.0, 5.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
		dataGroup.model		 = glm::mat4(1.0);

		// 测试字节对齐，由于都是1所以相乘不变，只修改一个元素便可测试效果，可以替换testAlign2为1或0
		dataGroup.testAlign2.a = 0.2f;
		// dataGroup.testAlign2.r = 0.2f;
		// dataGroup.testAlign2.g = 0.2f;
		// dataGroup.testAlign2.b = 0.2f;


		dataLight.colors[0]  = glm::vec4(1.0, 0.0, 0.0, 0.0);
		dataLight.colors[1]  = glm::vec4(0.0, 1.0, 0.0, 0.0);
		dataLight.colors[2]  = glm::vec4(0.0, 0.0, 1.0, 0.0);

		// 创建缓冲
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, buffer);
		glBufferData(GL_UNIFORM_BUFFER, SizeTotal, nullptr, GL_STATIC_DRAW);
		glBindBufferRange(GL_UNIFORM_BUFFER, BufferIndexMatrices, buffer, 0, SizeBlock0);
		glBindBufferRange(GL_UNIFORM_BUFFER, BufferIndexLight, buffer, OffsetBlock0, SizeBlock1);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, SizeBlock0, &dataGroup);
		glBufferSubData(GL_UNIFORM_BUFFER, OffsetBlock0, SizeBlock1, &dataLight);

		std::string vertex = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;

			layout (std140) uniform MatrixBlock 
			{
				vec3 testAlign0;
				mat4 project;
				vec3 testAlign1;
				mat4 view;
                mat4 model;
				vec3 testAlign2;
			};

			out vec3 AlignTest;

            void main()
            {
                gl_Position = project * view * model * vec4(aPos, 1.0);
				AlignTest 	= testAlign0 * testAlign1 * testAlign2;
            }
		)";

		std::string fragments[CountProgram] = {
			R"(
            	#version 330 core
				layout (std140) uniform LightBlock
				{
					vec3 colors[3];
				};
	
            	out vec4 FragColor;
				in vec3 AlignTest;

            	void main()
            	{
            	     FragColor = vec4(colors[0] * AlignTest, 1.0f);
            	}
			)",
			R"(
            	#version 330 core

				layout (std140) uniform LightBlock
				{
					vec3 colors[3];
				};
	
            	out vec4 FragColor;
				in vec3 AlignTest;

            	void main()
            	{
            	     FragColor = vec4(colors[1] * AlignTest, 1.0f);
            	}
			)",
			R"(
            	#version 330 core

				layout (std140) uniform LightBlock
				{
					vec3 colors[3];
				};
	
            	out vec4 FragColor;
				in vec3 AlignTest;

            	void main()
            	{
            	     FragColor = vec4(colors[2] * AlignTest, 1.0f);
            	}
			)"
		};

		for (uint16_t i = 0; i < CountProgram; ++i)
		{
			programs[i] = createShaderProgram(vertex, fragments[i]);

			GLint blockIndexMatrices = glGetUniformBlockIndex(programs[i], "MatrixBlock");
			if (blockIndexMatrices < 0)
			{
				ERROR("shader %d has no Matrices", i);
				continue;
			}
			glUniformBlockBinding(programs[i], blockIndexMatrices, BufferIndexMatrices);

			GLint blockIndexLight = glGetUniformBlockIndex(programs[i], "LightBlock");
			if (blockIndexLight < 0)
			{
				ERROR("shader %d has no Light", i);
				continue;
			}
			glUniformBlockBinding(programs[i], blockIndexLight, BufferIndexLight);
		}

		offset = OffsetBlock0;
		inited = true;
	}

	uint32_t program = programs[index];
	if (program != 0)
	{
		static glm::mat4 models[CountProgram] = {
			glm::translate(glm::mat4(1.f), glm::vec3( 2.5f, 2.5f, 0.0)),
			glm::translate(glm::mat4(1.f), glm::vec3( 2.5f,-2.5f, 0.0)),
			glm::translate(glm::mat4(1.f), glm::vec3(-2.5f,-2.5f, 0.0))
		};
		dataGroup.model = models[index];

		glUseProgram(program);
		glBindBuffer(GL_UNIFORM_BUFFER, buffer);
		glBufferSubData(GL_UNIFORM_BUFFER, offsetof(MatrixGroup, model), sizeof(glm::mat4), &dataGroup.model);
	}
}

void drawScene()
{
    static bool inited = false;
	// 顶点信息
	static uint32_t va = 0;
    static uint32_t vb = 0;
	static uint32_t ib = 0;
	static uint16_t count = 0;
    
    if (!inited)
    {
		float vertexes[] = {
			// 后面
		   -1.0f,  1.0f, -1.0f, // 0. 右下
			1.0f,  1.0f, -1.0f, // 1. 左下
			1.0f, -1.0f, -1.0f, // 2. 左上
		   -1.0f, -1.0f, -1.0f, // 3. 右上

		   // 前面
			1.0f, -1.0f,  1.0f, // 4. 右下
		   -1.0f, -1.0f,  1.0f, // 5. 左下
		   -1.0f,  1.0f,  1.0f, // 6. 左上
			1.0f,  1.0f,  1.0f  // 7. 右上
		};
		uint32_t indices[] = {
			4, 5, 6, 6, 7, 4, // 前
			0, 1, 2, 2, 3, 0, // 后
			5, 3, 0, 0, 6, 5, // 左
			2, 4, 7, 7, 1, 2, // 右
			7, 6, 0, 0, 1, 7, // 上
			2, 3, 5, 5, 4, 2  // 下
		};
		count = _countof(indices);
		glGenVertexArrays(1, &va);
        glGenBuffers(1, &vb);
		glGenBuffers(1, &ib);
		glBindVertexArray(va);
		glBindBuffer(GL_ARRAY_BUFFER, vb);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_STATIC_DRAW);        
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
		glEnableVertexAttribArray(0);
		
		inited = true;
    }
   
	glBindVertexArray(va);
	shaderUseUniform();
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
	for (uint16_t i = 0; i < CountProgram; ++i)
	{
		shaderUseUniformBlock(i);
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
	}
	
}

int main(int argc, const char* argv[])
{
	GLFWwindow* window = nullptr;
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(sWidth, sHeight, "uniform", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		INFO("%s", "Failed to initialize OpenGL context");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &BlockAlienment);

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, sWidth, sHeight);

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawScene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
    return 0;
}