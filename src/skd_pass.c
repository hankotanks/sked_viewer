#include "skd_pass.h"
#include <GL/glew.h>
#include <stdio.h>
#if defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#else
#include <time.h>
#endif
#include <glenv.h>
#include "skd.h"
#include "camera.h"
#include "ui.h"
#include "util/log.h"
#include "util/mjd.h"
#include "util/shaders.h"

#define CLOCK_SPEED_DEFAULT 5
#define CLOCK_SPEED_MAX 11

typedef enum { EVENT_START, EVENT_FINAL } EventType;
typedef struct { 
    size_t idx; 
    double jd;
    EventType type;
} Event;

void sort_event_buffer(Event* const buf, size_t scan_count) {
    Event temp;
    for(size_t i = 0, j; i < scan_count * 2 - 1; ++i) {
        j = i;
        for(size_t k = i + 1; k < scan_count * 2; ++k) {
            if(buf[k].jd < buf[j].jd) j = k;
        }
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
    }
}

struct __SKD_PASS_H__SchedulePass {
    GLuint VAO[2], VBO[2], shader_program;
    size_t pts_count;
    double jd, jd_max;
    size_t event_idx;
    Event* events;
    size_t max_active_scans;
    ssize_t* active_scans;
    unsigned int paused, restarted;
    unsigned long long clock, clock_speed;
};

void update_active_scans(ssize_t* active_scans, size_t count, Event event) {
    ssize_t fst, snd;
    fst = (event.type == EVENT_START) ? (ssize_t) event.idx : -1;
    snd = (event.type == EVENT_START) ? -1 : (ssize_t) event.idx;
    for(size_t i = 0; i < count; ++i) {
        if(active_scans[i] == snd) {
            active_scans[i] = fst;
            break;
        }
    }
}
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
    // set up Scan pointer vector buffers
    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    buffer_size *= skd.stations_ant.size * 6 * sizeof(GLfloat);
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
    pass->pts_count = pts_count;
    // find jd_max
    ScanFAM* current = Schedule_get_scan(skd, 0);
    pass->jd = Datetime_to_jd(current->timestamp);
    Datetime last_start = current->timestamp;
    Datetime last_final = Datetime_add_seconds(last_start, current->cal_duration + current->obs_duration);
    Datetime temp_start, temp_final;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    for(size_t i = skd.scan_count; (--i) >= 0;) {
        current = Schedule_get_scan(skd, i);
        temp_final = Datetime_add_seconds(current->timestamp, current->cal_duration + current->obs_duration);
        if(Datetime_to_jd(last_final) < Datetime_to_jd(temp_final)) {
            last_start = current->timestamp;
            last_final = temp_final;
        }
        if(Datetime_to_jd(temp_final) > Datetime_to_jd(last_start)) break;
    }
#pragma GCC diagnostic pop
    pass->jd_max = Datetime_to_jd(last_final);
    // build and sort Event buffer
    pass->event_idx = 0;
    pass->events = (Event*) malloc(skd.scan_count * 2 * sizeof(Event));
    if(pass->events == NULL) {
        LOG_ERROR("Unable to allocate Event buffer in SchedulePass.");
        glDeleteProgram(shader_program);
        glDeleteVertexArrays(2, VAO);
        glDeleteBuffers(2, VBO);
        return NULL;
    }
    for(size_t i = 0; i < skd.scan_count; ++i) {
        current = Schedule_get_scan(skd, i);
        temp_start = current->timestamp;
        pass->events[i * 2 + 0] = (Event) { .idx = i, .jd = Datetime_to_jd(temp_start), .type = EVENT_START };
        temp_final = Datetime_add_seconds(temp_start, current->obs_duration);
        pass->events[i * 2 + 1] = (Event) { .idx = i, .jd = Datetime_to_jd(temp_final), .type = EVENT_FINAL };
    }
    sort_event_buffer(pass->events, skd.scan_count);
    size_t max_active_scans = 0;
    for(size_t i = 0, j = 0; i < skd.scan_count * 2; ++i) {
        j = (pass->events[i].type == EVENT_START) ? j + 1 : j - 1;
        if(j > max_active_scans) max_active_scans = j;
    }
    pass->max_active_scans = max_active_scans;
    // allocate buffer for active scan indices
    pass->active_scans = (ssize_t*) malloc(max_active_scans * sizeof(ssize_t));
    if(pass->active_scans == NULL) {
        LOG_ERROR("Failed to allocate active scan buffer in SchedulePass.");
        glDeleteProgram(shader_program);
        glDeleteVertexArrays(2, VAO);
        glDeleteBuffers(2, VBO);
        free(pass->events);
        return NULL;
    }
    for(size_t i = 0; i < max_active_scans; ++i) pass->active_scans[i] = (ssize_t) -1;
    // tracking program state
    pass->paused = 1;
    pass->restarted = 1;
    pass->clock_speed = CLOCK_SPEED_DEFAULT;
    return pass;
}

void SchedulePass_free(const SchedulePass* const pass) { 
    glDeleteProgram(pass->shader_program);
    glDeleteVertexArrays(2, pass->VAO);
    glDeleteBuffers(2, pass->VBO);
    free(pass->events);
    free(pass->active_scans);
    free((SchedulePass*) pass);
}

unsigned int render_current_scan(Schedule skd, size_t idx, unsigned char mask[]) {
    ScanFAM* current = Schedule_get_scan(skd, idx);
    NamedPoint* src;
    char* id;
    id = (char*) HashMap_get(skd.sources_alias, current->source);
    src = (NamedPoint*) ((id == NULL) ? HashMap_get(skd.sources, current->source) : HashMap_get(skd.sources, id));
    if(src == NULL) return 0;
    // build observation geometry
    size_t i, j, ant_count = strlen(current->ids);
    GLfloat vec[ant_count * 6];
    NamedPoint* ant;
    char key[2]; key[1] = '\0';
    for(i = 0, j = 0; i < ant_count; ++i) {
        if(mask[i] == (unsigned char) 0) continue;
        key[0] = current->ids[i];
        id = (char*) HashMap_get(skd.stations_ant, key);
        Overlay_add_station(id);
        // printf("%s\n", id);
        ant = (NamedPoint*) HashMap_get(skd.stations_pos, id);
        vec[j++] = (GLfloat) ant->lam;
        vec[j++] = (GLfloat) ant->phi;
        vec[j++] = (GLfloat) 0.f;
        vec[j++] = (GLfloat) src->alf;
        vec[j++] = (GLfloat) src->phi;
        vec[j++] = (GLfloat) 1.f;
    }
    size_t buffer_size = j * sizeof(GLfloat);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) buffer_size, vec, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, (GLsizei) (j / 2));
    return 1;
}

long long get_time_ms() {
#if defined(_WIN32) || defined(__WIN32__)
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (uli.QuadPart / 10000); // Convert to milliseconds
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
#endif
}

void SchedulePass_update_and_draw(SchedulePass* const pass, Schedule skd, const Camera* const cam) {
    // update timestamp and get the change in jd
    unsigned long long temp = pass->clock;
    pass->clock = get_time_ms();
    unsigned long long temp_speed = (1 << pass->clock_speed);
    temp = (pass->clock - temp) * temp_speed;
    double dt = (double) temp / 86400000.0;
    // printf("%.15lf\n", dt1);
    // double dt = 0.000075f;
    // get current greenwich sidereal time (degrees)
    double gmst = jd2gmst(pass->jd);
#ifndef NO_UI
    // update OverlayControls
    OverlayControls controls = (OverlayControls) {
        .jd = pass->jd,
        .gmst = gmst,
        .speed = temp_speed,
        .paused = pass->paused,
    };
    Overlay_set_controls(controls);
#endif
    // update camera uniforms
    Camera_update_uniforms(cam, pass->shader_program);
    // set up OpenGL state
    glEnable(GL_DEPTH_TEST);
    glPointSize(5.f);
    glUseProgram(pass->shader_program);
    glUniform1f(glGetUniformLocation(pass->shader_program, "gmst"), (float) gmst);
    glBindVertexArray(pass->VAO[0]);    
    glDrawArrays(GL_POINTS, 0, (GLsizei) pass->pts_count);
    // forward declare some shared variables
    ScanFAM* current; size_t i, j, k = 0;
    // check if the entire schedule was rendered
    if(pass->jd > pass->jd_max) {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);
        glDisable(GL_DEPTH_TEST);
    } else {
        glBindVertexArray(pass->VAO[1]);  
        glBindBuffer(GL_ARRAY_BUFFER, pass->VBO[1]);
        // render each set of pointing vectors
        if(!(pass->paused)) {
            Event current;
            for(; pass->event_idx < (skd.scan_count * 2); ++(pass->event_idx)) {
                current = pass->events[pass->event_idx];
                if(current.jd > (pass->jd + dt)) break;
                update_active_scans(pass->active_scans, pass->max_active_scans, current);
            }
        }
        unsigned char mask[skd.stations_ant.size];
        Datetime start_with_offset;
        for(i = 0, k = 0; i < pass->max_active_scans; ++i) {
            if(pass->active_scans[i] == -1) continue;
            current = Schedule_get_scan(skd, (size_t) pass->active_scans[i]);
            for(j = 0; j < strlen(current->ids); ++j) {
                start_with_offset = Datetime_add_seconds(current->timestamp, current->scan_offsets[j]);
                mask[j] = (pass->jd < Datetime_to_jd(start_with_offset)) ? 1 : 0;
            }
            render_current_scan(skd, (size_t) pass->active_scans[i], mask);
            k++;
        }
        // restore OpenGL state 
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);
        glDisable(GL_DEPTH_TEST);
    }
#ifndef NO_UI
    // push currently active sources to OverlayState
    NamedPoint* src;
    char* id;
    for(size_t i = 0; i < pass->max_active_scans; ++i) {
        if(pass->active_scans[i] == -1) continue;
        current = Schedule_get_scan(skd, (size_t) pass->active_scans[i]);
        id = (char*) HashMap_get(skd.sources_alias, current->source);
        src = (NamedPoint*) ((id == NULL) ? HashMap_get(skd.sources, current->source) : HashMap_get(skd.sources, id));
        if(src == NULL) continue;
        Overlay_add_active_scan((id == NULL) ? current->source : id);
    }
#endif
    // increment current julian date timestamp
    if(!(pass->paused) && pass->jd <= pass->jd_max) pass->jd += dt;
}

void SchedulePass_handle_action(SchedulePass* const pass, Schedule skd, const OverlayAction act) {
    ScanFAM* current;
    switch(act) {
        case ACTION_SKD_PASS_FASTER:
            if(pass->clock_speed < CLOCK_SPEED_MAX) pass->clock_speed += 1;
            break;
        case ACTION_SKD_PASS_SLOWER:
            if(pass->clock_speed > 0) pass->clock_speed -= 1;
            break;
        case ACTION_SKD_PASS_PAUSE:
            pass->paused = !(pass->paused);
            pass->restarted = 0;
            break;
        case ACTION_SKD_PASS_RESET:
            pass->event_idx = 0;
            for(size_t i = 0; i < pass->max_active_scans; ++i) pass->active_scans[i] = -1;
            current = Schedule_get_scan(skd, 0);
            pass->jd = Datetime_to_jd(current->timestamp);
            pass->paused = 1;
            pass->restarted = 1;
            break;
        default: return;
    }
}

void SchedulePass_handle_input(SchedulePass* const pass, Schedule skd, const RGFW_window* const win) {
    if(win->event.type != RGFW_keyPressed) return;
    if(RGFW_isPressed(NULL, RGFW_shiftL) || RGFW_isPressed(NULL, RGFW_shiftR)) {
        switch(win->event.key) {
            case RGFW_comma:
                SchedulePass_handle_action(pass, skd, ACTION_SKD_PASS_SLOWER);
                break;
            case RGFW_period:
                SchedulePass_handle_action(pass, skd, ACTION_SKD_PASS_FASTER);
            default: break;
        }
    } else {
        switch(win->event.key) {
            case RGFW_space: // see if the current observation was advanced
                SchedulePass_handle_action(pass, skd, ACTION_SKD_PASS_PAUSE);
                break;
            case RGFW_r:
                SchedulePass_handle_action(pass, skd, ACTION_SKD_PASS_RESET);
            default: break;
        }
    }
}
