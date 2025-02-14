#ifndef SOURCES_H
#define SOURCES_H

#include <string.h>

#include "./util/shaders.h"
#include "./util/mjd.h"
#include "skd.h"
#include "camera.h"

typedef struct {
    GLsizei source_count;
    GLuint VAO, VBO, shader_program;
    size_t current;
    GLfloat* source_pos;
} SourcePass;

void SourcePass_free(SourcePass pass) {
    free(pass.source_pos);
    glDeleteVertexArrays(1, &(pass.VAO));
    glDeleteBuffers(1, &(pass.VBO));
    glDeleteProgram(pass.shader_program);
}

void SourcePass_update_and_draw(SourcePass pass, Camera cam) {
    Camera_update_uniforms(cam, pass.shader_program);
    glEnable(GL_DEPTH_TEST);
    glPointSize(5.f);
    glUseProgram(pass.shader_program);
    glBindVertexArray(pass.VAO);
    glDrawArrays(GL_POINTS, 0, pass.source_count);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
}

void SourcePass_next(SourcePass* pass, Schedule skd) {
    if(++(pass->current) == skd.obs_count) {
        printf("Wrapping around to beginning of Schedule.\n");
        pass->current = 0;
    }
    glUseProgram(pass->shader_program);
    Obs* current = Schedule_get_observation(skd, pass->current);
    double gst = Datetime_greenwich_sidereal_time(current->timestamp);
    glUniform1f(glGetUniformLocation(pass->shader_program, "gst"), (float) gst);
    glUseProgram(0);
}

unsigned int SourcePass_build(SourcePass* pass, SchedPassDesc desc, Schedule skd) {
    unsigned int failure;
    size_t source_count = skd.sources.size;
    pass->source_pos = (GLfloat*) malloc(sizeof(GLfloat) * source_count * 2);
    if(pass->source_pos == NULL) {
        LOG_ERROR("Failed to allocate while building ScheduleStations.");
        return 1;
    }
    Node* node;
    SourceQuasar* current_source;
    for(size_t i = 0, j = 0; i < skd.sources.bucket_count; ++i) {
        node = skd.sources.buckets[i];
        while(node != NULL) {
            current_source = (SourceQuasar*) Node_value(node);
            if(current_source == NULL) {
                LOG_INFO("Skipping a source while building ScheduleStations. Schedule might not have been validated.");
                source_count--;
            }
            pass->source_pos[j++] = (GLfloat) current_source->alf;
            pass->source_pos[j++] = (GLfloat) current_source->phi;
            node = node->next;
        }
    }
    pass->source_count = (GLsizei) source_count;
    pass->current = 0;
    GLuint shader_program;
    failure = assemble_shader_program(&shader_program, desc.shader_vert, desc.shader_frag);
    if(failure) {
        LOG_ERROR("Failed to assemble StationPass shader program.");
        free(pass->source_pos);
        return 1;
    }
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, pass->source_count * 2 * sizeof(GLfloat), pass->source_pos, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glUseProgram(shader_program);
    glUniform3f(glGetUniformLocation(shader_program, "point_color"), desc.color[0], desc.color[1], desc.color[2]);
    Obs* current_obs = Schedule_get_observation(skd, 0);
    double gst = Datetime_greenwich_sidereal_time(current_obs->timestamp);
    glUniform1f(glGetUniformLocation(shader_program, "gst"), (double) gst);
    glUseProgram(0);
    pass->VAO = VAO;
    pass->VBO = VBO;
    pass->shader_program = shader_program;
    return 0;
}

#endif /* SOURCES_H */