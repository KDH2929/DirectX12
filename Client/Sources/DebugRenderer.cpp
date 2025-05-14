#include "DebugRenderer.h"
#include "Renderer.h"

DebugRenderer& DebugRenderer::GetInstance() {
    static DebugRenderer instance;
    return instance;
}

bool DebugRenderer::Initialize(Renderer* renderer) {
    this->renderer = renderer;
    CreateBoxGeometry();
    CreateSphereGeometry();

    auto device = renderer->GetDevice();
    auto shaderManager = renderer->GetShaderManager();
    vertexShader = shaderManager->GetVertexShader(L"WireframeShader");
    pixelShader = shaderManager->GetPixelShader(L"WireframeShader");
    inputLayout = shaderManager->GetInputLayout(L"WireframeShader");

    // Create vertex/index buffers
    D3D11_BUFFER_DESC vbDesc = { sizeof(LineVertex) * (UINT)boxVertices.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER };
    D3D11_SUBRESOURCE_DATA vbData = { boxVertices.data() };
    device->CreateBuffer(&vbDesc, &vbData, boxVertexBuffer.GetAddressOf());

    D3D11_BUFFER_DESC ibDesc = { sizeof(UINT) * (UINT)boxIndices.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER };
    D3D11_SUBRESOURCE_DATA ibData = { boxIndices.data() };
    device->CreateBuffer(&ibDesc, &ibData, boxIndexBuffer.GetAddressOf());

    vbDesc.ByteWidth = sizeof(LineVertex) * (UINT)sphereVertices.size();
    vbData.pSysMem = sphereVertices.data();
    device->CreateBuffer(&vbDesc, &vbData, sphereVB.GetAddressOf());

    ibDesc.ByteWidth = sizeof(UINT) * (UINT)sphereIndices.size();
    ibData.pSysMem = sphereIndices.data();
    device->CreateBuffer(&ibDesc, &ibData, sphereIB.GetAddressOf());

    D3D11_BUFFER_DESC cbd = { sizeof(CB_MVP), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER };
    device->CreateBuffer(&cbd, nullptr, constantBuffer.GetAddressOf());

    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_WIREFRAME;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = true;
    device->CreateRasterizerState(&rsDesc, wireframeState.GetAddressOf());

    return true;
}

void DebugRenderer::CreateBoxGeometry() {
    boxVertices = {
        {{-1,-1,-1}}, {{1,-1,-1}}, {{-1,1,-1}}, {{1,1,-1}},
        {{-1,-1,1}},  {{1,-1,1}},  {{-1,1,1}},  {{1,1,1}}
    };
    boxIndices = {
        0,1, 0,2, 1,3, 2,3,
        4,5, 4,6, 5,7, 6,7,
        0,4, 1,5, 2,6, 3,7
    };
}

void DebugRenderer::CreateSphereGeometry() {
    const int slices = 12;
    const int stacks = 6;
    for (int stack = 0; stack <= stacks; ++stack) {
        float phi = XM_PI * stack / stacks;
        for (int slice = 0; slice <= slices; ++slice) {
            float theta = 2.0f * XM_PI * slice / slices;
            float x = sinf(phi) * cosf(theta);
            float y = cosf(phi);
            float z = sinf(phi) * sinf(theta);
            sphereVertices.push_back({ {x, y, z} });
        }
    }
    for (int stack = 0; stack < stacks; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            int curr = stack * (slices + 1) + slice;
            int next = curr + slices + 1;
            sphereIndices.push_back(curr);
            sphereIndices.push_back(curr + 1);
            sphereIndices.push_back(curr);
            sphereIndices.push_back(next);
        }
    }
}

void DebugRenderer::DrawBox(const PxVec3& center, const PxVec3& halfExtents, const PxQuat& rotation) {
    XMMATRIX scale = XMMatrixScaling(halfExtents.x, halfExtents.y, halfExtents.z);
    XMMATRIX rot = XMMatrixRotationQuaternion(XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w));
    XMMATRIX trans = XMMatrixTranslation(center.x, center.y, center.z);
    drawCalls.push_back({ scale * rot * trans, DebugShapeType::Box });
}

void DebugRenderer::DrawSphere(const PxVec3& center, float radius) {
    XMMATRIX scale = XMMatrixScaling(radius, radius, radius);
    XMMATRIX trans = XMMatrixTranslation(center.x, center.y, center.z);
    drawCalls.push_back({ scale * trans, DebugShapeType::Sphere });
}

void DebugRenderer::Render(Renderer* renderer) {
    auto context = renderer->GetDeviceContext();
    auto view = renderer->GetCamera()->GetViewMatrix();
    auto proj = renderer->GetCamera()->GetProjectionMatrix();

    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);

    ID3D11RasterizerState* prevState = nullptr;
    context->RSGetState(&prevState);
    context->RSSetState(wireframeState.Get());

    for (const auto& draw : drawCalls) {
        CB_MVP cb = {
            XMMatrixTranspose(draw.world),
            XMMatrixTranspose(view),
            XMMatrixTranspose(proj)
        };
        context->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
        context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

        UINT stride = sizeof(LineVertex), offset = 0;
        if (draw.shapeType == DebugShapeType::Box) {
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            context->IASetVertexBuffers(0, 1, boxVertexBuffer.GetAddressOf(), &stride, &offset);
            context->IASetIndexBuffer(boxIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
            context->DrawIndexed((UINT)boxIndices.size(), 0, 0);
        }
        else {
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            context->IASetVertexBuffers(0, 1, sphereVB.GetAddressOf(), &stride, &offset);
            context->IASetIndexBuffer(sphereIB.Get(), DXGI_FORMAT_R32_UINT, 0);
            context->DrawIndexed((UINT)sphereIndices.size(), 0, 0);
        }
    }

    drawCalls.clear();

    context->RSSetState(prevState);
    if (prevState) prevState->Release();
}
