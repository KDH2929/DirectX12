#pragma once

#include <memory>
#include <string>
#include <d3d12.h>
#include "Texture.h"


class Renderer;

// EnvironmentMaps: IBL�� ȯ��� ���ҽ� ����
class EnvironmentMaps
{
public:
    // ������ ��ο��� IBL ���ҽ� �ε�
    // ��ΰ� ��ȿ�ϸ� true ��ȯ
    bool Load(
        Renderer* renderer,
        const std::wstring& irradianceMapPath,
        const std::wstring& specularMapPath,
        const std::wstring& brdfLutPath);
    

    // ��õ� ��Ʈ �ε����� SRV ���ε�
    void Bind(
        ID3D12GraphicsCommandList* commandList,
        UINT rootIndexIrradiance,
        UINT rootIndexSpecular,
        UINT rootIndexBrdfLut) const;

private:
    std::shared_ptr<Texture> irradianceMap;      // Ȯ�� IBL�� ť���
    std::shared_ptr<Texture> specularMap;        // ����ŧ�� �������� ť���
    std::shared_ptr<Texture> brdfLutTexture;     // BRDF ���� LUT
};
