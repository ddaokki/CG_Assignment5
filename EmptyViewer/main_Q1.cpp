#include <iostream>
#include <cmath>
#include <vector>
#include <limits>
#include <fstream>

#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>
#include <limits>
#include <cmath>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int width = 512;
const int height = 512;

struct Vec3 {
    float x, y, z;
};

struct Triangle {
    int v0, v1, v2;
};

std::vector<Vec3> vertices;
std::vector<Triangle> triangles;
float depthBuffer[height][width];
unsigned char image[height][width * 3]; // RGB

Vec3 transformModel(const Vec3& p) {
    return { 2 * p.x, 2 * p.y, 2 * p.z - 7.0f };
}

Vec3 transformProjection(const Vec3& p) {
    float l = -0.1f, r = 0.1f, b = -0.1f, t = 0.1f, n = -0.1f, f = -1000.0f;
    float x = (2 * n / (r - l)) * p.x / -p.z;
    float y = (2 * n / (t - b)) * p.y / -p.z;
    float z = (f + n) / (n - f) + (2 * f * n / (n - f)) / -p.z;
    return { x, y, z };
}

void transformViewport(Vec3& p) {
    p.x = (p.x + 1) * 0.5f * width;
    p.y = (1 - p.y) * 0.5f * height;
}

void setPixel(int x, int y) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        image[y][x * 3 + 0] = 255;
        image[y][x * 3 + 1] = 255;
        image[y][x * 3 + 2] = 255;
    }
}

void rasterize(const std::vector<Vec3>& screenVerts) {
    for (auto& tri : triangles) {
        Vec3 v0 = screenVerts[tri.v0];
        Vec3 v1 = screenVerts[tri.v1];
        Vec3 v2 = screenVerts[tri.v2];

        int minX = std::max(0, (int)std::floor(std::min({ v0.x, v1.x, v2.x })));
        int maxX = std::min(width - 1, (int)std::ceil(std::max({ v0.x, v1.x, v2.x })));
        int minY = std::max(0, (int)std::floor(std::min({ v0.y, v1.y, v2.y })));
        int maxY = std::min(height - 1, (int)std::ceil(std::max({ v0.y, v1.y, v2.y })));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float denom = ((v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y));
                if (fabs(denom) < 1e-5f) continue;
                float a = ((v1.y - v2.y) * (x - v2.x) + (v2.x - v1.x) * (y - v2.y)) / denom;
                float b = ((v2.y - v0.y) * (x - v2.x) + (v0.x - v2.x) * (y - v2.y)) / denom;
                float c = 1 - a - b;
                if (a >= 0 && b >= 0 && c >= 0) {
                    float z = a * v0.z + b * v1.z + c * v2.z;
                    if (z < depthBuffer[y][x]) {
                        depthBuffer[y][x] = z;
                        setPixel(x, y);
                    }
                }
            }
        }
    }
}

void createSphere() {
    int w = 32, h = 16;
    for (int j = 1; j < h - 1; ++j) {
        for (int i = 0; i < w; ++i) {
            float theta = (float)j / (h - 1) * M_PI;
            float phi = (float)i / (w - 1) * 2 * M_PI;
            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = -sinf(theta) * sinf(phi);
            vertices.push_back({ x, y, z });
        }
    }
    vertices.push_back({ 0, 1, 0 });
    vertices.push_back({ 0, -1, 0 });

    for (int j = 0; j < h - 3; ++j) {
        for (int i = 0; i < w - 1; ++i) {
            int a = j * w + i;
            int b = (j + 1) * w + (i + 1);
            int c = j * w + (i + 1);
            int d = (j + 1) * w + i;
            triangles.push_back({ a, b, c });
            triangles.push_back({ a, d, b });
        }
    }
    int north = vertices.size() - 2;
    int south = vertices.size() - 1;
    for (int i = 0; i < w - 1; ++i) {
        triangles.push_back({ north, i, i + 1 });
        triangles.push_back({ south, (h - 3) * w + i + 1, (h - 3) * w + i });
    }
}

void render() {
    createSphere();
    std::vector<Vec3> screenVerts;
    for (const auto& v : vertices) {
        Vec3 m = transformModel(v);
        Vec3 p = transformProjection(m);
        transformViewport(p);
        screenVerts.push_back(p);
    }
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            depthBuffer[y][x] = std::numeric_limits<float>::infinity();
    std::fill(&image[0][0], &image[0][0] + sizeof(image), 0);
    rasterize(screenVerts);
}

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(width, height, "Rasterization", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    render();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}