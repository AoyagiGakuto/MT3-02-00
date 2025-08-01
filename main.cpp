#include <Novice.h>
#include <cmath>
#include <imgui.h>

const char kWindowTitle[] = "LE2D_02_アオヤギ_ガクト_確認課題";

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// -------------------- 構造体 --------------------
struct Vector3 {
    float x, y, z;
};

struct Matrix4x4 {
    float m[4][4];
};

struct Segment {
    Vector3 origin;
    Vector3 diff;
};

struct Sphere {
    Vector3 center;
    float radius;
};

// -------------------- ベクトル --------------------
Vector3 Add(const Vector3& a, const Vector3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vector3 Subtract(const Vector3& a, const Vector3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vector3 Multiply(const Vector3& v, float s) { return { v.x * s, v.y * s, v.z * s }; }
float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float LengthSquared(const Vector3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }

Vector3 Project(const Vector3& v1, const Vector3& v2)
{
    float d = Dot(v2, v2);
    if (d == 0.0f)
        return { 0, 0, 0 };
    float t = Dot(v1, v2) / d;
    return Multiply(v2, t);
}

Vector3 ClosestPoint(const Vector3& point, const Segment& segment)
{
    Vector3 v = Subtract(point, segment.origin);
    float t = Dot(v, segment.diff) / LengthSquared(segment.diff);
    t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
    return Add(segment.origin, Multiply(segment.diff, t));
}

// -------------------- 行列 --------------------
Matrix4x4 MultiplyMatrix(const Matrix4x4& a, const Matrix4x4& b)
{
    Matrix4x4 r {};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                r.m[i][j] += a.m[i][k] * b.m[k][j];
    return r;
}

Matrix4x4 MakeViewProjectionMatrix(const Vector3& t, const Vector3& r)
{
    float cy = cosf(r.y), sy = sinf(r.y);
    float cx = cosf(r.x), sx = sinf(r.x);
    float cz = cosf(r.z), sz = sinf(r.z);

    Matrix4x4 rotY { { cy, 0, sy, 0, 0, 1, 0, 0, -sy, 0, cy, 0, 0, 0, 0, 1 } };
    Matrix4x4 rotX { { 1, 0, 0, 0, 0, cx, -sx, 0, 0, sx, cx, 0, 0, 0, 0, 1 } };
    Matrix4x4 rotZ { { cz, -sz, 0, 0, sz, cz, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };

    Matrix4x4 rot = MultiplyMatrix(MultiplyMatrix(rotZ, rotX), rotY);
    Matrix4x4 trans { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -t.x, -t.y, -t.z, 1 } };

    Matrix4x4 view = MultiplyMatrix(trans, rot);

    float fov = 60.0f * (M_PI / 180.0f), aspect = 1280.0f / 720.0f, nearZ = 0.1f, farZ = 100.0f;
    float f = 1.0f / tanf(fov / 2.0f);
    Matrix4x4 proj {};
    proj.m[0][0] = f / aspect;
    proj.m[1][1] = f;
    proj.m[2][2] = farZ / (farZ - nearZ);
    proj.m[2][3] = (-nearZ * farZ) / (farZ - nearZ);
    proj.m[3][2] = 1;

    return MultiplyMatrix(view, proj);
}

Matrix4x4 MakeViewportForMatrix(float l, float t, float w, float h, float n, float f)
{
    Matrix4x4 m {};
    m.m[0][0] = w / 2.0f;
    m.m[1][1] = -h / 2.0f;
    m.m[2][2] = f - n;
    m.m[3][0] = l + w / 2.0f;
    m.m[3][1] = t + h / 2.0f;
    m.m[3][2] = n;
    m.m[3][3] = 1.0f;
    return m;
}

Vector3 Transform(const Vector3& v, const Matrix4x4& m)
{
    float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
    if (w != 0.0f) {
        x /= w;
        y /= w;
        z /= w;
    }
    return { x, y, z };
}

// -------------------- 描画 --------------------
void DrawGrid(const Matrix4x4& viewProj, const Matrix4x4& viewport)
{
    const float half = 2.0f;
    const uint32_t div = 10;
    const float every = (half * 2.0f) / float(div);

    for (uint32_t i = 0; i <= div; i++) {
        float x = -half + i * every;
        Vector3 start = Transform(Transform({ x, 0, -half }, viewProj), viewport);
        Vector3 end = Transform(Transform({ x, 0, half }, viewProj), viewport);
        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }

    for (uint32_t i = 0; i <= div; i++) {
        float z = -half + i * every;
        Vector3 start = Transform(Transform({ -half, 0, z }, viewProj), viewport);
        Vector3 end = Transform(Transform({ half, 0, z }, viewProj), viewport);
        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }
}

void DrawSegment(const Segment& s, const Matrix4x4& vp, const Matrix4x4& viewport, uint32_t color)
{
    Vector3 start = Transform(Transform(s.origin, vp), viewport);
    Vector3 end = Transform(Transform(Add(s.origin, s.diff), vp), viewport);
    Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, color);
}

void DrawSphere(const Sphere& sphere, const Matrix4x4& vp, const Matrix4x4& viewport, uint32_t color)
{
    const uint32_t div = 16;
    const float latStep = M_PI / div;
    const float lonStep = 2 * M_PI / div;

    for (uint32_t lat = 0; lat < div; ++lat) {
        float theta = -M_PI / 2 + lat * latStep;
        for (uint32_t lon = 0; lon < div; ++lon) {
            float phi = lon * lonStep;

            Vector3 a = {
                sphere.center.x + sphere.radius * cosf(theta) * cosf(phi),
                sphere.center.y + sphere.radius * sinf(theta),
                sphere.center.z + sphere.radius * cosf(theta) * sinf(phi)
            };
            Vector3 b = {
                sphere.center.x + sphere.radius * cosf(theta + latStep) * cosf(phi),
                sphere.center.y + sphere.radius * sinf(theta + latStep),
                sphere.center.z + sphere.radius * cosf(theta + latStep) * sinf(phi)
            };
            Vector3 c = {
                sphere.center.x + sphere.radius * cosf(theta) * cosf(phi + lonStep),
                sphere.center.y + sphere.radius * sinf(theta),
                sphere.center.z + sphere.radius * cosf(theta) * sinf(phi + lonStep)
            };

            a = Transform(Transform(a, vp), viewport);
            b = Transform(Transform(b, vp), viewport);
            c = Transform(Transform(c, vp), viewport);

            Novice::DrawLine((int)a.x, (int)a.y, (int)b.x, (int)b.y, color);
            Novice::DrawLine((int)a.x, (int)a.y, (int)c.x, (int)c.y, color);
        }
    }
}

// -------------------- メイン --------------------
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Novice::Initialize(kWindowTitle, 1280, 720);
    char keys[256] = {}, preKeys[256] = {};

    Vector3 cameraT = { 0.2f, -8.0f, 20.0f };
    Vector3 cameraR = { 0.4f, 3.15f, 0.0f };
    Segment segment { { -1.5f, 0.5f, 0.3f }, { 3.0f, -2.0f, 2.0f } };
    Vector3 point { -1.5f, -0.3f, 0.6f };
    Matrix4x4 viewport = MakeViewportForMatrix(0, 0, 1280, 720, 0, 1);

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        Matrix4x4 vp = MakeViewProjectionMatrix(cameraT, cameraR);
        Vector3 v = Subtract(point, segment.origin);
        Vector3 project = Project(v, segment.diff);
        Vector3 closest = ClosestPoint(point, segment);

        ImGui::Begin("Window");
        ImGui::DragFloat3("Point", &point.x, 0.01f);
        ImGui::DragFloat3("Segment origin", &segment.origin.x, 0.01f);
        ImGui::DragFloat3("Segment diff", &segment.diff.x, 0.01f);
        ImGui::DragFloat3("CameraT", &cameraT.x, 0.1f);
        ImGui::DragFloat3("CameraR", &cameraR.x, 0.01f);
        ImGui::InputFloat3("Project", &project.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::End();

        DrawGrid(vp, viewport);
        DrawSegment(segment, vp, viewport, WHITE);

        // 点を球で描画（赤：基準点、黒：最近接点）
        DrawSphere({ point, 0.01f }, vp, viewport, RED);
        DrawSphere({ closest, 0.01f }, vp, viewport, BLACK);

        Novice::EndFrame();
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0)
            break;
    }

    Novice::Finalize();
    return 0;
}
