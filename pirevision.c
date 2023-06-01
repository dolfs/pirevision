/*
 * Convert Raspberry Pi revision codes (hex) to readable interpretive text.
 *
 * Copyright (C) 2023 Dolf Starreveld
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

typedef unsigned int revcode_32;

#define ARRAY_CNT(a) (sizeof(a) / sizeof(a[0]))

#define SONY_UK     (0 << 16)
#define EGOMAN      (1 << 16)
#define EMBEST      (2 << 16)
#define SONY_JAPAN  (3 << 16)
#define STADIUM     (5 << 16)
/*
 * Special index because not used in new style.
 * Problem if newstyle starts using this index.
 */
#define QISDA       (0xF << 16)

#define MEM_256M    (0 << 20)
#define MEM_512M    (1 << 20)
#define MEM_1G      (2 << 20)
#define MEM_2G      (3 << 20)
#define MEM_4G      (4 << 20)
#define MEM_8G      (5 << 20)

#define MODEL_A     (0 << 4)
#define MODEL_B     (1 << 4)
#define MODEL_APLUS (2 << 4)
#define MODEL_BPLUS (3 << 4)
#define MODEL_CM1   (6 << 4)

#define REV_1_0     (0 << 0)
#define REV_1_1     (0 << 0)
#define REV_1_2     (0 << 0)
#define REV_1_3     (0 << 0)
#define REV_1_4     (0 << 0)
#define REV_1_5     (0 << 0)
#define REV_2_0     (0xF << 0)  /* Special case highest unused index */

#define OLD_REV_NOT_VALID 0xFFFFFFFF

/**
 * Return the entry from a lookup table, indexed as specified.
 *
 * The index is checked against the count and if valid, the lookup result will
 * be returned. If the index is out of bounds and a `invalid_index_str` is
 * specified, AND if the index is equal to the `invalid_index` than this
 * substitute string is returned. Otherwise "???" is returned.
 *
 * @param lut The lookup table (const char *array[])
 * @param lut_count Number of entries in lut
 * @param index Index for lookup
 * @param invalid_index Index which, if given, is considered invalid and will
 *                      cause the substitute string to be used (if not NULL)
 * @param invalid_index_str String to be used if supplied index equal to
 *                          invalid_index. If NULL this will not be
 *                          used or checked
 * @returns String from table, substitute invalid string, or "???"
 */
const char *
lut_to_str_with_invalid(const char *lut[],
                        const int lut_count,
                        const int index,
                        const int invalid_index,
                        const char *invalid_index_str)
{
    if (index < lut_count) {
        return lut[index];
    }
    if ((invalid_index_str != NULL) && (index == invalid_index)) {
        return invalid_index_str;
    }
    return "???";
}

/**
 * Return the entry from a lookup table, indexed as specified.
 *
 * The index is checked against the count and if valid, the lookup result will
 * be returned. Otherwise "???" is returned.
 *
 * @param lut   The lookup table (const char *array[])
 * @param lut_count Number of entries in lut
 * @param index Index for lookup
 * @returns String from table, or "???"
 */
const char *
lut_to_str(const char *lut[],
           const int lut_count,
           const int index)
{
    return lut_to_str_with_invalid(lut,
                                   lut_count,
                                   index,
                                   0,
                                   NULL); // No invalid substitution!
}

int
overvoltage_allowed(const revcode_32 revision_code)
{
    // NOTE: 0 means allowed, 1 means disallowed
    return (revision_code >> 31 & 0x1) == 0;
}

const char *
overvoltage_allowed_str(const revcode_32 revision_code)
{
    return overvoltage_allowed(revision_code) ? "Allowed" : "Disallowed";
}

int
otp_programming_allowed(const revcode_32 revision_code)
{
    // NOTE: 0 means allowed, 1 means disallowed
    return (revision_code >> 30 & 0x1) == 0;
}

int
otp_reading_allowed(const revcode_32 revision_code)
{
    // NOTE: 0 means allowed, 1 means disallowed
    return (revision_code >> 29 & 0x1) == 0;
}

const char *
otp_programming_allowed_str(const revcode_32 revision_code)
{
    return otp_programming_allowed(revision_code) ? "Allowed" : "Disallowed";
}

const char *
otp_reading_allowed_str(const revcode_32 revision_code)
{
    return otp_reading_allowed(revision_code) ? "Allowed" : "Disallowed";
}

int
warranty_intact(const revcode_32 revision_code)
{
    // NOTE: 0 means intact, 1 means voided
    return (revision_code >> 25 & 0x1) == 0;
}

const char *
warranty_intact_str(const revcode_32 revision_code)
{
    return warranty_intact(revision_code) ? "Intact" : "Voided";
}

int
revision_new_style(const revcode_32 revision_code)
{
    // NOTE: 1 means new style, 0 means old style
    return (revision_code >> 23 & 0x1) == 1;
}

unsigned int
type_index(const revcode_32 revision_code)
{
    return (revision_code >> 4) & 0xFF;
}

const char *
type_str(const revcode_32 revision_code)
{
    static const char *type_map[] = {
        "A",
        "B",
        "A+",
        "B+",
        "2B",
        "Alpha",
        "CM1",
        "0x07",
        "3B",
        "Zero",
        "CM3",
        "0x0B",
        "Zero W",
        "3B+",
        "3A+",
        "Internal use only",
        "CM3+",
        "4B",
        "Zero 2 W",
        "400",
        "CM4",
        "CM4S",
        /* Lots of room for future: 256 entries */
    };
    return lut_to_str(type_map, ARRAY_CNT(type_map), type_index(revision_code));
}

unsigned int
physical_memory_index(const revcode_32 revision_code)
{
    return (revision_code >> 20) & 0x7;
}

/**
 * Return physical amount of memory in MB.
 */
unsigned int
physical_memory_mbytes(const revcode_32 revision_code)
{
    // 1GB is 0x4000_0000 (fits 32 bits)
    // 2GB is 0x8000_0000 (fits 32 bits)
    // 8GB is 0x1_0000_0000 does not fit 32 bits)
    // Therefore we return in units of MB which removes the need for the last
    // 20 bits, making this fit (8GB = 8192MB = 0x2000).
    // We have to care because on 32-bit systems (Raspberry) this long is the
    // same size as int, which is 32-bits and the 8GB entry would overflow.
    static unsigned int mem_mbytes_map[] = {
             256, /* 0 */
             512, /* 1 */
        1 * 1024, /* 2 */
        2 * 1024, /* 3 */
        4 * 1024, /* 4 */
        8 * 1024, /* 5 */
        /* 6 and 7 still available for future use */
    };
    const int index = physical_memory_index(revision_code);
    return (index > ARRAY_CNT(mem_mbytes_map)) ? 0 : mem_mbytes_map[index];
}

/**
 * Return string version expressing amount of physical memory.
 *
 * The amount is expressed in GB if there is 1GB or more memory, otherwise
 * it is expressed in MB. Fractional amounts are being ignored, so for
 * example 3.5GB is reported as 3GB. For normal Raspberry devices this is
 * a non-issue as such fractional amounts are not used anyway.
 *
 * The string is formatted as "DDDSB" and null terminated (total 7 bytes
 * or less). The amount is expressed in MB (last two characters) if the
 * total amount is less than 1GB. Thus the string ends in MB or GB. The "DDDD"
 * portion is the numeric value without leading zeros are space padding.
 * Therefore, 512MB is reported as "512MB" and not as "0512MB", nor as " 512MB".
 *
 * If the result bbuffer is not large enough to receive the result, including
 * a terminating null character, it will receive as much is will fit, possibly
 * leaving out a terminating null character. To avoid this the size should be
 * at least 7 bytes.
 *
 * @param revision_code The Raspberry revision code from which the memory
 *                      configuration is to be extracted
 * @param result Pointer to a memory buffer that will receive the result
 * @param result_size Maximum length of result buffer
 * @returns Pointer to result string (buffer)
 */
char *
physical_memory_str(const revcode_32 revision_code,
                    char *result,
                    const size_t result_size)
{
    char result_str[4 + 2 + 1] = { '\0' }; // Large enough for DDDDSB\0
    const unsigned int mega_bytes = physical_memory_mbytes(revision_code);
    if (mega_bytes >= 1024) {   // 1GB or more
        // Compute multiples of GB
        unsigned int giga_bytes = mega_bytes >> 10;
        // Check so we can honor format promise, return empty string otherwise
        if (giga_bytes <= 9999) {
            sprintf(result_str, "%dGB", giga_bytes);
        }
    }
    else {
        // We have < 1024MB), report straight in MB
        sprintf(result_str, "%dMB", mega_bytes);
    }
    strncpy(result, result_str, result_size - 1);
    if (result_size > 0) {
        result[result_size - 1] = '\0';
    }
    return result;
}

unsigned int
processor_index(const revcode_32 revision_code)
{
    return (revision_code >> 12) & 0xF;
}

const char *
processor_str(const revcode_32 revision_code)
{
    static const char *processor_map[] = {
        "BCM2835", /* 0 */
        "BCM2836", /* 1 */
        "BCM2837", /* 2 */
        "BCM2711", /* 3 */
        /* Entries 4-15  still available for future use */
    };
    return lut_to_str(processor_map,
                      ARRAY_CNT(processor_map),
                      processor_index(revision_code));
}

unsigned int
manufacturer_index(const revcode_32 revision_code)
{
    return (revision_code >> 16) & 0xF;
}

const char *
manufacturer_str(const revcode_32 revision_code)
{
    static const char *manufacturer_map[] = {
        "Sony UK",      /* 0 */
        "Egoman",       /* 1 */
        "Embest",       /* 2 */
        "Sony Japan",   /* 3 */
        "Embest",       /* 4 */
        "Stadium",      /* 5 */
        /* Entries 6-14  still available for future use */
        /* Index 15 used by this program for special purpose so a problem
         * will arise if Raspberry starts to use that index.
         */
    };
    const int index = manufacturer_index(revision_code);
    return lut_to_str_with_invalid(manufacturer_map,
                                   ARRAY_CNT(manufacturer_map),
                                   index,
                                   (QISDA >> 16),
                                   "Qisda");
}

unsigned int
revision_index(const revcode_32 revision_code)
{
    return (revision_code >> 0) & 0xF;
}

const char *
revision_str(const revcode_32 revision_code)
{
    static const char *revision_map[] = {
        "1.0", /* 0 */
        "1.1", /* 1 */
        "1.2", /* 2 */
        "1.3", /* 3 */
        "1.4", /* 4 */
        "1.5", /* 5 */
        /* Entries 6-14  still available for future use */
        /* Index 15 used by this program for special purpose so a problem
         * will arise if Raspberry starts to use that index.
         */
    };
    const int index = revision_index(revision_code);
    return lut_to_str_with_invalid(revision_map,
                                   ARRAY_CNT(revision_map),
                                   index,
                                   (REV_2_0 >> 0),
                                   "2.0");
}

revcode_32
map_old_to_new(const revcode_32 revision_code)
{
    // Map old style revisions to new style
    static int old_revision_map[] = {
        /* 0000 */  OLD_REV_NOT_VALID,
        /* 0001 */  OLD_REV_NOT_VALID,
        /* 0002 */  MODEL_B | REV_1_0 | MEM_256M | EGOMAN,
        /* 0003 */  MODEL_B | REV_1_0 | MEM_256M | EGOMAN,
        /* 0004 */  MODEL_B | REV_2_0 | MEM_256M | SONY_UK,
        /* 0005 */  MODEL_B | REV_2_0 | MEM_256M | QISDA,
        /* 0006 */  MODEL_B | REV_2_0 | MEM_256M | EGOMAN,
        /* 0007 */  MODEL_A | REV_2_0 | MEM_256M | EGOMAN,
        /* 0008 */  MODEL_A | REV_2_0 | MEM_256M | SONY_UK,
        /* 0009 */  MODEL_A | REV_2_0 | MEM_256M | QISDA,
        /* 000a */  OLD_REV_NOT_VALID,
        /* 000b */  OLD_REV_NOT_VALID,
        /* 000c */  OLD_REV_NOT_VALID,
        /* 000d */  MODEL_B | REV_2_0 | MEM_512M | EGOMAN,
        /* 000e */  MODEL_B | REV_2_0 | MEM_512M | SONY_UK,
        /* 000f */  MODEL_B | REV_2_0 | MEM_512M | EGOMAN,
        /* 0010 */  MODEL_BPLUS | REV_1_2 | MEM_512M | SONY_UK,
        /* 0011 */  MODEL_CM1 | REV_1_0 | MEM_512M | SONY_UK,
        /* 0012 */  MODEL_APLUS | REV_1_1 | MEM_256M | SONY_UK,
        /* 0013 */  MODEL_BPLUS | REV_1_2 | MEM_512M | EMBEST,
        /* 0014 */  MODEL_CM1 | REV_1_0 | MEM_512M | EMBEST,
        // This next model comes with 256MB or 512MB, so under report is our
        // effort because we have to choose just one (there are not separate
        // old style revision codes for this!
        /* 0015 */  MODEL_APLUS | REV_1_1 | MEM_256M | EMBEST,
    };

    revcode_32 new_revision_code = revision_code;
    if (!revision_new_style(new_revision_code)) {
        new_revision_code = (new_revision_code < ARRAY_CNT(old_revision_map))
                    ? old_revision_map[new_revision_code]
                    : OLD_REV_NOT_VALID;
        if (new_revision_code == OLD_REV_NOT_VALID) {
            fprintf(stderr, "Invalid old style revision!\n");
            exit(EXIT_FAILURE);
        }
    }
    return new_revision_code;
}

int
print_revision_text(const revcode_32 revision_code)
{
    printf("Revision code 0x%0X interpreted:\n", revision_code);
    const char *field_format = "    %-16s: %s\n";

    revcode_32 code = map_old_to_new(revision_code);
    int new_style = revision_new_style(code);

    printf(field_format, "Style", new_style ? "New" : "Old");
    if (new_style) {
        printf(field_format, "Overvoltage", overvoltage_allowed_str(code));
        printf(field_format, "OTP Programming",
               otp_programming_allowed_str(code));
        printf(field_format, "OTP Reading", otp_reading_allowed_str(code));
        printf(field_format, "Warranty", warranty_intact_str(code));
    }

    printf(field_format, "Type/Model", type_str(code));
    printf(field_format, "Revision", revision_str(code));

    if (new_style) {
        printf(field_format, "Processor/SOC", processor_str(code));
    }

    char str[8];
    printf(field_format, "Memory", physical_memory_str(code, str, sizeof(str)));
    printf(field_format, "Manufacturer", manufacturer_str(code));

    return EXIT_SUCCESS;
}

const char *
bool_json(const int value)
{
    return value ? "true" : "false";
}

int
print_revision_json(const revcode_32 revision_code)
{
    const char *field_format = "    %s: %s%s\n";
    const char *str_field_format = "    %s: \"%s\"%s\n";

    revcode_32 code = map_old_to_new(revision_code);
    int new_style = revision_new_style(code);

    printf("{\n");
    char hex_revision[20];
    sprintf(hex_revision, "0x%0X", revision_code);
    printf(str_field_format, "revision_code", hex_revision, ",");
    printf(str_field_format, "style", new_style ? "new" : "old", ",");
    if (new_style) {
        printf(field_format, "overvoltage_allowed",
               bool_json(overvoltage_allowed(code)),
               ",");
        printf(field_format, "otp_programming_allowed",
               bool_json(otp_programming_allowed(code)),
               ",");
        printf(field_format, "otp_reading_allowed",
               bool_json(otp_reading_allowed(code)),
               ",");
        printf(field_format, "warranty_intact",
               bool_json(warranty_intact(code)),
               ",");
    }

    printf(str_field_format, "type", type_str(code), ",");
    printf(str_field_format, "revision", revision_str(code), ",");

    if (new_style) {
        printf(str_field_format, "processor", processor_str(code), ",");
    }

    char str[8];
    printf(str_field_format, "memory", physical_memory_str(code, str, sizeof(str)), ",");
    printf(str_field_format, "manufacturer", manufacturer_str(code), "");
    printf("}\n");
    return EXIT_SUCCESS;
}

revcode_32
str_to_revision(const char *input)
{
    errno = 0;
    char *endptr;
    // On 32-bit systems long might be same size as int, but that would still be
    // 32-bits, which is enough for our purpose
    unsigned long value = strtoul(input, &endptr, 16);
    if (errno == ERANGE) {
        fprintf(stderr, "Revision code \"%s\" too large or too small\n", input);
        exit(EXIT_FAILURE);
    }
    if ((errno != 0) || (endptr == input)) {
        fprintf(stderr, "Could not parse revision code \"%s\"\n", input);
        exit(EXIT_FAILURE);
    }
    if ((sizeof(long) > sizeof(int)) && (value > 0xFFFFFFFF)) {
        fprintf(stderr, "Revision code \"%s\" (%lx) larger than 32 bits\n",
                input,
                value);
        exit(EXIT_FAILURE);
    }
    return (revcode_32) value;
}

int
process_rev_codes(const char **codes,
                  const int code_count,
                  const int print_json)
{
    char rev_code_str[32] = { '\0' };
    int exit_status = EXIT_SUCCESS;

    // Process all extra args as revision codes
    for (int index = 0;
         index < code_count && (exit_status == 0); ++index) {
        rev_code_str[0] = '\0';
        strncpy(rev_code_str, codes[index], sizeof(rev_code_str) - 1);
        rev_code_str[sizeof(rev_code_str) - 1] = '\0';
        revcode_32 revision_code = str_to_revision(rev_code_str);
        if (print_json) {
            exit_status = print_revision_json(revision_code);
        }
        else {
            exit_status = print_revision_text(revision_code);
        }
    }
    return exit_status;
}

int
read_proc_cpuinfo(char *buffer, const size_t buffer_size)
{
    if (buffer_size <= 0) {
        return EXIT_FAILURE;
    }

    const char *cpuinfo = "/proc/cpuinfo";
    FILE *fp = fopen(cpuinfo, "rt");

    if (fp == NULL) {
        fprintf(stderr, "Could not open %s\n", cpuinfo);
        return EXIT_FAILURE;
    }

    char line[256];
    char format[32];// Creating limiting format to avoid buffer overflow

    snprintf(format,
             sizeof(format), "Revision : %%%ds",
             (int)(buffer_size - 1));

    buffer[0] = '\0';
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, format, buffer) == 1)
           break;
    }
    fclose(fp);
    return EXIT_SUCCESS;
}

int
process_proc_cpuinfo(const int print_json)
{
    char rev_code_str[32] = { '\0' };
    if (read_proc_cpuinfo(rev_code_str, sizeof(rev_code_str)) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    const char *codes[] = { rev_code_str };
    return process_rev_codes(codes, 1, print_json);
}

/**
 * Usage: pirevision [-j|--json] [revision code...]
 *
 * -j flag causes JSON output instead of text
 * If no revision code(s) supplied, attempt to get it from /proc/cpuinfo and
 * use that, if succesful. Otherwise process each argument as a separate
 * revision code. These must be specified as hexadecimal codes, with, or without
 * 0x or 0X prefix.
 */
int
main(const int argc, const char *argv[])
{
    int print_json = 0;
    int first_code_index = 1;

    if (argc > 1) {
        if ((strcmp(argv[1], "-j") == 0) || (strcmp(argv[1], "--json") == 0)) {
            print_json = 1;
            first_code_index++;
        }
    }

    // If no extra args, attempt to read from /proc/cpuinfo
    if (first_code_index >= argc) {
        return process_proc_cpuinfo(print_json);
    }
    return process_rev_codes(&argv[first_code_index],
                             argc - first_code_index,
                             print_json);
}
