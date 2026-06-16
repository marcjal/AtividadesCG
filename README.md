# Visualizador 3D com OpenGL Moderna - Atividade Final Grau B

Este repositório contém o projeto final da disciplina de Computação Gráfica: um leitor/visualizador de cenas 3D desenvolvido em C++ com OpenGL moderna.

O objetivo do projeto é integrar, em uma única aplicação, os principais conceitos trabalhados durante o semestre:

- Carregamento de modelos 3D no formato `.obj`;
- Leitura de materiais no formato `.mtl`;
- Mapeamento de texturas;
- Iluminação local com modelo de Phong;
- Controle de câmera sintética;
- Seleção e transformação de objetos;
- Animação de objetos por trajetória paramétrica;
- Organização da cena por arquivo de configuração externo.

A aplicação final é executada por meio do arquivo `DioramaFinal.cpp`, que carrega uma cena composta por múltiplos objetos 3D, fontes de luz, câmera navegável e objetos animados.

---

## 1. Setup

### 1.1 Dependências utilizadas

O projeto utiliza as seguintes dependências:

- **C++17**
- **CMake 3.10 ou superior**
- **OpenGL 3.3 Core Profile**
- **GLFW 3.4**
- **GLM**
- **GLAD**
- **stb_image**

As bibliotecas **GLFW**, **GLM** e **stb_image** são obtidas automaticamente pelo CMake usando `FetchContent`.

A biblioteca **GLAD** deve estar disponível manualmente no projeto, conforme a estrutura abaixo:

```
include/glad/glad.h
common/glad.c
```

Caso esses arquivos não existam, gere a GLAD em:

```text
https://glad.dav1d.de/
```

Configuração recomendada:

```text
Language: C/C++
Specification: OpenGL
API: gl Version 3.3
Profile: Core
```

---

### 1.2 Estrutura esperada do projeto

```
AtividadesCG/
├── assets/
│   ├── scene_final.txt
│   ├── Modelos3D/
│   │   ├── Cube.obj
│   │   ├── Suzanne.obj
│   │   └── SuzanneSubdiv1.obj
│   └── tex/
│       └── pixelWall.png
├── common/
│   └── glad.c
├── include/
│   └── glad/
│       └── glad.h
├── src/
│   ├── DioramaFinal.cpp
│   ├── ObjTexViewer.cpp
│   ├── CameraFPS.cpp
│   ├── TrajetoriasObjetos.cpp
│   └── AtividadeVivencialTresPontos.cpp
├── CMakelists.txt
└── README.md
```

Observação: em alguns ambientes, o CMake espera o arquivo com o nome `CMakeLists.txt`, com `L` maiúsculo. Caso o comando `cmake ..` não reconheça o projeto, renomeie:

```bash
mv CMakelists.txt CMakeLists.txt
```

---

### 1.3 Compilação

A partir da raiz do repositório, execute:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

No Windows, usando Visual Studio como gerador do CMake, o executável pode ser criado dentro de uma subpasta como:

```
build/Debug/
```

ou

```
build/Release/
```

dependendo da configuração selecionada.

---

### 1.4 Execução

Após compilar, execute o visualizador final:

#### Linux/macOS

```bash
./DioramaFinal
```

ou, dependendo da pasta de build:

```bash
./build/DioramaFinal
```

#### Windows

```bash
DioramaFinal.exe
```

ou, dependendo da configuração:

```bash
build/Debug/DioramaFinal.exe
```

---

### 1.5 Arquivo de configuração da cena

A cena final é descrita pelo arquivo:

```text
assets/scene_final.txt
```

Formato utilizado:

```text
camera px py pz yaw pitch fov near far
light nome px py pz r g b intensity enabled
object nome caminho.obj px py pz rx ry rz escala
bezier nomeObjeto p0x p0y p0z p1x p1y p1z p2x p2y p2z p3x p3y p3z velocidade
```

Exemplo:

```text
camera 0.0 1.5 7.0 -90.0 -8.0 45.0 0.1 100.0

light Key -4.0 4.0 4.0 1.0 0.95 0.85 2.4 1
light Fill 4.0 3.0 4.0 0.75 0.85 1.0 0.9 1
light Back 0.0 4.0 -4.0 1.0 1.0 1.0 1.5 1

object Cube assets/Modelos3D/Cube.obj -2.0 0.0 0.0 0.0 25.0 0.0 0.8
object Suzanne assets/Modelos3D/Suzanne.obj -1.5 0.0 0.0 0.0 0.0 0.0 0.8
object SuzanneSubdiv1 assets/Modelos3D/SuzanneSubdiv1.obj 2.0 0.0 0.0 0.0 0.0 0.0 0.8

bezier Suzanne -1.5 0.0 0.0 -0.5 1.2 1.0 0.5 1.2 -1.0 1.5 0.0 0.0 0.25
```

---

### 1.6 Controles da aplicação

| Tecla             | Função                                               |
| ----------------- | ---------------------------------------------------- |
| `ESC`             | Fecha a aplicação                                    |
| `TAB`             | Seleciona o próximo objeto                           |
| `T`               | Ativa modo de translação                             |
| `R`               | Ativa modo de rotação                                |
| `E`               | Ativa modo de escala uniforme                        |
| `Setas`           | Move o objeto selecionado nos eixos X/Y              |
| `PageUp/PageDown` | Move o objeto selecionado no eixo Z                  |
| `X/Y/Z`           | Aplica transformação no eixo correspondente          |
| `Shift + X/Y/Z`   | Aplica transformação no sentido negativo             |
| `+ / -`           | Aumenta ou diminui a escala uniforme                 |
| `W/A/S/D`         | Move a câmera no plano XZ                            |
| `Q/C`             | Move a câmera para baixo/cima                        |
| `Mouse`           | Rotaciona a câmera                                   |
| `Scroll`          | Altera o campo de visão/FOV                          |
| `M`               | Alterna entre sólido e wireframe                     |
| `F`               | Liga/desliga texturas                                |
| `K`               | Alterna visualização de material: final, Ka, Kd, Ks  |
| `1`               | Liga/desliga a luz principal, Key Light              |
| `2`               | Liga/desliga a luz de preenchimento, Fill Light      |
| `3`               | Liga/desliga a luz de fundo, Back Light              |
| `L`               | Seleciona a próxima luz                              |
| `[ / ]`           | Diminui/aumenta a intensidade da luz selecionada     |
| `SPACE`           | Inicia/pausa a animação Bézier do objeto selecionado |
| `B`               | Reinicia a animação Bézier                           |

---

## 2. Funcionalidades implementadas

### 2.1 Carregamento de malhas

O visualizador carrega modelos no formato `.obj`, lendo:

- Posições dos vértices: `v`;
- Coordenadas de textura: `vt`;
- Normais: `vn`;
- Faces: `f`.

As faces são convertidas para triângulos quando necessário, usando triangulação simples por fan.

---

### 2.2 Materiais

O projeto lê arquivos `.mtl` associados aos modelos `.obj`.

São utilizados os seguintes coeficientes:

| Coeficiente | Significado                 |
| ----------- | --------------------------- |
| `Ka`        | Componente ambiente         |
| `Kd`        | Componente difusa           |
| `Ks`        | Componente especular        |
| `Ns`        | Expoente especular / brilho |
| `map_Kd`    | Textura difusa              |

Esses valores são enviados ao Fragment Shader e utilizados no cálculo da iluminação.

---

### 2.3 Texturas

As texturas são carregadas com a biblioteca `stb_image`.

O shader utiliza `sampler2D` para aplicar a textura difusa do material. A textura pode ser ativada ou desativada durante a execução com a tecla `F`.

---

### 2.4 Iluminação

A cena utiliza iluminação local baseada no modelo de Phong, com os seguintes componentes:

- Ambiente;
- Difusa;
- Especular.

O projeto implementa uma lógica de três pontos de luz:

- **Key Light**: luz principal;
- **Fill Light**: luz de preenchimento;
- **Back Light**: luz de fundo.

As luzes podem ser ligadas e desligadas individualmente durante a execução.

---

### 2.5 Câmera

A câmera é controlada em estilo FPS, usando teclado e mouse.

A matriz `View` é calculada com `lookAt`, enquanto a matriz de projeção é calculada com `perspective`.

---

### 2.6 Transformações

Cada objeto possui sua própria matriz de modelo, composta por:

- Translação;
- Rotação;
- Escala uniforme.

A matriz `Model` é enviada ao Vertex Shader como uniform.

---

### 2.7 Animação

O projeto implementa animação por curva Bézier cúbica.

A posição do objeto animado é calculada com quatro pontos de controle:

```cpp
P(t) =
(1 - t)^3 P0 +
3(1 - t)^2 t P1 +
3(1 - t)t^2 P2 +
t^3 P3
```

A animação pode ser iniciada, pausada e reiniciada durante a execução.

---

## 3. Assets

### 3.1 Modelos 3D utilizados

| Asset               | Arquivo                               | Fonte / procedência                                                                 | Link                     | Licença                 | Processamento prévio                           |
| ------------------- | ------------------------------------- | ----------------------------------------------------------------------------------- | ------------------------ | ----------------------- | ---------------------------------------------- |
| Cubo                | `assets/Modelos3D/Cube.obj`           | Modelo simples utilizado nas atividades da disciplina / gerado para testes de malha | Não aplicável            | Uso acadêmico           | Pode ter sido triangularizado/exportado em OBJ |
| Suzanne             | `assets/Modelos3D/Suzanne.obj`        | Modelo Suzanne, primitiva clássica do Blender                                       | https://www.blender.org/ | Blender / uso acadêmico | Exportado em OBJ                               |
| Suzanne subdividida | `assets/Modelos3D/SuzanneSubdiv1.obj` | Variação da Suzanne com subdivisão                                                  | https://www.blender.org/ | Blender / uso acadêmico | Subdivisão aplicada no Blender                 |

Observação: caso algum modelo tenha sido baixado de repositórios externos, o link específico do asset deve ser incluído na tabela acima.

Sites recomendados para obtenção de modelos 3D gratuitos:

- https://sketchfab.com/features/free-3d-models
- https://www.turbosquid.com/Search/3D-Models/free
- https://free3d.com/
- https://www.cgtrader.com/free-3d-models

---

### 3.2 Texturas utilizadas

| Textura                 | Arquivo                    | Fonte / procedência                                | Link                          | Licença       | Uso                              |
| ----------------------- | -------------------------- | -------------------------------------------------- | ----------------------------- | ------------- | -------------------------------- |
| Textura de parede/pixel | `assets/tex/pixelWall.png` | Textura local utilizada nas atividades do semestre | Não aplicável / asset de aula | Uso acadêmico | Aplicada em objetos texturizados |

Caso a textura seja substituída por outra de repositório externo, registrar a fonte específica.

Sites recomendados para obtenção de texturas:

- https://ambientcg.com/
- https://polyhaven.com/textures
- https://www.textures.com/
- https://polyhaven.com/hdris

---

### 3.3 Softwares de processamento prévio

Durante o desenvolvimento e preparação dos assets, podem ser utilizados os seguintes softwares:

| Software   | Uso                                                                                                              |
| ---------- | ---------------------------------------------------------------------------------------------------------------- |
| Blender    | Exportação de modelos para `.obj`, triangulação, ajuste de escala, correção de normais e aplicação de subdivisão |
| MeshLab    | Verificação e limpeza de malhas, quando necessário                                                               |
| OBS Studio | Gravação do vídeo de demonstração                                                                                |

No projeto atual, os modelos principais são simples e adequados para uso acadêmico. Caso novos modelos sejam adicionados, recomenda-se verificar se estão triangularizados, com normais e coordenadas de textura antes da execução.

---

## 4. Referências

### 4.1 Documentação técnica

- OpenGL Reference Pages
  https://registry.khronos.org/OpenGL-Refpages/

- OpenGL Wiki
  https://www.khronos.org/opengl/wiki/

- GLFW Documentation
  https://www.glfw.org/documentation.html

- GLM Documentation
  https://github.com/g-truc/glm

- stb_image
  https://github.com/nothings/stb

- GLAD
  https://glad.dav1d.de/

---

### 4.2 Tutoriais consultados

- LearnOpenGL
  https://learnopengl.com/

- OpenGL Tutorial
  https://www.opengl-tutorial.org/

- Anton's OpenGL 4 Tutorials
  https://antongerdelan.net/opengl/

---

### 4.3 Conteúdos relacionados ao projeto

Foram consultados materiais sobre:

- Pipeline gráfico programável;
- Vertex Shader e Fragment Shader;
- Matrizes Model, View e Projection;
- Transformações geométricas;
- Câmera sintética;
- Mapeamento de textura;
- Arquivos OBJ e MTL;
- Modelo de iluminação Phong;
- Curvas paramétricas e curvas Bézier;
- Uso de VAO, VBO e uniforms em OpenGL moderna.

---

## 5. Observações finais

Este projeto foi desenvolvido com fins acadêmicos para a atividade final de Computação Gráfica.

A cena final busca integrar, em uma única aplicação, os componentes principais do pipeline gráfico:

1. Leitura de cena;
2. Carregamento de malhas;
3. Materiais e texturas;
4. Iluminação local;
5. Câmera;
6. Transformações;
7. Animação;
8. Interação em tempo real.
