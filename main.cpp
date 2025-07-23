#include <Novice.h>
#include <cmath>
#include <imgui.h>

const char kWindowTitle[] = "LE2D_02_アオヤギ_ガクト_確認課題";

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

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

//------------------------------
// ベクトル計算
//------------------------------
Vector3 Add(const Vector3& a, const Vector3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vector3 Subtract(const Vector3& a, const Vector3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vector3 Multiply(const Vector3& v, float s) { return { v.x * s, v.y * s, v.z * s }; }
float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float LengthSquared(const Vector3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }

//------------------------------
// 正射影ベクトル
//------------------------------
Vector3 Project(const Vector3& v1, const Vector3& v2)
{
    float d = Dot(v2, v2);
    if (d == 0.0f)
        return { 0, 0, 0 };
    float t = Dot(v1, v2) / d;
    return Multiply(v2, t);
}

//------------------------------
// 最近接点
//------------------------------
Vector3 ClosestPoint(const Vector3& point, const Segment& segment)
{
    Vector3 v = Subtract(point, segment.origin);
    float t = Dot(v, segment.diff) / LengthSquared(segment.diff);
    // 区間内に制限
    t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
    return Add(segment.origin, Multiply(segment.diff, t));
}

//------------------------------
// 行列系
//------------------------------
Matrix4x4 MultiplyMatrix(const Matrix4x4& a, const Matrix4x4& b)
{
    Matrix4x4 r {};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                r.m[i][j] += a.m[i][k] * b.m[k][j];
    return r;
}

Matrix4x4 MakeViewProjectionMatrix(const Vector3& cameraTranslate, const Vector3& cameraRotate)
{
    float cy = cosf(cameraRotate.y), sy = sinf(cameraRotate.y);
    float cx = cosf(cameraRotate.x), sx = sinf(cameraRotate.x);
    float cz = cosf(cameraRotate.z), sz = sinf(cameraRotate.z);

    Matrix4x4 rotY { { cy, 0, sy, 0, 0, 1, 0, 0, -sy, 0, cy, 0, 0, 0, 0, 1 } };
    Matrix4x4 rotX { { 1, 0, 0, 0, 0, cx, -sx, 0, 0, sx, cx, 0, 0, 0, 0, 1 } };
    Matrix4x4 rotZ { { cz, -sz, 0, 0, sz, cz, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };
    Matrix4x4 rot = MultiplyMatrix(MultiplyMatrix(rotZ, rotX), rotY);

    Matrix4x4 trans { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -cameraTranslate.x, -cameraTranslate.y, -cameraTranslate.z, 1 } };
    Matrix4x4 view = MultiplyMatrix(trans, rot);

    float fovY = 60.0f * (M_PI / 180.0f), aspect = 1280.0f / 720.0f, nearZ = 0.1f, farZ = 100.0f;
    float f = 1.0f / tanf(fovY / 2.0f);
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
    m.m[0][0] = w * 0.5f;
    m.m[1][1] = -h * 0.5f;
    m.m[2][2] = f - n;
    m.m[3][0] = l + w * 0.5f;
    m.m[3][1] = t + h * 0.5f;
    m.m[3][2] = n;
    m.m[3][3] = 1;
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

//------------------------------
// グリッド描画
//------------------------------
void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix)
{
    const float kGridHalfWidth = 2.0f;
    const uint32_t kSubdivision = 10;
    const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

    // X方向（縦線）
    for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
        float x = -kGridHalfWidth + xIndex * kGridEvery;
        Vector3 start = { x, 0.0f, -kGridHalfWidth };
        Vector3 end = { x, 0.0f, kGridHalfWidth };

        start = Transform(start, viewProjectionMatrix);
        start = Transform(start, viewportMatrix);
        end = Transform(end, viewProjectionMatrix);
        end = Transform(end, viewportMatrix);

        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }

    // Z方向（横線）
    for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
        float z = -kGridHalfWidth + zIndex * kGridEvery;
        Vector3 start = { -kGridHalfWidth, 0.0f, z };
        Vector3 end = { kGridHalfWidth, 0.0f, z };

        start = Transform(start, viewProjectionMatrix);
        start = Transform(start, viewportMatrix);
        end = Transform(end, viewProjectionMatrix);
        end = Transform(end, viewportMatrix);

        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }
}

//------------------------------
// メイン
//------------------------------
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, 1280, 720);

    // キー入力結果を受け取る箱
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    Vector3 cameraT = { 0.2f, -8.0f, 20.0f };
    Vector3 cameraR = { 0.4f, 3.15f, 0.0f };

    // 初期値
    Segment segment { { -1.5f, 0.5f, 0.3f }, { 3.0f, -2.0f, 2.0f } };
    Vector3 point { -1.5f, -0.3f, 0.6f };

    Matrix4x4 viewport = MakeViewportForMatrix(0, 0, 1280, 720, 0, 1);

    // ウィンドウの×ボタンが押されるまでループ
    while (Novice::ProcessMessage() == 0) {
        // フレームの開始
        Novice::BeginFrame();

        // キー入力を受け取る
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ///
        /// ↓更新処理ここから
        ///

        // ViewProjection更新
        Matrix4x4 vp = MakeViewProjectionMatrix(cameraT, cameraR);

        // 正射影ベクトル&最近接点計算
        Vector3 v = Subtract(point, segment.origin);
        Vector3 project = Project(v, segment.diff);
        Vector3 closest = ClosestPoint(point, segment);

        ///
        /// ↑更新処理ここまで
        ///

        ///
        /// ↓描画処理ここから
        ///

        // ImGui操作
        ImGui::Begin("Window");
        ImGui::DragFloat3("Point", &point.x, 0.01f);
        ImGui::DragFloat3("Segment origin", &segment.origin.x, 0.01f);
        ImGui::DragFloat3("Segment diff", &segment.diff.x, 0.01f);
        ImGui::DragFloat3("CameraT", &cameraT.x, 0.1f);
        ImGui::DragFloat3("CameraR", &cameraR.x, 0.01f);
        ImGui::InputFloat3("Project", &project.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::End();

        // グリッド
        DrawGrid(vp, viewport);

        // 線分両端
        Vector3 segStart = Transform(Transform(segment.origin, vp), viewport);
        Vector3 segEnd = Transform(Transform(Add(segment.origin, segment.diff), vp), viewport);
        Novice::DrawLine((int)segStart.x, (int)segStart.y, (int)segEnd.x, (int)segEnd.y, WHITE);

        // 点描画（赤=基準点, 黒=最近接点）
        Vector3 sp = Transform(Transform(point, vp), viewport);
        Vector3 cp = Transform(Transform(closest, vp), viewport);
        Novice::DrawEllipse((int)sp.x, (int)sp.y, 5, 5, 0.0f, RED, kFillModeSolid);
        Novice::DrawEllipse((int)cp.x, (int)cp.y, 5, 5, 0.0f, BLACK, kFillModeSolid);

        ///
        /// ↑描画処理ここまで
        ///

        // フレームの終了
        Novice::EndFrame();

        // ESCキーが押されたらループを抜ける
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    // ライブラリの終了
    Novice::Finalize();
    return 0;
}