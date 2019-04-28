#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

const size_t buffer_size = 0x80000;
const uint64_t split_size = 0xFFFC0000ULL;

const char *get_file_ext(const char *filename) {
    return strrchr(filename, '.');
}

const char *get_file_name(const char *filename) {
    const char *n = strrchr(filename, '/');
#ifdef _WIN32
    const char *n2 = strrchr(filename, '\\');
    if (n2 > n) n = n2;
    n2 = strrchr(filename, ':');
    if (n2 > n) n = n2;
#endif
    return n == NULL ? filename : (n + 1);
}

int main(int argc, char *argv[]) {
    const char *ext;
    FILE *f;
    uint8_t type;
    uint64_t units;
    uint64_t cart_size;
    uint64_t file_size;
    uint64_t data_size;

    /* options */
    int check_extra_data = 0;
    int split = 1;

    ext = get_file_ext(argv[1]);
    if (strcasecmp(ext, ".xci") != 0) return -1;
    ext = get_file_ext(argv[2]);
    if (strcasecmp(ext, ".xci") == 0) split = 0;
    f = fopen(argv[1], "rb");
    fseeko(f, 0, SEEK_END);
    file_size = ftello(f);
    fseeko(f, 0x10D, SEEK_SET);
    fread(&type, 1, 1, f);
    switch(type) {
        case 0xFA:
            cart_size = 952ULL * 0x100000ULL;
            break;
        case 0xF8:
            cart_size = 1904ULL * 0x100000ULL;
            break;
        case 0xF0:
            cart_size = 3808ULL * 0x100000ULL;
            break;
        case 0xE0:
            cart_size = 7616ULL * 0x100000ULL;
            break;
        case 0xE1:
            cart_size = 15232ULL * 0x100000ULL;
            break;
        case 0xE2:
            cart_size = 30464ULL * 0x100000ULL;
            break;
    }
    fseeko(f, 0x118, SEEK_SET);
    fread(&units, 8, 1, f);
    data_size = 0x200ULL + units * 0x200ULL;
    printf("Cart size: %16llu\n", cart_size);
    printf("File size: %16llu\n", file_size);
    printf("Data size: %16llu\n", data_size);
    if (check_extra_data) {
        uint64_t pos;
        uint8_t *buffer;
        pos = data_size;
        fseeko(f, pos, SEEK_SET);
        buffer = (uint8_t*)malloc(buffer_size);
        while (1) {
            int i;
            int bytes_read = fread(buffer, 1, buffer_size, f);
            if (bytes_read <= 0) break;
            for (i = 0; i < bytes_read; ++i) {
                if (buffer[i] != 0xFF) {
                    printf("Found data %02X @ position: 0x%llX\n", buffer[i], pos + i);
                }
            }
            pos += buffer_size;
        }
        free(buffer);
    }
    {
        uint8_t *buffer;
        int curr_index = 0;
        uint64_t left = data_size;
        fseeko(f, 0, SEEK_SET);
        buffer = (uint8_t*)malloc(buffer_size);
        while(left > 0) {
            char curr_filename[512];
            FILE *fo;
            char path[512];
            int bytes;
            double percent = -1.;
            uint64_t part = split && left > split_size ? split_size : left;
            uint64_t left2 = part;
            strcpy(path, argv[2]);
            if (split) {
                strcpy(curr_filename, get_file_name(argv[1]));
                curr_filename[strlen(curr_filename) - 1] = '0' + curr_index;
                strcat(path, curr_filename);
            }
            fo = fopen(path, "wb");
            if (fo == NULL) {
                fprintf(stderr, "Unable to create file %s! Exiting...\n", path);
                break;
            }
            left -= left2;
            while (left2 > 0) {
                double newpercent;
                bytes = fread(buffer, 1, buffer_size > left2 ? (size_t)left2 : buffer_size, f);
                if (bytes <= 0) break;
                fwrite(buffer, 1, bytes, fo);
                left2 -= (uint64_t)bytes;
                newpercent = 100. * (part - left2) / part;
                if (newpercent - percent >= 0.1) {
                    printf("\rWriting to %s - %4.1f%% %llu/%llu", path, newpercent, part - left2, part);
                    percent = newpercent;
                }
            }
            printf("\rWriting to %s - %4.1f%% %llu/%llu\n", path, 100., part, part);
            fclose(fo);
            ++curr_index;
        }
        free(buffer);
    }
    fclose(f);
    return 0;
}
