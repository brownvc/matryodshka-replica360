#include "VBOAttribMarker.h"

VBOAttribMarker::VBOAttribMarker(GLuint index, GLuint numElementsPerVertex, int offset, DATA_TYPE type , bool normalize) :
    index(index),
    dataType(type),
    dataNormalize(normalize ? GLTRUE : GLFALSE),
    numElements(numElementsPerVertex),
    offset(offset)
{
}
