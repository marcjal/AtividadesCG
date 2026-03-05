/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <algorithm>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();

// Dimensões da janela
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = "#version 330 core\n"
								   "layout (location = 0) in vec3 position;\n"
								   "layout (location = 1) in vec3 color;\n"
								   "uniform mat4 model;\n"
								   "out vec4 finalColor;\n"
								   "void main()\n"
								   "{\n"
								   "gl_Position = model * vec4(position, 1.0);\n"
								   "finalColor = vec4(color, 1.0);\n"
								   "}\0";

// Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = "#version 330 core\n"
									 "in vec4 finalColor;\n"
									 "out vec4 color;\n"
									 "void main()\n"
									 "{\n"
									 "color = finalColor;\n"
									 "}\n\0";

// ============================
// CONTROLES
// ============================
enum class RotAxis
{
	None,
	X,
	Y,
	Z
};

struct CubeInstance
{
	glm::vec3 pos{0.0f, 0.0f, 0.0f};
	float scale = 1.0f;
	RotAxis rot = RotAxis::None;
};

std::vector<CubeInstance> cubes = {CubeInstance{}};
int activeCube = 0;

const float MOVE_STEP = 0.05f;
const float SCALE_STEP = 0.05f;
const float MIN_SCALE = 0.05f;

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Marcelo!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros das funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Gerando buffer com a geometria do cubo
	GLuint VAO = setupGeometry();

	glUseProgram(shaderID);

	// Localização da uniform "model"
	GLint modelLoc = glGetUniformLocation(shaderID, "model");

	glEnable(GL_DEPTH_TEST);

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		float angle = (GLfloat)glfwGetTime();

		glBindVertexArray(VAO);

		// Desenha todas as instâncias
		for (const auto &c : cubes)
		{
			glm::mat4 model = glm::mat4(1.0f);

			// Translação
			model = glm::translate(model, c.pos);

			// Rotação (escolhida por tecla X/Y/Z no cubo ativo)
			if (c.rot == RotAxis::X)
				model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
			else if (c.rot == RotAxis::Y)
				model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
			else if (c.rot == RotAxis::Z)
				model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

			// Escala uniforme
			model = glm::scale(model, glm::vec3(c.scale));

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			// Cubo = 12 triângulos = 36 vértices
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glDrawArrays(GL_POINTS, 0, 36);
		}

		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glfwTerminate();
	return 0;
}

// Callback de teclado
// Teclas:
//   X/Y/Z -> define eixo de rotação do cubo ativo
//   A/D -> move em X
//   W/S -> move em Z
//   I/J -> move em Y
//   [ e ] -> escala uniforme
//   N -> instancia um novo cubo (vira o ativo)
//   TAB -> alterna cubo ativo
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}

	// Para permitir segurar teclas
	if (!(action == GLFW_PRESS || action == GLFW_REPEAT))
		return;

	// Segurança: garante que existe pelo menos 1 instância
	if (cubes.empty())
		cubes.push_back(CubeInstance{});

	// Referência para o cubo ativo
	CubeInstance &c = cubes[activeCube];

	// Rotação
	if (key == GLFW_KEY_X)
		c.rot = RotAxis::X;
	if (key == GLFW_KEY_Y)
		c.rot = RotAxis::Y;
	if (key == GLFW_KEY_Z)
		c.rot = RotAxis::Z;

	// Translação (WASD: X/Z; IJ: Y)
	if (key == GLFW_KEY_A)
		c.pos.x -= MOVE_STEP;
	if (key == GLFW_KEY_D)
		c.pos.x += MOVE_STEP;

	if (key == GLFW_KEY_W)
		c.pos.z -= MOVE_STEP;
	if (key == GLFW_KEY_S)
		c.pos.z += MOVE_STEP;

	if (key == GLFW_KEY_I)
		c.pos.y += MOVE_STEP;
	if (key == GLFW_KEY_J)
		c.pos.y -= MOVE_STEP;

	// Escala uniforme
	if (key == GLFW_KEY_LEFT_BRACKET)
		c.scale = std::max(MIN_SCALE, c.scale - SCALE_STEP);
	if (key == GLFW_KEY_RIGHT_BRACKET)
		c.scale += SCALE_STEP;

	// Instanciar mais um cubo
	if (key == GLFW_KEY_N && action == GLFW_PRESS)
	{
		glm::vec3 p = c.pos + glm::vec3(0.7f, 0.0f, 0.0f);
		if (p.x > 0.9f)
			p.x = -0.9f; // wrap simples

		cubes.push_back(CubeInstance{p, 1.0f, RotAxis::None});
		activeCube = (int)cubes.size() - 1;
	}

	// Alternar cubo ativo
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
	{
		activeCube = (activeCube + 1) % (int)cubes.size();
	}
}

// Shader setup
int setupShader()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Geometria do CUBO (36 vértices, cada face com uma cor)
int setupGeometry()
{
	GLfloat vertices[] = {
		// Face +Z (frente) - vermelho
		-0.5f,
		-0.5f,
		0.5f,
		1,
		0,
		0,
		0.5f,
		-0.5f,
		0.5f,
		1,
		0,
		0,
		0.5f,
		0.5f,
		0.5f,
		1,
		0,
		0,
		0.5f,
		0.5f,
		0.5f,
		1,
		0,
		0,
		-0.5f,
		0.5f,
		0.5f,
		1,
		0,
		0,
		-0.5f,
		-0.5f,
		0.5f,
		1,
		0,
		0,

		// Face -Z (trás) - verde
		-0.5f,
		-0.5f,
		-0.5f,
		0,
		1,
		0,
		-0.5f,
		0.5f,
		-0.5f,
		0,
		1,
		0,
		0.5f,
		0.5f,
		-0.5f,
		0,
		1,
		0,
		0.5f,
		0.5f,
		-0.5f,
		0,
		1,
		0,
		0.5f,
		-0.5f,
		-0.5f,
		0,
		1,
		0,
		-0.5f,
		-0.5f,
		-0.5f,
		0,
		1,
		0,

		// Face -X (esquerda) - azul
		-0.5f,
		-0.5f,
		-0.5f,
		0,
		0,
		1,
		-0.5f,
		-0.5f,
		0.5f,
		0,
		0,
		1,
		-0.5f,
		0.5f,
		0.5f,
		0,
		0,
		1,
		-0.5f,
		0.5f,
		0.5f,
		0,
		0,
		1,
		-0.5f,
		0.5f,
		-0.5f,
		0,
		0,
		1,
		-0.5f,
		-0.5f,
		-0.5f,
		0,
		0,
		1,

		// Face +X (direita) - amarelo
		0.5f,
		-0.5f,
		-0.5f,
		1,
		1,
		0,
		0.5f,
		0.5f,
		-0.5f,
		1,
		1,
		0,
		0.5f,
		0.5f,
		0.5f,
		1,
		1,
		0,
		0.5f,
		0.5f,
		0.5f,
		1,
		1,
		0,
		0.5f,
		-0.5f,
		0.5f,
		1,
		1,
		0,
		0.5f,
		-0.5f,
		-0.5f,
		1,
		1,
		0,

		// Face +Y (topo) - magenta
		-0.5f,
		0.5f,
		-0.5f,
		1,
		0,
		1,
		-0.5f,
		0.5f,
		0.5f,
		1,
		0,
		1,
		0.5f,
		0.5f,
		0.5f,
		1,
		0,
		1,
		0.5f,
		0.5f,
		0.5f,
		1,
		0,
		1,
		0.5f,
		0.5f,
		-0.5f,
		1,
		0,
		1,
		-0.5f,
		0.5f,
		-0.5f,
		1,
		0,
		1,

		// Face -Y (base) - ciano
		-0.5f,
		-0.5f,
		-0.5f,
		0,
		1,
		1,
		0.5f,
		-0.5f,
		-0.5f,
		0,
		1,
		1,
		0.5f,
		-0.5f,
		0.5f,
		0,
		1,
		1,
		0.5f,
		-0.5f,
		0.5f,
		0,
		1,
		1,
		-0.5f,
		-0.5f,
		0.5f,
		0,
		1,
		1,
		-0.5f,
		-0.5f,
		-0.5f,
		0,
		1,
		1,
	};

	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Atributo cor (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}