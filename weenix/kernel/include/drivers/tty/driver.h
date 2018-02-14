/******************************************************************************/
/* Important Spring 2017 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5f2e8d450c0c5851acd538befe33744efca0f1c4f9fb5f       */
/*         3c8feabc561a99e53d4d21951738da923cd1c7bbd11b30a1afb11172f80b       */
/*         984b1acfbbf8fae6ea57e0583d2610a618379293cb1de8e1e9d07e6287e8       */
/*         de7e82f3d48866aa2009b599e92c852f7dbf7a6e573f1c7228ca34b9f368       */
/*         faaef0c0fcf294cb                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#pragma once

struct tty_driver;
struct tty_device;

typedef void (*tty_driver_callback_t)(void *, char);

typedef struct tty_driver_ops {
        /**
         * Write the given character to the tty driver.
         *
         * @param ttyd the tty driver
         * @param c the character to write to the tty driver
         */
        void (*provide_char)(struct tty_driver *ttyd, char c);

        /**
         * Registers a callback to be called when the tty driver has
         * received a character from an input device and returns the
         * previous callback handler.
         *
         * @param ttyd the tty driver
         * @param callback the callback to register
         * @param arg the argument the callback will receive when called
         * @return returns the previous callback handler
         */
        tty_driver_callback_t (*register_callback_handler)(
                struct tty_driver *ttyd,
                tty_driver_callback_t callback,
                void *arg);

        /**
         * Unregisters the currently registered callback handler if
         * one exists and returns it.
         *
         * @param ttyd the tty driver
         * @return the current callback handler
         */
        tty_driver_callback_t (*unregister_callback_handler)(
                struct tty_driver *ttyd);

        /**
         * Blocks I/O from this driver. This guarantees that no
         * interrupts will be generated by this driver.
         *
         * @param ttyd the tty driver
         * @return implementation specific data that must be passed to
         * unblock_io().
         */
        void *(*block_io)(struct tty_driver *ttyd);

        /**
         * Unblocks I/O from this driver. This re-allows interrupts to
         * be generated by this driver.
         *
         * @param ttyd the tty driver
         * @param data the value returned by block_io()
         */
        void (*unblock_io)(struct tty_driver *ttyd, void *data);
} tty_driver_ops_t;

typedef struct tty_driver {
        tty_driver_ops_t     *ttd_ops;
        tty_driver_callback_t ttd_callback;
        void                 *ttd_callback_arg;
} tty_driver_t;
