#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

const GLuint WIDTH = 1000;
const GLuint HEIGHT = 1000;

enum class TransformMode
{
    TRANSLATE,
    ROTATE,
    SCALE
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

    vector<vec3> trajectoryPoints;
    int currentTrajectoryPoint = 0;
    float trajectorySpeed = 1.0f;
    bool followTrajectory = false;
};

vector<Object3D> objects;

int selectedObject = 0;
TransformMode currentMode = TransformMode::TRANSLATE;

bool wireframeMode = false;

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
void applyTransformation(int key, int mods);
void deleteObjectBuffers();

void addTrajectoryPoint();
void clearTrajectory();
void toggleTrajectory();
void increaseTrajectorySpeed();
void decreaseTrajectorySpeed();
void updateTrajectory(Object3D &obj, float deltaTime);
void printTrajectoryInfo(const Object3D &obj);

const GLchar *vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

const GLchar *fragmentShaderSource = R"(
#version 330 core

uniform vec3 objectColor;

out vec4 color;

void main()
{
    color = vec4(objectColor, 1.0);
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
        "Trajetorias de Objetos - Computacao Grafica",
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

    printControls();
    printSelectedObject();

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

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));

    glEnable(GL_DEPTH_TEST);

    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        for (Object3D &obj : objects)
        {
            updateTrajectory(obj, deltaTime);
        }

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

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        addTrajectoryPoint();
        return;
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        toggleTrajectory();
        return;
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        clearTrajectory();
        return;
    }

    if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS)
    {
        increaseTrajectorySpeed();
        return;
    }

    if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS)
    {
        decreaseTrajectorySpeed();
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

void addTrajectoryPoint()
{
    if (objects.empty())
    {
        return;
    }

    Object3D &obj = objects[selectedObject];

    obj.trajectoryPoints.push_back(obj.position);

    cout << "Ponto de trajetoria adicionado ao objeto "
         << obj.name
         << ": ("
         << obj.position.x << ", "
         << obj.position.y << ", "
         << obj.position.z << ")"
         << endl;

    printTrajectoryInfo(obj);
}

void clearTrajectory()
{
    if (objects.empty())
    {
        return;
    }

    Object3D &obj = objects[selectedObject];

    obj.trajectoryPoints.clear();
    obj.currentTrajectoryPoint = 0;
    obj.followTrajectory = false;

    cout << "Trajetoria limpa para o objeto " << obj.name << endl;
}

void toggleTrajectory()
{
    if (objects.empty())
    {
        return;
    }

    Object3D &obj = objects[selectedObject];

    if (obj.trajectoryPoints.empty())
    {
        cout << "O objeto " << obj.name
             << " ainda nao possui pontos de trajetoria." << endl;
        return;
    }

    obj.followTrajectory = !obj.followTrajectory;

    if (obj.followTrajectory)
    {
        obj.currentTrajectoryPoint = 0;
        cout << "Movimento por trajetoria ATIVADO para "
             << obj.name << endl;
    }
    else
    {
        cout << "Movimento por trajetoria DESATIVADO para "
             << obj.name << endl;
    }
}

void increaseTrajectorySpeed()
{
    if (objects.empty())
    {
        return;
    }

    Object3D &obj = objects[selectedObject];
    obj.trajectorySpeed += 0.2f;

    cout << "Velocidade da trajetoria de "
         << obj.name
         << ": "
         << obj.trajectorySpeed
         << endl;
}

void decreaseTrajectorySpeed()
{
    if (objects.empty())
    {
        return;
    }

    Object3D &obj = objects[selectedObject];
    obj.trajectorySpeed -= 0.2f;
    obj.trajectorySpeed = std::max(obj.trajectorySpeed, 0.1f);

    cout << "Velocidade da trajetoria de "
         << obj.name
         << ": "
         << obj.trajectorySpeed
         << endl;
}

void updateTrajectory(Object3D &obj, float deltaTime)
{
    if (!obj.followTrajectory || obj.trajectoryPoints.empty())
    {
        return;
    }

    if (obj.trajectoryPoints.size() == 1)
    {
        obj.position = obj.trajectoryPoints[0];
        return;
    }

    vec3 target = obj.trajectoryPoints[obj.currentTrajectoryPoint];
    vec3 direction = target - obj.position;

    float distance = length(direction);
    const float epsilon = 0.02f;

    if (distance < epsilon)
    {
        obj.position = target;

        obj.currentTrajectoryPoint++;

        if (obj.currentTrajectoryPoint >= (int)obj.trajectoryPoints.size())
        {
            obj.currentTrajectoryPoint = 0;
        }

        return;
    }

    vec3 normalizedDirection = normalize(direction);
    float step = obj.trajectorySpeed * deltaTime;

    if (step > distance)
    {
        step = distance;
    }

    obj.position += normalizedDirection * step;
}

void printTrajectoryInfo(const Object3D &obj)
{
    cout << "Objeto " << obj.name
         << " possui "
         << obj.trajectoryPoints.size()
         << " ponto(s) de trajetoria."
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
    vector<vec3> finalVertices;

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
                    finalVertices.push_back(positions[faceIndices[0]]);
                    finalVertices.push_back(positions[faceIndices[i]]);
                    finalVertices.push_back(positions[faceIndices[i + 1]]);
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
        finalVertices.size() * sizeof(vec3),
        finalVertices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(vec3),
        (GLvoid *)0);

    glEnableVertexAttribArray(0);

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
    cout << "TAB              seleciona o proximo objeto" << endl;
    cout << "T                modo translacao" << endl;
    cout << "R                modo rotacao" << endl;
    cout << "S                modo escala" << endl;
    cout << "M                alterna solido/wireframe" << endl;
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
    cout << endl;
    cout << "TRAJETORIA:" << endl;
    cout << "P                salva a posicao atual como ponto da trajetoria" << endl;
    cout << "O                liga/desliga movimento ciclico pela trajetoria" << endl;
    cout << "C                limpa a trajetoria do objeto selecionado" << endl;
    cout << "[                diminui a velocidade da trajetoria" << endl;
    cout << "]                aumenta a velocidade da trajetoria" << endl;
    cout << "===========================================" << endl;
    cout << endl;
}

void printSelectedObject()
{
    if (objects.empty())
    {
        return;
    }

    cout << "Objeto selecionado: "
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
