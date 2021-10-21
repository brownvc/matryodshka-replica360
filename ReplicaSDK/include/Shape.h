#ifndef SHAPE_H
#define SHAPE_H

#include "CommonIncludes.h"
#include "VBO.h"

#include <vector>
#include <memory>

/**
 * @brief The Shape class
 *
 * Creates a shape with the given shape data
 */
class Shape
{
public:
    Shape(VBO::GEOMETRY_LAYOUT layout=VBO::GEOMETRY_LAYOUT::LAYOUT_TRIANGLES);

    Shape(const std::vector<float> &vertices,
          VBO::GEOMETRY_LAYOUT layout=VBO::GEOMETRY_LAYOUT::LAYOUT_TRIANGLES);

    Shape(const std::vector<float> &vertices,
          const std::vector<int> &faces,
          VBO::GEOMETRY_LAYOUT layout=VBO::GEOMETRY_LAYOUT::LAYOUT_TRIANGLES);

    Shape(const std::vector<float> &positions,
          const std::vector<float> &normals,
          const std::vector<float> &texCoords,
          VBO::GEOMETRY_LAYOUT layout=VBO::GEOMETRY_LAYOUT::LAYOUT_TRIANGLES);

    Shape(const std::vector<float> &positions,
          const std::vector<float> &normals,
          const std::vector<float> &texCoords,
          const std::vector<int> &faces,
          VBO::GEOMETRY_LAYOUT layout=VBO::GEOMETRY_LAYOUT::LAYOUT_TRIANGLES);

    Shape(std::string shape);
    virtual ~Shape();

    Shape(const Shape&) = delete;
    Shape& operator=(const Shape&) = delete;
    Shape(Shape &&that);
    Shape& operator=(Shape && that);

    void addPosition(float x, float y, float z);
    void addPositions(const std::vector<float> &positions);

    void addNormal(float x, float y, float z);
    void addNormals(const std::vector<float> &normals);

    void addTextureCoordinate(float s, float t);
    void addTextureCoordinates(const std::vector<float> &texCoords);

    void addVertex(float x, float y, float z, float n1, float n2, float n3, float s, float t);
    void addVertices(const std::vector<float> &vertices);

    void addFace(int v1, int v2, int v3);
    void addFaces(const std::vector<int> &faces);

    void draw();

    bool printDebug();

private:
    void build();

private:
    std::vector<float> m_positions;
    std::vector<float> m_normals;
    std::vector<float> m_texCoords;
    std::vector<int> m_faces;

    VBO::GEOMETRY_LAYOUT m_vboTriangleLayout;
    GLenum m_triangleLayout;
    GLuint m_numVertices;
    GLuint m_handle;

    bool m_indexed;
    bool m_built;
};

#endif // SHAPE_H
