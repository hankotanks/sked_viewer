#ifndef STATIONS_H
#define STATIONS_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "./util/log.h"
#include "camera.h"

typedef struct {
    char (*ids)[2];
    GLfloat* pos;
    size_t station_count;
} Catalog;

void Catalog_free(Catalog cat) {
    free(cat.ids);
    free(cat.pos);
}

Catalog Catalog_parse_from_file(const char* raw) {
    Catalog cat;
    cat.station_count = 0;
    FILE* stream;
    stream = fopen(raw, "rb");
    if(stream == NULL) {
        LOG_ERROR("Station position catalog couldn't be opened.");
        return cat;
    }
    int failure;
    failure = fseek(stream, 0L, SEEK_END);
    CLOSE_STREAM_ON_FAILURE(stream, failure, cat, "Failed to seek to end of file.");
    long file_size = ftell(stream);
    CLOSE_STREAM_ON_FAILURE(stream, file_size < 0, cat, "File size is invalid.");
    failure = fseek(stream, 0L, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, cat, "Failed to seek to beginning of file.");
    ssize_t flag;
    char* line = NULL;
    size_t len = 0;
    size_t station_count = 0;
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
    CLOSE_STREAM_ON_FAILURE(stream, flag == -1, cat, "Failed to count stations before parsing.");
    failure = fseek(stream, 0L, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, cat, "Failed to seek to beginning of catalog.");
    cat.ids = (char (*)[2]) malloc(station_count * sizeof(*(cat.ids)));
    CLOSE_STREAM_ON_FAILURE(stream, cat.ids == NULL, cat, "Unable to allocate memory for station ids.");
    cat.pos = (GLfloat*) malloc(station_count * 2 * sizeof(GLfloat));
    if(cat.pos == NULL) {
        free(cat.ids);
        cat.ids = NULL;
        CLOSE_STREAM_ON_FAILURE(stream, 1, cat, "Unable to allocate memory for station coords.");
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
            free(cat.ids); cat.ids = NULL;
            free(cat.pos); cat.pos = NULL;
            CLOSE_STREAM_ON_FAILURE(stream, 1, cat, "Position catalog has invalid entries.");
        }
        cat.ids[idx][0] = id[0];
        cat.ids[idx][1] = id[1];
        cat.pos[idx * 2 + 0] = (GLfloat) (lam - 5.f) * M_PI / 180.f; // TODO: Not sure why this 5 degree offset is required
        cat.pos[idx * 2 + 1] = (GLfloat) (180.f - (phi + 90.f)) * M_PI / 180.f;
        idx++;
        if(file_size - ftell(stream) == 0) {
            flag = 0;
            break;
        }
    }
    CLOSE_STREAM_ON_FAILURE(stream, flag == -1, cat, "Failed to parse stations.");
    cat.station_count = station_count;
    return cat;
}

typedef struct {
    Catalog stations;
    GLuint VAO, VBO, shader_program;
} StationPass;

void StationPass_free(StationPass pass) {
    Catalog_free(pass.stations);
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
    glDrawArrays(GL_POINTS, 0, (GLsizei) pass.stations.station_count);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
}

typedef struct {
    float globe_radius;
    GLuint shader_lat_lon;
    const char* path_frag_shader;
    const char* path_pos_catalog;
} StationPassDesc;

// StationPass does not 'own' its shaders in the same way that GlobePass does.
// It is not its responsibility to free them (yet).
unsigned int StationPass_init(StationPass* pass, StationPassDesc desc) {
    unsigned int failure;
    Catalog cat;
    cat = Catalog_parse_from_file(desc.path_pos_catalog);
    if(cat.station_count == 0) return 1;
    // configure station shaders
    GLuint frag;
    failure = compile_shader(&frag, GL_FRAGMENT_SHADER, desc.path_frag_shader);
    if(failure) {
        Catalog_free(cat);
        return 1;
    }
    GLuint shader_program;
    failure = assemble_shader_program(&shader_program, desc.shader_lat_lon, frag);
    if(failure) {
        Catalog_free(cat);
        glDeleteShader(frag);
        return 1;
    }
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, cat.station_count * 2 * sizeof(GLfloat), cat.pos, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glUseProgram(shader_program);
    glUniform1f(glGetUniformLocation(shader_program, "globe_radius"), desc.globe_radius);
    glUseProgram(0);
    pass->VAO = VAO;
    pass->VBO = VBO;
    pass->shader_program = shader_program;
    pass->stations = cat;
    return 0;
}

#endif /* STATIONS_H */