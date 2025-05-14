#include "ModelLoader.h"
#include "Mesh.h"
#include "Renderer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


bool ModelLoader::LoadModel(const std::string& filePath) {
    Clear();

    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_ConvertToLeftHanded
    );

    if (!scene || !scene->HasMeshes()) {
        return false;
    }

    XMMATRIX identity = XMMatrixIdentity();
    ProcessNode(scene->mRootNode, scene, identity);
    return true;
}

std::shared_ptr<Mesh> ModelLoader::LoadMesh(Renderer* renderer, const std::string& filePath)
{
    Clear();

    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_ConvertToLeftHanded
    );

    if (!scene || !scene->HasMeshes()) {
        return nullptr;
    }

    XMMATRIX identity = XMMatrixIdentity();
    ProcessNode(scene->mRootNode, scene, identity);

    // Vertex 포맷 변환
    std::vector<MeshVertex> meshVertices;
    for (const auto& v : vertices) {
        meshVertices.push_back({ v.position, v.normal, v.texCoords });
    }

    auto mesh = std::make_shared<Mesh>();
    if (!mesh->Initialize(renderer->GetDevice(), meshVertices, indices)) {
        return nullptr;
    }

    return mesh;
}

void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, const XMMATRIX& parentTransform) {
    // 노드의 로컬 변환을 행렬로 변환
    XMMATRIX nodeTransform = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&node->mTransformation)));

    // 누적된 변환 계산
    XMMATRIX globalTransform = nodeTransform * parentTransform;

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene, globalTransform);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene, globalTransform);
    }
}

void ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, const XMMATRIX& transform) {
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        XMFLOAT3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        XMVECTOR p = XMLoadFloat3(&pos);
        p = XMVector3Transform(p, transform);
        XMStoreFloat3(&vertex.position, p);

        if (mesh->HasNormals()) {
            vertex.normal = XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        else {
            vertex.normal = XMFLOAT3(0, 0, 0);
        }

        if (mesh->HasTextureCoords(0)) {
            vertex.texCoords = XMFLOAT2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else {
            vertex.texCoords = XMFLOAT2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
}

void ModelLoader::Clear() {
    vertices.clear();
    indices.clear();
}

const std::vector<ModelLoader::Vertex>& ModelLoader::GetVertices() const {
    return vertices;
}

const std::vector<unsigned int>& ModelLoader::GetIndices() const {
    return indices;
}

const std::vector<XMFLOAT3>& ModelLoader::GetNormals() const
{
    return normals;
}

const std::vector<XMFLOAT2>& ModelLoader::GetTexCoords() const
{
    return texCoords;
}
