#include "skd_pass.h"
#include <GL/glew.h>
#include "util/log.h"
#include "util/mjd.h"
#include "util/shaders.h"
#include "skd.h"
#include "camera.h"

struct __SKD_PASS_H__SchedulePass {
    GLuint VAO[2], VBO[2], shader_program;
    size_t idx;
    double jd;
    GLsizei pts_count;
};

SchedulePass* SchedulePass_init_from_schedule(SchedulePassDesc desc, Schedule skd) {
    unsigned int failure;
    // configure shader program and set constant uniforms
    GLuint shader_program;
    failure = assemble_shader_program(&shader_program, desc.vert, desc.frag);
    if(failure) {
        LOG_ERROR("Failed to compile shader program in SchedulePass.");
        return NULL;
    }
    glUseProgram(shader_program);
    GLint loc;
    loc = glGetUniformLocation(shader_program, "globe_radius");
    if(loc == -1) {
        LOG_ERROR("Shader provided to SchedulePass had no float 'globe_radius' uniform.");
        glDeleteProgram(shader_program);
        return NULL;
    }
    glUniform1f(loc, (GLfloat) desc.globe_radius);
    loc = glGetUniformLocation(shader_program, "shell_radius");
    if(loc == -1) {
        LOG_ERROR("Shader provided to SchedulePass had no float 'shell_radius' uniform.");
        glDeleteProgram(shader_program);
        return NULL;
    }
    glUniform1f(loc, (GLfloat) desc.shell_radius);
    // build array of stations and sources
    size_t i, j, pts_count = skd.stations_pos.size + skd.sources.size;
    GLfloat pts[pts_count * 3];
    Node* node;
    char ant_id[3];
    NamedPoint* ant;
    for(i = 0, j = 0; i < skd.stations_ant.bucket_count; ++i) {
        node = skd.stations_ant.buckets[i];
        for(; node != NULL; node = node->next, ++j) {
            strcpy(ant_id, (char*) Node_value(node));
            ant = (NamedPoint*) HashMap_get(skd.stations_pos, ant_id);
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
    NamedPoint* src;
    for(i = 0; i < skd.sources.bucket_count; ++i) {
        node = skd.sources.buckets[i];
        for(; node != NULL; node = node->next, ++j) {
            src = (NamedPoint*) HashMap_get(skd.sources, node->contents);
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
    // configure vertex arrays and buffers
    GLuint VAO[2], VBO[2];
    glGenVertexArrays(2, VAO);
    glGenBuffers(2, VBO);
    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    size_t buffer_size;
    buffer_size = pts_count * 3 * sizeof(GLfloat);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) buffer_size, pts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glUseProgram(shader_program);
    loc = glGetUniformLocation(shader_program, "fst_color");
    if(loc == -1) {
        LOG_ERROR("Shader provided to SchedulePass had no vec3 'fst_color' uniform.");
        glDeleteProgram(shader_program);
        return NULL;
    }
    glUniform3f(loc, desc.color_ant[0], desc.color_ant[1], desc.color_ant[2]);
    loc = glGetUniformLocation(shader_program, "snd_color");
    if(loc == -1) {
        LOG_ERROR("Shader provided to SchedulePass had no vec3 'snd_color' uniform.");
        glDeleteProgram(shader_program);
        return NULL;
    }
    glUniform3f(loc, desc.color_src[0], desc.color_src[1], desc.color_src[2]);
    glUseProgram(0);
    // set up observation buffers
    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    buffer_size *= 2;
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) buffer_size, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    SchedulePass* pass = (SchedulePass*) malloc(sizeof(SchedulePass));
    if(pass == NULL) {
        LOG_ERROR("Unable to allocate SchedulePass.");
        glDeleteProgram(shader_program);
        glDeleteVertexArrays(2, VAO);
        glDeleteBuffers(2, VBO);
        return NULL;
    }
    pass->VAO[0] = VAO[0];
    pass->VAO[1] = VAO[1];
    pass->VBO[0] = VBO[0];
    pass->VBO[1] = VBO[1];
    pass->shader_program = shader_program;
    pass->idx = 0;
    Scan* initial = Schedule_get_scan(skd, pass->idx);
    pass->jd = Datetime_to_jd(initial->timestamp);
    pass->pts_count = (GLsizei) pts_count;
    return pass;
}

void SchedulePass_free(const SchedulePass* const pass) { 
    glDeleteProgram(pass->shader_program);
    glDeleteVertexArrays(2, pass->VAO);
    glDeleteBuffers(2, pass->VBO);
    free((SchedulePass*) pass);
}

void render_current_scan(Schedule skd, size_t idx, unsigned int debug) {
    Scan* current = Schedule_get_scan(skd, idx);
    NamedPoint* src;
    char* id;
    id = (char*) HashMap_get(skd.sources_alias, current->source);
    src = (NamedPoint*) ((id == NULL) ? HashMap_get(skd.sources, current->source) : HashMap_get(skd.sources, id));
    if(src == NULL) return; // TODO: Maybe log info here
    if(debug) printf("%s [", (id == NULL) ? current->source : id);
    // build observation geometry
    size_t i, ant_count = strlen(current->ids);
    GLfloat vec[ant_count * 6];
    NamedPoint* ant;
    char key[2]; key[1] = '\0';
    for(i = 0; i < ant_count; ++i) {
        key[0] = current->ids[i];
        id = (char*) HashMap_get(skd.stations_ant, key);
        if(debug) printf("%c", key[0]);
        ant = (NamedPoint*) HashMap_get(skd.stations_pos, id);
        vec[i * 6 + 0] = (GLfloat) ant->lam;
        vec[i * 6 + 1] = (GLfloat) ant->phi;
        vec[i * 6 + 2] = (GLfloat) 0.f;
        vec[i * 6 + 3] = (GLfloat) src->alf;
        vec[i * 6 + 4] = (GLfloat) src->phi;
        vec[i * 6 + 5] = (GLfloat) 1.f;
    }
    if(debug) printf("], ");
    size_t buffer_size = ant_count * 6 * sizeof(GLfloat);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) buffer_size, vec, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, ant_count * 3);
}

// TODO: This should be configurable
#define MJD_INC 0.00005

void SchedulePass_update_and_draw(SchedulePass* const pass, Schedule skd, const Camera* const cam, unsigned int paused) {
    Camera_update_uniforms(cam, pass->shader_program);
    // set up OpenGL state
    glEnable(GL_DEPTH_TEST);
    glPointSize(5.f);
    glUseProgram(pass->shader_program);
    double gst = jd2gst(pass->jd); // set greenwich sidereal time
    glUniform1f(glGetUniformLocation(pass->shader_program, "gst"), (float) gst);
    glBindVertexArray(pass->VAO[0]);    
    glDrawArrays(GL_POINTS, 0, pass->pts_count);
    glBindVertexArray(pass->VAO[1]);  
    glBindBuffer(GL_ARRAY_BUFFER, pass->VBO[1]);
    // render each set of pointing vectors
    Datetime end_dt; double end_jd;
    Scan* curr;
    if(!paused) printf("%13lf (%7.3lf): ", pass->jd, gst);
    for(size_t i = pass->idx; i < skd.scan_count; ++i) {
        curr = Schedule_get_scan(skd, i);
        end_dt = curr->timestamp;
        end_dt.sec += curr->obs_duration + curr->cal_duration;
        end_jd = Datetime_to_jd(end_dt);
        if(Datetime_to_jd(curr->timestamp) < pass->jd && end_jd > pass->jd) {
            render_current_scan(skd, i, !paused); // TODO
        }
    }
    if(!paused) printf("\n");
    // restore OpenGL state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    if(!paused) pass->jd += MJD_INC;
}
