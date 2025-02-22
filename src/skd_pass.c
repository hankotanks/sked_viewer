#include "skd_pass.h"
#include <GL/glew.h>
#include <stdio.h>
#include "RFont.h"
#include "util/log.h"
#include "util/mjd.h"
#include "util/shaders.h"
#include "skd.h"
#include "camera.h"

#define CHECK_SCAN_RENDERING

static size_t __RENDER_LINE_COUNT = 0;
#define RENDER_FONT_SIZE 30
#define RENDER_RESET() \
    do { \
        __RENDER_LINE_COUNT = 0; \
    } while(0)
#define RENDER_LINE(font, format, ...) \
    do { \
        int len = snprintf(NULL, 0, (format), __VA_ARGS__); \
        if (len < 0) break; \
        char buf[len + 1]; \
        snprintf(buf, len + 1, (format), __VA_ARGS__); \
        RFont_draw_text((font), buf, 0, ((float) (RENDER_FONT_SIZE)) * (__RENDER_LINE_COUNT++), RENDER_FONT_SIZE); \
    } while(0)

struct __SKD_PASS_H__SchedulePass {
    GLuint VAO[2], VBO[2], shader_program;
    unsigned int paused, restarted;
    size_t idx, pts_count;
    double jd, jd_inc, jd_max;
#ifdef CHECK_SCAN_RENDERING
    unsigned char* scans_rendered;
#else
    void* __p0;
#endif
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
    pass->paused = 1;
    pass->restarted = 1;
    pass->idx = 0;
    pass->pts_count = pts_count;
    Scan* current = Schedule_get_scan(skd, pass->idx);
    pass->jd = Datetime_to_jd(current->timestamp);
    pass->jd_inc = desc.jd_inc;
    Datetime last_start = current->timestamp, last_final = last_start, temp_final;
    last_final.sec += current->obs_duration + current->cal_duration;
    printf("%zu\n", sizeof(SchedulePass));
    for(size_t i = skd.scan_count; (--i) >= 0;) {
        current = Schedule_get_scan(skd, i);
        temp_final = current->timestamp;
        temp_final.sec += current->obs_duration + current->cal_duration;
        if(Datetime_to_jd(last_final) < Datetime_to_jd(temp_final)) {
            last_start = current->timestamp;
            last_final = temp_final;
        }
        if(Datetime_to_jd(temp_final) > Datetime_to_jd(last_start)) break;
    }
    pass->jd_max = Datetime_to_jd(last_final);
#ifdef CHECK_SCAN_RENDERING
    pass->scans_rendered = (unsigned char*) calloc(skd.scan_count, sizeof(unsigned char));
    if(pass->scans_rendered == NULL) LOG_INFO("Failed to allocate the SchedulePass's debug struct members. Station coverage will not be tracked.");
#endif
    return pass;
}

void SchedulePass_free(const SchedulePass* const pass) { 
    glDeleteProgram(pass->shader_program);
    glDeleteVertexArrays(2, pass->VAO);
    glDeleteBuffers(2, pass->VBO);
#ifdef CHECK_SCAN_RENDERING
    if(pass->scans_rendered) free(pass->scans_rendered);
#endif
    free((SchedulePass*) pass);
}

unsigned int render_current_scan(Schedule skd, size_t idx) {
    Scan* current = Schedule_get_scan(skd, idx);
    NamedPoint* src;
    char* id;
    id = (char*) HashMap_get(skd.sources_alias, current->source);
    src = (NamedPoint*) ((id == NULL) ? HashMap_get(skd.sources, current->source) : HashMap_get(skd.sources, id));
    if(src == NULL) return 0;
    // build observation geometry
    size_t i, ant_count = strlen(current->ids);
    GLfloat vec[ant_count * 6];
    NamedPoint* ant;
    char key[2]; key[1] = '\0';
    for(i = 0; i < ant_count; ++i) {
        key[0] = current->ids[i];
        id = (char*) HashMap_get(skd.stations_ant, key);
        ant = (NamedPoint*) HashMap_get(skd.stations_pos, id);
        vec[i * 6 + 0] = (GLfloat) ant->lam;
        vec[i * 6 + 1] = (GLfloat) ant->phi;
        vec[i * 6 + 2] = (GLfloat) 0.f;
        vec[i * 6 + 3] = (GLfloat) src->alf;
        vec[i * 6 + 4] = (GLfloat) src->phi;
        vec[i * 6 + 5] = (GLfloat) 1.f;
    }
    size_t buffer_size = ant_count * 6 * sizeof(GLfloat);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) buffer_size, vec, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, ant_count * 3);
    return 1;
}

// returns 1 on completed walk through Schedule
void SchedulePass_update_and_draw(SchedulePass* const pass, Schedule skd, const Camera* const cam, RFont_font* font) {
    RENDER_RESET();
    if(pass->restarted && pass->paused) RENDER_LINE(font, "Press [SPACE] to start/pause", NULL);
    // update camera uniforms
    Camera_update_uniforms(cam, pass->shader_program);
    // set up OpenGL state
    glEnable(GL_DEPTH_TEST);
    glPointSize(5.f);
    glUseProgram(pass->shader_program);
    double gmst = jd2gmst(pass->jd); // set greenwich sidereal time
    glUniform1f(glGetUniformLocation(pass->shader_program, "gmst"), (float) gmst);
    glBindVertexArray(pass->VAO[0]);    
    glDrawArrays(GL_POINTS, 0, (GLsizei) pass->pts_count);
    if(pass->jd > pass->jd_max) {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);
        glDisable(GL_DEPTH_TEST);
        if(font) {
            RENDER_LINE(font, "Schedule finished at %lf [%zu scans]", pass->jd, skd.scan_count);
            RENDER_LINE(font, "Press [R] to restart", NULL);
        }
        return;
    }
    glBindVertexArray(pass->VAO[1]);  
    glBindBuffer(GL_ARRAY_BUFFER, pass->VBO[1]);
    // render each set of pointing vectors
    Datetime end_dt; double end_jd;
    Scan* curr;
    size_t active = 1;
    for(size_t i = pass->idx; i < skd.scan_count; ++i) {
        curr = Schedule_get_scan(skd, i);
        end_dt = curr->timestamp;
        end_dt.sec += curr->obs_duration + curr->cal_duration;
        end_jd = Datetime_to_jd(end_dt);
        if(Datetime_to_jd(curr->timestamp) < pass->jd && end_jd > pass->jd) {
            active += render_current_scan(skd, i); // TODO
#ifdef CHECK_SCAN_RENDERING
            pass->scans_rendered[i]++;
#endif
        }
    }
    // restore OpenGL state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    // display debug information
    if(!(pass->paused) && font) {
        RENDER_LINE(font, "jd: %13lf", pass->jd);
        RENDER_LINE(font, "gmst: %7.3lf", gmst);
        Scan* current;
        NamedPoint* src;
        char* id;
        for(size_t i = pass->idx; i < skd.scan_count && active > 0; ++i) {
            curr = Schedule_get_scan(skd, i);
            end_dt = curr->timestamp;
            end_dt.sec += curr->obs_duration + curr->cal_duration;
            end_jd = Datetime_to_jd(end_dt);
            if(Datetime_to_jd(curr->timestamp) < pass->jd && end_jd > pass->jd) {
                current = Schedule_get_scan(skd, i);
                id = (char*) HashMap_get(skd.sources_alias, current->source);
                src = (NamedPoint*) ((id == NULL) ? HashMap_get(skd.sources, current->source) : HashMap_get(skd.sources, id));
                if(src == NULL) continue;
                active--;
                RENDER_LINE(font, "%s [%s]", (id == NULL) ? current->source : id, current->ids);
            }
        }
    }
    if(!(pass->paused)) pass->jd += pass->jd_inc;
}

void SchedulePass_handle_input(SchedulePass* const pass, Schedule skd, const RGFW_window* const win) {
    Scan* current;
    switch(win->event.type) {
        case RGFW_keyPressed: switch(win->event.key) {
            case RGFW_space: // see if the current observation was advanced
                pass->paused = !(pass->paused);
                pass->restarted = 0;
                break;
            case RGFW_r:
                pass->idx = 0;
                current = Schedule_get_scan(skd, pass->idx);
                pass->jd = Datetime_to_jd(current->timestamp);
                pass->paused = 1;
                pass->restarted = 1;
#ifdef CHECK_SCAN_RENDERING
                size_t rendered_scan_count = 0;
                for(size_t i = 0; i < skd.scan_count; ++i) {
                    if(pass->scans_rendered[i]) rendered_scan_count++;
                    pass->scans_rendered[i] = (unsigned char) 0;
                }
                printf("Rendered %zu out of %zu scans.\n", rendered_scan_count, skd.scan_count);
#endif
                break;
            default: break;
        }
    }
}
