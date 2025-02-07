
#include "mesh_generator.h"
#include "cut_face_preview.h"
#include <QDebug>
#include <QElapsedTimer>
#include <dust3d/mesh/smooth_normal.h>
#include <dust3d/mesh/trim_vertices.h>

MeshGenerator::MeshGenerator(dust3d::Snapshot* snapshot)
    : dust3d::MeshGenerator(snapshot)
{
}

MeshGenerator::~MeshGenerator()
{
}

std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>* MeshGenerator::takeComponentPreviewMeshes()
{
    return m_componentPreviewMeshes.release();
}

MonochromeMesh* MeshGenerator::takeWireframeMesh()
{
    return m_wireframeMesh.release();
}

void MeshGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();

    generate();

    if (nullptr != m_object)
        m_resultMesh = std::make_unique<ModelMesh>(*m_object);

    m_componentPreviewMeshes = std::make_unique<std::map<dust3d::Uuid, std::unique_ptr<ModelMesh>>>();
    for (const auto& componentId : m_generatedPreviewComponentIds) {
        auto it = m_generatedComponentPreviews.find(componentId);
        if (it == m_generatedComponentPreviews.end())
            continue;
        std::vector<std::array<dust3d::Vector2, 3>> triangleUvs;
        if (!it->second.triangleUvs.empty()) {
            triangleUvs.resize(it->second.triangles.size());
            for (size_t i = 0; i < it->second.triangles.size(); ++i) {
                const auto& triangle = it->second.triangles[i];
                auto findUv = it->second.triangleUvs.find({ dust3d::PositionKey(it->second.vertices[triangle[0]]),
                    dust3d::PositionKey(it->second.vertices[triangle[1]]),
                    dust3d::PositionKey(it->second.vertices[triangle[2]]) });
                if (findUv != it->second.triangleUvs.end()) {
                    triangleUvs[i] = findUv->second;
                }
            }
        }
        dust3d::trimVertices(&it->second.vertices, true);
        for (auto& it : it->second.vertices) {
            it *= 2.0;
        }
        std::vector<dust3d::Vector3> previewTriangleNormals;
        previewTriangleNormals.reserve(it->second.triangles.size());
        for (const auto& face : it->second.triangles) {
            previewTriangleNormals.emplace_back(dust3d::Vector3::normal(
                it->second.vertices[face[0]],
                it->second.vertices[face[1]],
                it->second.vertices[face[2]]));
        }
        std::vector<std::vector<dust3d::Vector3>> previewTriangleVertexNormals;
        dust3d::smoothNormal(it->second.vertices,
            it->second.triangles,
            previewTriangleNormals,
            0,
            &previewTriangleVertexNormals);
        (*m_componentPreviewMeshes)[componentId] = std::make_unique<ModelMesh>(it->second.vertices,
            it->second.triangles,
            previewTriangleVertexNormals,
            it->second.color,
            it->second.metalness,
            it->second.roughness,
            it->second.vertexProperties.empty() ? nullptr : &it->second.vertexProperties,
            triangleUvs.empty() ? nullptr : &triangleUvs);
    }

    if (nullptr != m_object)
        m_wireframeMesh = std::make_unique<MonochromeMesh>(*m_object);

    qDebug() << "The mesh generation took" << countTimeConsumed.elapsed() << "milliseconds";

    emit finished();
}

ModelMesh* MeshGenerator::takeResultMesh()
{
    return m_resultMesh.release();
}
