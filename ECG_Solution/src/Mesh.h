#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

#include "Shader.h"


struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct MeshTexture {
    unsigned int id;
    string type;
    string path;
};

class Mesh {

public:

    //mesh data
    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;
    std::vector<MeshTexture> _textures;
    unsigned int VAO;


    //constructor
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<MeshTexture> textures);

    //render mesh
    void Draw(Shader* shader);

private:

    //vertex and element buffer
    unsigned int VBO, EBO;

    //set vertex buffers and attribute pointers
    void setupMesh();

};