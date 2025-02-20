#ifndef GLOBE_H
#define GLOBE_H

#include <GL/glew.h>
#include <math.h>
#include <stddef.h>
#include "util/shaders.h"
#include "camera.h"

// incomplete binding for Globe
typedef struct __GLOBE_H__Globe Globe;
// user-facing configuration options for the Globe mesh
typedef struct {
    size_t slices, stacks;
    float globe_radius;
} GlobeConfig;
// generate a UV sphere
const Globe* Globe_generate(GlobeConfig cfg);
// free the mesh
void Globe_free(const Globe* const mesh);
// GlobePass doesn't need to be exposed
typedef struct __GLOBE_H__GlobePass GlobePass;
// user configures GlobePass with this descriptor
typedef struct {
    float globe_radius, globe_tex_offset;
    Shader* shader_vert;
    Shader* shader_frag;
    const char* path_globe_texture;
} GlobePassDesc;
// initialize GlobePass
const GlobePass* GlobePass_init(GlobePassDesc desc, const Globe* const mesh);
// free GlobePass
void GlobePass_free(const GlobePass* const pass);
// update GlobePass (called during each event loop pass)
void GlobePass_update_and_draw(const GlobePass* const pass, const Camera* const cam);

#endif /* GLOBE_H */
