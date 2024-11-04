/*
 * Author: dofahy
 * Date: 30/08/2024
 *
 * Description: This program compares system memory size reported by 
 * various commands on Solaris SPARC. It retrieves the total system memory size using
 * sysconf, extracts memory information from lgrpinfo, and reads
 * memory size from prtconf. The program then computes and displays
 * the memory sizes from these sources and calculates the difference
 * between them to validate their accuracy.
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#define BUFFER_SIZE 256
// Function to convert memory string (e.g., 4.0G, 512M) to bytes
long long memory_to_bytes(const char *mem_str) {
    double value;
    char unit;
    long long bytes = 0;
    if (sscanf(mem_str, "%lf%c", &value, &unit) == 2) {
        switch (unit) {
            case 'G': case 'g':
                bytes = (long long)(value * 1024 * 1024 * 1024);
                break;
            case 'M': case 'm':
                bytes = (long long)(value * 1024 * 1024);
                break;
            case 'K': case 'k':
                bytes = (long long)(value * 1024);
                break;
            default:
                bytes = (long long)value;
        }
    }
    return bytes;
}
// Function to convert memory size in megabytes to bytes
long long megabytes_to_bytes(const char *mb_str) {
    double value;
    if (sscanf(mb_str, "%lf", &value) == 1) {
        return (long long)(value * 1024 * 1024);
    }
    return 0;
}
// Function to get memory size from lgrpinfo for the root lgroup
long long get_root_lgroup_memory() {
    FILE *fp;
    char buffer[BUFFER_SIZE];
    long long root_memory_bytes = 0;
    int root_lgroup_found = 0;
    fp = popen("lgrpinfo -a", "r");
    if (fp == NULL) {
        perror("popen failed");
        return 0;
    }
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Check for the start of root lgroup section
        if (strstr(buffer, "lgroup 0 (root)") != NULL) {
            root_lgroup_found = 1;
        }
        if (root_lgroup_found) {
            if (strstr(buffer, "Memory: installed") != NULL) {
                char size_str[64];
                char *start = strstr(buffer, "installed");
                if (start) {
                    start += strlen("installed") + 1;
                    sscanf(start, "%63s", size_str);
                    root_memory_bytes = memory_to_bytes(size_str);
                }
                break; // Stop processing after finding root memory
            }
        }
    }
    pclose(fp);
    return root_memory_bytes;
}
// Function to get memory size from prtconf
long long get_prtconf_memory() {
    FILE *fp;
    char buffer[BUFFER_SIZE];
    long long prtconf_memory_bytes = 0;
    fp = popen("prtconf -v | grep 'Memory size'", "r");
    if (fp == NULL) {
        perror("popen failed");
        return 0;
    }
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        char size_str[64];
        if (sscanf(buffer, "Memory size: %63s", size_str) == 1) {
            prtconf_memory_bytes = megabytes_to_bytes(size_str);
            break; // Stop processing after finding the memory size
        }
    }
    pclose(fp);
    return prtconf_memory_bytes;
}
// Function to get total system memory size using sysconf
long long get_sysconf_memory() {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (long long)pages * page_size;
}
int main() {
    long long root_lgroup_memory = get_root_lgroup_memory();
    long long prtconf_memory = get_prtconf_memory();
    long long sysconf_memory = get_sysconf_memory();
    long long difference_root = sysconf_memory - root_lgroup_memory;
    long long difference_prtconf = sysconf_memory - prtconf_memory;
    // Output the results
    printf("Root lgroup memory size: %lld bytes\n", root_lgroup_memory);
    printf("prtconf memory size: %lld bytes\n", prtconf_memory);
    printf("Total system memory size (sysconf): %lld bytes\n", sysconf_memory);
    printf("Difference between sysconf and lgroup memory size: %lld bytes\n", difference_root);
    printf("Difference between sysconf and prtconf memory size: %lld bytes\n", difference_prtconf);
    return 0;
}
