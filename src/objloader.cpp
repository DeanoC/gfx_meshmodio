#include "al2o3_platform/platform.h"
#include "al2o3_cmath/scalar.h"
#include "al2o3_vfile/vfile.h"
#include "al2o3_vfile/memory.h"
#include "al2o3_syoyo/tiny_objloader.h"
#include "utils_misccpp/compiletimehash.hpp"
#include "gfx_meshmod/mesh.h"
#include "gfx_meshmodio/io.h"
#include "gfx_meshmod/vertex/position.h"
#include "gfx_meshmod/vertex/normal.h"
#include "gfx_meshmod/vertex/uv.h"
#include "gfx_meshmod/edge/halfedge.h"
#include "gfx_meshmod/polygon/tribrep.h"

AL2O3_EXTERN_C MeshMod_MeshHandle MeshModIO_LoadObjWithDefaultRegistry(VFile_Handle file) {
	return MeshModIO_LoadObj(NULL, file);
}

AL2O3_EXTERN_C MeshMod_MeshHandle MeshModIO_LoadObj(MeshMod_RegistryHandle registry, VFile_Handle file) {

	char* data = NULL;
	uint64_t dataLen = 0;
	MeshMod_MeshHandle mesh = NULL;
	tinyobj_attrib_t attrib;
	tinyobj_shape_t* shapes = NULL;
	size_t numShapes;
	tinyobj_material_t* materials = NULL;
	size_t numMaterials;

	dataLen = VFile_Size(file);
	if (VFile_GetType(file) == VFile_Type_Memory) {
		VFile_MemFile_t* memfile = (VFile_MemFile_t*)VFile_GetTypeSpecificData(file);
		data = ((char*)memfile->memory);
	}
	else {
		data = (char*)MEMORY_TEMP_MALLOC(dataLen);
		VFile_Read(file, data, dataLen);
	}
	unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
	int ret = tinyobj_parse_obj(&attrib, &shapes, &numShapes, &materials,
		&numMaterials, data, dataLen, flags);
	if (ret != TINYOBJ_SUCCESS) { goto failexit; }


	// TODO for now just do the first shape
	if (numShapes) {
		tinyobj_shape_t* shape = &shapes[0];		
		if (registry == NULL) {
			mesh = MeshMod_MeshCreateWithDefaultRegistry(shape->name);
		}
		else {
			mesh = MeshMod_MeshCreate(registry, shape->name);
		}
		if (mesh == NULL) { goto failexit; }

		MeshMod_DataContainerHandle vertices = MeshMod_MeshGetVertices(mesh);
		MeshMod_DataContainerHandle edges = MeshMod_MeshGetEdges(mesh);
		MeshMod_DataContainerHandle polygons = MeshMod_MeshGetPolygons(mesh);

		CADT_VectorHandle positionVector = MeshMod_DataContainerAdd(vertices, MeshMod_VertexPositionTag);
		CADT_VectorHandle halfEdgeVector = MeshMod_DataContainerAdd(edges, MeshMod_EdgeHalfEdgeTag);
		CADT_VectorHandle triBRepVector = MeshMod_DataContainerAdd(polygons, MeshMod_PolygonTriBRepTag);

		CADT_VectorHandle normalVector = NULL;
		if (attrib.normals != NULL) {
			normalVector = MeshMod_DataContainerAdd(vertices, MeshMod_VertexNormalTag);
		}

		CADT_VectorHandle uvVector = NULL;
		if (attrib.texcoords != NULL) {
			uvVector = MeshMod_DataContainerAdd(vertices, MeshMod_VertexUvTag);
		}

		size_t const baseVertexIndex = MeshMod_DataContainerSize(vertices);
		size_t const baseEdgeIndex = MeshMod_DataContainerSize(edges);
		size_t const basePolygonIndex = MeshMod_DataContainerSize(polygons);

		MeshMod_DataContainerResize(vertices, shape->length * 3);
		MeshMod_DataContainerResize(edges, shape->length * 3);
		MeshMod_DataContainerResize(polygons, shape->length);

		Math_Vec3F_t* positionData = (Math_Vec3F_t *) CADT_VectorAt(positionVector, baseVertexIndex);
		Math_Vec3F_t* normalData = (Math_Vec3F_t*) (normalVector ? CADT_VectorAt(normalVector, baseVertexIndex) : NULL);
		Math_Vec2F_t* uvData = (Math_Vec2F_t*)(uvVector ? CADT_VectorAt(uvVector, baseVertexIndex) : NULL);

		MeshMod_EdgeHalfEdge* halfEdgeData = (MeshMod_EdgeHalfEdge*)CADT_VectorAt(halfEdgeVector, baseEdgeIndex);
		MeshMod_PolygonTriBRep* triBRepData = (MeshMod_PolygonTriBRep*)CADT_VectorAt(triBRepVector, basePolygonIndex);

		// we deindex the data as obj have more complex indexing than
		// meshmod does.
		// MeshMod has algorithms to dedup and fast similar support
		// so we don't really lose anything by doing it this way
		tinyobj_vertex_index_t* faces = attrib.faces + shape->face_offset;
		ASSERT(attrib.face_num_verts[shape->face_offset] == 3);
		for (size_t faceIndex = 0; faceIndex < shape->length; ++faceIndex) {

			for (size_t i = 0; i < 3; ++i) {

				tinyobj_vertex_index_t* v = faces + faceIndex + i;
				memcpy(positionData + (faceIndex * 3) + i, attrib.vertices + v->v_idx, sizeof(float) * 3);
				if (normalData) {
					memcpy(normalData + (faceIndex * 3) + i, attrib.vertices + v->vn_idx, sizeof(float) * 3);
				}
				if (uvData) {
					memcpy(uvData + (faceIndex * 3) + i, attrib.vertices + v->vt_idx, sizeof(float) * 2);
				}
				halfEdgeData[(faceIndex * 3) + i].vertex = faceIndex * 3 + i;
				halfEdgeData[(faceIndex * 3) + i].polygon = faceIndex;

				triBRepData[faceIndex].edge[i] = (faceIndex * 3) + i;
			}
		}
	}

	tinyobj_attrib_free(&attrib);
	tinyobj_materials_free(materials, numMaterials);
	tinyobj_shapes_free(shapes, numShapes);

	if (VFile_GetType(file) != VFile_Type_Memory) {
		MEMORY_TEMP_FREE(data);
	}
	return mesh;

failexit:
	if (mesh != NULL) { MeshMod_MeshDestroy(mesh); }

	tinyobj_attrib_free(&attrib);
	tinyobj_materials_free(materials, numMaterials);
	tinyobj_shapes_free(shapes, numShapes);

	if (VFile_GetType(file) != VFile_Type_Memory) {
		MEMORY_TEMP_FREE(data);
	}
	return NULL;
}
