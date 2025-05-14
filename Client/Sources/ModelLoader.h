#pragma once

#include <vector>
#include <string>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <DirectXMath.h>
#include <memory>

using namespace DirectX;

class Renderer;
class Mesh;

class ModelLoader {
public:
    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 texCoords;  // UV ÅØ½ºÃÄ ÁÂÇ¥
    };

    ModelLoader() = default;
    ~ModelLoader() = default;

    bool LoadModel(const std::string& filePath);
    std::shared_ptr<Mesh> LoadMesh(Renderer* renderer, const std::string& filePath);

    const std::vector<Vertex>& GetVertices() const;
    const std::vector<unsigned int>& GetIndices() const;
    const std::vector<XMFLOAT3>& GetNormals() const;
    const std::vector<XMFLOAT2>& GetTexCoords() const;

private:
    void ProcessNode(aiNode* node, const aiScene* scene, const XMMATRIX& parentTransform);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene, const XMMATRIX& transform);

    void Clear();

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT2> texCoords;

    Assimp::Importer importer;
};
