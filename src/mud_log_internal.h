#ifndef MUD_LOG_INTERNAL_H
#define MUD_LOG_INTERNAL_H

#include "mud_log_sink.h"

#ifdef __cplusplus
extern "C" {
#endif

/*  Internal bridge used by mud_log.c and mud_log_thread.c
    Dispatches one fully-formed record to the registered sink[s] 
    Sink list access:
    @brief 3 different approaches to iterate over the sink list
        - Option A: Expose a sink iteration function (from mud_log.c)
            Add something like `mud_log_dispatch_to_sinks(const MudLogRecord* record)` that does the mutex-protected iteration
            The log thread calls this instead of accessing `g_log` directly.
            This is the cleanest approach and encapsulation, but means the log thread still touches the mutex in `mud_log.c`
        - Option B: Move sink ownership into the log thread
            The log thread owns the sink list.
            `mud_log_add_sink() enqueues a special "add sink" message.
            Purest single-threaded model but makes sink management async (can't add sink and immediately log)
        - Option C: Share the sink list with a read-write lock
            Sink list rarely changes (only during init/shutdown).
            Use `pthread_rwlock_t` -- producers don't touch it, the consumer takes a read lock.
            `add_sink`/`clear_sinks` take write locks.
            Most performant under the assummption that sink list mutations are rare
*/
void mud_log_dispatch_record(const MudLogRecord* record);

/*  @brief Save and track the Log State for a thread

    @param sinks Pointer to array of sinks
    @param sink_count Number of sinks
    @param min_level Minimum log level
    @param mutex Mutex to use for thread-safety
    @param initialized True if initialized False if not

    typedef struct LogState{
        MudLogSink* sinks[];
        size_t sink_count;
        MudLogLevel min_level;
        pthread_mutex_t mutex;
        bool initialized;
    } LogState;*/

#ifdef __cplusplus
}
#endif

#endif // MUD_LOG_INTERNAL_H
