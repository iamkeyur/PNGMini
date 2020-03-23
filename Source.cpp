#include <algorithm>
#include <sstream>
#include <iostream>
#include <iterator>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>
#include <stdlib.h>

typedef struct {
    unsigned char* length;
    unsigned char* chunk_type;
    unsigned char* data;
    unsigned char* crc;
} chunk;

static unsigned int read32bitInt(const unsigned char* buffer) {
    return (((unsigned int)buffer[0] << 24u) | ((unsigned int)buffer[1] << 16u) |
        ((unsigned int)buffer[2] << 8u) | (unsigned int)buffer[3]);
}

static void getChunkType(char type[5], unsigned char* chunkType) {
    unsigned i;
    for (i = 0; i != 4; ++i) type[i] = (char)chunkType[i];
    type[4] = 0; /*null termination char*/
}

static unsigned char isChunkTypeEqualTo(const unsigned char* ChunkType,
    const char* type) {
    if (strlen(type) != 4) return 0;
    return (ChunkType[0] == type[0] && ChunkType[1] == type[1] &&
        ChunkType[2] == type[2] && ChunkType[3] == type[3]);
}

static unsigned char isCriticalChunk(const unsigned char* ChunkType) {
    return isChunkTypeEqualTo(ChunkType, "IHDR") ||
        isChunkTypeEqualTo(ChunkType, "IDAT") ||
        isChunkTypeEqualTo(ChunkType, "PLTE") ||
        isChunkTypeEqualTo(ChunkType, "IEND");
}

static unsigned char writeChunk(FILE** out, chunk* c) {
    if (fwrite(c->length, 1, sizeof(c->length), *out) != 4) return 0;
    if (fwrite(c->chunk_type, 1, sizeof(c->chunk_type), *out) != 4) return 0;
    unsigned int dataLength = read32bitInt(c->length);
    if (dataLength != 0) {
        if (fwrite(c->data, 1, dataLength, *out) != dataLength) return 0;
    }
    if (fwrite(c->crc, 1, sizeof(c->crc), *out) != 4) return 0;
    return 1;
}

static bool readChunk(FILE** file, FILE** out, chunk* c) {
    unsigned char* length =
        (unsigned char*)malloc(sizeof(4 * sizeof(unsigned char)));
    if (!length) return false;
    size_t readsize;

    readsize = fread(length, 1, 4, *file);
    if (readsize != 4) return false;

    // https://stackoverflow.com/questions/7059299/how-to-properly-convert-an-unsigned-char-array-into-an-uint32-t
    unsigned int dataLength = read32bitInt(c->length);

    c->crc = (unsigned char*)malloc(4 * sizeof(unsigned char));
    c->chunk_type = (unsigned char*)malloc(4 * sizeof(unsigned char));
    c->length = (unsigned char*)malloc(4 * sizeof(unsigned char));
    if (NULL == c->chunk_type || NULL == c->crc || NULL == c->length)
        return false;

    memcpy(c->length, length, 4);

    readsize = fread(c->chunk_type, 1, 4, *file);
    if (readsize != 4) return false;

    if (dataLength != 0) {
        c->data = (unsigned char*)malloc(dataLength * sizeof(unsigned char));
        if (NULL == c->data) return false;
        readsize = fread(c->data, 1, dataLength, *file);
        if (readsize != dataLength) return false;
    }

    readsize = fread(c->crc, 1, 4, *file);
    if (readsize != 4) return false;

    free(length);
    return true;
}

int main(int argc, char* argv[]) {
    std::string out_file =
        "C:\\Users\\keyur\\Desktop\\Heuristic\\test_out_cmp_3.png";
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " <filename>\n";
    }
    else {
        std::cout << argv[1] << std::endl;
        FILE* file;
        errno_t err = fopen_s(&file, argv[1], "rb");
        if (!err && file != NULL) {
            unsigned char* header = new unsigned char[8];
            size_t readsize;

            readsize = fread(header, 1, 8, file);
            if (readsize != 8) return 78;
            if (!(header[0] == 0x89 && header[1] == 0x50 && header[2] == 0x4e &&
                header[3] == 0x47 && header[4] == 0x0d && header[5] == 0x0a &&
                header[6] == 0x1a && header[7] == 0x0a)) {
                exit(EXIT_FAILURE);
            }
            std::cout << '\n';
            FILE* out;
            errno_t err = fopen_s(&out, out_file.c_str(), "wb");
            if (out == NULL || err) return 79;

            fwrite(header, 1, 8, out);

            unsigned char ret;
            while (!feof(file)) {
                chunk* c = (chunk*)malloc(sizeof(chunk));
                if (NULL == c) exit(EXIT_FAILURE);
                ret = readChunk(&file, &out, c);
                if (ret) {
                    unsigned int len = read32bitInt(c->length);
                    if (isCriticalChunk(c->chunk_type) == 1)
                        if (!writeChunk(&out, c)) exit(EXIT_FAILURE);
                    if (len != 0) {
                        free(c->data);
                    }
                    free(c->length);
                    free(c->chunk_type);
                    free(c->crc);
                    free(c);
                }
            }

            std::cout << "Done";
            fclose(file);
            fclose(out);
        }
        else {
            std::cout << "Unable to open file";
            return 78;
        }
    }
}