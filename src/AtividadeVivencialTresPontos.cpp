#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstddef>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

const GLuint WIDTH = 1000;
const GLuint HEIGHT = 1000;

const int KEY_LIGHT = 0;
const int FILL_LIGHT = 1;
const int BACK_LIGHT = 2;
const int NUMBER_OF_LIGHTS = 3;

enum class TransformMode
{
    TRANSLATE,
    ROTATE,
    SCALE
};

struct Vertex
{
    vec3 position;
    vec3 normal;
};

struct Object3D
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLsizei nVertices = 0;

    string name;

    vec3 position = vec3(0.0f);
    vec3 rotation = vec3(0.0f);
    vec3 scale = vec3(1.0f);

    vec3 color = vec3(0.8f, 0.8f, 0.8f);
};

struct PointLight
{
    vec3 position = vec3(0.0f);
    vec3 color = vec3(1.0f);
    vec3 intensity = vec3(1.0f);

    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    bool enabled = true;
};

vector<Object3D> objects;
PointLight lights[NUMBER_OF_LIGHTS];

int selectedObject = 0;
TransformMode currentMode = TransformMode::TRANSLATE;

bool wireframeMode = false;

int lightingDebugMode = 0;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

int setupShader();

Object3D loadSimpleOBJ(
    const string &filePath,
    const string &objectName,
    const vec3 &objectColor);

int parseOBJVertexIndex(const string &token, int vertexCount);

bool fileExists(const string &path);
string resolvePath(const string &path);

void printControls();
void printSelectedObject();
void printLightStatus(int lightIndex);
void applyTransformation(int key, int mods);
void deleteObjectBuffers();

void initializeThreePointLights();
void updateThreePointLightsFromObject(const Object3D &mainObject);
void uploadLights(GLuint shaderID);
void toggleLight(int lightIndex);
string lightName(int lightIndex);

const GLchar *vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    vec4 worldPosition = model * vec4(position, 1.0);
    FragPos = vec3(worldPosition);
    Normal = mat3(transpose(inverse(model))) * normal;

    gl_Position = projection * view * worldPosition;
}
)";

const GLchar *fragmentShaderSource = R"(
#version 330 core

#define NUMBER_OF_LIGHTS 3

uniform vec3 objectColor;

// Uniforms separados para evitar problemas com array de struct/bool em alguns drivers.
uniform vec3 lightPositions[NUMBER_OF_LIGHTS];
uniform vec3 lightColors[NUMBER_OF_LIGHTS];
uniform vec3 lightIntensities[NUMBER_OF_LIGHTS];
uniform float lightConstants[NUMBER_OF_LIGHTS];
uniform float lightLinears[NUMBER_OF_LIGHTS];
uniform float lightQuadratics[NUMBER_OF_LIGHTS];
uniform int lightEnabled[NUMBER_OF_LIGHTS];

// 0 = iluminação final, 1 = cor sem luz, 2 = normais, 3 = só contribuição das luzes.
uniform int lightingDebugMode;

in vec3 FragPos;
in vec3 Normal;

out vec4 color;

vec3 calculateDiffuse(int lightIndex, vec3 normalDirection)
{
    if (lightEnabled[lightIndex] == 0)
    {
        return vec3(0.0);
    }

    vec3 lightVector = lightPositions[lightIndex] - FragPos;
    float distanceToLight = length(lightVector);
    vec3 lightDirection = normalize(lightVector);

    // abs() evita que OBJ com normal/winding invertido fique totalmente preto.
    float diffuseFactor = abs(dot(normalDirection, lightDirection));

    // Mantém um mínimo difuso para ficar visível durante a apresentação.
    diffuseFactor = 0.15 + 0.85 * diffuseFactor;

    // Atenuação da luz pontual. Valores suaves para a escala dos modelos usados.
    float attenuation = 1.0 / (
        lightConstants[lightIndex] +
        lightLinears[lightIndex] * distanceToLight +
        lightQuadratics[lightIndex] * distanceToLight * distanceToLight
    );

    vec3 diffuse = objectColor * lightColors[lightIndex] * lightIntensities[lightIndex] * diffuseFactor;

    return diffuse * attenuation;
}

void main()
{
    vec3 normalDirection = normalize(Normal);

    if (lightingDebugMode == 1)
    {
        // Modo diagnóstico: mostra a cor do objeto sem depender de luz.
        color = vec4(objectColor, 1.0);
        return;
    }

    if (lightingDebugMode == 2)
    {
        // Modo diagnóstico: mostra as normais como cores.
        color = vec4(normalDirection * 0.5 + 0.5, 1.0);
        return;
    }

    vec3 lightContribution = vec3(0.0);

    for (int i = 0; i < NUMBER_OF_LIGHTS; i++)
    {
        lightContribution += calculateDiffuse(i, normalDirection);
    }

    if (lightingDebugMode == 3)
    {
        // Modo diagnóstico: mostra apenas as luzes, sem ambiente.
        color = vec4(clamp(lightContribution, vec3(0.0), vec3(1.0)), 1.0);
        return;
    }

    // Ambiente suficiente para a cena não ficar preta quando as luzes forem desligadas.
    vec3 ambient = 0.28 * objectColor;
    vec3 result = ambient + lightContribution;

    color = vec4(clamp(result, vec3(0.0), vec3(1.0)), 1.0);
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

    GLFWwindow *window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "Atividade Vivencial - Iluminacao de Tres Pontos",
        nullptr,
        nullptr);

    if (!window)
    {
        cout << "Erro ao criar janela GLFW." << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Erro ao inicializar GLAD." << endl;
        glfwTerminate();
        return -1;
    }

    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);

    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported: " << version << endl;

    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    Object3D cube = loadSimpleOBJ(
        resolvePath("assets/Modelos3D/Cube.obj"),
        "Cube",
        vec3(0.2f, 0.8f, 1.0f));

    if (cube.VAO != 0)
    {
        cube.position = vec3(-2.0f, 0.0f, 0.0f);
        cube.scale = vec3(0.8f);
        objects.push_back(cube);
    }

    Object3D suzanne = loadSimpleOBJ(
        resolvePath("assets/Modelos3D/Suzanne.obj"),
        "Suzanne",
        vec3(1.0f, 0.7f, 0.2f));

    if (suzanne.VAO != 0)
    {
        suzanne.position = vec3(0.0f, 0.0f, 0.0f);
        suzanne.scale = vec3(0.8f);
        objects.push_back(suzanne);
    }

    Object3D suzanneSubdiv = loadSimpleOBJ(
        resolvePath("assets/Modelos3D/SuzanneSubdiv1.obj"),
        "SuzanneSubdiv1",
        vec3(0.6f, 1.0f, 0.4f));

    if (suzanneSubdiv.VAO != 0)
    {
        suzanneSubdiv.position = vec3(2.0f, 0.0f, 0.0f);
        suzanneSubdiv.scale = vec3(0.8f);
        objects.push_back(suzanneSubdiv);
    }

    if (objects.empty())
    {
        cout << "Nenhum objeto foi carregado. Verifique o caminho dos arquivos OBJ." << endl;
        glfwTerminate();
        return -1;
    }

    selectedObject = 0;
    initializeThreePointLights();

    printControls();
    printSelectedObject();
    printLightStatus(KEY_LIGHT);
    printLightStatus(FILL_LIGHT);
    printLightStatus(BACK_LIGHT);

    mat4 projection = perspective(
        radians(45.0f),
        (float)WIDTH / (float)HEIGHT,
        0.1f,
        100.0f);

    mat4 view = mat4(1.0f);
    view = translate(view, vec3(0.0f, 0.0f, -6.0f));

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projectionLoc = glGetUniformLocation(shaderID, "projection");
    GLint colorLoc = glGetUniformLocation(shaderID, "objectColor");
    GLint lightingDebugModeLoc = glGetUniformLocation(shaderID, "lightingDebugMode");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (wireframeMode)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        updateThreePointLightsFromObject(objects[selectedObject]);
        glUniform1i(lightingDebugModeLoc, lightingDebugMode);
        uploadLights(shaderID);

        for (int i = 0; i < (int)objects.size(); i++)
        {
            Object3D &obj = objects[i];

            mat4 model = mat4(1.0f);

            model = translate(model, obj.position);

            model = rotate(model, radians(obj.rotation.x), vec3(1.0f, 0.0f, 0.0f));
            model = rotate(model, radians(obj.rotation.y), vec3(0.0f, 1.0f, 0.0f));
            model = rotate(model, radians(obj.rotation.z), vec3(0.0f, 0.0f, 1.0f));

            model = scale(model, obj.scale);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

            if (i == selectedObject)
            {
                glUniform3f(colorLoc, 1.0f, 0.2f, 0.4f);
            }
            else
            {
                glUniform3fv(colorLoc, 1, value_ptr(obj.color));
            }

            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    deleteObjectBuffers();

    glDeleteProgram(shaderID);

    glfwTerminate();

    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
    {
        return;
    }

    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        toggleLight(KEY_LIGHT);
        return;
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        toggleLight(FILL_LIGHT);
        return;
    }

    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        toggleLight(BACK_LIGHT);
        return;
    }

    if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        lightingDebugMode = (lightingDebugMode + 1) % 4;

        if (lightingDebugMode == 0)
        {
            cout << "Modo de visualizacao: iluminacao final" << endl;
        }
        else if (lightingDebugMode == 1)
        {
            cout << "Modo de visualizacao: cor sem iluminacao" << endl;
        }
        else if (lightingDebugMode == 2)
        {
            cout << "Modo de visualizacao: normais" << endl;
        }
        else
        {
            cout << "Modo de visualizacao: apenas contribuicao das luzes" << endl;
        }

        return;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        selectedObject++;

        if (selectedObject >= (int)objects.size())
        {
            selectedObject = 0;
        }

        printSelectedObject();
        return;
    }

    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        currentMode = TransformMode::TRANSLATE;
        cout << "Modo atual: TRANSLACAO" << endl;
        return;
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        currentMode = TransformMode::ROTATE;
        cout << "Modo atual: ROTACAO" << endl;
        return;
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        currentMode = TransformMode::SCALE;
        cout << "Modo atual: ESCALA" << endl;
        return;
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        wireframeMode = !wireframeMode;

        if (wireframeMode)
        {
            cout << "Visualizacao: WIREFRAME" << endl;
        }
        else
        {
            cout << "Visualizacao: SOLIDA" << endl;
        }

        return;
    }

    applyTransformation(key, mods);
}

void applyTransformation(int key, int mods)
{
    if (objects.empty())
    {
        return;
    }

    Object3D &obj = objects[selectedObject];

    float direction = 1.0f;

    if (mods & GLFW_MOD_SHIFT)
    {
        direction = -1.0f;
    }

    const float translationStep = 0.1f;
    const float rotationStep = 5.0f;
    const float scaleStep = 0.1f;
    const float minScale = 0.05f;

    if (currentMode == TransformMode::TRANSLATE)
    {
        if (key == GLFW_KEY_LEFT)
        {
            obj.position.x -= translationStep;
        }
        else if (key == GLFW_KEY_RIGHT)
        {
            obj.position.x += translationStep;
        }
        else if (key == GLFW_KEY_UP)
        {
            obj.position.y += translationStep;
        }
        else if (key == GLFW_KEY_DOWN)
        {
            obj.position.y -= translationStep;
        }
        else if (key == GLFW_KEY_PAGE_UP)
        {
            obj.position.z += translationStep;
        }
        else if (key == GLFW_KEY_PAGE_DOWN)
        {
            obj.position.z -= translationStep;
        }
        else if (key == GLFW_KEY_X)
        {
            obj.position.x += direction * translationStep;
        }
        else if (key == GLFW_KEY_Y)
        {
            obj.position.y += direction * translationStep;
        }
        else if (key == GLFW_KEY_Z)
        {
            obj.position.z += direction * translationStep;
        }
    }
    else if (currentMode == TransformMode::ROTATE)
    {
        if (key == GLFW_KEY_X)
        {
            obj.rotation.x += direction * rotationStep;
        }
        else if (key == GLFW_KEY_Y)
        {
            obj.rotation.y += direction * rotationStep;
        }
        else if (key == GLFW_KEY_Z)
        {
            obj.rotation.z += direction * rotationStep;
        }
    }
    else if (currentMode == TransformMode::SCALE)
    {
        if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
        {
            obj.scale += vec3(scaleStep);
        }
        else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
        {
            obj.scale -= vec3(scaleStep);
        }
        else if (key == GLFW_KEY_X)
        {
            obj.scale.x += direction * scaleStep;
        }
        else if (key == GLFW_KEY_Y)
        {
            obj.scale.y += direction * scaleStep;
        }
        else if (key == GLFW_KEY_Z)
        {
            obj.scale.z += direction * scaleStep;
        }

        obj.scale.x = std::max(obj.scale.x, minScale);
        obj.scale.y = std::max(obj.scale.y, minScale);
        obj.scale.z = std::max(obj.scale.z, minScale);
    }
}

void initializeThreePointLights()
{
    lights[KEY_LIGHT].color = vec3(1.0f, 0.96f, 0.88f);
    lights[KEY_LIGHT].intensity = vec3(2.40f);
    lights[KEY_LIGHT].enabled = true;

    lights[FILL_LIGHT].color = vec3(0.85f, 0.90f, 1.0f);
    lights[FILL_LIGHT].intensity = vec3(0.90f);
    lights[FILL_LIGHT].enabled = true;

    lights[BACK_LIGHT].color = vec3(1.0f, 1.0f, 1.0f);
    lights[BACK_LIGHT].intensity = vec3(1.50f);
    lights[BACK_LIGHT].enabled = true;

    for (int i = 0; i < NUMBER_OF_LIGHTS; i++)
    {
        lights[i].constant = 1.0f;
        lights[i].linear = 0.025f;
        lights[i].quadratic = 0.002f;
    }
}

void updateThreePointLightsFromObject(const Object3D &mainObject)
{
    vec3 center = mainObject.position;

    float objectScale = std::max(mainObject.scale.x, std::max(mainObject.scale.y, mainObject.scale.z));
    float distance = std::max(2.5f, objectScale * 4.0f);
    float height = std::max(1.4f, objectScale * 2.5f);

    lights[KEY_LIGHT].position = center + vec3(-distance, height, distance);
    lights[FILL_LIGHT].position = center + vec3(distance, height * 0.70f, distance);
    lights[BACK_LIGHT].position = center + vec3(0.0f, height, -distance);
}

void uploadLights(GLuint shaderID)
{
    glUseProgram(shaderID);

    for (int i = 0; i < NUMBER_OF_LIGHTS; i++)
    {
        string index = "[" + to_string(i) + "]";

        glUniform3fv(
            glGetUniformLocation(shaderID, ("lightPositions" + index).c_str()),
            1,
            value_ptr(lights[i].position));

        glUniform3fv(
            glGetUniformLocation(shaderID, ("lightColors" + index).c_str()),
            1,
            value_ptr(lights[i].color));

        glUniform3fv(
            glGetUniformLocation(shaderID, ("lightIntensities" + index).c_str()),
            1,
            value_ptr(lights[i].intensity));

        glUniform1f(
            glGetUniformLocation(shaderID, ("lightConstants" + index).c_str()),
            lights[i].constant);

        glUniform1f(
            glGetUniformLocation(shaderID, ("lightLinears" + index).c_str()),
            lights[i].linear);

        glUniform1f(
            glGetUniformLocation(shaderID, ("lightQuadratics" + index).c_str()),
            lights[i].quadratic);

        glUniform1i(
            glGetUniformLocation(shaderID, ("lightEnabled" + index).c_str()),
            lights[i].enabled ? 1 : 0);
    }
}

void toggleLight(int lightIndex)
{
    if (lightIndex < 0 || lightIndex >= NUMBER_OF_LIGHTS)
    {
        return;
    }

    lights[lightIndex].enabled = !lights[lightIndex].enabled;
    printLightStatus(lightIndex);
}

string lightName(int lightIndex)
{
    if (lightIndex == KEY_LIGHT)
    {
        return "Luz principal / Key light";
    }

    if (lightIndex == FILL_LIGHT)
    {
        return "Luz de preenchimento / Fill light";
    }

    if (lightIndex == BACK_LIGHT)
    {
        return "Luz de fundo / Back light";
    }

    return "Luz desconhecida";
}

void printLightStatus(int lightIndex)
{
    cout << lightName(lightIndex)
         << ": "
         << (lights[lightIndex].enabled ? "ligada" : "desligada")
         << endl;
}

Object3D loadSimpleOBJ(
    const string &filePath,
    const string &objectName,
    const vec3 &objectColor)
{
    Object3D obj;
    obj.name = objectName;
    obj.color = objectColor;

    ifstream file(filePath);

    if (!file.is_open())
    {
        cerr << "Erro ao abrir OBJ: " << filePath << endl;
        return obj;
    }

    vector<vec3> positions;
    vector<Vertex> finalVertices;

    string line;

    while (getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        istringstream iss(line);

        string prefix;
        iss >> prefix;

        if (prefix.empty() || prefix[0] == '#')
        {
            continue;
        }

        if (prefix == "v")
        {
            vec3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        else if (prefix == "f")
        {
            vector<int> faceIndices;
            string token;

            while (iss >> token)
            {
                int index = parseOBJVertexIndex(token, (int)positions.size());

                if (index >= 0 && index < (int)positions.size())
                {
                    faceIndices.push_back(index);
                }
            }

            if (faceIndices.size() >= 3)
            {
                for (int i = 1; i < (int)faceIndices.size() - 1; i++)
                {
                    vec3 p0 = positions[faceIndices[0]];
                    vec3 p1 = positions[faceIndices[i]];
                    vec3 p2 = positions[faceIndices[i + 1]];

                    vec3 faceNormal = cross(p1 - p0, p2 - p0);

                    if (length(faceNormal) > 0.00001f)
                    {
                        faceNormal = normalize(faceNormal);
                    }
                    else
                    {
                        faceNormal = vec3(0.0f, 1.0f, 0.0f);
                    }

                    finalVertices.push_back({p0, faceNormal});
                    finalVertices.push_back({p1, faceNormal});
                    finalVertices.push_back({p2, faceNormal});
                }
            }
        }
    }

    file.close();

    if (finalVertices.empty())
    {
        cerr << "OBJ sem vertices finais: " << filePath << endl;
        return obj;
    }

    glGenVertexArrays(1, &obj.VAO);
    glGenBuffers(1, &obj.VBO);

    glBindVertexArray(obj.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        finalVertices.size() * sizeof(Vertex),
        finalVertices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (GLvoid *)0);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (GLvoid *)offsetof(Vertex, normal));

    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    obj.nVertices = (GLsizei)finalVertices.size();

    cout << "OBJ carregado: " << objectName
         << " | arquivo: " << filePath
         << " | vertices desenhados: " << obj.nVertices
         << endl;

    return obj;
}

int parseOBJVertexIndex(const string &token, int vertexCount)
{
    string indexString;

    size_t slashPosition = token.find('/');

    if (slashPosition == string::npos)
    {
        indexString = token;
    }
    else
    {
        indexString = token.substr(0, slashPosition);
    }

    if (indexString.empty())
    {
        return -1;
    }

    int index = stoi(indexString);

    if (index > 0)
    {
        return index - 1;
    }

    if (index < 0)
    {
        return vertexCount + index;
    }

    return -1;
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

bool fileExists(const string &path)
{
    ifstream file(path);
    return file.good();
}

string resolvePath(const string &path)
{
    vector<string> candidates =
        {
            path,
            "../" + path,
            "../../" + path,
            "../../../" + path};

    for (const string &candidate : candidates)
    {
        if (fileExists(candidate))
        {
            return candidate;
        }
    }

    return path;
}

void printControls()
{
    cout << endl;
    cout << "================ CONTROLES ================" << endl;
    cout << "ESC              fecha a janela" << endl;
    cout << "TAB              seleciona o proximo objeto principal" << endl;
    cout << "T                modo translacao" << endl;
    cout << "R                modo rotacao" << endl;
    cout << "S                modo escala" << endl;
    cout << "M                alterna solido/wireframe" << endl;
    cout << "1                liga/desliga luz principal" << endl;
    cout << "2                liga/desliga luz de preenchimento" << endl;
    cout << "3                liga/desliga luz de fundo" << endl;
    cout << "D                alterna modo diagnostico da iluminacao" << endl;
    cout << endl;
    cout << "MODO TRANSLACAO:" << endl;
    cout << "Setas            move em X/Y" << endl;
    cout << "PageUp/PageDown  move em Z" << endl;
    cout << "X/Y/Z            move no eixo escolhido" << endl;
    cout << "Shift + X/Y/Z    move no sentido negativo do eixo" << endl;
    cout << endl;
    cout << "MODO ROTACAO:" << endl;
    cout << "X/Y/Z            rotaciona no eixo escolhido" << endl;
    cout << "Shift + X/Y/Z    rotaciona no sentido contrario" << endl;
    cout << endl;
    cout << "MODO ESCALA:" << endl;
    cout << "+ ou keypad +    aumenta escala uniforme" << endl;
    cout << "- ou keypad -    diminui escala uniforme" << endl;
    cout << "X/Y/Z            aumenta escala no eixo escolhido" << endl;
    cout << "Shift + X/Y/Z    diminui escala no eixo escolhido" << endl;
    cout << "===========================================" << endl;
    cout << endl;
}

void printSelectedObject()
{
    if (objects.empty())
    {
        return;
    }

    cout << "Objeto principal selecionado: "
         << selectedObject
         << " - "
         << objects[selectedObject].name
         << endl;
}

void deleteObjectBuffers()
{
    for (Object3D &obj : objects)
    {
        if (obj.VAO != 0)
        {
            glDeleteVertexArrays(1, &obj.VAO);
            obj.VAO = 0;
        }

        if (obj.VBO != 0)
        {
            glDeleteBuffers(1, &obj.VBO);
            obj.VBO = 0;
        }
    }
}