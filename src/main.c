#include "cli.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    CliContext ctx;
    cli_init(&ctx);

    if (argc >= 2) {
        printf("[INFO] Loading initial file from command line argument: %s\n", argv[1]);
        cli_load_file(&ctx, argv[1], stdout);
    }

    int exit_code = cli_run(&ctx);

    cli_free(&ctx);
    return exit_code;
}
