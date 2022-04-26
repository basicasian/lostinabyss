
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <assimp/Importer.hpp>

#include "Geometry.h"
#include "Material.h"

#include "Shader.h"
#include "Mesh.h"
#include "Utils.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

//unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

class ModelLoader {

private:

    // model data
    std::vector<Mesh> meshes;
    string directory;
    std::vector<MeshTexture> textures_loaded;
    glm::mat4 _modelMatrix;
    std::shared_ptr<Material> _material;


    //loads model via assimp and stores meshes in meshes vector
    void loadModel(string path);

    //retrieves mesh data from nodes
    void processNode(aiNode* node, const aiScene* scene);

    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    //retrieve the texture's file location and load and generate the texture 
    //stores data in a Vertex struct
    std::vector<MeshTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);

public:
    std::vector<Mesh> getMeshes();

    unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

    ModelLoader(char* path, glm::mat4 modelMatrix, std::shared_ptr<Material> material);
    void Draw();

    /*!
     * Sets the model matrix to the parameter
     * @param modelMatrix: new model matrix to be set
     */
    void SetModelMatrix(glm::mat4 modelMatrix);

    void setShader(std::shared_ptr <Shader> shader);

};

//unsigned int TextureFromFile(const char* path, const string& directory, bool gamma);


