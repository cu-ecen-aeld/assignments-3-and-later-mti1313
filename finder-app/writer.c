#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
    // Check if all required arguments are provided
    if (argc != 3) {
        printf("Error: Two arguments are required.\n");
        return 1;
    }

    char *writefile = argv[1];
    char *writestr = argv[2];

    // Open the file for writing
    FILE *file = fopen(writefile, "w");
    if (file == NULL) {
        printf("Error: The file could not be opened.\n");
        return 1;
    }

    // Write the content to the file
    fprintf(file, "%s", writestr);

    // Close the file
    fclose(file);

    // Log the message using syslog
    openlog("writer", LOG_PID, LOG_USER);
    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
    closelog();

    return 0;
}