#include <stdio.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

int main() {
    char buffer[BUFFER_SIZE];

    // Text von stdin oder Pipe lesen
    FILE *input_stream = stdin;

    // Text zeilenweise einlesen und mit Verzögerung ausgeben
    while (fgets(buffer, sizeof(buffer), input_stream) != NULL) {
        usleep(1000000);  // 100000 Mikrosekunden = 100 Millisekunden
        for (int i = 0; buffer[i] != '\0'; ++i) {
            putchar(buffer[i]);
            fflush(stdout);

            // Verzögerung von 100 Millisekunden

        }
    }

    return 0;
}
