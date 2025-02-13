#ifndef SKD_RENDER_H
#define SKD_RENDER_H

#include <string.h>

#include "./util/shaders.h"

#include "camera.h"
#include "skd.h"
#include "util/mjd.h"

typedef struct {
    GLsizei station_count, source_count;
    char* ids;
    GLfloat* stations_pos;
    GLfloat* source_quasar_pos;
} ScheduleArrays;

void ScheduleArrays_free(ScheduleArrays data) {
    free(data.ids);
    free(data.stations_pos);
}

unsigned int ScheduleArrays_build(ScheduleArrays* data, Schedule* skd) {
    size_t station_count = skd->stations_ant.size;
    size_t source_count = skd->sources.size;
    data->ids = (char*) calloc(station_count + 1, sizeof(char));
    if(data->ids == NULL) {
        LOG_ERROR("Failed to allocate while building ScheduleArrays.");
        return 1;
    }
    data->ids[skd->stations_ant.size] = '\0';
    data->stations_pos = (GLfloat*) malloc(sizeof(GLfloat) * station_count * 2);
    if(data->stations_pos == NULL) {
        LOG_ERROR("Failed to allocate while building ScheduleArrays.");
        return 1;
    }
    // TODO: Remove
    // HashMap_dump(skd->stations_ant, debug_station_antenna_keys);
    // HashMap_dump(skd->stations_pos, Station_debug);
    Node* node;
    char current_id[3];
    Station* current_station;
    for(size_t i = 0, j = 0; i < skd->stations_ant.bucket_count; ++i) {
        node = skd->stations_ant.buckets[i];
        while(node != NULL) {
            strcpy(current_id, (char*) Node_value(node));
            current_station = (Station*) HashMap_get(skd->stations_pos, current_id);
            if(current_station == NULL) {
                LOG_INFO("Skipping a station while building ScheduleArrays. Schedule might not have been validated.");
                station_count--;
            }
            data->stations_pos[j * 2 + 0] = (GLfloat) current_station->lam;
            data->stations_pos[j * 2 + 1] = (GLfloat) current_station->phi;
            data->ids[j++] = node->contents[0];
            node = node->next;
        }
    }
    data->station_count = (GLsizei) station_count;
    data->source_quasar_pos = (GLfloat*) malloc(sizeof(GLfloat) * source_count * 2);
    if(data->source_quasar_pos == NULL) {
        LOG_ERROR("Failed to allocate while building ScheduleArrays.");
        return 1;
    }
    SourceQuasar* current_source;
    for(size_t i = 0, j = 0; i < skd->sources.bucket_count; ++i) {
        node = skd->sources.buckets[i];
        while(node != NULL) {
            current_source = (SourceQuasar*) Node_value(node);
            if(current_source == NULL) {
                LOG_INFO("Skipping a source while building ScheduleArrays. Schedule might not have been validated.");
                source_count--;
            }
            data->source_quasar_pos[j++] = (GLfloat) current_source->lam;
            data->source_quasar_pos[j++] = (GLfloat) current_source->alf;
            node = node->next;
        }
    }
    data->source_count = (GLsizei) source_count;
    //
    // Below is unfinished
    //

    return 0;
}

typedef struct {
    ScheduleArrays data;
    GLuint VAO, VBO, shader_program;
} SchedulePass;

void SchedulePass_free(SchedulePass pass) {
    ScheduleArrays_free(pass.data);
    glDeleteVertexArrays(1, &(pass.VAO));
    glDeleteBuffers(1, &(pass.VBO));
    glDeleteProgram(pass.shader_program);
}

void SchedulePass_update_and_draw(SchedulePass pass, Camera cam) {
    Camera_update_uniforms(cam, pass.shader_program);
    glEnable(GL_DEPTH_TEST);
    glPointSize(5.f);
    glUseProgram(pass.shader_program);
    glBindVertexArray(pass.VAO);    
    glDrawArrays(GL_POINTS, 0, pass.data.station_count);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
}

typedef struct {
    float globe_radius;
    Shader* shader_vert;
    Shader* shader_frag;
} SchedulePassDesc;

unsigned int SchedulePass_init(SchedulePass* pass, SchedulePassDesc desc, Schedule* skd) {
    unsigned int failure;
    // read data from schedule in buffer arrays
    ScheduleArrays data;
    failure = ScheduleArrays_build(&data, skd);
    if(failure) {
        LOG_ERROR("Failed to build ScheduleArrays.");
        return 1;
    }
    // configure station shaders
    GLuint shader_program;
    failure = assemble_shader_program(&shader_program, desc.shader_vert, desc.shader_frag);
    if(failure) {
        LOG_ERROR("Failed to assemble SchedulePass shader program.");
        ScheduleArrays_free(data);
        return 1;
    }
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, data.station_count * 2 * sizeof(GLfloat), data.stations_pos, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glUseProgram(shader_program);
    glUniform1f(glGetUniformLocation(shader_program, "globe_radius"), desc.globe_radius);
    glUseProgram(0);
    pass->VAO = VAO;
    pass->VBO = VBO;
    pass->shader_program = shader_program;
    pass->data = data;
    return 0;
}

#endif /* SKD_RENDER_H */