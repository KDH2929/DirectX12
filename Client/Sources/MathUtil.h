#pragma once

#include <cmath>
#include <string>
#include <DirectXMath.h>

using namespace DirectX;

inline std::string ToString(const DirectX::XMFLOAT3& v) {
    return "(" +
        std::to_string(v.x) + ", " +
        std::to_string(v.y) + ", " +
        std::to_string(v.z) + ")";
}

inline std::wstring ToWString(const DirectX::XMFLOAT3& v) {
    return L"(" +
        std::to_wstring(v.x) + L", " +
        std::to_wstring(v.y) + L", " +
        std::to_wstring(v.z) + L")";
}

inline XMFLOAT3 XMFloat3FromVector(XMVECTOR v) {
    XMFLOAT3 out;
    XMStoreFloat3(&out, v);
    return out;
}

inline XMFLOAT3 RandomOffset3D(float radius)
{
    float rx = ((rand() / (float)RAND_MAX) - 0.5f) * 2.0f * radius;
    float ry = ((rand() / (float)RAND_MAX) - 0.5f) * 2.0f * radius;
    float rz = ((rand() / (float)RAND_MAX) - 0.5f) * 2.0f * radius;
    return XMFLOAT3(rx, ry, rz);
}

template<typename T>
struct Vector3
{
    T x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(T x, T y, T z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(T scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    Vector3 operator/(T scalar) const {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }

    T Length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3 Normalize() const {
        T len = Length();
        if (len == 0) return Vector3(0, 0, 0);
        return *this / len;
    }

    static T Dot(const Vector3& a, const Vector3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vector3 Cross(const Vector3& a, const Vector3& b) {
        return Vector3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    XMFLOAT3 ToXMFLOAT3() const {
        return XMFLOAT3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    }

    static Vector3 FromXMFLOAT3(const XMFLOAT3& v) {
        return Vector3(v.x, v.y, v.z);
    }
};

using Vector3f = Vector3<float>;


