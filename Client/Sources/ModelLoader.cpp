#include "ModelLoader.h"
#include "Mesh.h"
#include "Renderer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


static void ComputeTangents(
    std::vector<ModelLoader::Vertex>& vertices,
    const std::vector<unsigned int>& indices)
{
    if (vertices.empty() || indices.empty()) return;

    constexpr float kEpsilon = 1e-6f;

    // 1) accumulator buffer for un-normalized tangents
    std::vector<XMFLOAT3> accum(vertices.size(), XMFLOAT3(0, 0, 0));

    // 2) iterate triangles and accumulate dP / dUV based tangents
    for (size_t f = 0; f + 2 < indices.size(); f += 3)
    {
        uint32_t i0 = indices[f];
        uint32_t i1 = indices[f + 1];
        uint32_t i2 = indices[f + 2];

        const XMFLOAT3& p0 = vertices[i0].position;
        const XMFLOAT3& p1 = vertices[i1].position;
        const XMFLOAT3& p2 = vertices[i2].position;

        const XMFLOAT2& uv0 = vertices[i0].texCoords;
        const XMFLOAT2& uv1 = vertices[i1].texCoords;
        const XMFLOAT2& uv2 = vertices[i2].texCoords;

        XMVECTOR dp1 = XMLoadFloat3(&p1) - XMLoadFloat3(&p0);
        XMVECTOR dp2 = XMLoadFloat3(&p2) - XMLoadFloat3(&p0);

        float du1 = uv1.x - uv0.x;
        float dv1 = uv1.y - uv0.y;
        float du2 = uv2.x - uv0.x;
        float dv2 = uv2.y - uv0.y;

        float denom = du1 * dv2 - dv1 * du2;
        if (fabsf(denom) < kEpsilon) denom = kEpsilon; // avoid divide-by-zero

        float inv = 1.0f / denom;
        XMVECTOR tangentVec = (dp1 * dv2 - dp2 * dv1) * inv;

        XMFLOAT3 tangent;
        XMStoreFloat3(&tangent, tangentVec);

        // add the face tangent to each of the three vertices
        for (uint32_t idx : { i0, i1, i2 })
        {
            accum[idx].x += tangent.x;
            accum[idx].y += tangent.y;
            accum[idx].z += tangent.z;
        }
    }

    // 3) normalize accumulated tangents and write back to vertex data
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        XMVECTOR v = XMLoadFloat3(&accum[i]);
        v = XMVector3Normalize(v);
        XMStoreFloat3(&vertices[i].tangent, v);
    }
}


bool ModelLoader::LoadModel(const std::string& filePath) {
    Clear();

    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
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

    if (needTangentFix) {
        ComputeTangents(vertices, indices);
    }


    // Vertex 포맷 변환
    std::vector<MeshVertex> meshVertices;
    for (const auto& v : vertices) {
        meshVertices.push_back({ v.position, v.normal, v.texCoords, v.tangent });
    }

    auto mesh = std::make_shared<Mesh>();
    
    if (!mesh->Initialize(renderer, meshVertices, indices)) {
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

        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent = XMFLOAT3(
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z);
        }
        else {
            vertex.tangent = XMFLOAT3(1, 0, 0);    // 기본값으로 채워넣기
            needTangentFix = true;
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
    needTangentFix = false;
}

const std::vector<ModelLoader::Vertex>& ModelLoader::GetVertices() const {
    return vertices;
}

const std::vector<unsigned int>& ModelLoader::GetIndices() const {
    return indices;
}