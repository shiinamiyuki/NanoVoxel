//
// Created by Shiina Miyuki on 2019/1/19.
//

#ifndef MIYUKI_TRANSFORM_H
#define MIYUKI_TRANSFORM_H

#include <miyuki/vec4.hpp>

namespace Miyuki {
    struct Matrix4x4 {
        Vec4f m[4];

        Matrix4x4();

        Matrix4x4(const Vec4f&r1, const Vec4f&r2, const Vec4f&r3, const Vec4f&r4);

        Matrix4x4(Float _m[4][4]);

        Matrix4x4 &operator+=(const Matrix4x4 &rhs);

        Matrix4x4 &operator-=(const Matrix4x4 &rhs);

        Matrix4x4 &operator*=(const Matrix4x4 &rhs);

        Matrix4x4 &operator/=(const Matrix4x4 &rhs);

        Matrix4x4 operator+(const Matrix4x4 &rhs) const;

        Matrix4x4 operator-(const Matrix4x4 &rhs) const;

        Matrix4x4 operator*(const Matrix4x4 &rhs) const;

        Matrix4x4 operator/(const Matrix4x4 &rhs) const;

		Vec4f mult(const Vec4f& rhs) const;

        Matrix4x4 mult(const Matrix4x4 &rhs) const;

        static Matrix4x4 identity();

        static Matrix4x4 translation(const Vec3f &rhs);

        static Matrix4x4 rotation(const Vec3f &axis, const Float);

        Float &operator[](uint32_t i) { return m[i / 4][i % 4]; }

        const Float &operator[](uint32_t i) const { return m[i / 4][i % 4]; }

        static bool inverse(const Matrix4x4 &in, Matrix4x4 &out);
    };

    struct Transform {
        Vec3f translation;
        Vec3f rotation;
        Float scale;

        Transform();

        Transform(const Vec3f &t, const Vec3f &r, Float s);

        //rotation then translation
        Vec3f apply(const Vec3f &, bool inverse = false) const;

        Vec3f applyRotation(const Vec3f &, bool inverse = false) const;
    };

}
#endif //MIYUKI_TRANSFORM_H
