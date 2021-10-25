/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdbool.h>
#include <stdlib.h>

#include <cpu/intc.h>
#include <vdp.h>

#include <dbgio.h>

#include <internal.h>

#define ASSERTION_MAX_COUNT 3

void __noreturn
_assert(const char * restrict file, const char * restrict line,
    const char * restrict func,
    const char * restrict failed_expr)
{
        /* In the case where we fail an assertion within _assert() */
        static uint32_t assertion_count = 0;

        if (assertion_count >= ASSERTION_MAX_COUNT) {
                abort();
        }

        if (assertion_count == 0) {
                _internal_reset();

                dbgio_dev_deinit();
                dbgio_dev_default_init(DBGIO_DEV_VDP2);

                vdp2_tvmd_vblank_in_next_wait(1);

                dbgio_dev_font_load();

                dbgio_puts("[H[2J");
        }

        if (assertion_count > 0) {
                dbgio_puts("\n");
        }

        assertion_count++;

        dbgio_puts("Assertion \"");
        dbgio_puts(failed_expr);
        dbgio_puts("\" failed");

        dbgio_puts(": file \"");
        dbgio_puts(file);
        dbgio_puts("\"");

        dbgio_puts(", line ");
        dbgio_puts(line);

        if ((func != NULL) && (*func != '\0')) {
                dbgio_puts(", function: ");
                dbgio_puts(func);
        }

        dbgio_puts("\n");

        vdp2_tvmd_vblank_in_next_wait(1);
        dbgio_flush();

        /* Use the internal calls to commit VDP2 to avoid dealing with VDP2 sync
         * state */
        _internal_vdp2_commit(0);
        _internal_vdp2_commit_wait(0);

        abort();
}
