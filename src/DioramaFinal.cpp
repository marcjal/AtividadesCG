#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

const GLuint WIDTH = 1000;
const GLuint HEIGHT = 1000;
const int MAX_LIGHTS = 8;

struct Vertex
{
    vec3 position;
    vec2 texCoord;
    vec3 normal;
};

struct Material
{
    string name = "Default";

    vec3 Ka = vec3(0.12f);
    vec3 Kd = vec3(0.80f);
    vec3 Ks = vec3(0.35f);

    float Ns = 32.0f;

    string mapKd;
    GLuint textureID = 0;
    bool hasTexture = false;
};

struct DrawBatch
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLsizei nVertices = 0;
    int materialIndex = 0;
};

struct Object3D
{
    string name;
    string objPath;

    vector<DrawBatch> batches;
    vector<Material> materials;

    vec3 position = vec3(0.0f);
    vec3 rotation = vec3(0.0f);
    vec3 scale = vec3(1.0f);

    bool animated = false;
    bool animationPaused = true;
    float bezierT = 0.0f;
    float bezierSpeed = 0.25f;

    vec3 bezierPoints[4] = {
        vec3(-1.5f, 0.0f, 0.0f),
        vec3(-0.5f, 1.2f, 1.0f),
        vec3(0.5f, 1.2f, -1.0f),
        vec3(1.5f, 0.0f, 0.0f)};
};

struct PointLight
{
    string name = "Light";

    vec3 position = vec3(0.0f);
    vec3 color = vec3(1.0f);

    float intensity = 1.0f;

    float constant = 1.0f;
    float linear = 0.025f;
    float quadratic = 0.002f;

    bool enabled = true;
};

class Camera
{
public:
    vec3 Position;
    vec3 Front;
    vec3 Up;
    vec3 Right;
    vec3 WorldUp;

    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Fov;
    float NearPlane;
    float FarPlane;

    Camera(
        vec3 position = vec3(0.0f, 1.5f, 7.0f),
        vec3 up = vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f,
        float pitch = -8.0f)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        MovementSpeed = 4.0f;
        MouseSensitivity = 0.08f;
        Fov = 45.0f;
        NearPlane = 0.1f;
        FarPlane = 100.0f;

        updateCameraVectors();
    }

    mat4 getViewMatrix() const
    {
        return lookAt(Position, Position + Front, Up);
    }

    mat4 getProjectionMatrix(float aspectRatio) const
    {
        return perspective(radians(Fov), aspectRatio, NearPlane, FarPlane);
    }

    void move(GLFWwindow *window, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;

        vec3 forward = normalize(vec3(Front.x, 0.0f, Front.z));
        vec3 right = normalize(vec3(Right.x, 0.0f, Right.z));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            Position += forward * velocity;

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            Position -= forward * velocity;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            Position -= right * velocity;

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            Position += right * velocity;

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            Position.y -= velocity;

        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            Position.y += velocity;
    }

    void rotate(float xoffset, float yoffset)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (Pitch > 89.0f)
            Pitch = 89.0f;

        if (Pitch < -89.0f)
            Pitch = -89.0f;

        updateCameraVectors();
    }

    void zoom(float yoffset)
    {
        Fov -= yoffset;

        if (Fov < 1.0f)
            Fov = 1.0f;

        if (Fov > 60.0f)
            Fov = 60.0f;
    }

    void forceUpdate()
    {
        updateCameraVectors();
    }

private:
    void updateCameraVectors()
    {
        vec3 front;

        front.x = cos(radians(Yaw)) * cos(radians(Pitch));
        front.y = sin(radians(Pitch));
        front.z = sin(radians(Yaw)) * cos(radians(Pitch));

        Front = normalize(front);
        Right = normalize(cross(Front, WorldUp));
        Up = normalize(cross(Right, Front));
    }
};

enum class TransformMode
{
    TRANSLATE,
    ROTATE,
    SCALE
};

struct FaceIndex
{
    int v = -1;
    int t = -1;
    int n = -1;
};

vector<Object3D> objects;
vector<PointLight> lights;

Camera camera;
TransformMode currentMode = TransformMode::TRANSLATE;

int selectedObject = 0;
int selectedLight = 0;

bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool wireframeMode = false;
bool showTexture = true;
int materialDebugMode = 0;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

int setupShader();

bool loadSceneFile(const string &scenePath);
bool loadDefaultScene();

Object3D loadOBJ(const string &filePath, const string &objectName);
vector<Material> loadMTL(const string &mtlPath);
void createBatchBuffers(Object3D &object, const map<int, vector<Vertex>> &verticesByMaterial);

GLuint loadTexture(const string &filePath, int &width, int &height);

void setupDefaultLights();
void uploadLights(GLuint shaderID);

void applyTransformation(int key, int mods);
void updateBezierAnimation(Object3D &obj, float dt);
vec3 bezierCubic(float t, const vec3 &p0, const vec3 &p1, const vec3 &p2, const vec3 &p3);

void printControls();
void printSelectedObject();
void printLightStatus();
void deleteResources();

string getDirectoryFromPath(const string &filePath);
string resolveRelativePath(const string &baseDir, const string &relativePath);
bool fileExists(const string &path);
string resolvePath(const string &path);
vector<string> tokenize(const string &line);

float toFloat(const string &s, float fallback = 0.0f);
int toInt(const string &s, int fallback = 0);

FaceIndex parseFaceToken(const string &token, int vCount, int tCount, int nCount);
int fixObjIndex(int idx, int size);

const GLchar *vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texc;
layout (location = 2) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec2 TexCoord;
out vec3 Normal;

void main()
{
    vec4 worldPosition = model * vec4(position, 1.0);

    FragPos = vec3(worldPosition);
    TexCoord = texc;
    Normal = mat3(transpose(inverse(model))) * normal;

    gl_Position = projection * view * worldPosition;
}
)";

const GLchar *fragmentShaderSource = R"(
#version 330 core

#define MAX_LIGHTS 8

in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;

out vec4 color;

uniform int numLights;

uniform vec3 lightPositions[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];
uniform float lightIntensities[MAX_LIGHTS];
uniform float lightConstants[MAX_LIGHTS];
uniform float lightLinears[MAX_LIGHTS];
uniform float lightQuadratics[MAX_LIGHTS];
uniform int lightEnabled[MAX_LIGHTS];

uniform vec3 viewPos;

uniform vec3 materialKa;
uniform vec3 materialKd;
uniform vec3 materialKs;
uniform float materialNs;

uniform sampler2D texBuff;

uniform int useTexture;
uniform int showTexture;
uniform int materialDebugMode;
uniform int isSelected;

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    if (materialDebugMode == 1)
    {
        color = vec4(materialKa, 1.0);
        return;
    }

    if (materialDebugMode == 2)
    {
        color = vec4(materialKd, 1.0);
        return;
    }

    if (materialDebugMode == 3)
    {
        color = vec4(materialKs, 1.0);
        return;
    }

    vec3 baseColor = materialKd;

    if (useTexture == 1 && showTexture == 1)
    {
        vec4 texColor = texture(texBuff, TexCoord);
        baseColor *= texColor.rgb;
    }

    vec3 result = vec3(0.0);

    for (int i = 0; i < numLights; i++)
    {
        if (lightEnabled[i] == 0)
        {
            continue;
        }

        vec3 Lvector = lightPositions[i] - FragPos;
        float distanceToLight = length(Lvector);
        vec3 L = normalize(Lvector);

        vec3 R = reflect(-L, N);

        float diff = max(dot(N, L), 0.0);
        float spec = pow(max(dot(V, R), 0.0), materialNs);

        float attenuation = 1.0 / (
            lightConstants[i] +
            lightLinears[i] * distanceToLight +
            lightQuadratics[i] * distanceToLight * distanceToLight
        );

        vec3 ambient = materialKa * baseColor * lightColors[i] * 0.35;
        vec3 diffuse = materialKd * baseColor * diff * lightColors[i];
        vec3 specular = materialKs * spec * lightColors[i];

        result += (ambient + diffuse + specular) * lightIntensities[i] * attenuation;
    }

    result += materialKa * baseColor * 0.20;

    if (isSelected == 1)
    {
        result = mix(result, vec3(1.0, 0.15, 0.35), 0.25);
    }

    color = vec4(clamp(result, vec3(0.0), vec3(1.0)), 1.0);
}
)";

int main()
{
    if (!glfwInit())
    {
        cout << "Erro ao inicializar GLFW." << endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "Diorama Final - Visualizador 3D",
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
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Erro ao inicializar GLAD." << endl;
        glfwTerminate();
        return -1;
    }

    cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL: " << glGetString(GL_VERSION) << endl;

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    glEnable(GL_DEPTH_TEST);

    if (!loadSceneFile(resolvePath("assets/scene_final.txt")))
    {
        cout << "Usando cena padrao." << endl;
        loadDefaultScene();
    }

    if (lights.empty())
    {
        setupDefaultLights();
    }

    if (objects.empty())
    {
        cout << "Nenhum objeto carregado." << endl;
        glfwTerminate();
        return -1;
    }

    printControls();
    printSelectedObject();
    printLightStatus();

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projectionLoc = glGetUniformLocation(shaderID, "projection");
    GLint viewPosLoc = glGetUniformLocation(shaderID, "viewPos");

    GLint materialKaLoc = glGetUniformLocation(shaderID, "materialKa");
    GLint materialKdLoc = glGetUniformLocation(shaderID, "materialKd");
    GLint materialKsLoc = glGetUniformLocation(shaderID, "materialKs");
    GLint materialNsLoc = glGetUniformLocation(shaderID, "materialNs");

    GLint useTextureLoc = glGetUniformLocation(shaderID, "useTexture");
    GLint showTextureLoc = glGetUniformLocation(shaderID, "showTexture");
    GLint materialDebugModeLoc = glGetUniformLocation(shaderID, "materialDebugMode");
    GLint isSelectedLoc = glGetUniformLocation(shaderID, "isSelected");

    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

    lastFrame = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        camera.move(window, deltaTime);

        for (Object3D &obj : objects)
        {
            updateBezierAnimation(obj, deltaTime);
        }

        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        if (framebufferHeight == 0)
            framebufferHeight = 1;

        glViewport(0, 0, framebufferWidth, framebufferHeight);

        if (wireframeMode)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 view = camera.getViewMatrix();
        mat4 projection = camera.getProjectionMatrix(
            (float)framebufferWidth / (float)framebufferHeight);

        glUseProgram(shaderID);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));
        glUniform3fv(viewPosLoc, 1, value_ptr(camera.Position));

        glUniform1i(showTextureLoc, showTexture ? 1 : 0);
        glUniform1i(materialDebugModeLoc, materialDebugMode);

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
            glUniform1i(isSelectedLoc, i == selectedObject ? 1 : 0);

            for (const DrawBatch &batch : obj.batches)
            {
                if (batch.materialIndex < 0 ||
                    batch.materialIndex >= (int)obj.materials.size())
                {
                    continue;
                }

                const Material &mat = obj.materials[batch.materialIndex];

                glUniform3fv(materialKaLoc, 1, value_ptr(mat.Ka));
                glUniform3fv(materialKdLoc, 1, value_ptr(mat.Kd));
                glUniform3fv(materialKsLoc, 1, value_ptr(mat.Ks));
                glUniform1f(materialNsLoc, mat.Ns);

                bool canUseTexture = mat.hasTexture && mat.textureID != 0;

                glUniform1i(useTextureLoc, canUseTexture ? 1 : 0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, canUseTexture ? mat.textureID : 0);

                glBindVertexArray(batch.VAO);
                glDrawArrays(GL_TRIANGLES, 0, batch.nVertices);
                glBindVertexArray(0);
            }
        }

        glfwSwapBuffers(window);
    }

    deleteResources();
    glDeleteProgram(shaderID);

    glfwTerminate();

    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_TAB)
        {
            selectedObject++;

            if (selectedObject >= (int)objects.size())
                selectedObject = 0;

            printSelectedObject();
            return;
        }

        if (key == GLFW_KEY_T)
        {
            currentMode = TransformMode::TRANSLATE;
            cout << "Modo: translacao" << endl;
            return;
        }

        if (key == GLFW_KEY_R)
        {
            currentMode = TransformMode::ROTATE;
            cout << "Modo: rotacao" << endl;
            return;
        }

        if (key == GLFW_KEY_E)
        {
            currentMode = TransformMode::SCALE;
            cout << "Modo: escala uniforme" << endl;
            return;
        }

        if (key == GLFW_KEY_M)
        {
            wireframeMode = !wireframeMode;
            cout << "Wireframe: " << (wireframeMode ? "ligado" : "desligado") << endl;
            return;
        }

        if (key == GLFW_KEY_F)
        {
            showTexture = !showTexture;
            cout << "Textura: " << (showTexture ? "ligada" : "desligada") << endl;
            return;
        }

        if (key == GLFW_KEY_K)
        {
            materialDebugMode = (materialDebugMode + 1) % 4;

            if (materialDebugMode == 0)
                cout << "Material: final" << endl;
            else if (materialDebugMode == 1)
                cout << "Material: Ka" << endl;
            else if (materialDebugMode == 2)
                cout << "Material: Kd" << endl;
            else
                cout << "Material: Ks" << endl;

            return;
        }

        if (key == GLFW_KEY_1 || key == GLFW_KEY_2 || key == GLFW_KEY_3)
        {
            int lightIndex = key - GLFW_KEY_1;

            if (lightIndex >= 0 && lightIndex < (int)lights.size())
            {
                selectedLight = lightIndex;
                lights[lightIndex].enabled = !lights[lightIndex].enabled;
                printLightStatus();
            }

            return;
        }

        if (key == GLFW_KEY_L)
        {
            if (!lights.empty())
            {
                selectedLight = (selectedLight + 1) % (int)lights.size();
                printLightStatus();
            }

            return;
        }

        if (key == GLFW_KEY_RIGHT_BRACKET)
        {
            if (!lights.empty())
            {
                lights[selectedLight].intensity *= 1.10f;
                printLightStatus();
            }

            return;
        }

        if (key == GLFW_KEY_LEFT_BRACKET)
        {
            if (!lights.empty())
            {
                lights[selectedLight].intensity *= 0.90f;
                printLightStatus();
            }

            return;
        }

        if (key == GLFW_KEY_SPACE)
        {
            if (!objects.empty())
            {
                Object3D &obj = objects[selectedObject];

                if (obj.animated)
                {
                    obj.animationPaused = !obj.animationPaused;

                    cout << "Animacao de "
                         << obj.name
                         << ": "
                         << (obj.animationPaused ? "pausada" : "executando")
                         << endl;
                }
                else
                {
                    cout << "Objeto nao possui animacao Bezier." << endl;
                }
            }

            return;
        }

        if (key == GLFW_KEY_B)
        {
            if (!objects.empty())
            {
                Object3D &obj = objects[selectedObject];

                if (obj.animated)
                {
                    obj.bezierT = 0.0f;
                    obj.position = obj.bezierPoints[0];

                    cout << "Animacao reiniciada: " << obj.name << endl;
                }
            }

            return;
        }
    }

    applyTransformation(key, mods);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.rotate(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.zoom((float)yoffset);
}

void applyTransformation(int key, int mods)
{
    if (objects.empty())
        return;

    Object3D &obj = objects[selectedObject];

    float direction = 1.0f;

    if (mods & GLFW_MOD_SHIFT)
        direction = -1.0f;

    const float translationStep = 0.10f;
    const float rotationStep = 5.0f;
    const float scaleStep = 0.10f;
    const float minScale = 0.05f;

    if (currentMode == TransformMode::TRANSLATE)
    {
        if (key == GLFW_KEY_LEFT)
            obj.position.x -= translationStep;
        else if (key == GLFW_KEY_RIGHT)
            obj.position.x += translationStep;
        else if (key == GLFW_KEY_UP)
            obj.position.y += translationStep;
        else if (key == GLFW_KEY_DOWN)
            obj.position.y -= translationStep;
        else if (key == GLFW_KEY_PAGE_UP)
            obj.position.z += translationStep;
        else if (key == GLFW_KEY_PAGE_DOWN)
            obj.position.z -= translationStep;
        else if (key == GLFW_KEY_X)
            obj.position.x += direction * translationStep;
        else if (key == GLFW_KEY_Y)
            obj.position.y += direction * translationStep;
        else if (key == GLFW_KEY_Z)
            obj.position.z += direction * translationStep;
    }
    else if (currentMode == TransformMode::ROTATE)
    {
        if (key == GLFW_KEY_X)
            obj.rotation.x += direction * rotationStep;
        else if (key == GLFW_KEY_Y)
            obj.rotation.y += direction * rotationStep;
        else if (key == GLFW_KEY_Z)
            obj.rotation.z += direction * rotationStep;
    }
    else if (currentMode == TransformMode::SCALE)
    {
        if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD || key == GLFW_KEY_X)
            obj.scale += vec3(scaleStep);
        else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
            obj.scale -= vec3(scaleStep);

        obj.scale.x = std::max(obj.scale.x, minScale);
        obj.scale.y = std::max(obj.scale.y, minScale);
        obj.scale.z = std::max(obj.scale.z, minScale);
    }
}

bool loadSceneFile(const string &scenePath)
{
    ifstream file(scenePath);

    if (!file.is_open())
    {
        cerr << "Erro ao abrir arquivo de cena: " << scenePath << endl;
        return false;
    }

    cout << "Carregando cena: " << scenePath << endl;

    string line;

    while (getline(file, line))
    {
        vector<string> tokens = tokenize(line);

        if (tokens.empty())
            continue;

        string type = tokens[0];

        if (type == "camera" && tokens.size() >= 9)
        {
            camera.Position = vec3(
                toFloat(tokens[1]),
                toFloat(tokens[2]),
                toFloat(tokens[3]));

            camera.Yaw = toFloat(tokens[4], -90.0f);
            camera.Pitch = toFloat(tokens[5], -8.0f);
            camera.Fov = toFloat(tokens[6], 45.0f);
            camera.NearPlane = toFloat(tokens[7], 0.1f);
            camera.FarPlane = toFloat(tokens[8], 100.0f);

            camera.forceUpdate();
        }
        else if (type == "light" && tokens.size() >= 10)
        {
            PointLight light;

            light.name = tokens[1];

            light.position = vec3(
                toFloat(tokens[2]),
                toFloat(tokens[3]),
                toFloat(tokens[4]));

            light.color = vec3(
                toFloat(tokens[5], 1.0f),
                toFloat(tokens[6], 1.0f),
                toFloat(tokens[7], 1.0f));

            light.intensity = toFloat(tokens[8], 1.0f);
            light.enabled = toInt(tokens[9], 1) != 0;

            lights.push_back(light);
        }
        else if (type == "object" && tokens.size() >= 10)
        {
            string objectName = tokens[1];
            string objPath = resolvePath(tokens[2]);

            Object3D obj = loadOBJ(objPath, objectName);

            obj.name = objectName;
            obj.objPath = objPath;

            obj.position = vec3(
                toFloat(tokens[3]),
                toFloat(tokens[4]),
                toFloat(tokens[5]));

            obj.rotation = vec3(
                toFloat(tokens[6]),
                toFloat(tokens[7]),
                toFloat(tokens[8]));

            float uniformScale = toFloat(tokens[9], 1.0f);
            obj.scale = vec3(uniformScale);

            if (!obj.batches.empty())
                objects.push_back(obj);
        }
        else if (type == "bezier" && tokens.size() >= 15)
        {
            string objectName = tokens[1];

            for (Object3D &obj : objects)
            {
                if (obj.name == objectName)
                {
                    obj.animated = true;
                    obj.animationPaused = true;

                    obj.bezierPoints[0] = vec3(
                        toFloat(tokens[2]),
                        toFloat(tokens[3]),
                        toFloat(tokens[4]));

                    obj.bezierPoints[1] = vec3(
                        toFloat(tokens[5]),
                        toFloat(tokens[6]),
                        toFloat(tokens[7]));

                    obj.bezierPoints[2] = vec3(
                        toFloat(tokens[8]),
                        toFloat(tokens[9]),
                        toFloat(tokens[10]));

                    obj.bezierPoints[3] = vec3(
                        toFloat(tokens[11]),
                        toFloat(tokens[12]),
                        toFloat(tokens[13]));

                    obj.bezierSpeed = toFloat(tokens[14], 0.25f);
                    obj.bezierT = 0.0f;
                    obj.position = obj.bezierPoints[0];

                    cout << "Bezier configurada para " << obj.name << endl;
                    break;
                }
            }
        }
    }

    return !objects.empty();
}

bool loadDefaultScene()
{
    setupDefaultLights();

    Object3D cube = loadOBJ(resolvePath("assets/Modelos3D/Cube.obj"), "Cube");
    cube.position = vec3(-2.0f, 0.0f, 0.0f);
    cube.rotation = vec3(0.0f, 25.0f, 0.0f);
    cube.scale = vec3(0.8f);

    if (!cube.batches.empty())
        objects.push_back(cube);

    Object3D suzanne = loadOBJ(resolvePath("assets/Modelos3D/Suzanne.obj"), "Suzanne");
    suzanne.position = vec3(-1.5f, 0.0f, 0.0f);
    suzanne.scale = vec3(0.8f);
    suzanne.animated = true;
    suzanne.animationPaused = true;
    suzanne.bezierSpeed = 0.25f;

    suzanne.bezierPoints[0] = vec3(-1.5f, 0.0f, 0.0f);
    suzanne.bezierPoints[1] = vec3(-0.5f, 1.2f, 1.0f);
    suzanne.bezierPoints[2] = vec3(0.5f, 1.2f, -1.0f);
    suzanne.bezierPoints[3] = vec3(1.5f, 0.0f, 0.0f);

    if (!suzanne.batches.empty())
        objects.push_back(suzanne);

    Object3D suzanneSubdiv = loadOBJ(
        resolvePath("assets/Modelos3D/SuzanneSubdiv1.obj"),
        "SuzanneSubdiv1");

    suzanneSubdiv.position = vec3(2.0f, 0.0f, 0.0f);
    suzanneSubdiv.scale = vec3(0.8f);

    if (!suzanneSubdiv.batches.empty())
        objects.push_back(suzanneSubdiv);

    return !objects.empty();
}

Object3D loadOBJ(const string &filePath, const string &objectName)
{
    Object3D object;

    object.name = objectName;
    object.objPath = filePath;

    Material defaultMaterial;
    defaultMaterial.name = "Default";
    defaultMaterial.Ka = vec3(0.12f);
    defaultMaterial.Kd = vec3(0.80f, 0.72f, 0.55f);
    defaultMaterial.Ks = vec3(0.35f);
    defaultMaterial.Ns = 32.0f;

    object.materials.push_back(defaultMaterial);

    ifstream file(filePath);

    if (!file.is_open())
    {
        cerr << "Erro ao abrir OBJ: " << filePath << endl;
        return object;
    }

    string baseDir = getDirectoryFromPath(filePath);

    vector<vec3> positions;
    vector<vec2> texCoords;
    vector<vec3> normals;

    map<int, vector<Vertex>> verticesByMaterial;

    int currentMaterialIndex = 0;

    string line;

    while (getline(file, line))
    {
        vector<string> tokens = tokenize(line);

        if (tokens.empty())
            continue;

        string prefix = tokens[0];

        if (prefix == "mtllib" && tokens.size() >= 2)
        {
            string mtlPath = resolveRelativePath(baseDir, tokens[1]);
            vector<Material> loadedMaterials = loadMTL(mtlPath);

            for (const Material &m : loadedMaterials)
                object.materials.push_back(m);
        }
        else if (prefix == "usemtl" && tokens.size() >= 2)
        {
            string materialName = tokens[1];
            currentMaterialIndex = 0;

            for (int i = 0; i < (int)object.materials.size(); i++)
            {
                if (object.materials[i].name == materialName)
                {
                    currentMaterialIndex = i;
                    break;
                }
            }
        }
        else if (prefix == "v" && tokens.size() >= 4)
        {
            positions.push_back(vec3(
                toFloat(tokens[1]),
                toFloat(tokens[2]),
                toFloat(tokens[3])));
        }
        else if (prefix == "vt" && tokens.size() >= 3)
        {
            texCoords.push_back(vec2(
                toFloat(tokens[1]),
                toFloat(tokens[2])));
        }
        else if (prefix == "vn" && tokens.size() >= 4)
        {
            normals.push_back(vec3(
                toFloat(tokens[1]),
                toFloat(tokens[2]),
                toFloat(tokens[3])));
        }
        else if (prefix == "f" && tokens.size() >= 4)
        {
            vector<FaceIndex> face;

            for (int i = 1; i < (int)tokens.size(); i++)
            {
                face.push_back(parseFaceToken(
                    tokens[i],
                    (int)positions.size(),
                    (int)texCoords.size(),
                    (int)normals.size()));
            }

            for (int i = 1; i < (int)face.size() - 1; i++)
            {
                FaceIndex idx[3] = {face[0], face[i], face[i + 1]};

                if (idx[0].v < 0 || idx[1].v < 0 || idx[2].v < 0)
                    continue;

                vec3 p0 = positions[idx[0].v];
                vec3 p1 = positions[idx[1].v];
                vec3 p2 = positions[idx[2].v];

                vec3 faceNormal = cross(p1 - p0, p2 - p0);

                if (length(faceNormal) > 0.00001f)
                    faceNormal = normalize(faceNormal);
                else
                    faceNormal = vec3(0.0f, 1.0f, 0.0f);

                for (int k = 0; k < 3; k++)
                {
                    Vertex vertex;

                    vertex.position = positions[idx[k].v];

                    if (idx[k].t >= 0 && idx[k].t < (int)texCoords.size())
                        vertex.texCoord = texCoords[idx[k].t];
                    else
                        vertex.texCoord = vec2(0.0f);

                    if (idx[k].n >= 0 && idx[k].n < (int)normals.size())
                        vertex.normal = normals[idx[k].n];
                    else
                        vertex.normal = faceNormal;

                    verticesByMaterial[currentMaterialIndex].push_back(vertex);
                }
            }
        }
    }

    file.close();

    for (Material &mat : object.materials)
    {
        if (!mat.mapKd.empty())
        {
            int imgWidth = 0;
            int imgHeight = 0;

            string texPath = resolveRelativePath(baseDir, mat.mapKd);

            mat.textureID = loadTexture(texPath, imgWidth, imgHeight);
            mat.hasTexture = mat.textureID != 0;
        }
    }

    createBatchBuffers(object, verticesByMaterial);

    cout << "OBJ carregado: "
         << object.name
         << " | batches: "
         << object.batches.size()
         << " | materiais: "
         << object.materials.size()
         << endl;

    return object;
}

vector<Material> loadMTL(const string &mtlPath)
{
    vector<Material> materials;

    ifstream file(mtlPath);

    if (!file.is_open())
    {
        cerr << "Erro ao abrir MTL: " << mtlPath << endl;
        return materials;
    }

    Material current;
    bool hasCurrent = false;

    string line;

    while (getline(file, line))
    {
        vector<string> tokens = tokenize(line);

        if (tokens.empty())
            continue;

        string word = tokens[0];

        if (word == "newmtl" && tokens.size() >= 2)
        {
            if (hasCurrent)
                materials.push_back(current);

            current = Material();
            current.name = tokens[1];
            hasCurrent = true;
        }
        else if (word == "Ka" && tokens.size() >= 4)
        {
            current.Ka = vec3(
                toFloat(tokens[1]),
                toFloat(tokens[2]),
                toFloat(tokens[3]));
        }
        else if (word == "Kd" && tokens.size() >= 4)
        {
            current.Kd = vec3(
                toFloat(tokens[1]),
                toFloat(tokens[2]),
                toFloat(tokens[3]));
        }
        else if (word == "Ks" && tokens.size() >= 4)
        {
            current.Ks = vec3(
                toFloat(tokens[1]),
                toFloat(tokens[2]),
                toFloat(tokens[3]));
        }
        else if (word == "Ns" && tokens.size() >= 2)
        {
            current.Ns = toFloat(tokens[1], 32.0f);
        }
        else if (word == "map_Kd" && tokens.size() >= 2)
        {
            current.mapKd = tokens[1];
        }
    }

    if (hasCurrent)
        materials.push_back(current);

    return materials;
}

void createBatchBuffers(Object3D &object, const map<int, vector<Vertex>> &verticesByMaterial)
{
    for (const auto &entry : verticesByMaterial)
    {
        int materialIndex = entry.first;
        const vector<Vertex> &vertices = entry.second;

        if (vertices.empty())
            continue;

        DrawBatch batch;

        batch.materialIndex = materialIndex;
        batch.nVertices = (GLsizei)vertices.size();

        glGenVertexArrays(1, &batch.VAO);
        glGenBuffers(1, &batch.VBO);

        glBindVertexArray(batch.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, batch.VBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            vertices.size() * sizeof(Vertex),
            vertices.data(),
            GL_STATIC_DRAW);

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *)offsetof(Vertex, position));

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *)offsetof(Vertex, texCoord));

        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            2,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *)offsetof(Vertex, normal));

        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        object.batches.push_back(batch);
    }
}

GLuint loadTexture(const string &filePath, int &width, int &height)
{
    if (!fileExists(filePath))
    {
        cerr << "Textura nao encontrada: " << filePath << endl;
        return 0;
    }

    GLuint texID;

    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrChannels = 0;

    stbi_set_flip_vertically_on_load(true);

    unsigned char *data = stbi_load(
        filePath.c_str(),
        &width,
        &height,
        &nrChannels,
        0);

    if (!data)
    {
        cerr << "Falha ao carregar textura: " << filePath << endl;

        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &texID);

        return 0;
    }

    GLenum format = GL_RGB;

    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data);

    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);

    cout << "Textura carregada: " << filePath << endl;

    return texID;
}

void setupDefaultLights()
{
    lights.clear();

    PointLight key;
    key.name = "Key";
    key.position = vec3(-4.0f, 4.0f, 4.0f);
    key.color = vec3(1.0f, 0.95f, 0.85f);
    key.intensity = 2.4f;
    lights.push_back(key);

    PointLight fill;
    fill.name = "Fill";
    fill.position = vec3(4.0f, 3.0f, 4.0f);
    fill.color = vec3(0.75f, 0.85f, 1.0f);
    fill.intensity = 0.9f;
    lights.push_back(fill);

    PointLight back;
    back.name = "Back";
    back.position = vec3(0.0f, 4.0f, -4.0f);
    back.color = vec3(1.0f);
    back.intensity = 1.5f;
    lights.push_back(back);
}

void uploadLights(GLuint shaderID)
{
    int count = std::min((int)lights.size(), MAX_LIGHTS);

    glUniform1i(glGetUniformLocation(shaderID, "numLights"), count);

    for (int i = 0; i < count; i++)
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

        glUniform1f(
            glGetUniformLocation(shaderID, ("lightIntensities" + index).c_str()),
            lights[i].intensity);

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

void updateBezierAnimation(Object3D &obj, float dt)
{
    if (!obj.animated || obj.animationPaused)
        return;

    obj.bezierT += dt * obj.bezierSpeed;

    if (obj.bezierT > 1.0f)
        obj.bezierT = 0.0f;

    obj.position = bezierCubic(
        obj.bezierT,
        obj.bezierPoints[0],
        obj.bezierPoints[1],
        obj.bezierPoints[2],
        obj.bezierPoints[3]);
}

vec3 bezierCubic(float t, const vec3 &p0, const vec3 &p1, const vec3 &p2, const vec3 &p3)
{
    float u = 1.0f - t;

    return u * u * u * p0 +
           3.0f * u * u * t * p1 +
           3.0f * u * t * t * p2 +
           t * t * t * p3;
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
             << infoLog
             << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);

        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
             << infoLog
             << endl;
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
             << infoLog
             << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void printControls()
{
    cout << endl;
    cout << "================ CONTROLES - DIORAMA FINAL ================" << endl;
    cout << "ESC              fecha a janela" << endl;
    cout << "TAB              seleciona o proximo objeto" << endl;
    cout << "T                modo translacao" << endl;
    cout << "R                modo rotacao" << endl;
    cout << "E                modo escala uniforme" << endl;
    cout << "Setas            translacao em X/Y" << endl;
    cout << "PageUp/PageDown  translacao em Z" << endl;
    cout << "X/Y/Z            transforma no eixo do modo atual" << endl;
    cout << "Shift + X/Y/Z    transforma no sentido negativo" << endl;
    cout << "+ / -            aumenta/diminui escala uniforme" << endl;
    cout << "M                alterna solido/wireframe" << endl;
    cout << "F                liga/desliga textura" << endl;
    cout << "K                debug material: final / Ka / Kd / Ks" << endl;
    cout << "1,2,3            liga/desliga Key, Fill e Back light" << endl;
    cout << "L                seleciona proxima luz" << endl;
    cout << "[ / ]            diminui/aumenta intensidade da luz selecionada" << endl;
    cout << "SPACE            inicia/pausa animacao Bezier" << endl;
    cout << "B                reinicia animacao Bezier" << endl;
    cout << endl;
    cout << "CAMERA FPS:" << endl;
    cout << "W/A/S/D          navega no plano XZ" << endl;
    cout << "Q/C              desce/sobe a camera" << endl;
    cout << "Mouse            rotaciona a camera" << endl;
    cout << "Scroll           zoom/FOV" << endl;
    cout << "============================================================" << endl;
    cout << endl;
}

void printSelectedObject()
{
    if (objects.empty())
        return;

    cout << "Objeto selecionado: "
         << selectedObject
         << " - "
         << objects[selectedObject].name
         << endl;
}

void printLightStatus()
{
    if (lights.empty())
        return;

    cout << "Luzes:" << endl;

    for (int i = 0; i < (int)lights.size(); i++)
    {
        cout << "  "
             << (i == selectedLight ? "> " : "  ")
             << i
             << " - "
             << lights[i].name
             << " | "
             << (lights[i].enabled ? "ligada" : "desligada")
             << " | intensidade="
             << lights[i].intensity
             << endl;
    }
}

void deleteResources()
{
    for (Object3D &obj : objects)
    {
        for (DrawBatch &batch : obj.batches)
        {
            if (batch.VAO != 0)
                glDeleteVertexArrays(1, &batch.VAO);

            if (batch.VBO != 0)
                glDeleteBuffers(1, &batch.VBO);
        }

        for (Material &mat : obj.materials)
        {
            if (mat.textureID != 0)
                glDeleteTextures(1, &mat.textureID);
        }
    }
}

string getDirectoryFromPath(const string &filePath)
{
    size_t pos = filePath.find_last_of("/\\");

    if (pos == string::npos)
        return "";

    return filePath.substr(0, pos + 1);
}

string resolveRelativePath(const string &baseDir, const string &relativePath)
{
    if (relativePath.empty())
        return relativePath;

    if (relativePath.size() > 1 && relativePath[1] == ':')
        return relativePath;

    if (relativePath[0] == '/' || relativePath[0] == '\\')
        return relativePath;

    return baseDir + relativePath;
}

bool fileExists(const string &path)
{
    ifstream file(path);
    return file.good();
}

string resolvePath(const string &path)
{
    vector<string> candidates = {
        path,
        "../" + path,
        "../../" + path,
        "../../../" + path};

    for (const string &candidate : candidates)
    {
        if (fileExists(candidate))
            return candidate;
    }

    return path;
}

vector<string> tokenize(const string &line)
{
    vector<string> tokens;

    string cleanLine = line;

    size_t commentPosition = cleanLine.find('#');

    if (commentPosition != string::npos)
        cleanLine = cleanLine.substr(0, commentPosition);

    istringstream iss(cleanLine);

    string token;

    while (iss >> token)
        tokens.push_back(token);

    return tokens;
}

float toFloat(const string &s, float fallback)
{
    try
    {
        return stof(s);
    }
    catch (...)
    {
        return fallback;
    }
}

int toInt(const string &s, int fallback)
{
    try
    {
        return stoi(s);
    }
    catch (...)
    {
        return fallback;
    }
}

FaceIndex parseFaceToken(const string &token, int vCount, int tCount, int nCount)
{
    FaceIndex result;

    vector<string> parts;
    string part;

    istringstream ss(token);

    while (getline(ss, part, '/'))
        parts.push_back(part);

    if (parts.size() >= 1 && !parts[0].empty())
        result.v = fixObjIndex(toInt(parts[0]), vCount);

    if (parts.size() >= 2 && !parts[1].empty())
        result.t = fixObjIndex(toInt(parts[1]), tCount);

    if (parts.size() >= 3 && !parts[2].empty())
        result.n = fixObjIndex(toInt(parts[2]), nCount);

    return result;
}

int fixObjIndex(int idx, int size)
{
    if (idx > 0)
        return idx - 1;

    if (idx < 0)
        return size + idx;

    return -1;
}