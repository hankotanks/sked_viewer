#ifndef STATIONS_H
#define STATIONS_H

#include <string.h>

#include "./util/shaders.h"

#include "skd.h"
#include "camera.h"

typedef struct {
    GLsizei station_count;
    char (*ids)[2];
    GLfloat* stations_pos;
    GLuint VAO, VBO, shader_program;
} StationPass;

void StationPass_free(StationPass pass) {
    free(pass.ids);
    free(pass.stations_pos);
    glDeleteVertexArrays(1, &(pass.VAO));
    glDeleteBuffers(1, &(pass.VBO));
    glDeleteProgram(pass.shader_program);
}

void StationPass_update_and_draw(StationPass pass, Camera cam) {
    Camera_update_uniforms(cam, pass.shader_program);
    glEnable(GL_DEPTH_TEST);
    glPointSize(5.f);
    glUseProgram(pass.shader_program);
    glBindVertexArray(pass.VAO);    
    glDrawArrays(GL_POINTS, 0, pass.station_count);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
}

typedef struct {
    float globe_radius;
    float color[3];
    Shader* shader_vert;
    Shader* shader_frag;
} StationPassDesc;

unsigned int StationPass_configure_buffers(StationPass* pass, StationPassDesc desc) {
    unsigned int failure;
    GLuint shader_program;
    failure = assemble_shader_program(&shader_program, desc.shader_vert, desc.shader_frag);
    if(failure) {
        LOG_ERROR("Failed to assemble StationPass shader program.");
        free(pass->ids);
        free(pass->stations_pos);
        return 1;
    }
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, pass->station_count * 2 * sizeof(GLfloat), pass->stations_pos, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glUseProgram(shader_program);
    glUniform1f(glGetUniformLocation(shader_program, "globe_radius"), desc.globe_radius);
    glUniform3f(glGetUniformLocation(shader_program, "point_color"), desc.color[0], desc.color[1], desc.color[2]);
    glUseProgram(0);
    pass->VAO = VAO;
    pass->VBO = VBO;
    pass->shader_program = shader_program;
    return 0;
}

unsigned int StationPass_build_from_schedule(StationPass* pass, StationPassDesc desc, Schedule skd) {
    size_t station_count = skd.stations_ant.size;
    pass->ids = (char (*)[2]) malloc(sizeof(*(pass->ids)) * station_count + 1);
    if(pass->ids == NULL) {
        LOG_ERROR("Failed to allocate while building ScheduleStationPass.");
        return 1;
    }
    ((char*) pass->ids)[station_count * 2] = '\0';
    pass->stations_pos = (GLfloat*) malloc(sizeof(GLfloat) * station_count * 2);
    if(pass->stations_pos == NULL) {
        LOG_ERROR("Failed to allocate while building ScheduleStationPass.");
        return 1;
    }
    // TODO: Remove
    // HashMap_dump(skd->stations_ant, debug_station_antenna_keys);
    // HashMap_dump(skd->stations_pos, Station_debug);
    Node* node;
    char current_id[3];
    Station* current_station;
    for(size_t i = 0, j = 0; i < skd.stations_ant.bucket_count; ++i) {
        node = skd.stations_ant.buckets[i];
        for(; node != NULL; node = node->next, ++j) {
            strcpy(current_id, (char*) Node_value(node));
            memcpy(&(pass->ids[j]), current_id, 2);
            current_station = (Station*) HashMap_get(skd.stations_pos, current_id);
            if(current_station == NULL) {
                LOG_INFO("Skipping a station while building ScheduleStationPass. Schedule might not have been validated.");
                station_count--;
                j--;
                continue;
            }
            pass->stations_pos[j * 2 + 0] = (GLfloat) current_station->lam;
            pass->stations_pos[j * 2 + 1] = (GLfloat) current_station->phi;
        }
    }
    // printf("%s\n", (char*) pass->ids);
    // HashMap_dump(skd->stations_ant, debug_station_antenna_keys);
    pass->station_count = (GLsizei) station_count;
    return StationPass_configure_buffers(pass, desc);
}

unsigned int StationPass_build_from_catalog(StationPass* pass, StationPassDesc desc, const char* path) {
    int failure;
    pass->station_count = 0;
    FILE* stream;
    stream = fopen(path, "rb");
    if(stream == NULL) {
        LOG_ERROR("Station position catalog couldn't be opened.");
        return 1;
    }
    failure = fseek(stream, 0L, SEEK_END);
    CLOSE_STREAM_ON_FAILURE(stream, failure, 1, "Failed to seek to end of file.");
    long file_size = ftell(stream);
    CLOSE_STREAM_ON_FAILURE(stream, file_size < 0, 1, "File size is invalid.");
    failure = fseek(stream, 0L, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, 1, "Failed to seek to beginning of file.");
    ssize_t flag;
    char* line = NULL;
    size_t len = 0, station_count = 0;
    while((flag = getline(&line, &len, stream)) != -1) {
        unsigned int comment = 1;
        for(size_t i = 0; i < (size_t) flag; ++i) {
            switch(line[i]) {
                case ' ':
                case '\t': break;
                case '*':
                case '\r':
                case '\n': goto comment_check_end;
                default:
                    comment = 0;
                    goto comment_check_end;
            }
        } 
comment_check_end:
        if(!comment) station_count++;
        if(file_size - ftell(stream) == 0) {
            flag = 0;
            break;
        }
    }
    free(line);
    line = NULL;
    CLOSE_STREAM_ON_FAILURE(stream, flag == -1, 1, "Failed to count stations before parsing.");
    failure = fseek(stream, 0L, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, 1, "Failed to seek to beginning of catalog.");
    pass->ids = (char (*)[2]) malloc(station_count * sizeof(*(pass->ids)));
    CLOSE_STREAM_ON_FAILURE(stream, pass->ids == NULL, 1, "Unable to allocate memory for station ids.");
    pass->stations_pos = (GLfloat*) malloc(station_count * 2 * sizeof(GLfloat));
    if(pass->stations_pos == NULL) {
        free(pass->ids);
        pass->ids = NULL;
        CLOSE_STREAM_ON_FAILURE(stream, 1, 1, "Unable to allocate memory for station coords.");
    }
    size_t idx = 0;
    while((flag = getline(&line, &len, stream)) != -1) {
        unsigned int comment = 1;
        size_t i;
        for(i = 0; i < len; ++i) {
            switch(line[i]) {
                case ' ':
                case '\t': break;
                case '*':
                case '\r':
                case '\n': goto station_parse_start;
                default:
                    comment = 0;
                    goto station_parse_start;
            }
        } 
station_parse_start:
        if(comment && file_size > ftell(stream)) continue;
        char id[3];
        float lam, phi;
        int ret = sscanf(line, "%2s %*s %*f %*f %*f %*d %f %f %*s", id, &lam, &phi);
        if(ret != 3) {
            free(pass->ids); pass->ids = NULL;
            free(pass->stations_pos); pass->stations_pos = NULL;
            CLOSE_STREAM_ON_FAILURE(stream, 1, 1, "Position catalog has invalid entries.");
        }
        pass->ids[idx][0] = id[0];
        pass->ids[idx][1] = id[1];
        pass->stations_pos[idx * 2 + 0] = (GLfloat) lam;
        pass->stations_pos[idx * 2 + 1] = (GLfloat) (90.f - phi);
        idx++;
        if(file_size - ftell(stream) == 0) {
            flag = 0;
            break;
        }
    }
    free(line);
    CLOSE_STREAM_ON_FAILURE(stream, flag == -1, 1, "Failed to parse stations.");
    pass->station_count = station_count;
    return StationPass_configure_buffers(pass, desc);
}

#endif /* STATIONS_H */