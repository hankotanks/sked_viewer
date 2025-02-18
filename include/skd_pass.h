#ifndef SKD_PASS_H
#define SKD_PASS_H

#include <GL/glew.h>
#include "util/log.h"
#include "util/shaders.h"
#include "skd.h"
#include "camera.h"

typedef struct {
    GLuint VAO[2], VBO[2], shader_program;
    ssize_t idx;
    GLsizei pts_count, ant_count;
} SchedulePass;

typedef struct {
    GLfloat color_ant[3], color_src[3];
    float globe_radius, shell_radius;
    Shader* vert;
    Shader* frag;
} SchedulePassDesc;

void SchedulePass_free(SchedulePass pass) { 
    (void) pass; /* STUB */ 
}

void SchedulePass_update_and_draw(SchedulePass pass, Camera cam) {
    Camera_update_uniforms(cam, pass.shader_program);
    glEnable(GL_DEPTH_TEST);
    glPointSize(5.f);
    glUseProgram(pass.shader_program);
    glBindVertexArray(pass.VAO[0]);    
    glDrawArrays(GL_POINTS, 0, pass.pts_count);
    glBindVertexArray(pass.VAO[1]);
    glDrawArrays(GL_LINES, 0, pass.ant_count * 3);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
}

void SchedulePass_next_observation(SchedulePass* pass, Schedule skd, unsigned int debug) {
    if((size_t) (++(pass->idx)) == skd.obs_count) {
        printf("Wrapping around to beginning of Schedule.\n");
        pass->idx = 0;
    }
    glUseProgram(pass->shader_program);
    Obs* current = Schedule_get_observation(skd, pass->idx);
    // determine gst offset
    float gst = (float) Datetime_greenwich_sidereal_time(current->timestamp);
    glUniform1f(glGetUniformLocation(pass->shader_program, "gst"), gst);
    // find the current source
    SourceQuasar* src;
    char* id;
    id = (char*) HashMap_get(skd.sources_alias, current->source);
    if(debug) printf("%s: ", (id == NULL) ? current->source : id);
    src = (SourceQuasar*) ((id == NULL) ? HashMap_get(skd.sources, current->source) : HashMap_get(skd.sources, id));
    if(src == NULL) {
        LOG_INFO("Unable to draw observations for the current scan. Unable to find source.");
        return;
    }
    // build observation geometry
    size_t i, ant_count = strlen(current->ids);
    GLfloat vec[ant_count * 6];
    Station* ant;
    char key[2]; key[1] = '\0';
    for(i = 0; i < ant_count; ++i) {
        key[0] = current->ids[i];
        id = (char*) HashMap_get(skd.stations_ant, key);
        if(debug) printf("%c [%c%c], ", key[0], id[0], id[1]);
        ant = (Station*) HashMap_get(skd.stations_pos, id);
        vec[i * 6 + 0] = (GLfloat) ant->lam;
        vec[i * 6 + 1] = (GLfloat) ant->phi;
        vec[i * 6 + 2] = (GLfloat) 0.f;
        vec[i * 6 + 3] = (GLfloat) src->alf;
        vec[i * 6 + 4] = (GLfloat) src->phi;
        vec[i * 6 + 5] = (GLfloat) 1.f;
    }
    if(debug) printf("\n");
    pass->ant_count = ant_count;
    glBindVertexArray(pass->VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, pass->VBO[1]);
    size_t buffer_size = ant_count * 6 * sizeof(GLfloat);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) buffer_size, vec, GL_DYNAMIC_DRAW);
    glUseProgram(0);
}

unsigned int SchedulePass_init_from_schedule(SchedulePass* pass, SchedulePassDesc desc, Schedule skd) {
    unsigned int failure;
    // configure shader program and set constant uniforms
    failure = assemble_shader_program(&(pass->shader_program), desc.vert, desc.frag);
    if(failure) {
        LOG_ERROR("Failed to compile shader program in SchedulePass.");
        return 1;
    }
    glUseProgram(pass->shader_program);
    GLint loc;
    loc = glGetUniformLocation(pass->shader_program, "globe_radius");
    if(loc == -1) {
        LOG_ERROR("Shader provided to SchedulePass had no float 'globe_radius' uniform.");
        glDeleteProgram(pass->shader_program);
        return 1;
    }
    glUniform1f(loc, (GLfloat) desc.globe_radius);
    loc = glGetUniformLocation(pass->shader_program, "shell_radius");
    if(loc == -1) {
        LOG_ERROR("Shader provided to SchedulePass had no float 'shell_radius' uniform.");
        glDeleteProgram(pass->shader_program);
        return 1;
    }
    glUniform1f(loc, (GLfloat) desc.shell_radius);
    // build array of stations and sources
    size_t i, j, pts_count = skd.stations_pos.size + skd.sources.size;
    GLfloat pts[pts_count * 3];
    Node* node;
    char ant_id[3];
    Station* ant;
    for(i = 0, j = 0; i < skd.stations_ant.bucket_count; ++i) {
        node = skd.stations_ant.buckets[i];
        for(; node != NULL; node = node->next, ++j) {
            strcpy(ant_id, (char*) Node_value(node));
            ant = (Station*) HashMap_get(skd.stations_pos, ant_id);
            if(ant == NULL) {
                LOG_INFO("Skipping a station while building SchedulePass. Schedule might not have been validated.");
                pts_count--;
                j--;
                continue;
            }
            pts[j * 3 + 0] = (GLfloat) ant->lam;
            pts[j * 3 + 1] = (GLfloat) ant->phi;
            pts[j * 3 + 2] = (GLfloat) 0.f;
        }
    }
    SourceQuasar* src;
    for(i = 0; i < skd.sources.bucket_count; ++i) {
        node = skd.sources.buckets[i];
        for(; node != NULL; node = node->next, ++j) {
            src = (SourceQuasar*) HashMap_get(skd.sources, node->contents);
            if(src == NULL) {
                LOG_INFO("Skipping a quasar source while building SchedulePass. Schedule might not have been validated.");
                pts_count--;
                j--;
                continue;
            }
            pts[j * 3 + 0] = (GLfloat) src->alf;
            pts[j * 3 + 1] = (GLfloat) src->phi;
            pts[j * 3 + 2] = (GLfloat) 1.f;
        }
    }
    pass->pts_count = (GLsizei) pts_count;
    pass->idx = -1;
    // configure vertex arrays and buffers
    glGenVertexArrays(2, pass->VAO);
    glGenBuffers(2, pass->VBO);
    glBindVertexArray(pass->VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, pass->VBO[0]);
    size_t buffer_size;
    buffer_size = pts_count * 3 * sizeof(GLfloat);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) buffer_size, pts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glUseProgram(pass->shader_program);
    loc = glGetUniformLocation(pass->shader_program, "fst_color");
    if(loc == -1) {
        LOG_ERROR("Shader provided to SchedulePass had no vec3 'fst_color' uniform.");
        glDeleteProgram(pass->shader_program);
        return 1;
    }
    glUniform3f(loc, desc.color_ant[0], desc.color_ant[1], desc.color_ant[2]);
    loc = glGetUniformLocation(pass->shader_program, "snd_color");
    if(loc == -1) {
        LOG_ERROR("Shader provided to SchedulePass had no vec3 'snd_color' uniform.");
        glDeleteProgram(pass->shader_program);
        return 1;
    }
    glUniform3f(loc, desc.color_src[0], desc.color_src[1], desc.color_src[2]);
    glUseProgram(0);
    // set up observation buffers
    glBindVertexArray(pass->VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, pass->VBO[1]);
    buffer_size *= 2;
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) buffer_size, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    SchedulePass_next_observation(pass, skd, 1);
    return 0;
}

#endif /* SKD_PASS_H */