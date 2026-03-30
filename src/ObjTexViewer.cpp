#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

struct MaterialInfo
{
    string name;
    string mapKd;
};

struct ModelData
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint textureID = 0;
    int nVertices = 0;
    string textureFile;
};

// Protótipos
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
int setupShader();
string getDirectoryFromPath(const string &filePath);
map<string, MaterialInfo> loadMTL(const string &mtlPath);
ModelData loadSimpleOBJTextured(const string &filePath);
GLuint loadTexture(string filePath, int &width, int &height);

// Dimensões da janela
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Vertex Shader
const GLchar *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texc;
layout (location = 2) in vec3 normal;

uniform mat4 projection;
uniform mat4 model;

out vec2 texCoord;
out vec3 fragNormal;

void main()
{
    gl_Position = projection * model * vec4(position, 1.0);
    texCoord = texc;
    fragNormal = mat3(model) * normal;
}
)";

// Fragment Shader
const GLchar *fragmentShaderSource = R"(
#version 330 core
in vec2 texCoord;
in vec3 fragNormal;

uniform sampler2D texBuff;
out vec4 color;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(vec3(1.0, 1.0, 1.0));

    float diffuse = max(dot(N, L), 0.2);
    vec4 texColor = texture(texBuff, texCoord);

    color = vec4(texColor.rgb * diffuse, texColor.a);
}
)";

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "OBJ Texturizado", nullptr, nullptr);
    if (!window)
    {
        cout << "Erro ao criar janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        glfwTerminate();
        return -1;
    }

    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported: " << version << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    ModelData modelData = loadSimpleOBJTextured("assets/Modelos3D/Cube.obj");

    if (modelData.VAO == 0)
    {
        cout << "Falha ao carregar o OBJ." << endl;
        glfwTerminate();
        return -1;
    }

    if (modelData.textureFile.empty())
    {
        cout << "Nenhuma textura encontrada no arquivo MTL. Usando textura padrão." << endl;
        modelData.textureFile = "assets/tex/pixelWall.png";
    }

    cout << "Textura encontrada: " << modelData.textureFile << endl;

    int imgWidth, imgHeight;
    modelData.textureID = loadTexture(modelData.textureFile, imgWidth, imgHeight);

    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glActiveTexture(GL_TEXTURE0);

    mat4 projection = perspective(radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = (float)glfwGetTime();

        mat4 model = mat4(1.0f);
        model = translate(model, vec3(0.0f, 0.0f, -3.0f));
        model = rotate(model, angle, vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

        glBindVertexArray(modelData.VAO);
        glBindTexture(GL_TEXTURE_2D, modelData.textureID);

        glDrawArrays(GL_TRIANGLES, 0, modelData.nVertices);

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &modelData.VAO);
    glDeleteBuffers(1, &modelData.VBO);
    glDeleteTextures(1, &modelData.textureID);

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

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
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
             << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
             << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
             << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

string getDirectoryFromPath(const string &filePath)
{
    size_t pos = filePath.find_last_of("/\\");
    if (pos == string::npos)
        return "";
    return filePath.substr(0, pos + 1);
}

map<string, MaterialInfo> loadMTL(const string &mtlPath)
{
    map<string, MaterialInfo> materials;

    ifstream file(mtlPath);
    if (!file.is_open())
    {
        cerr << "Erro ao abrir MTL: " << mtlPath << endl;
        return materials;
    }

    string line, word;
    MaterialInfo current;
    bool hasCurrent = false;

    while (getline(file, line))
    {
        istringstream iss(line);
        iss >> word;

        if (word.empty() || word[0] == '#')
            continue;

        if (word == "newmtl")
        {
            if (hasCurrent)
            {
                materials[current.name] = current;
            }

            current = MaterialInfo();
            iss >> current.name;
            hasCurrent = true;
        }
        else if (word == "map_Kd")
        {
            iss >> current.mapKd;
        }
    }

    if (hasCurrent)
    {
        materials[current.name] = current;
    }

    return materials;
}

ModelData loadSimpleOBJTextured(const string &filePath)
{
    ModelData modelData;

    vector<vec3> vertices;
    vector<vec2> texCoords;
    vector<vec3> normals;
    vector<GLfloat> vBuffer;

    ifstream file(filePath);
    if (!file.is_open())
    {
        cerr << "Erro ao tentar ler o arquivo " << filePath << endl;
        return modelData;
    }

    string baseDir = getDirectoryFromPath(filePath);
    map<string, MaterialInfo> materials;
    string currentMaterial;

    string line, word;

    while (getline(file, line))
    {
        istringstream iss(line);
        iss >> word;

        if (word.empty() || word[0] == '#')
            continue;

        if (word == "mtllib")
        {
            string mtlFile;
            iss >> mtlFile;
            materials = loadMTL(baseDir + mtlFile);
        }
        else if (word == "usemtl")
        {
            iss >> currentMaterial;
        }
        else if (word == "v")
        {
            vec3 v;
            iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (word == "vt")
        {
            vec2 t;
            iss >> t.x >> t.y;
            texCoords.push_back(t);
        }
        else if (word == "vn")
        {
            vec3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (word == "f")
        {
            for (int i = 0; i < 3; i++)
            {
                string token;
                iss >> token;

                int vi = -1, ti = -1, ni = -1;
                string part;
                istringstream tokenStream(token);

                if (getline(tokenStream, part, '/') && !part.empty())
                    vi = stoi(part) - 1;
                if (getline(tokenStream, part, '/') && !part.empty())
                    ti = stoi(part) - 1;
                if (getline(tokenStream, part, '/') && !part.empty())
                    ni = stoi(part) - 1;

                vec3 pos = vertices.at(vi);
                vec2 uv = vec2(0.0f, 0.0f);
                vec3 normal = vec3(0.0f, 0.0f, 1.0f);

                if (ti >= 0 && ti < (int)texCoords.size())
                {
                    uv = texCoords.at(ti);
                }

                if (ni >= 0 && ni < (int)normals.size())
                {
                    normal = normals.at(ni);
                }

                vBuffer.push_back(pos.x);
                vBuffer.push_back(pos.y);
                vBuffer.push_back(pos.z);

                vBuffer.push_back(uv.x);
                vBuffer.push_back(uv.y);

                vBuffer.push_back(normal.x);
                vBuffer.push_back(normal.y);
                vBuffer.push_back(normal.z);
            }
        }
    }

    if (!currentMaterial.empty() && materials.count(currentMaterial))
    {
        if (!materials[currentMaterial].mapKd.empty())
        {
            modelData.textureFile = baseDir + materials[currentMaterial].mapKd;
        }
    }
    else if (materials.size() == 1)
    {
        auto it = materials.begin();
        if (!it->second.mapKd.empty())
        {
            modelData.textureFile = baseDir + it->second.mapKd;
        }
    }

    glGenBuffers(1, &modelData.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, modelData.VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &modelData.VAO);
    glBindVertexArray(modelData.VAO);

    // Atributo posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Atributo coordenada de textura
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Atributo normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    modelData.nVertices = (int)vBuffer.size() / 8;

    return modelData;
}

GLuint loadTexture(string filePath, int &width, int &height)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        if (nrChannels == 3)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }

        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        cout << "Failed to load texture " << filePath << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}