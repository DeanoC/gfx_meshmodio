#include "al2o3_platform/platform.h"
#include "al2o3_cmath/scalar.h"
#include "al2o3_vfile/vfile.h"
#include "al2o3_vfile/memory.h"
#include "al2o3_syoyo/tiny_objloader.h"
#include "utils_misccpp/compiletimehash.hpp"
#include "render_meshmod/mesh.h"
#include "render_meshmodio/io.h"
#include "render_meshmod/vertex/position.h"
#include "render_meshmod/vertex/normal.h"
#include "render_meshmod/vertex/uv.h"
#include "render_meshmod/edge/halfedge.h"
#include "render_meshmod/polygon/convexbrep.h"

AL2O3_EXTERN_C MeshMod_MeshHandle MeshModIO_LoadObjWithDefaultRegistry(VFile_Handle file) {
	return MeshModIO_LoadObj({0}, file);
}

AL2O3_EXTERN_C MeshMod_MeshHandle MeshModIO_LoadObj(MeshMod_RegistryHandle registry, VFile_Handle file) {

#define FAILEXIT \
	MeshMod_MeshDestroy(mesh); \
	tinyobj_attrib_free(&attrib); \
	tinyobj_materials_free(materials, numMaterials); \
	tinyobj_shapes_free(shapes, numShapes); \
	if (VFile_GetType(file) != VFile_Type_Memory) { \
		MEMORY_TEMP_FREE(data); \
	} \
	return {0};

	char* data = NULL;
	uint64_t dataLen = 0;
	MeshMod_MeshHandle mesh = {0};
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
	if (ret != TINYOBJ_SUCCESS) {
		FAILEXIT
	}

	mesh = MeshMod_MeshCreate(registry, VFile_GetName(file));
	if (!MeshMod_MeshHandleIsValid(mesh ) )
	{
		FAILEXIT
	}

	MeshMod_MeshVertexTagEnsure(mesh, MeshMod_VertexPositionTag);
	MeshMod_MeshEdgeTagEnsure(mesh, MeshMod_EdgeHalfEdgeTag);
	MeshMod_MeshPolygonTagEnsure(mesh, MeshMod_PolygonConvexBRepTag);

	if (attrib.num_normals != 0) {
		MeshMod_MeshVertexTagEnsure(mesh, MeshMod_VertexNormalTag);
	}
	if (attrib.num_texcoords != 0) {
		MeshMod_MeshVertexTagEnsure(mesh, MeshMod_VertexUvTag);
	}

	// we deindex the data as obj have more complex indexing than
	// meshmod does.
	// MeshMod has algorithms to dedup and fast similar support
	// so we don't really lose anything by doing it this way
	tinyobj_vertex_index_t* faces = attrib.faces;
	for (size_t faceIndex = 0; faceIndex < attrib.num_face_num_verts; ++faceIndex) {

		MeshMod_PolygonHandle polygonHandle = MeshMod_MeshPolygonAlloc(mesh);
		MeshMod_PolygonConvexBRep* brep = (MeshMod_PolygonConvexBRep*) MeshMod_MeshPolygonTagHandleToPtr(mesh,
																																																		 MeshMod_PolygonConvexBRepTag,
																																																		 polygonHandle);

		uint8_t const faceCount = Math_MinU8((uint8_t)attrib.face_num_verts[faceIndex], MeshMod_PolygonConvexMaxEdges);
		brep->numEdges = faceCount;

		for (size_t i = 0; i < faceCount; ++i) {
			tinyobj_vertex_index_t* v = faces + faceIndex + i;
			brep->edge[i] = MeshMod_MeshEdgeAlloc(mesh);

			MeshMod_VertexHandle vhandle = MeshMod_MeshVertexAlloc(mesh);

			MeshMod_EdgeHalfEdge* hedge = (MeshMod_EdgeHalfEdge*) MeshMod_MeshEdgeTagHandleToPtr(mesh,
																																													 MeshMod_EdgeHalfEdgeTag,
																																													 brep->edge[i]);
			hedge->polygon = polygonHandle;
			hedge->vertex = vhandle;

			MeshMod_VertexPosition* pos = (MeshMod_VertexPosition*) MeshMod_MeshVertexTagHandleToPtr(mesh,
																																															 MeshMod_VertexPositionTag,
																																															 vhandle);
			memcpy(pos, attrib.vertices + v->v_idx, sizeof(float) * 3);
			if (attrib.num_normals != 0) {
				MeshMod_VertexNormal* normal = (MeshMod_VertexNormal*) MeshMod_MeshVertexTagHandleToPtr(mesh,
																																																MeshMod_VertexNormalTag,
																																																vhandle);
				memcpy(normal, attrib.vertices + v->vn_idx, sizeof(float) * 3);
			}
			if (attrib.num_texcoords != 0) {
				MeshMod_VertexUv* uv =  (MeshMod_VertexUv*) MeshMod_MeshVertexTagHandleToPtr(mesh, MeshMod_VertexUvTag, vhandle);
				memcpy(uv, attrib.vertices + v->vt_idx, sizeof(float) * 2);
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


}
