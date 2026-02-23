#ifndef IC_APP_H
#define IC_APP_H

/**
 * Initializes application modules and runtime state.
 *
 * This function is expected to be called once during system startup
 * before entering the main application loop.
 */
void ic_app_init(void);

/**
 * Runs one iteration of the application task.
 *
 * This function is expected to be called repeatedly from the main loop.
 */
void ic_app_run(void);

#endif /* IC_APP_H */
