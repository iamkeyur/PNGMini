#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>

typedef struct {
    /** @Length: 4-byte representing the number of bytes in the chunk's data field */
    unsigned char* length;
    /** @ChunkType: 4-byte chunk type code */
    unsigned char* chunk_type;
    /**
     * @Data: The data bytes appropriate to the chunk type, if any.
     * This field can be of zero length.
     */
    unsigned char* data;
    /** @CRC: A 4-byte CRC (Cyclic Redundancy Check) */
    unsigned char* crc;
} chunk;

static unsigned int read32bitInt(const unsigned char* Buffer) {
    return (((unsigned int)Buffer[0] << 24u) | ((unsigned int)Buffer[1] << 16u) |
        ((unsigned int)Buffer[2] << 8u) | (unsigned int)Buffer[3]);
}

static void getChunkType(char Type[5], unsigned char* ChunkType) {
    unsigned i;
    for (i = 0; i != 4; ++i) Type[i] = (char)ChunkType[i];
    Type[4] = 0;
}

static unsigned char isChunkTypeEqualTo(const unsigned char* ChunkType,
    const char* Type) {
    if (strlen(Type) != 4) return 0;
    return (ChunkType[0] == Type[0] && ChunkType[1] == Type[1] &&
        ChunkType[2] == Type[2] && ChunkType[3] == Type[3]);
}

static unsigned char isCriticalChunk(const unsigned char* ChunkType) {
    return isChunkTypeEqualTo(ChunkType, "IHDR") ||
           isChunkTypeEqualTo(ChunkType, "IDAT") ||
           isChunkTypeEqualTo(ChunkType, "PLTE") ||
           isChunkTypeEqualTo(ChunkType, "IEND");
}

static unsigned char writeChunk(FILE** out, chunk* c) {
    if (fwrite(c->length, 1, 4, *out) != 4) return 0;
    if (fwrite(c->chunk_type, 1, 4, *out) != 4) return 0;
    unsigned int dataLength = read32bitInt(c->length);
    if (dataLength != 0) {
        if (fwrite(c->data, 1, dataLength, *out) != dataLength) return 0;
    }
    if (fwrite(c->crc, 1, 4, *out) != 4) return 0;
    return 1;
}

static unsigned char readChunk(FILE** file, FILE** out, chunk* c) {
    unsigned char* length =
        (unsigned char*)malloc(sizeof(4 * sizeof(unsigned char)));

    if (!length) return false;

    size_t readsize;

	/* Reading data length*/
    readsize = fread(length, 1, 4, *file);
    if (readsize != 4) return false;

    /**
     * https://stackoverflow.com/questions/7059299/how-to-properly-convert-an-unsigned-char-array-into-an-uint32-t
     */
    unsigned int dataLength = read32bitInt(length);

    c->crc = (unsigned char*)malloc(4);
    c->chunk_type = (unsigned char*)malloc(4 );
    c->length = (unsigned char*)malloc(4);
    if (NULL == c->chunk_type || NULL == c->crc || NULL == c->length)
        return false;

    memcpy(c->length, length, 4);
	/* Reading chunk type*/
    readsize = fread(c->chunk_type, 1, 4, *file);
    if (readsize != 4) return false;
	/* Reading chunk data */
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
        std::cout << "usage: " << argv[0] << " <input.png> <output.png>\n";
    }
    else {
        std::cout << argv[1] << std::endl;
        FILE* file;
        errno_t err = fopen_s(&file, argv[1], "rb");
        if (!err && file != NULL) {
            unsigned char* header = new unsigned char[8];
            size_t readsize;
			/* Reading PNG header*/
            readsize = fread(header, 1, 8, file);
            if (readsize != 8) return 78;
            /**
             * Check for PNG Header
             * The first eight bytes of a PNG file always contain the following values:
             */
            if (!(header[0] == 0x89 && header[1] == 0x50 && header[2] == 0x4e &&
                header[3] == 0x47 && header[4] == 0x0d && header[5] == 0x0a &&
                header[6] == 0x1a && header[7] == 0x0a)) {
                exit(EXIT_FAILURE);
            }

            FILE* out;
            errno_t err = fopen_s(&out, out_file.c_str(), "wb");
            if (out == NULL || err) return 79;

            fwrite(header, 1, 8, out);

            unsigned char ret;
            int c;
            while ((c = fgetc(file)) != EOF) {
                ungetc(c, file);
                chunk* c = (chunk*)malloc(sizeof(chunk));
                if (NULL == c) exit(EXIT_FAILURE);
                ret = readChunk(&file, &out, c);
                if (ret) {
                    if (isCriticalChunk(c->chunk_type))
                        if (!writeChunk(&out, c)) exit(EXIT_FAILURE);
                    if (read32bitInt(c->length) != 0) {
                        free(c->data);
                    }
                    free(c->length);
                    free(c->chunk_type);
                    free(c->crc);
                    free(c);
                }
                else {
                    std::cout << "Error occured while reading chunk\n";
                    exit(EXIT_FAILURE);
                }
            }

            fclose(file);
            fclose(out);
        }
        else {
            std::cout << "Unable to open file";
            return 78;
        }
    }
}
